#include "Base.h"

#if defined(AOSORA_REQUIRED_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <cerrno>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <mutex>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

using SOCKET = int;
const SOCKET INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;

inline int closesocket(SOCKET fd) {
	return close(fd);
}
#endif // AOSORA_REQUIRED_WIN32
#include <thread>
#include <cassert>
#include <list>
#include <string>
#include <map>
#include <set>
#include <atomic>
#include "Version.h"
#include "CoreLibrary/CoreLibrary.h"
#include "CommonLibrary/CommonClasses.h"
#include "Debugger/DebuggerCore.h"
#include "Debugger/DebuggerUtility.h"
#include "Misc/Json.h"
#include "Misc/Message.h"

#if defined(AOSORA_ENABLE_DEBUGGER)
#if defined(AOSORA_REQUIRED_WIN32)
#pragma comment(lib, "Ws2_32.lib")
#endif // AOSORA_REQUIRED_WIN32

//デバッグ通信系。
namespace sakura {

	//実行状態
	enum class ExecutingState {
		Execute,		//通常実行
		StepOut,		//ステップアウト中
		StepIn,			//ステップイン
		StepOver,		//ステップオーバー
		Break			//ブレーク
	};

#if defined(AOSORA_REQUIRED_WIN32)
	//クリティカルセクションロック
	class CriticalSection : public ILock {
	private:
		CRITICAL_SECTION cs;

	public:
		CriticalSection() {
			InitializeCriticalSection(&cs);
		}

		~CriticalSection() {
			DeleteCriticalSection(&cs);
		}

		virtual void Lock() override {
			EnterCriticalSection(&cs);
		}

		virtual void Unlock() override {
			LeaveCriticalSection(&cs);
		}
	};

	//イベント(非同期処理用)
	class SyncEvent : public IEvent {
	private:
		HANDLE eventHandle;

	public:
		SyncEvent() {
			eventHandle = CreateEventA(nullptr, FALSE, FALSE, NULL);
			assert(eventHandle != NULL);
		}

		~SyncEvent() {
			CloseHandle(eventHandle);
		}

		virtual bool Wait(int32_t timeoutMs = -1) override {
			const DWORD timeout = timeoutMs < 0 ? INFINITE : timeoutMs;
			const DWORD waitResult = WaitForSingleObject(eventHandle, timeout);

			if (waitResult == WAIT_OBJECT_0) {
				return true;
			}
			else {
				return false;
			}
		}

		virtual void Raise() override {
			SetEvent(eventHandle);
		}
	};

#else
	using CriticalSection = std::mutex;
	class SyncEvent {
		private:
			std::condition_variable cond;
		public:
			bool Wait(LockScope& lock, int32_t timeoutMs = -1) {
				std::cv_status status = std::cv_status::no_timeout;
				if (timeoutMs >= 0) {
					status = cond.wait_for(lock, std::chrono::milliseconds(timeoutMs));
				}
				else {
					cond.wait(lock);
				}
				if (status == std::cv_status::timeout) {
					return false;
				}
				return true;
			}

			void Raise() {
				cond.notify_all();
			}
	};
#endif // AOSORA_REQUIRED_WIN32

	//ブレークポイント管理
	class BreakPointManager {
	private:
		std::map<std::string, std::set<uint32_t>> breakPoints;
		std::map<std::string, std::set<uint32_t>> loadedFileBreakLines;		//読み込み済みのファイルでブレークポイント設定可能な行のコレクション
		bool isBreakAllError;
		bool isBreakUncaughtError;
		CriticalSection lockObj;

	private:
		std::string NormalizeFilename(const std::string& fullname) {
			//windowsの場合を想定して大文字小文字を考慮しないファイル名にする
			//VSCodeがドライブレターを小文字にしてくることがあるのでその対策として
			std::string f(fullname);
			ToLower(f);
			return f;
		}

	public:
		//ブレークポイントの追加
		void AddBreakPoint(const std::string& fullname, uint32_t line) {
			LockScope ls(lockObj);

			auto normalizedName = NormalizeFilename(fullname);
			auto fileHit = breakPoints.find(normalizedName);
			if (fileHit == breakPoints.end()) {
				//ファイルごと新規登録
				breakPoints.insert(decltype(breakPoints)::value_type(normalizedName, { line }));
				return;
			}

			//行だけ追加
			fileHit->second.insert(line);
		}

		//ブレークポイントの削除
		void RemoveBreakPoint(const std::string& fullname, uint32_t line) {
			LockScope ls(lockObj);

			auto normalizedName = NormalizeFilename(fullname);
			auto fileHit = breakPoints.find(normalizedName);
			if (fileHit == breakPoints.end()) {
				return;
			}

			//削除
			fileHit->second.erase(line);

			if (fileHit->second.empty()) {
				//からっぽになったのでファイルを削除する
				breakPoints.erase(fileHit);
			}
		}

		//ブレークポイントのファイル単位上書き
		void SetBreakPoints(const std::string& fullname, const uint32_t* lines, uint32_t lineCount) {
			LockScope lc(lockObj);

			auto normalizedName = NormalizeFilename(fullname);
			if (lineCount == 0) {
				//削除
				breakPoints.erase(normalizedName);
			}
			else {
				//設定
				assert(lines != nullptr);
				std::set<uint32_t> enabledLines;

				//ブレーク可能かどうかを検証
				auto breakableLines = loadedFileBreakLines.find(normalizedName);
				if (breakableLines != loadedFileBreakLines.end()) {
					for (uint32_t i = 0; i < lineCount; i++) {
						if (breakableLines->second.contains(lines[i])) {
							enabledLines.insert(lines[i]);
						}
					}
				}
				breakPoints[normalizedName] = std::move(enabledLines);
			}
		}

		JsonArrayRef GetBreakpoints(const std::string& fullname) {
			LockScope lc(lockObj);

			auto normalizedName = NormalizeFilename(fullname);
			auto breakLines = breakPoints.find(normalizedName);
			JsonArrayRef result = JsonSerializer::MakeArray();
			if (breakLines != breakPoints.end()) {
				for (uint32_t line : breakLines->second) {
					result->Add(JsonSerializer::From(line));
				}
			}
			return result;
		}

		//ブレークポイントを全クリア
		void ClearBreakPoint() {
			LockScope ls(lockObj);
			breakPoints.clear();
		}

		//ブレークポイントに引っかかるかチェック
		bool QueryBreakPoint(const std::string& fullname, uint32_t line) {
			LockScope ls(lockObj);
			auto normalizedName = NormalizeFilename(fullname);

			auto fileHit = breakPoints.find(normalizedName);
			if (fileHit == breakPoints.end()) {
				return false;
			}

			auto lineHit = fileHit->second.find(line);
			if (lineHit == fileHit->second.end()) {
				return false;
			}

			return true;
		}

		//例外ブレーク設定
		void SetRuntimeErrorBreaks(bool enableAll, bool enableUncaught) {
			LockScope lc(lockObj);
			isBreakAllError = enableAll;
			isBreakUncaughtError = enableUncaught;
		}

		bool IsBreakOnAllRuntimeError() {
			LockScope lc(lockObj);
			return isBreakAllError;
		}
	
		bool IsBreakUncaughtError() {
			LockScope lc(lockObj);
			return isBreakUncaughtError;
		}

		//ロード済みのファイル情報を登録、ブレーク可能な行を照会できるようにする
		void AddLoadedFile(const std::string& fullname, const ASTParseResult& astParseResult) {
			LockScope lc(lockObj);
			std::set<uint32_t> lines;
			for (auto lineIndex : astParseResult.breakableLines) {
				lines.insert(lineIndex);
			}
			auto normalizedName = NormalizeFilename(fullname);
			loadedFileBreakLines[normalizedName] = std::move(lines);
		}

		//ブレーク可能な行情報を収集
		JsonArrayRef FindBreakableLines(const std::string& fullname) {
			LockScope lc(lockObj);
			auto normalizedName = NormalizeFilename(fullname);
			auto it = loadedFileBreakLines.find(normalizedName);
			JsonArrayRef result = JsonSerializer::MakeArray();

			if (it != loadedFileBreakLines.end()) {
				for (const uint32_t line : it->second) {
					result->Add(JsonSerializer::From(line));
				}
			}
			
			return result;
		}
	};

	//デバッグスナップショットのオブジェクトを外部から特定するためのハンドル変換器
	class ScriptReferenceHandleManager {
	private:
		static const uint32_t HANDLE_BASE_OBJECTS = 1000;	//通常のハンドルのオフセット

	public:
		enum class MetadataType : uint32_t {
			LocalVariables,			//スタックフレームローカル変数
			ShioriRequest,			//SHIORI Request
			SaveData,				//SaveData
			CurrentUnit,				//カレントユニット
			Units					//ユニット一覧
		};

		enum class RefType {
			Null,
			Object,
			Metadata,
			UnitPath
		};

		//メタデータ、64bitであること
		struct Metadata {
			MetadataType metadataType;
			uint32_t stackIndex;
		};

		struct RefData {
			RefType type;
			ObjectRef objectRef;	//スクリプトオブジェクト参照
			Metadata metadata;		//スクリプトオブジェクト以外のメタデータ参照
			std::string unitPath;	//参照ユニット

			RefData(const ObjectRef& objRef) :
				type(RefType::Object),
				objectRef(objRef)
			{ }

			RefData(const Metadata& meta):
				type(RefType::Metadata),
				metadata(meta)
			{ }

			RefData(const std::string& unitPath):
				type(RefType::UnitPath),
				unitPath(unitPath)
			{ }

			RefData():
				type(RefType::Null)
			{ }
		};

	private:
		//NOTE: GCに対して抵抗できないので注意。ブレークセッション中はGCは起動しないつもりで⋯

		//ハンドルからリファレンスにするマップ
		std::map<uint32_t, RefData> refMap;

		//リファレンスからハンドルにするマップ
		std::map<const void*, uint32_t> addressCache;
		std::map<uint64_t, uint32_t> metadataCache;
		std::map<std::string, uint32_t> unitCache;

	private:
		//ハンドルの払い出し
		uint32_t GenerateHandle() {
			const uint32_t handle = static_cast<uint32_t>(refMap.size()) + HANDLE_BASE_OBJECTS;
		}

	public:
		//ハンドルの予約ベース
		//結局可変で必要になるのでメタ情報を格納しないといけない
		static const uint32_t HANDLE_NULL = 0;		//null
		static const uint32_t HANDLE_LOCAL_VARIABLES = 1;
		static const uint32_t HANDLE_SHIORI_REQUEST = 2;

		//ハンドルに変換
		uint32_t From(const ObjectRef& ref) {

			//同じオブジェクトに対して別のハンドルを発行しないようにキャッシュ情報を先に検索
			auto cache = addressCache.find(ref.Get());
			if (cache != addressCache.end()) {
				return cache->second;
			}

			//ハンドルの発行
			const uint32_t handle = static_cast<uint32_t>(refMap.size()) + HANDLE_BASE_OBJECTS;

			//参照先の格納
			refMap.insert(decltype(refMap)::value_type(handle, ref));
			addressCache.insert(decltype(addressCache)::value_type(ref.Get(), handle));
			return handle;
		}

		//メタデータをハンドルに変換
		uint32_t From(const Metadata& meta) {
			const uint64_t metaVal = *reinterpret_cast<const uint64_t*>(&meta);	//64bitにつめこむ(キーにできるので)

			//同じオブジェクトに対して別のハンドルを発行しないようにチェック
			auto cache = metadataCache.find(metaVal);
			if (cache != metadataCache.end()) {
				return cache->second;
			}

			//ハンドルの発行
			const uint32_t handle = static_cast<uint32_t>(refMap.size()) + HANDLE_BASE_OBJECTS;

			//参照先の格納
			refMap.insert(decltype(refMap)::value_type(handle, meta));
			metadataCache.insert(decltype(metadataCache)::value_type(metaVal, handle));
			return handle;
		}

		//ユニットパスをハンドルに変換
		uint32_t From(const std::string& unitPath) {

			//同じパスに対して別のハンドルを発行しないようにチェック
			auto cache = unitCache.find(unitPath);
			if (cache != unitCache.end()) {
				return cache->second;
			}

			//ハンドルの発行
			const uint32_t handle = static_cast<uint32_t>(refMap.size()) + HANDLE_BASE_OBJECTS;
			
			//参照先の格納
			refMap.insert(decltype(refMap)::value_type(handle, unitPath));
			unitCache.insert(decltype(unitCache)::value_type(unitPath, handle));
			return handle;
		}

		//ハンドルから変換
		RefData To(uint32_t handle) {
			if (handle == HANDLE_NULL) {
				return RefData();
			}

			auto it = refMap.find(handle);
			if (it != refMap.end()) {
				return it->second;
			}
			else {
				return RefData();
			}
		}

		//クリア
		void Clear() {
			refMap.clear();
		}
	};

	//ブレークセッション
	//ブレーク状態の開始から終了までの情報を持つもの
	class BreakSession {
	private:
		ScriptReferenceHandleManager handleManager;
		const ASTNodeBase& breakingNode;				//次実行されるノード
		ScriptExecuteContext& executeContext;

	public:
		BreakSession(const ASTNodeBase& breakingNode, ScriptExecuteContext& executeContext):
			breakingNode(breakingNode),
			executeContext(executeContext)
		{ }

		uint32_t ObjectToHandle(const ObjectRef& ref) {
			return handleManager.From(ref);
		}

		uint32_t MetadataToHandle(const ScriptReferenceHandleManager::Metadata& meta) {
			return handleManager.From(meta);
		}

		uint32_t UnitToHandle(const std::string& unitName) {
			return handleManager.From(unitName);
		}

		ScriptReferenceHandleManager::RefData HandleToObject(uint32_t handle) {
			return handleManager.To(handle);
		}

		ScriptExecuteContext& GetContext() { return executeContext; }
		const ASTNodeBase& GetBreakASTNode() { return breakingNode; }

	};

	//接続管理
	class DebugConnection {
	private:
		static DebugConnection* instance;

	public:
		using ReceiveCallback = void (*)(JsonObject&);
		using ConnectionStateCallback = void (*)(bool isConnected);

	private:
		//スレッド
		std::shared_ptr<std::thread> listenThread;
		std::shared_ptr<std::thread> sendThread;
		std::shared_ptr<std::thread> recvThread;

		//送信キュー
		std::list<std::shared_ptr<JsonObject>> sendQueue;
		CriticalSection sendQueueLock;

		//受信コールバック
		ReceiveCallback receiveCallback;
		ConnectionStateCallback connectionStateCallback;

		//接続
		std::atomic<bool> isConnected;
		std::atomic<bool> isClosing;
		std::atomic<bool> isStartupFailed;
		SOCKET clientSocket;
		SOCKET listenSocket;
		CriticalSection connectionLock;
		CriticalSection threadLock;
		const uint32_t connectionPort;

	public:

		DebugConnection(uint32_t connectionPort, ReceiveCallback receiveCallback, ConnectionStateCallback connectionStateCallback):
			receiveCallback(receiveCallback),
			connectionStateCallback(connectionStateCallback),
			isConnected(false),
			isClosing(false),
			isStartupFailed(false),
			clientSocket(INVALID_SOCKET),
			listenSocket(INVALID_SOCKET),
			connectionPort(connectionPort)
		{
			assert(receiveCallback != nullptr);
			assert(connectionStateCallback != nullptr);

#if defined(AOSORA_REQUIRED_WIN32)
			//通信系の起動
			WSADATA wsaData;
			if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
				//起動失敗
				assert(false);
			}
#endif // AOSORA_REQUIRED_WIN32
		}

		~DebugConnection()
		{
			Disconnect();
#if defined(AOSORA_REQUIRED_WIN32)
			WSACleanup();
#endif // AOSORA_REQUIRED_WIN32
		}

		static void Create(uint32_t connectionPort, ReceiveCallback receiveCallback, ConnectionStateCallback connectionStateCallback)
		{
			assert(instance == nullptr);
			if (instance == nullptr) {
				instance = new DebugConnection(connectionPort, receiveCallback, connectionStateCallback);
			}
		}

		static void Destroy()
		{
			assert(instance != nullptr);
			if (instance != nullptr) {
				delete instance;
				instance = nullptr;
			}
		}

		static DebugConnection* GetInstance() { return instance; }

		static void CallListenThread() { instance->ListenThread(); }
		static void CallSendThread() { instance->SendThread(); }
		static void CallRecvThread() { instance->RecvThread(); }

		//接続中かどうかを取得
		bool IsConnected() const { return isConnected; }

		void ListenThread()
		{
			//前のスレッドが残ってたら待機
			if (recvThread != nullptr && recvThread->joinable()) {
				recvThread->join();
				recvThread = nullptr;
			}

			if (sendThread != nullptr && sendThread->joinable()) {
				sendThread->join();
				recvThread = nullptr;
			}

			addrinfo hints;
			addrinfo* result = nullptr;
			memset(&hints, 0, sizeof(hints));

			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_flags = AI_PASSIVE;

			//ポートをローカル向きに開く
			if (getaddrinfo("127.0.0.1", std::to_string(connectionPort).c_str(), &hints, &result) != 0) {
				assert(false);
				isStartupFailed = true;
				return;
			}
			
			{
				LockScope ls(connectionLock);

				listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
				if (listenSocket == INVALID_SOCKET) {
					assert(false);
					freeaddrinfo(result);
					isStartupFailed = true;
					return;
				}

				//重複で開こうとするとここで失敗する
				if (bind(listenSocket, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR) {
#if defined(AOSORA_REQUIRED_WIN32)
					int err = WSAGetLastError();
#else
					int err = errno;
#endif
					assert(false);
					freeaddrinfo(result);
					closesocket(listenSocket);
					isStartupFailed = true;
					return;
				}
			}

			freeaddrinfo(result);

			//リッスン
			if (listen(listenSocket, SOMAXCONN) != 0) {
				closesocket(listenSocket);
				isStartupFailed = true;
				return;
			}

#if !defined(AOSORA_REQUIRED_WIN32)
			int old = fcntl(listenSocket, F_GETFL);
			fcntl(listenSocket, F_SETFL, old | O_NONBLOCK);
#endif // not AOSORA_REQUIRED_WIN32

			//accept
			while(!isClosing) {
				auto acceptedSocket = accept(listenSocket, nullptr, nullptr);
				if (acceptedSocket == INVALID_SOCKET) {
#if defined(AOSORA_REQUIRED_WIN32)
					auto errorCode = WSAGetLastError();
					if (errorCode != WSAECONNRESET) {
						//相手側のエラーなど再リッスンでよさそうなものは続行
						continue;
					}
#else
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						timespec t = {0, 1000000};
						nanosleep(&t, NULL);
						continue;
					}
#endif // AOSORA_REQUIRED_WIN32

					//終了要求
					if(isClosing)
					{
						return;
					}

					//失敗
					{
						LockScope lc(connectionLock);
						closesocket(listenSocket);
						listenSocket = INVALID_SOCKET;
					}
					isStartupFailed = true;
					return;
				}

				{
					LockScope lc(connectionLock);
					clientSocket = acceptedSocket;

					//クライアントは1個でいいのでリッスンをやめる
					closesocket(listenSocket);
					listenSocket = INVALID_SOCKET;
				}

				//正常系として続行
				break;
			}

			if (isClosing) {
				return;
			}

			{
				LockScope lc(threadLock);

				//ステートを接続状態に
				isConnected = true;
				connectionStateCallback(true);


				//送受信スレッドを開始
				sendThread.reset(new std::thread(&DebugConnection::CallSendThread));
				recvThread.reset(new std::thread(&DebugConnection::CallRecvThread));

#if defined(AOSORA_REQUIRED_WIN32)
				/*
				//Windows10以降でないと起動しないので一旦消す
				SetThreadDescription(sendThread->native_handle(), L"Aosora Debug Send");
				SetThreadDescription(recvThread->native_handle(), L"Aosora Debug Receive");
				*/
#endif // AOSORA_REQUIRED_WIN32
			}
		}

		void SendThread()
		{
			//送信ループ
			while (!isClosing && isConnected) {

				std::shared_ptr<JsonObject> sendObject = nullptr;
				{
					//キューからいっことりだす
					LockScope ls(sendQueueLock);
					if (!sendQueue.empty()) {
						sendObject = *sendQueue.begin();
						sendQueue.pop_front();
					}
					else {
						sendObject = nullptr;
					}
				}

				if (sendObject != nullptr) {
					//文字列にシリアライズしてそのまま送信
					const std::string sendStr = JsonSerializer::Serialize(sendObject);
					DebugOut(sendStr.c_str());
					DebugOut("\n");
					//末尾\0まで送信する
					if (send(clientSocket, sendStr.c_str(), sendStr.size()+1, 0) == SOCKET_ERROR) {

						//終了要求
						if (isClosing)
						{
							break;
						}

						//送信失敗
						{
							//接続を閉じる、recvスレッドを失敗させて再接続待ちに遷移
							LockScope ls(connectionLock);
							if (clientSocket != INVALID_SOCKET) {
								closesocket(clientSocket);
								clientSocket = INVALID_SOCKET;
							}
						}
						break;
					}
				}
				else {
					//送信することがなかったのでCPUを休める
					//TODO: イベントで待つなど負荷下げる対応をいれたい?
#if defined(AOSORA_REQUIRED_WIN32)
					Sleep(1);
#else
					timespec t = {0, 1000000};
					nanosleep(&t, NULL);
#endif
				}
			}
		}

		void RecvThread()
		{
			//バッファサイズ: 足りなくなったら増やすようにしていく
			int recvBufferSize = 1024;
			char* buffer = static_cast<char*>(malloc(recvBufferSize));

			while (!isClosing) {
				//受信処理
				const size_t size = recv(clientSocket, (buffer), recvBufferSize, 0);

				//終了要求
				if (isClosing)
				{
					break;
				}

				if (size > 0) {
					//受信成功
					size_t offset = 0;
					while (offset < size) {
						//複数同時に届く可能性があるので分離
						const char* ptr = static_cast<const char*>(memchr(buffer + offset, 0, size - offset));
						const size_t index = ptr - buffer;
						if (ptr == nullptr) {
							//終端がない
							assert(false);
							break;
						}

						std::string src(buffer + offset, index - offset);
						JsonDeserializeResult deserializeResult = JsonSerializer::Deserialize(src);

						//デシリアライズに成功していたら処理に回す、失敗していたらそのフォーマットは想定してないので捨てる
						assert(deserializeResult.success);
						assert(deserializeResult.token->GetType() == JsonTokenType::Object);
						if (deserializeResult.success && deserializeResult.token->GetType() == JsonTokenType::Object) {
							receiveCallback(*static_cast<JsonObject*>(deserializeResult.token.get()));
						}
						offset = index + 1;
					}
				}
				else if (size == 0) {
					//切断
					break;
				}
				else {
					//受信失敗
#if defined(AOSORA_REQUIRED_WIN32)
					int errorCode = WSAGetLastError();
					if (errorCode == WSAEMSGSIZE) {
						//サイズ不足なのでサイズを倍にして再試行する
						recvBufferSize *= 2;
						free(buffer);
						buffer = static_cast<char*>(malloc(recvBufferSize));
					}
					else if (errorCode == WSAECONNRESET) {
						//切断
						break;
					}
#else
					if (false) {
						// nop
					}
#endif // AOSORA_REQUIRED_WIN32
					else {
						//切断？
						assert(false);
						break;
					}
				}
			}

			//最終的なバッファの削除
			free(buffer);

			//切断処理
			{
				LockScope ls(connectionLock);
				if (clientSocket != INVALID_SOCKET)
				{
					closesocket(clientSocket);
					clientSocket = INVALID_SOCKET;	//TODO: ここも終了指示の方法を排他制御つきでやること
				}
			}

			//ブレーク中なら再開
			connectionStateCallback(false);
			isConnected = false;

			if (isClosing) {
				//終了に流れていく場合はなにもしない
				return;
			}

			{
				LockScope ls(threadLock);	

				//送信スレッドの終了をまつ
				if (sendThread->joinable()) {
					sendThread->join();
				}

				//リッスンスレッドも念の為待つ
				if (listenThread->joinable()) {
					listenThread->join();
				}

				//リッスンスレッドからやり直す
				listenThread.reset(new std::thread(&DebugConnection::CallListenThread));
#if defined(AOSORA_REQUIRED_WIN32)
				/*
				//Windows10以降でないと起動しないので一旦消す
				SetThreadDescription(listenThread->native_handle(), L"Aosora Debug Listen");
				*/
#endif // AOSORA_REQUIRED_WIN32
			}
		}

		//送信キューにオブジェクトを積む
		void Send(const std::shared_ptr<JsonObject>& sendObj) {
			LockScope ls(sendQueueLock);
			sendQueue.push_back(sendObj);

			//
			DebugOut(JsonSerializer::Serialize(sendObj).c_str());
			DebugOut("\n");
		}

		void Start()
		{
			{
				LockScope ls(connectionLock);

				//リッスンスレッド起動
				listenThread.reset(new std::thread(&DebugConnection::CallListenThread));
#if defined(AOSORA_REQUIRED_WIN32)
				/*
				//Windows10以降でないと起動しないので一旦消す
				SetThreadDescription(listenThread->native_handle(), L"Aosora Debug Listen");
				*/
#endif // AOSORA_REQUIRED_WIN32
			}
		}

		void Disconnect()
		{
			//終了状態を指示、スレッドの停止をまつ
			isClosing = true;

			{
				LockScope ls(connectionLock);

				//ソケットを閉じる
				if (clientSocket != INVALID_SOCKET) {
					closesocket(clientSocket);
				}
				if (listenSocket != INVALID_SOCKET) {
					closesocket(listenSocket);
				}
			}

			{
				LockScope ls(threadLock);

				//スレッド待機
				if (listenThread && listenThread->joinable()) {
					listenThread->join();
				}
				if (sendThread && sendThread->joinable()) {
					sendThread->join();
				}
				if (recvThread && recvThread->joinable()) {
					recvThread->join();
				}
			}
		}
	};

	//Aosoraデバッガ本体
	class DebugSystem {
	public:
		struct ErrorInfo {
			std::string errorMessage;
			std::string errorType;
		};

	private:
		static DebugSystem* instance;

	private:
#pragma region DebugCommands
		//ブレーク中に実行するコマンド
		class IBreakingCommand {
		public:
			virtual ExecutingState Execute(BreakSession& breakSession) = 0;
		};

		//実行再開コマンド
		class ExecuteStateCommand : public IBreakingCommand {
		private:
			ExecutingState nextState;
		public:
			ExecuteStateCommand(ExecutingState state) :
				nextState(state)
			{
			}

			virtual ExecutingState Execute(BreakSession&) override {
				return nextState;
			}
		};

		//オブジェクト展開コマンド
		class MembersRequestCommand : public IBreakingCommand {
		private:
			uint32_t handle;
			std::string requestId;

		public:
			MembersRequestCommand(uint32_t handle, const std::string& requestId) :
				handle(handle),
				requestId(requestId)
			{
			}

			virtual ExecutingState Execute(BreakSession& breakSession) override {
				instance->SendResponse(instance->RequestMembers(handle, breakSession), requestId);
				return ExecutingState::Break;
			}
		};

		//スコープ展開コマンド
		class ScopesRequestCommand : public IBreakingCommand {
		private:
			uint32_t stackIndex;
			std::string requestId;

		public:
			ScopesRequestCommand(uint32_t stackIndex, const std::string& requestId) :
				stackIndex(stackIndex),
				requestId(requestId)
			{
			}

			virtual ExecutingState Execute(BreakSession& breakSession) override {
				instance->SendResponse(instance->RequestScopes(stackIndex, breakSession), requestId);
				return ExecutingState::Break;
			}
		};


		//式評価コマンド(ウォッチ式など)
		class EvaluateExpressionCommand : public IBreakingCommand {
		private:
			uint32_t stackIndex;
			std::string expression;
			std::string requestId;

		public:
			EvaluateExpressionCommand(uint32_t stackIndex, const std::string& expression, const std::string& requestId):
				stackIndex(stackIndex),
				expression(expression),
				requestId(requestId)
			{ }

			virtual ExecutingState Execute(BreakSession& breakSession) override {
				instance->SendResponse(instance->RequestEvaluateExpression(stackIndex, expression, breakSession), requestId);
				return ExecutingState::Break;
			}
		};

#pragma endregion

	private:

		//実行状態
		ExecutingState executingState;
		bool isConnected;
		std::atomic<bool> isSetupCompleted;
		std::atomic<bool> isDebugBootstrapped;

		//ブレーク中のコマンドキュー
		std::list<std::shared_ptr<IBreakingCommand>> breakingCommandQueue;
		CriticalSection breakingCommandQueueLock;

		//ブレークポイント
		BreakPointManager breakPoints;

		//ファイル
		LoadedSourceManager loadedSources;

		//最後にブレークした位置
		std::string lastBreakFullPath;
		uint32_t lastBreakLineIndex;
		size_t lastBreakStackLevel;

		//最後にAST通過した位置
		std::string lastPassFullPath;
		uint32_t lastPassLineIndex;
		size_t lastPassStackLevel;

		//ポート番号
		uint32_t connectionPort;

	public:
		static void Create(uint32_t connectionPort)
		{
			assert(instance == nullptr);
			if (instance == nullptr) {
				instance = new DebugSystem(connectionPort);
			}
		}

		static void Destroy()
		{
			assert(instance != nullptr);
			if (instance != nullptr) {
				delete instance;
				instance = nullptr;
			}
		}

		static DebugSystem* GetInstance() { return instance; }

	private:
		DebugSystem(uint32_t connectionPort) :
			executingState(ExecutingState::Execute),
			isConnected(false),
			isSetupCompleted(false),
			isDebugBootstrapped(false)
		{
			//同時に通信系を起動する
			DebugConnection::Create(connectionPort, &DebugSystem::OnNotifyReceive, &DebugSystem::OnNotifyConnectionState);
			DebugConnection::GetInstance()->Start();
		}

		~DebugSystem()
		{
			//通信系を止める
			DebugConnection::Destroy();
		}

		//通信系からの受信コールバック
		static void OnNotifyReceive(JsonObject& json) {
			GetInstance()->Recv(json);
		}

		//通信系からの接続状態コールバック
		static void OnNotifyConnectionState(bool connected) {
			if (GetInstance()->isConnected != connected) {
				GetInstance()->isConnected = connected;
				if (!connected) {
					//デバッガから直接起動されているとき、切断されたらゴーストを強制的に停止させる
					if (GetInstance()->isDebugBootstrapped) {
						Debugger::TerminateProcess();
					}
					//切断時、ブレークポイント等のステートをすべてリセットする
					GetInstance()->ClearState();
				}
				else {
					GetInstance()->NotifyLog(TextSystem::Find("AOSORA_DEBUGGER_001"), false);
				}
			}
		}

		void Recv(const JsonObject& jsonObject);

		void Send(const std::shared_ptr<JsonObject>& obj) {
			DebugConnection::GetInstance()->Send(obj);
		}

		void SendResponse(const std::shared_ptr<JsonObject>& body, const std::string& requestId) {
			auto responseData = JsonSerializer::MakeObject();
			responseData->Add("type", JsonSerializer::From("response"));
			responseData->Add("responseId", JsonSerializer::From(requestId));
			responseData->Add("body", body);
			Send(responseData);
		}

		void SendResponse(const std::string& requestId) {
			//追加情報なしのレスポンス
			auto responseData = JsonSerializer::MakeObject();
			responseData->Add("type", JsonSerializer::From("response"));
			responseData->Add("responseId", JsonSerializer::From(requestId));
			responseData->Add("body", JsonSerializer::MakeObject());
			Send(responseData);
		}

		void SendRequest(const std::shared_ptr<JsonObject>& body, const std::string& requestType) {
			auto requestData = JsonSerializer::MakeObject();
			requestData->Add("type", JsonSerializer::From(requestType));
			requestData->Add("body", body);
			requestData->Add("responseId", JsonSerializer::From(""));
			Send(requestData);
		}

		void ClearState()
		{
			lastBreakFullPath = "";
			lastBreakLineIndex = 0;
			lastBreakStackLevel = 0;
			lastPassFullPath = "";
			lastPassLineIndex = 0;
			lastPassStackLevel = 0;
			executingState = ExecutingState::Execute;
		}

		void ClearDebuggerData()
		{
			isSetupCompleted = false;
			breakPoints.ClearBreakPoint();
			ClearState();
		}

	public:
		//外側向けには設定完了を接続完了とする
		bool IsConnected() const { return isSetupCompleted; }

		void NotifyASTExecute(const ASTNodeBase& node, ScriptExecuteContext& executeContext);
		void NotifyThrowExceotion(const ScriptError& runtimeError, const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext);
		void NotifyScriptFileLoaded(const std::string& fileBody, const std::string& fullName, const ASTParseResult& astParseResult);
		void NotifyLog(const std::string& log, bool isError);
		void NotifyLog(const std::string& log, const ASTNodeBase& node, bool isError);
		void NotifyLog(const std::string& log, const SourceCodeRange& sourceCodeRange, bool isError);
		void NotifyScriptReturned();
		void NotifyRequestBreak(const ASTNodeBase& node, ScriptExecuteContext& executeContext);

		void Break(const std::string& filepath, uint32_t line, size_t stackLevel, const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext, const ErrorInfo* errorInfo);
		void SendBreakInformation(const ASTNodeBase& node, ScriptExecuteContext& executeContext, const ErrorInfo* errorInfo);

		void AddBreakingCommand(IBreakingCommand* command) {
			LockScope ls(breakingCommandQueueLock);
			breakingCommandQueue.push_back(std::shared_ptr<IBreakingCommand>(command));
		}

		bool IsDebugBootstrapped() { return isDebugBootstrapped; }
		void SetDebugBootstrapped(bool isBootstrapped) { isDebugBootstrapped = isBootstrapped; }

		JsonObjectRef RequestMembers(uint32_t handle, BreakSession& breakSession);
		JsonObjectRef RequestScopes(uint32_t stackIndex, BreakSession& breakSession);
		JsonObjectRef RequestEvaluateExpression(uint32_t stackIndex, const std::string& expression, BreakSession& breakSession);
	};

	DebugConnection* DebugConnection::instance = nullptr;
	DebugSystem* DebugSystem::instance = nullptr;


	//ASTノード実行時
	void DebugSystem::NotifyASTExecute(const ASTNodeBase& node, ScriptExecuteContext& executeContext) {

		if (executeContext.GetInterpreter().IsDebuggerScope()) {
			//すでにデバッガスコープにある場合は、再入禁止
			//ウォッチなどからブレークにはまるのを避けるため
			return;
		}

		ScriptInterpreter::DebuggerScope debuggerScope(executeContext.GetInterpreter());

		bool isBreak = false;

		//エディタスタックレベルの確認
		auto stackTrace = executeContext.MakeStackTrace(node, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName());
		auto stackLevel = stackTrace.size();
		const std::string fullPath = node.GetSourceRange().GetSourceFileFullPath();
		const uint32_t line = node.GetSourceRange().GetBeginLineIndex();

		if (lastPassLineIndex != line || stackLevel != lastPassStackLevel || lastPassFullPath != fullPath) {

			//異なる行にきていればブレークの検証対象
			lastPassFullPath = fullPath;
			lastPassLineIndex = line;
			lastPassStackLevel = stackLevel;

			//ブレークポイントに引っかかるかを検証
			if (breakPoints.QueryBreakPoint(node.GetSourceRange().GetSourceFileFullPath(), node.GetSourceRange().GetBeginLineIndex())) {
				isBreak = true;
			}
			else {

				//ステップオーバー等のヒットの可能性があるので実行形式を検証
				if (executingState == ExecutingState::StepIn) {
					//同じ位置では連続で停止しない
					if (fullPath != lastBreakFullPath || line != lastBreakLineIndex) {
						//ステップインなら無条件に停止
						isBreak = true;
					}
				}
				else if (executingState == ExecutingState::StepOut) {
					//ステップアウトならスタックレベルが戻っていれば
					if (stackLevel < lastBreakStackLevel) {
						isBreak = true;
					}
				}
				else if (executingState == ExecutingState::StepOver) {
					//ステップオーバーなら同階層以前なら
					if (stackLevel <= lastBreakStackLevel) {
						isBreak = true;
					}
					else if (lastBreakStackLevel < stackTrace.size() && stackTrace[stackTrace.size() - lastBreakStackLevel].isJumping) {
						//ジャンプの場合ステップイン扱いになるがステップオーバーで引っ掛けたい
						isBreak = true;
					}
				}
			}
		}

		if (isBreak) {
			Break(fullPath, line, stackLevel, node, executeContext, nullptr);
		}
	}

	//例外発生時
	void DebugSystem::NotifyThrowExceotion(const ScriptError& runtimeError, const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext) {

		if (executeContext.GetInterpreter().IsDebuggerScope()) {
			//すでにデバッガスコープにある場合は、再入禁止
			//ウォッチなどからブレークにはまるのを避けるため
			return;
		}

		ScriptInterpreter::DebuggerScope debuggerScope(executeContext.GetInterpreter());
		bool isBreak = false;
		
		if (executeContext.GetStack().IsTryBlock() && runtimeError.CanCatch()) {
			//キャッチ可能
			isBreak = breakPoints.IsBreakOnAllRuntimeError();
		}
		else {
			//キャッチされない
			isBreak = breakPoints.IsBreakOnAllRuntimeError() || breakPoints.IsBreakUncaughtError();
		}

		if (isBreak) {
			//エディタスタックレベルの確認
			auto stackLevel = executeContext.MakeStackTrace(executingNode, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName()).size();
			const std::string fullPath = executingNode.GetSourceRange().GetSourceFileFullPath();
			const uint32_t line = executingNode.GetSourceRange().GetBeginLineIndex();

			ErrorInfo info;
			info.errorMessage = runtimeError.GetErrorMessage();
			info.errorType = executeContext.GetInterpreter().GetClassTypeName(runtimeError.GetInstanceTypeId());
			Break(fullPath, line, stackLevel, executingNode, executeContext, &info);
		}
	}

	void DebugSystem::NotifyScriptFileLoaded(const std::string& fileBody, const std::string& fullName, const ASTParseResult& astParseResult) {
		breakPoints.AddLoadedFile(fullName, astParseResult);
		loadedSources.AddSource(fileBody, fullName);
	}

	void DebugSystem::NotifyLog(const std::string& log, bool isError) {
		auto requestBody = JsonSerializer::MakeObject();
		requestBody->Add("message", JsonSerializer::From(log));
		requestBody->Add("isError", JsonSerializer::From(isError));
		SendRequest(requestBody, "message");
	}

	void DebugSystem::NotifyLog(const std::string& log, const ASTNodeBase& node, bool isError) {
		NotifyLog(log, node.GetSourceRange(), isError);
	}

	void DebugSystem::NotifyLog(const std::string& log, const SourceCodeRange& sourceCodeRange, bool isError) {
		auto requestBody = JsonSerializer::MakeObject();
		requestBody->Add("message", JsonSerializer::From(log));
		requestBody->Add("isError", JsonSerializer::From(isError));
		requestBody->Add("filepath", JsonSerializer::From(sourceCodeRange.GetSourceFileFullPath()));
		requestBody->Add("line", JsonSerializer::From(sourceCodeRange.GetBeginLineIndex() + 1));
		SendRequest(requestBody, "message");
	}

	void DebugSystem::NotifyScriptReturned() {
		//ステップイン等を解除する
		ClearState();
	}

	void DebugSystem::NotifyRequestBreak(const ASTNodeBase& node, ScriptExecuteContext& executeContext) {
		//スクリプト側からのブレークリクエスト
		if (executeContext.GetInterpreter().IsDebuggerScope()) {
			//すでにデバッガスコープにある場合は、再入禁止
			//ウォッチなどからブレークにはまるのを避けるため
			return;
		}

		//接続中のみ
		if (!IsConnected()) {
			return;
		}

		ScriptInterpreter::DebuggerScope debuggerScope(executeContext.GetInterpreter());

		auto stackTrace = executeContext.MakeStackTrace(node, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName());
		auto stackLevel = stackTrace.size();
		const std::string fullPath = node.GetSourceRange().GetSourceFileFullPath();
		const uint32_t line = node.GetSourceRange().GetBeginLineIndex();

		Break(fullPath, line, stackLevel, node, executeContext, nullptr);
	}

	void DebugSystem::Break(const std::string& fullPath, uint32_t line, size_t stackLevel, const ASTNodeBase& node, ScriptExecuteContext& executeContext, const ErrorInfo* errorInfo) {
		//ブレーク位置を記憶
		lastBreakFullPath = fullPath;
		lastBreakLineIndex = line;
		lastBreakStackLevel = stackLevel;

		//ブレークセッションを作成
		BreakSession breakSession(node, executeContext);

		//ブレーク情報を送る
		SendBreakInformation(node, executeContext, errorInfo);

		//続行系の指示やその他情報要求系の指示を受ける
		while (isConnected) {
			std::shared_ptr<IBreakingCommand> command;
			{
				LockScope ls(breakingCommandQueueLock);
				if (!breakingCommandQueue.empty()) {
					command = *breakingCommandQueue.begin();
					breakingCommandQueue.pop_front();
				}
			}

			if (command != nullptr) {
				//コマンドを実行する
				ExecutingState nextState = command->Execute(breakSession);
				executingState = nextState;

				//続行指示のためブレークして実行中断
				if (executingState != ExecutingState::Break) {
					break;
				}
			}
			else {
				//TODO: 必要に応じて停止する仕組みが必要そう
#if defined(AOSORA_REQUIRED_WIN32)
				Sleep(1);
#else
				timespec t = {0, 1000000};
				nanosleep(&t, NULL);
#endif // AOSORA_REQUIRED_WIN32
			}
		}
	}

	//ブレーク位置などの情報を送信
	void DebugSystem::SendBreakInformation(const ASTNodeBase& node, ScriptExecuteContext& executeContext, const ErrorInfo* errorInfo) {
		//ブレークポイントに引っかかったことを通知
		auto data = JsonSerializer::MakeObject();
		data->Add("type", JsonSerializer::From("break"));

		auto body = JsonSerializer::MakeObject();
		data->Add("body", body);
		data->Add("responseId", JsonSerializer::From(""));

		body->Add("filename", JsonSerializer::From(node.GetSourceRange().GetSourceFileFullPath()));
		body->Add("line", JsonSerializer::From(node.GetSourceRange().GetBeginLineIndex()));

		if (errorInfo) {
			body->Add("errorMessage", JsonSerializer::From(errorInfo->errorMessage));
			body->Add("errorType", JsonSerializer::From(errorInfo->errorType));
		}
		else {
			body->Add("errorMessage", JsonSerializer::MakeNull());
			body->Add("errorType", JsonSerializer::MakeNull());
		}

		//コールスタックの収集
		auto stackArray = JsonSerializer::MakeArray();
		auto stackFrame = executeContext.MakeStackTrace(node, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName());
		uint32_t stackLevel = 0;
		uint32_t nativeStackLevel = 0;
		for (auto& frame : stackFrame) {
			if (!frame.hasSourceRange) {
				nativeStackLevel++;	//純粋な数を数える
				continue;	//スクリプトのスタックフレームのみ
			}

			//行番号等を格納
			auto f = JsonSerializer::MakeObject();
			f->Add("id", JsonSerializer::From(nativeStackLevel));
			f->Add("index", JsonSerializer::From(stackLevel));
			f->Add("name", JsonSerializer::From(frame.funcName));
			f->Add("filename", JsonSerializer::From(frame.sourceRange.GetSourceFileFullPath()));
			f->Add("line", JsonSerializer::From(frame.sourceRange.GetBeginLineIndex()));
			stackArray->Add(f);
			stackLevel++;
			nativeStackLevel++;
		}

		body->Add("stackTrace", stackArray);

		Send(data);
	}

	//変数情報の整形
	JsonObjectRef MakeVariableInfo(const std::string& name, const ScriptValueRef& value, BreakSession& breakSession) {
		ScriptInterpreter& interpreter = breakSession.GetContext().GetInterpreter();

		auto variableInfo = JsonSerializer::MakeObject();
		variableInfo->Add("key", JsonSerializer::From(name));
		std::string primitiveType;
		std::string objectType;
		uint32_t objectHandle = ScriptReferenceHandleManager::HANDLE_NULL;

		switch (value->GetValueType()) {
		case ScriptValueType::Number:
			primitiveType = "number";
			variableInfo->Add("value", JsonSerializer::From(value->ToNumber()));
			break;
		case ScriptValueType::String:
			primitiveType = "string";
			variableInfo->Add("value", JsonSerializer::From(value->ToString()));
			break;
		case ScriptValueType::Boolean:
			primitiveType = "boolean";
			variableInfo->Add("value", JsonSerializer::From(value->ToBoolean()));
			break;
		case ScriptValueType::Null:
		case ScriptValueType::Undefined:
			primitiveType = "null";
			variableInfo->Add("value", JsonSerializer::MakeNull());
			break;
		case ScriptValueType::Object:
			primitiveType = "object";
			objectType = interpreter.GetClassTypeName(value->GetObjectRef()->GetInstanceTypeId());
			objectHandle = breakSession.ObjectToHandle(value->GetObjectRef());
			variableInfo->Add("value", JsonSerializer::MakeNull());

			//TODO: とりあえずオブジェクトは分解しているけれど、分解すべきでないものあるように思える
			//たとえばまだないけど日付と時刻のような?

			break;
		}

		variableInfo->Add("primitiveType", JsonSerializer::From(primitiveType));
		variableInfo->Add("objectType", JsonSerializer::From(objectType));
		variableInfo->Add("objectHandle", JsonSerializer::From(objectHandle));
		return variableInfo;
	}

	//ユニット情報の作成
	JsonObjectRef MakeUnitInfo(const std::string& unitName, const UnitData& unitData, BreakSession& breakSession) {
		auto unitInfo = JsonSerializer::MakeObject();
		unitInfo->Add("value", JsonSerializer::MakeNull());
		unitInfo->Add("key", JsonSerializer::From(unitName));
		unitInfo->Add("primitiveType", JsonSerializer::From("object"));
		unitInfo->Add("objectType", JsonSerializer::From("unit"));
		unitInfo->Add("objectHandle", JsonSerializer::From(breakSession.UnitToHandle(unitName)));
		return unitInfo;
	}

	//ユニットの列挙
	JsonArrayRef EnumUnits(BreakSession& breakSession) {
		const auto& unitCollection = breakSession.GetContext().GetInterpreter().GetUnitCollection();
		JsonArrayRef result = JsonSerializer::MakeArray();
		for (auto kvp : unitCollection) {
			result->Add(MakeUnitInfo(kvp.first,	kvp.second, breakSession));
		}
		return result;
	}

	//ユニット変数の列挙
	JsonArrayRef EnumUnitVariables(const std::string& unitName, BreakSession& breakSession) {
		JsonArrayRef result = JsonSerializer::MakeArray();
		const auto* unitData = breakSession.GetContext().GetInterpreter().FindUnitData(unitName);
		if (unitData != nullptr) {
			for (auto kvp : unitData->unitVariables) {
				result->Add(MakeVariableInfo(kvp.first, kvp.second, breakSession));
			}
		}
		return result;
	}

	//カレントユニット変数の列挙
	JsonArrayRef EnumCurrentUnitVariables(uint32_t stackIndex, BreakSession& breakSession) {
		auto stackTrace = breakSession.GetContext().MakeStackTrace(breakSession.GetBreakASTNode(), breakSession.GetContext().GetBlockScope(), breakSession.GetContext().GetStack().GetFunctionName());
		JsonArrayRef result = JsonSerializer::MakeArray();

		uint32_t nativeStackLevel = 0;
		for (auto& frame : stackTrace) {
			if (!frame.hasSourceRange) {
				nativeStackLevel++;	//純粋な数を数える
				continue;	//スクリプトのスタックフレームのみ
			}

			if (stackIndex == nativeStackLevel) {
				//一致するスタックフレームの情報を送る
				result = EnumUnitVariables(frame.executingAstNode->GetScriptUnit()->GetUnit(), breakSession);
				break;
			}

			nativeStackLevel++;
		}

		return result;
	}

	//ローカル変数の列挙
	JsonArrayRef EnumLocalVariables(uint32_t stackIndex, BreakSession& breakSession) {
		auto stackTrace = breakSession.GetContext().MakeStackTrace(breakSession.GetBreakASTNode(), breakSession.GetContext().GetBlockScope(), breakSession.GetContext().GetStack().GetFunctionName());

		//デバッグ情報のような形でもいいのかも
		std::set<std::string> addedSet;
		JsonArrayRef result = JsonSerializer::MakeArray();
		uint32_t nativeStackLevel = 0;
		for (auto& frame : stackTrace) {
			if (!frame.hasSourceRange) {
				nativeStackLevel++;	//純粋な数を数える
				continue;	//スクリプトのスタックフレームのみ
			}

			if (stackIndex == nativeStackLevel) {
				//一致するスタックフレームの情報を送る
				Reference<BlockScope> scope = frame.blockScope;
				while (scope != nullptr) {

					//ローカル
					for (auto kvp : scope->GetLocalVariableCollection()) {
						if (!addedSet.contains(kvp.first)) {
							result->Add(MakeVariableInfo(kvp.first, kvp.second, breakSession));
							addedSet.insert(kvp.first);
						}
					}

					//this
					if (scope->GetThisValue() != nullptr) {
						result->Add(MakeVariableInfo("this", scope->GetThisValue(), breakSession));
						addedSet.insert("this");
					}

					scope = scope->GetParentScope();
				}
				break;
			}
			
			nativeStackLevel++;
		}

		return result;
	}

	//SHIORI Requestを送信
	JsonArrayRef EnumShioriRequests(BreakSession& breakSession) {
		JsonArrayRef result = JsonSerializer::MakeArray();
		ScriptInterpreter& interpreter = breakSession.GetContext().GetInterpreter();
		auto shioriVariable = interpreter.GetUnitVariable("Shiori", "system");
		ScriptObject* shioriObj = interpreter.InstanceAs<ScriptObject>(shioriVariable);
		if (shioriObj == nullptr) {
			//書き込まれているなどで期待した方でない場合無効
			return result;
		}

		auto referenceVariable = shioriObj->RawGet("Reference");
		ScriptArray* referenceArray = interpreter.InstanceAs<ScriptArray>(referenceVariable);
		if (referenceArray == nullptr) {
			//書き込まれているなどで期待した方でない場合無効
			return result;
		}

		//Referenceを優先で収集（多分１番必要なので）
		std::set<std::string> addedSet;
		for (size_t i = 0; i < referenceArray->Count(); i++) {
			std::string key = std::string("Reference") + std::to_string(i);
			auto elem = referenceArray->At(i);
			result->Add(MakeVariableInfo(key, elem, breakSession));
			addedSet.insert(key);
		}

		//そのあと把握してないヘッダを取り込む
		auto headersVariable = shioriObj->RawGet("Headers");
		ScriptObject* headersObj = interpreter.InstanceAs<ScriptObject>(headersVariable);
		if (headersObj == nullptr) {
			//書き込まれているなどで期待した方でない場合無効
			return result;
		}

		for (auto kvp : headersObj->GetInternalCollection()) {
			//Referenceは重複なのでパス
			if (!addedSet.contains(kvp.first)) {
				result->Add(MakeVariableInfo(kvp.first, kvp.second, breakSession));
			}
		}

		return result;
	}

	//SaveDataを送信
	JsonArrayRef EnumSaveData(BreakSession& breakSession) {
		auto saveDataVariable = SaveData::StaticGet("Data", breakSession.GetContext());
		ScriptInterpreter& interpreter = breakSession.GetContext().GetInterpreter();
		JsonArrayRef result = JsonSerializer::MakeArray();

		ScriptObject* object = interpreter.InstanceAs<ScriptObject>(saveDataVariable);
		if (object == nullptr) {
			//書き込まれているなどで期待した方でない場合無効
			return result;
		}

		for (auto item : object->GetInternalCollection()) {
			result->Add(MakeVariableInfo(item.first, item.second, breakSession));
		}
		return result;
	}

	//ScriptObjectのメンバ列挙
	JsonArrayRef EnumMembers(ScriptObject& object, BreakSession& breakSession) {

		//TODO: 辞書順か何かにするか？
		auto result = JsonSerializer::MakeArray();
		for (auto item : object.GetInternalCollection()) {
			result->Add(MakeVariableInfo(item.first, item.second, breakSession));
		}
		return result;
	}

	//ScriptArrayのメンバ列挙
	JsonArrayRef EnumMembers(ScriptArray& object, BreakSession& breakSession) {
		auto result = JsonSerializer::MakeArray();
		for (size_t i = 0; i < object.Count(); i++) {
			result->Add(MakeVariableInfo(std::to_string(i), object.At(i), breakSession));
		}
		return result;
	}

	//スクリプトオブジェクトタイプごとのメンバ列挙
	JsonArrayRef EnumMembers(uint32_t objectHandle, BreakSession& breakSession) {

		//オブジェクトを復元
		auto ref = breakSession.HandleToObject(objectHandle);
		if (ref.type == ScriptReferenceHandleManager::RefType::Metadata) {
			//メタデータの場合は特殊
			if (ref.metadata.metadataType == ScriptReferenceHandleManager::MetadataType::ShioriRequest) {
				return EnumShioriRequests(breakSession);
			}
			else if (ref.metadata.metadataType == ScriptReferenceHandleManager::MetadataType::LocalVariables) {
				return EnumLocalVariables(ref.metadata.stackIndex, breakSession);
			}
			else if (ref.metadata.metadataType == ScriptReferenceHandleManager::MetadataType::SaveData) {
				return EnumSaveData(breakSession);
			}
			else if (ref.metadata.metadataType == ScriptReferenceHandleManager::MetadataType::CurrentUnit) {
				return EnumCurrentUnitVariables(ref.metadata.stackIndex, breakSession);
			}
			else if (ref.metadata.metadataType == ScriptReferenceHandleManager::MetadataType::Units) {
				return EnumUnits(breakSession);
			}
		}

		if (ref.type == ScriptReferenceHandleManager::RefType::UnitPath) {
			return EnumUnitVariables(ref.unitPath, breakSession);
		}

		if (ref.type != ScriptReferenceHandleManager::RefType::Object) {
			assert(false);
			return JsonSerializer::MakeArray();
		}

		auto obj = ref.objectRef;
		if (obj == nullptr) {
			//おかしい
			assert(false);
			return JsonSerializer::MakeArray();
		}

		//もうちょっと良い分岐の仕方はあるだろうけどとりあえず
		ScriptInterpreter& interpreter = breakSession.GetContext().GetInterpreter();
		{
			ScriptObject* inst = interpreter.InstanceAs<ScriptObject>(obj);
			if (inst != nullptr) {
				return EnumMembers(*inst, breakSession);
			}
		}

		{
			ScriptArray* inst = interpreter.InstanceAs<ScriptArray>(obj);
			if (inst != nullptr) {
				return EnumMembers(*inst, breakSession);
			}
		}

		//クラスインスタンスは中身にアクセスさせる
		if (obj->GetNativeInstanceTypeId() == ClassInstance::TypeId()) {
			auto inst = obj.Cast<ClassInstance>();
			return EnumMembers(*inst->GetScriptStore().Get(), breakSession);
		}

		//対応する型がない: 組み込みオブジェクトなど。ただハンドルは生成するので（evalなどで･･･）とりあえず何もないことをかえす
		return JsonSerializer::MakeArray();
	}

	//スコープ情報の列挙
	JsonArrayRef EnumScopes(uint32_t stackLevel, BreakSession& breakSession) {

		auto result = JsonSerializer::MakeArray();

		//Scopes
		{
			auto obj = JsonSerializer::MakeObject();
			ScriptReferenceHandleManager::Metadata meta;
			meta.metadataType = ScriptReferenceHandleManager::MetadataType::LocalVariables;
			meta.stackIndex = stackLevel;

			const uint32_t handle = breakSession.MetadataToHandle(meta);
			obj->Add("handle", JsonSerializer::From(handle));
			obj->Add("name", JsonSerializer::From(TextSystem::Find("AOSORA_DEBUGGER_002")));
			result->Add(obj);
		}

		//SHIORI Reference
		{
			auto obj = JsonSerializer::MakeObject();
			ScriptReferenceHandleManager::Metadata meta;
			meta.metadataType = ScriptReferenceHandleManager::MetadataType::ShioriRequest;
			meta.stackIndex = 0;	//スタック関係ないはず

			const uint32_t handle = breakSession.MetadataToHandle(meta);
			obj->Add("handle", JsonSerializer::From(handle));
			obj->Add("name", JsonSerializer::From("SHIORI Request"));
			result->Add(obj);
		}

		//SaveData
		{
			auto obj = JsonSerializer::MakeObject();
			ScriptReferenceHandleManager::Metadata meta;
			meta.metadataType = ScriptReferenceHandleManager::MetadataType::SaveData;
			meta.stackIndex = 0;	//スタック関係ないはず
			const uint32_t handle = breakSession.MetadataToHandle(meta);
			obj->Add("handle", JsonSerializer::From(handle));
			obj->Add("name", JsonSerializer::From("SaveData"));
			result->Add(obj);
		}


		//Current Unit
		{
			auto obj = JsonSerializer::MakeObject();
			ScriptReferenceHandleManager::Metadata meta;
			meta.metadataType = ScriptReferenceHandleManager::MetadataType::CurrentUnit;
			meta.stackIndex = stackLevel;
			const uint32_t handle = breakSession.MetadataToHandle(meta);
			obj->Add("handle", JsonSerializer::From(handle));
			obj->Add("name", JsonSerializer::From("Current Unit"));
			result->Add(obj);
		}

		//Units
		{
			auto obj = JsonSerializer::MakeObject();
			ScriptReferenceHandleManager::Metadata meta;
			meta.metadataType = ScriptReferenceHandleManager::MetadataType::Units;
			meta.stackIndex = 0;
			const uint32_t handle = breakSession.MetadataToHandle(meta);
			obj->Add("handle", JsonSerializer::From(handle));
			obj->Add("name", JsonSerializer::From("Units"));
			result->Add(obj);
		}

		return result;
	}

	//リクエスト受信
	void DebugSystem::Recv(const JsonObject& json) {
		std::string requestType;
		std::string requestId;

		// "type" にリクエストの種別が格納されている
		if (!JsonSerializer::As(json.Get("type"), requestType)) {
			//対応なし
			assert(false);
			return;
		}

		// "id" はレスポンスを返すリクエストに埋め込むために使う
		JsonSerializer::As(json.Get("id"), requestId);

		// "body" にリクエスト本体がリクエストごとの任意フォーマットで格納される
		JsonObjectRef body;
		if (!JsonSerializer::As(json.Get("body"), body)) {
			assert(false);
			return;
		}
		
		//リクエストタイプごとに処理を分ける
		if (requestType == "version") {
			JsonObjectRef responseBody = JsonSerializer::MakeObject();
			responseBody->Add("version", JsonSerializer::From(AOSORA_SHIORI_VERSION));
			responseBody->Add("debuggerRevision", JsonSerializer::From(AOSORA_DEBUGGER_REVISION));
			SendResponse(responseBody, requestId);
			return;
		}
		else if (requestType == "set_breakpoints") {

			std::string filename;
			JsonArrayRef lineArray;

			if (!JsonSerializer::As(body->Get("filename"), filename)) {
				assert(false);
				return;
			}

			if (!JsonSerializer::As(body->Get("lines"), lineArray)) {
				assert(false);
				return;
			}

			//ブレークポイント情報を更新
			std::vector<uint32_t> lines;
			for (auto& token : lineArray->GetCollection()) {
				uint32_t l;
				if (JsonSerializer::As(token, l)) {
					lines.push_back(l);
				}
			}

			if (!lines.empty()) {
				breakPoints.SetBreakPoints(filename, &lines.at(0), lines.size());
			}
			else {
				breakPoints.SetBreakPoints(filename, nullptr, 0);
			}

			//レスポンスとして設定成功した行を返す
			JsonObjectRef responseBody = JsonSerializer::MakeObject();
			responseBody->Add("lines", breakPoints.GetBreakpoints(filename));

			SendResponse(responseBody, requestId);
			return;
		}
		else if (requestType == "set_exception_breakpoint") {
			std::shared_ptr<JsonArray> filters;
			if (JsonSerializer::As(body->Get("filters"), filters)) {
				//把握しているフィルタの状態を切り替え
				bool breakAll = false;
				bool breakUncaught = false;
				for (size_t i = 0; i < filters->GetCount(); i++) {
					std::string elem;
					if (JsonSerializer::As(filters->GetItem(i), elem)) {
						if (elem == "all") {
							breakAll = true;
						}
						else if (elem == "uncaught") {
							breakUncaught = true;
						}
					}
				}
				breakPoints.SetRuntimeErrorBreaks(breakAll, breakUncaught);

				//例外ブレークポイントの設定は有無にかかわらず呼び出され、この時点でセットアップ完了といえる
				isSetupCompleted = true;
				SendResponse(requestId);
				return;
			}
		}
		else if (requestType == "breakpoint_locations") {
			//ブレークポイント可能な位置をリストアップ
			std::string fullname;
			if (JsonSerializer::As(body->Get("filename"), fullname)) {
				auto responseData = JsonSerializer::MakeObject();
				auto responseLines = breakPoints.FindBreakableLines(fullname);
				responseData->Add("lines", responseLines);
				SendResponse(responseData, requestId);
				return;
			}
		}
		else if (requestType == "continue") {
			//続行指示
			AddBreakingCommand(new ExecuteStateCommand(ExecutingState::Execute));
			SendResponse(requestId);
			return;
		}
		else if (requestType == "stepin") {
			AddBreakingCommand(new ExecuteStateCommand(ExecutingState::StepIn));
			SendResponse(requestId);
			return;
		}
		else if (requestType == "stepover") {
			AddBreakingCommand(new ExecuteStateCommand(ExecutingState::StepOver));
			SendResponse(requestId);
			return;
		}
		else if (requestType == "stepout") {
			AddBreakingCommand(new ExecuteStateCommand(ExecutingState::StepOut));
			SendResponse(requestId);
			return;
		}
		else if (requestType == "members") {
			//メンバの展開
			uint32_t handle;
			if (JsonSerializer::As(body->Get("handle"), handle)) {
				AddBreakingCommand(new MembersRequestCommand(handle, requestId));
				return;
			}
		}
		else if (requestType == "scopes") {
			uint32_t stackIndex;
			if (JsonSerializer::As(body->Get("stackIndex"), stackIndex)) {
				AddBreakingCommand(new ScopesRequestCommand(stackIndex, requestId));
				return;
			}
		}
		else if (requestType == "evaluate") {
			uint32_t stackIndex;
			std::string expression;
			if (JsonSerializer::As(body->Get("stackIndex"), stackIndex) && JsonSerializer::As(body->Get("expression"), expression)) {
				AddBreakingCommand(new EvaluateExpressionCommand(stackIndex, expression, requestId));
				return;
			}
		}
		else if (requestType == "loaded_sources") {
			auto responseData = JsonSerializer::MakeObject();
			auto files = JsonSerializer::MakeArray();
			for (size_t i = 0; i < loadedSources.GetLoadedSourceCount(); i++) {
				auto fileRecord = JsonSerializer::MakeObject();
				const auto& loadedFile = loadedSources.GetLoadedSource(i);
				fileRecord->Add("path", JsonSerializer::From(loadedFile.fullName));
#if defined(AOSORA_DEBUGGER_ENABLE_MD5)
				fileRecord->Add("md5", JsonSerializer::From(loadedFile.md5));
#endif
				files->Add(fileRecord);
			}
			responseData->Add("files", files);
			SendResponse(responseData, requestId);
			return;
		}

		//returnされなかった場合は処理できなかったものとして扱う、エラーレスポンスとして返す
		{
			auto responseData = JsonSerializer::MakeObject();
			responseData->Add("type", JsonSerializer::From("error_response"));
			responseData->Add("responseId", JsonSerializer::From(requestId));
			Send(responseData);
		}
	}

	//受信指定オブジェクトのメンバを列挙する
	JsonObjectRef DebugSystem::RequestMembers(uint32_t handle, BreakSession& breakSession) {

		JsonObjectRef responseBody = JsonSerializer::MakeObject();
		responseBody->Add("variables", EnumMembers(handle, breakSession));
		return responseBody;
	}

	//スコープの列挙を返す
	JsonObjectRef DebugSystem::RequestScopes(uint32_t stackIndex, BreakSession& breakSession) {
		auto responseBody = JsonSerializer::MakeObject();
		responseBody->Add("scopes", EnumScopes(stackIndex, breakSession));
		return responseBody;
	}

	//式を評価する
	JsonObjectRef DebugSystem::RequestEvaluateExpression(uint32_t stackIndex, const std::string& expression, BreakSession& breakSession) {
		auto& context = breakSession.GetContext();
		auto& interpreter = context.GetInterpreter();
		auto responseBody = JsonSerializer::MakeObject();

		//スタックトレースを取得、対象のスタックインデックスを探す
		uint32_t nativeStackLevel = 0;
		auto stackFrame = breakSession.GetContext().MakeStackTrace(breakSession.GetBreakASTNode(), breakSession.GetContext().GetBlockScope(), breakSession.GetContext().GetStack().GetFunctionName());
		for (auto& frame : stackFrame) {
			if (!frame.hasSourceRange) {
				nativeStackLevel++;	//純粋な数を数える
				continue;	//スクリプトのスタックフレームのみ
			}

			if (stackIndex == nativeStackLevel) {
				//例外が発生しても影響しないように独立したスタックで実行する、変数参照は影響しないようにブロックスコープは継承する
				ScriptInterpreterStack debugWatchStack;
				Reference<BlockScope> debugWatchBlock = interpreter.CreateNativeObject<BlockScope>(frame.blockScope);
				ScriptExecuteContext debugWatchContext(interpreter, debugWatchStack, debugWatchBlock);

				//実行制限をリセット
				//デバッガ中は専用のリミッターに切り替わるので、ウォッチの評価毎にリセットしてよい
				interpreter.ResetScriptStep();

				//ユニットエイリアス等の参照を同じようにできるようメタデータをインポートして式を実行する
				auto result = interpreter.Eval(expression, debugWatchContext, frame.executingAstNode->GetSourceMetadata());

				//内容にあわせて結果を格納
				if (result.error != nullptr) {
					//パースエラーが出ている場合、パースエラー本文をエラーメッセージとして返す
					responseBody->Add("exception", MakeVariableInfo("error", ScriptValue::Make(result.error->GetData().message), breakSession));
				}
				else if (debugWatchStack.IsThrew()) {
					//例外が出ている場合、結果としてそのエラーということを返す
					responseBody->Add("exception", MakeVariableInfo("error", ScriptValue::Make(debugWatchStack.GetThrewError()), breakSession));
					responseBody->Add("errorType", JsonSerializer::From(debugWatchStack.GetThrewError()->GetClassTypeName(interpreter)));
				}
				else {
					//AST実行結果をレスポンスとして返す
					responseBody->Add("value", MakeVariableInfo("value", result.value, breakSession));
				}
			}
			nativeStackLevel++;
		}

		//
		DebugOut(JsonSerializer::Serialize(responseBody).c_str());
		DebugOut("\n");

		return responseBody;
	}

	//デバッグフック
	void Debugger::NotifyASTExecute(const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext) {
		if (DebugSystem::GetInstance() != nullptr) {
			//ブレークポイント要求に対して反応する想定
			DebugSystem::GetInstance()->NotifyASTExecute(executingNode, executeContext);
		}
	}

	void Debugger::NotifyError(const ScriptError& runtimeError, const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext) {
		if (DebugSystem::GetInstance() != nullptr) {
			DebugSystem::GetInstance()->NotifyThrowExceotion(runtimeError, executingNode, executeContext);
		}
	}

	void Debugger::NotifyScriptFileLoaded(const std::string& fileBody, const std::string& fullName, const ASTParseResult& astParseResult) {
		if (DebugSystem::GetInstance() != nullptr) {
			DebugSystem::GetInstance()->NotifyScriptFileLoaded(fileBody, fullName, astParseResult);
		}
	}

	void Debugger::NotifyLog(const std::string& log, bool isError) {
		if (DebugSystem::GetInstance() != nullptr) {
			DebugSystem::GetInstance()->NotifyLog(log, isError);
		}
	}

	void Debugger::NotifyLog(const std::string& log, const ASTNodeBase& node, bool isError) {
		if (DebugSystem::GetInstance() != nullptr) {
			DebugSystem::GetInstance()->NotifyLog(log, node, isError);
		}
	}

	void Debugger::NotifyLog(const std::string& log, const SourceCodeRange& sourceCodeRange, bool isError) {
		if (DebugSystem::GetInstance() != nullptr) {
			DebugSystem::GetInstance()->NotifyLog(log, sourceCodeRange, isError);
		}
	}

	void Debugger::NotifyEventReturned() {
		if (DebugSystem::GetInstance() != nullptr) {
			DebugSystem::GetInstance()->NotifyScriptReturned();
		}
	}

	void Debugger::NotifyRequestBreak(const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext) {
		if (DebugSystem::GetInstance() != nullptr) {
			DebugSystem::GetInstance()->NotifyRequestBreak(executingNode, executeContext);
		}
	}

	void Debugger::SetDebugBootstrapped(bool isBootstrapped) {
		if (DebugSystem::GetInstance() != nullptr) {
			DebugSystem::GetInstance()->SetDebugBootstrapped(isBootstrapped);
		}
	}

	bool Debugger::IsDebugBootstrapped() {
		if (DebugSystem::GetInstance() != nullptr) {
			return DebugSystem::GetInstance()->IsDebugBootstrapped();
		}
		return false;
	}

	void Debugger::Create(uint32_t connectionPort) {
		DebugSystem::Create(connectionPort);
	}

	void Debugger::Destroy() {
		DebugSystem::Destroy();
	}

	bool Debugger::IsCreated() {
		return (DebugSystem::GetInstance() != nullptr);
	}

	bool Debugger::IsConnected() {
		if (DebugSystem::GetInstance() != nullptr) {
			return DebugSystem::GetInstance()->IsConnected();
		}
		return false;
	}

	void Debugger::TerminateProcess() {
		//プロセス強制停止
#if defined(AOSORA_REQUIRED_WIN32)
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, GetCurrentProcessId());
		::TerminateProcess(hProcess, 0);
#else
		pid_t pid = getpid();
		kill(pid, SIGTERM);
#endif // AOSORA_REQUIRED_WIN32
	}
}

#else // defined(AOSORA_ENABLE_DEBUGGER)

//ダミー
namespace sakura {
	void Debugger::NotifyASTExecute(const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext){}
	void Debugger::NotifyError(const RuntimeError& runtimeError, const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext){}
	void Debugger::NotifyScriptFileLoaded(const std::string& fileBody, const std::string& fullName, const ASTParseResult& astParseResult){}
	void Debugger::NotifyLog(const std::string& log, bool isError){}
	void Debugger::NotifyLog(const std::string& log, const ASTNodeBase& node, bool isError){}
	void Debugger::NotifyLog(const std::string& log, const SourceCodeRange& range, bool isError){}
	void Debugger::NotifyEventReturned(){}
	void Debugger::NotifyRequestBreak(const ASTNodeBase& node, ScriptExecuteContext& executeContext) {}
	void Debugger::Create(uint32_t connectionPort){}
	void Debugger::Destroy(){}
	bool Debugger::IsCreated() { return false; }
	void Debugger::Bootstrap(){}
	bool Debugger::IsConnected() { return false; }
	void Debugger::SetDebugBootstrapped(bool isBootstrapped){}
	bool Debugger::IsDebugBootstrapped() { return false; }

	//呼び出されないはず
	void Debugger::TerminateProcess() { assert(false); }
}

#endif // defined(AOSORA_ENABLE_DEBUGGER)
