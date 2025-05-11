#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <cassert>
#include <list>
#include <string>
#include <map>
#include <set>
#include "CoreLibrary/CoreLibrary.h"
#include "Debugger/DebugIO.h"
#include "Misc/Json.h"
#pragma comment(lib, "Ws2_32.lib")

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

	enum class ConnectionState {
		Disconnected,
		Connected
	};

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

	//ブレークポイント類
	class BreakPointCollection {
	private:
		//TODO: 重複追加を考慮すべきだろうか
		std::map<std::string, std::set<uint32_t>> breakPoints;
		CriticalSection lockObj;

	public:
		//ブレークポイントの追加
		void AddBreakPoint(const std::string& filename, uint32_t line) {
			LockScope ls(lockObj);

			auto fileHit = breakPoints.find(filename);
			if (fileHit == breakPoints.end()) {
				//ファイルごと新規登録
				breakPoints.insert(decltype(breakPoints)::value_type(filename, { line }));
				return;
			}

			//行だけ追加
			fileHit->second.insert(line);
		}

		//ブレークポイントの削除
		void RemoveBreakPoint(const std::string& filename, uint32_t line) {
			LockScope ls(lockObj);

			auto fileHit = breakPoints.find(filename);
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
		void SetBreakPoints(const std::string& filename, const uint32_t* lines, uint32_t lineCount) {
			LockScope lc(lockObj);

			if (lineCount == 0) {
				//削除
				breakPoints.erase(filename);
			}
			else {
				//設定
				assert(lines != nullptr);
				breakPoints[filename] = std::set<uint32_t>(lines, lines + lineCount);
			}

		}

		//ブレークポイントを全クリア
		void ClearBreakPoint() {
			LockScope ls(lockObj);
			breakPoints.clear();
		}

		//ブレークポイントに引っかかるかチェック
		bool QueryBreakPoint(const std::string& filename, uint32_t line) {
			LockScope ls(lockObj);

			auto fileHit = breakPoints.find(filename);
			if (fileHit == breakPoints.end()) {
				return false;
			}

			auto lineHit = fileHit->second.find(line);
			if (lineHit == fileHit->second.end()) {
				return false;
			}

			return true;
		}

	};

	//デバッグスナップショットのオブジェクトを外部から特定するためのハンドル変換器
	//TODO: システム側のクラスやスコープ単位の情報など特殊なものをふくめて一意キーを発行する必要がある
	class ScriptReferenceHandleManager {
	private:
		static const uint32_t HANDLE_BASE_OBJECTS = 1000;	//通常のハンドルのオフセット

	public:
		enum class MetadataType : uint32_t {
			LocalVariables,			//スタックフレームローカル変数
			ShioriRequest,			//SHIORI Request
		};

		enum class RefType {
			Null,
			Object,
			Metadata
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

			RefData(const ObjectRef& objRef) :
				type(RefType::Object),
				objectRef(objRef)
			{ }

			RefData(const Metadata& meta):
				type(RefType::Metadata),
				metadata(meta)
			{ }

			RefData():
				type(RefType::Null)
			{ }
		};

	private:
		//NOTE: GCに対して抵抗できないので注意。ブレークセッション中はGCは起動しないつもりで⋯
		std::map<uint32_t, RefData> refMap;
		std::map<const void*, uint32_t> addressCache;
		std::map<uint64_t, uint32_t> metadataCache;

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
			const uint64_t metaVal = *reinterpret_cast<const uint64_t*>(&meta);	//64bitにつめこむ

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

		ScriptReferenceHandleManager::RefData HandleToObject(uint32_t handle) {
			return handleManager.To(handle);
		}

		ScriptExecuteContext& GetContext() { return executeContext; }
		const ASTNodeBase& GetBreakASTNode() { return breakingNode; }

	};

	//デバッグ通信系
	//TODO: ソケット通信系とデバッグ処理系をきりはなしたほうがいいかも
	class DebugIO {
	private:
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
			ExecuteStateCommand(ExecutingState state):
				nextState(state)
			{ }

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
			{}

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
			ScopesRequestCommand(uint32_t stackIndex, const std::string& requestId):
				stackIndex(stackIndex),
				requestId(requestId)
			{ }

			virtual ExecutingState Execute(BreakSession& breakSession) override {
				instance->SendResponse(instance->RequestScopes(stackIndex, breakSession), requestId);
				return ExecutingState::Break;
			}

		};

	private:
		static DebugIO* instance;

	private:
		std::shared_ptr<std::thread> listenThread;
		std::shared_ptr<std::thread> sendThread;
		std::shared_ptr<std::thread> recvThread;
		ExecutingState executingState;
		ConnectionState connectionState;
		SOCKET clientSocket;
		CriticalSection connectionLock;

		//送信キュー
		std::list<std::shared_ptr<JsonObject>> sendQueue;
		CriticalSection sendQueueLock;

		//ブレーク中のコマンドキュー
		std::list<std::shared_ptr<IBreakingCommand>> breakingCommandQueue;
		CriticalSection breakingCommandQueueLock;

		//ブレークポイント
		BreakPointCollection breakPoints;

		//最後にブレークした位置
		std::string lastBreakFullPath;
		uint32_t lastBreakLineIndex;
		size_t lastBreakStackLevel;

		//最後にAST通過した位置
		std::string lastPassFullPath;
		uint32_t lastPassLineIndex;
		size_t lastPassStackLevel;

	private:
		void ListenThread();
		void SendThread();
		void RecvThread();
		void ClearState();

		void SendBreakInformation(const ASTNodeBase& node, ScriptExecuteContext& executeContext);

		void AddBreakingCommand(IBreakingCommand* command) {
			LockScope ls(breakingCommandQueueLock);
			breakingCommandQueue.push_back(std::shared_ptr<IBreakingCommand>(command));
		}

	public:
		//AST実行時の通知およびトラップ
		void NotifyASTExecute(const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext);
		void NotifyRequest(const JsonObject& jsonObject);
		
		static void Create() { assert(instance == nullptr); if (instance == nullptr) { instance = new DebugIO(); } }
		static void Destroy() { assert(instance != nullptr); if (instance != nullptr) { delete instance; instance = nullptr; } }
		static DebugIO* Get() { return instance; }

		void Send(const std::shared_ptr<JsonObject>& sendObj);
		void SendResponse(const std::shared_ptr<JsonObject>& responseBody, const std::string& requestId);

		JsonObjectRef RequestScopes(const std::string& scopeType, uint32_t stackIndex, BreakSession& breakSession);	//TODO: こちらは削除
		JsonObjectRef RequestMembers(uint32_t handle, BreakSession& breakSession);
		JsonObjectRef RequestScopes(uint32_t stackIndex, BreakSession& breakSession);

		DebugIO();
		~DebugIO();
	};

	DebugIO* DebugIO::instance = nullptr;

	


	DebugIO::DebugIO()
	{
		executingState = ExecutingState::Execute;
		connectionState = ConnectionState::Disconnected;

		WSADATA wsaData;

		//通信系の起動
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0) {
			assert(false);
			return;
		}

		//リッスンスレッド起動
		listenThread.reset(new std::thread(
			[this]() {ListenThread();}
		));
		SetThreadDescription(listenThread->native_handle(), L"Aosora Debug Listen");
	}

	DebugIO::~DebugIO()
	{
		//終了処理
		WSACleanup();
	}

	void DebugIO::ClearState()
	{
		//ステートフルな内容をクリアする
		breakPoints.ClearBreakPoint();
		lastBreakFullPath = "";
		lastBreakLineIndex = 0;
		lastBreakStackLevel = 0;
		lastPassFullPath = "";
		lastPassLineIndex = 0;
		lastPassStackLevel = 0;
		executingState = ExecutingState::Execute;
		connectionState = ConnectionState::Disconnected;
	}

	void DebugIO::ListenThread()
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

		//ローカル向けに開く
		if (getaddrinfo("127.0.0.1", "27016", &hints, &result) != 0) {
			assert(false);
			return;
		}

		SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (listenSocket == INVALID_SOCKET) {
			assert(false);
			freeaddrinfo(result);
			return;
		}

		//TODO: 重複で開こうとするとここで失敗するので、うまく失敗させる流れをつくりたい
		if (bind(listenSocket, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR) {
			int err = WSAGetLastError();
			assert(false);
			freeaddrinfo(result);
			closesocket(listenSocket);
			return;
		}

		freeaddrinfo(result);

		//リッスン
		if (listen(listenSocket, SOMAXCONN) != 0) {
			closesocket(listenSocket);
			return;
		}

		//accept
		clientSocket = accept(listenSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET) {
			closesocket(listenSocket);
			return;
		}

		//クライアントは１個でいいのでlistenソケットをクローズ
		closesocket(listenSocket);

		//ステートを接続状態に
		connectionState = ConnectionState::Connected;

		//送受信スレッドを開始
		sendThread.reset(new std::thread(
			[this](){SendThread();}
		));
		
		recvThread.reset(new std::thread(
			[this](){RecvThread();}
		));
		SetThreadDescription(sendThread->native_handle(), L"Aosora Debug Send");
		SetThreadDescription(recvThread->native_handle(), L"Aosora Debug Receive");
	}

	//ASTノード実行時
	void DebugIO::NotifyASTExecute(const ASTNodeBase& node, ScriptExecuteContext& executeContext) {

		bool isBreak = false;

		//エディタスタックレベルの確認
		auto stackLevel = executeContext.MakeStackTrace(node, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName()).size();
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
				}
			}
		}

		if (isBreak) {

			//ブレーク位置を記憶
			lastBreakFullPath = fullPath;
			lastBreakLineIndex = line;
			lastBreakStackLevel = stackLevel;

			//ブレークセッションを作成
			BreakSession breakSession(node, executeContext);

			//ブレーク情報を送る
			SendBreakInformation(node, executeContext);

			//続行系の指示やその他情報要求系の指示を受ける
			while (connectionState == ConnectionState::Connected) {
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
					//TODO: 必要に応じてい停止する仕組みが必要そう
					Sleep(0);
				}
			}
		}
	}

	void DebugIO::SendBreakInformation(const ASTNodeBase& node, ScriptExecuteContext& executeContext) {
		//ブレークポイントに引っかかったことを通知
		auto data = JsonSerializer::MakeObject();
		data->Add("type", JsonSerializer::From("break"));

		auto body = JsonSerializer::MakeObject();
		data->Add("body", body);
		data->Add("responseId", JsonSerializer::From(""));

		body->Add("filename", JsonSerializer::From(node.GetSourceRange().GetSourceFileFullPath()));
		body->Add("line", JsonSerializer::From(node.GetSourceRange().GetBeginLineIndex()));


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
					for (auto& kvp : scope->GetLocalVariableCollection()) {
						if (!addedSet.contains(kvp.first)) {
							result->Add(MakeVariableInfo(kvp.first, kvp.second, breakSession));
							addedSet.insert(kvp.first);
						}
					}
					scope = scope->GetParentScope();
				}
				break;
			}
			
			nativeStackLevel++;
		}

		

		return result;
	}

	JsonArrayRef EnumShioriRequests(BreakSession& breakSession) {
		//TODO: nullチェックを
		ScriptInterpreter& interpreter = breakSession.GetContext().GetInterpreter();
		auto shioriVariable = interpreter.GetGlobalVariable("Shiori");
		ScriptObject* shioriObj = interpreter.InstanceAs<ScriptObject>(shioriVariable);

		auto referenceVariable = shioriObj->RawGet("Reference");
		ScriptArray* referenceArray = interpreter.InstanceAs<ScriptArray>(referenceVariable);

		//Referenceを優先で収集（多分１番必要なので）
		std::set<std::string> addedSet;
		JsonArrayRef result = JsonSerializer::MakeArray();
		for (size_t i = 0; i < referenceArray->Count(); i++) {
			std::string key = std::string("Reference") + std::to_string(i);
			auto elem = referenceArray->At(i);
			result->Add(MakeVariableInfo(key, elem, breakSession));
			addedSet.insert(key);
		}

		//そのあと把握してないヘッダを取り込む
		auto headersVariable = shioriObj->RawGet("Headers");
		ScriptObject* headersObj = interpreter.InstanceAs<ScriptObject>(headersVariable);
		for (auto kvp : headersObj->GetInternalCollection()) {
			//Referenceは重複なのでパス
			if (!addedSet.contains(kvp.first)) {
				result->Add(MakeVariableInfo(kvp.first, kvp.second, breakSession));
			}
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

		//TODO: オブジェクトのデータ型によってそれぞれ分解
		//const uint32_t typeId = obj->GetInstanceTypeId();

		//if(typeId == breakSession.GetContext().GetInterpreter().GetClassId()
		ScriptInterpreter& interpreter = breakSession.GetContext().GetInterpreter();

		//もうちょっと良い分岐の仕方はあるだろうけどとりあえず
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

		//対応する型がない: 本来展開できないものについてはハンドルを発行しないはず⋯。
		assert(false);
		return JsonSerializer::MakeArray();
	}

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
			obj->Add("name", JsonSerializer::From("ローカル変数"));
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

		return result;
	}

	

	void DebugIO::NotifyRequest(const JsonObject& json) {
		std::string requestType;
		std::string requestId;

		if (!JsonSerializer::As(json.Get("type"), requestType)) {
			//対応なし
			assert(false);
			return;
		}

		//ID情報があれば収集（レスポンス時に起因となったIDを指定して返すため）
		JsonSerializer::As(json.Get("id"), requestId);

		//リクエストボディオブジェクトを取得
		JsonObjectRef body;
		if (!JsonSerializer::As(json.Get("body"), body)) {
			assert(false);
			return;
		}
		
		//リクエストタイプごとに処理を分ける
		if (requestType == "set_breakpoints") {

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

			//WARN: VSCodeがドライブレターを小文字にするので一旦無理やり大文字にする
			if (filename.size() > 0) {
				filename[0] = toupper(filename[0]);
			}

			if (!lines.empty()) {
				breakPoints.SetBreakPoints(filename, &lines.at(0), lines.size());
			}
			else {
				breakPoints.SetBreakPoints(filename, nullptr, 0);
			}
			return;
		}
		else if (requestType == "continue") {
			//続行指示
			AddBreakingCommand(new ExecuteStateCommand(ExecutingState::Execute));
			return;
		}
		else if (requestType == "stepin") {
			AddBreakingCommand(new ExecuteStateCommand(ExecutingState::StepIn));
			return;
		}
		else if (requestType == "stepover") {
			AddBreakingCommand(new ExecuteStateCommand(ExecutingState::StepOver));
			return;
		}
		else if (requestType == "stepout") {
			AddBreakingCommand(new ExecuteStateCommand(ExecutingState::StepOut));
			return;
		}
		else if (requestType == "members") {
			//メンバの展開
			//TODO: スコープは通信で受け取るのではなく、あらかじめscopesリクエストで払い出したメタデータのハンドルを使用する形になる
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

		//returnされなかった場合は処理できなかったものとして扱う、エラーレスポンスとして返す
		{
			auto responseData = JsonSerializer::MakeObject();
			responseData->Add("type", JsonSerializer::From("error_response"));
			responseData->Add("responseId", JsonSerializer::From(requestId));
			Send(responseData);
		}
	}

	JsonObjectRef DebugIO::RequestScopes(const std::string& scopeType, uint32_t stackIndex, BreakSession& breakSession) {

		JsonObjectRef responseBody = JsonSerializer::MakeObject();
		if (scopeType == "shiori_request") {
			responseBody->Add("variables", EnumShioriRequests(breakSession));
		}
		else if (scopeType == "locals") {
			responseBody->Add("variables", EnumLocalVariables(stackIndex,breakSession));
		}
		else {
			//把握してないリクエスト
			assert(false);
			responseBody->Add("variables", JsonSerializer::MakeArray());
		}
		return responseBody;
	}

	JsonObjectRef DebugIO::RequestMembers(uint32_t handle, BreakSession& breakSession) {

		JsonObjectRef responseBody = JsonSerializer::MakeObject();
		responseBody->Add("variables", EnumMembers(handle, breakSession));
		return responseBody;
	}

	JsonObjectRef DebugIO::RequestScopes(uint32_t stackIndex, BreakSession& breakSession) {
		auto responseBody = JsonSerializer::MakeObject();
		responseBody->Add("scopes", EnumScopes(stackIndex, breakSession));
		return responseBody;
	}

	void DebugIO::Send(const std::shared_ptr<JsonObject>& obj) {
		LockScope ls(sendQueueLock);
		sendQueue.push_back(obj);
	}

	void DebugIO::SendResponse(const std::shared_ptr<JsonObject>& body, const std::string& requestId) {
		auto responseData = JsonSerializer::MakeObject();
		responseData->Add("type", JsonSerializer::From("response"));
		responseData->Add("responseId", JsonSerializer::From(requestId));
		responseData->Add("body", body);
		Send(responseData);
	}

	void DebugIO::SendThread()
	{
		//送信ループ
		while (clientSocket != INVALID_SOCKET) {
			
			std::shared_ptr<JsonObject> sendObject = nullptr;
			{
				//キューからいっことりだす
				LockScope ls(sendQueueLock);
				if (!sendQueue.empty()) {
					sendObject = *sendQueue.begin();
					sendQueue.pop_front();
				}
			}

			if (sendObject != nullptr) {
				//文字列にシリアライズしてそのまま送信
				const std::string sendStr = JsonSerializer::Serialize(sendObject);
				if (send(clientSocket, sendStr.c_str(), sendStr.size(), 0) == SOCKET_ERROR) {
					//送信失敗
					int errorCode = WSAGetLastError();
					assert(false);
					break;
				}
			}
			else {
				//送信することがなかったのでCPUを休める
				//TODO: イベントで待つなど負荷下げる対応をいれたい?
				Sleep(1);
			}
		}
	}

	//受信スレッド側
	void DebugIO::RecvThread()
	{
		//バッファサイズ: 足りなくなったら増やすようにしていく
		int recvBufferSize = 1024;
		char* buffer = static_cast<char*>(malloc(recvBufferSize));

		while (true) {
			//受信処理
			const int size = recv(clientSocket, (buffer), recvBufferSize, 0);
			if (size > 0) {
				//受信成功

				//Jsonデシリアライズ
				JsonDeserializeResult deserializeResult = JsonSerializer::Deserialize(std::string(buffer, size));

				//デシリアライズに成功していたら処理に回す、失敗していたらそのフォーマットは想定してないので捨てる
				if (deserializeResult.success && deserializeResult.token->GetType() == JsonTokenType::Object) {
					NotifyRequest(*static_cast<JsonObject*>(deserializeResult.token.get()));
				}
			}
			else if (size == 0) {
				//切断
				break;
			}
			else {
				//受信失敗
				int errorCode = WSAGetLastError();
				if (errorCode == WSAEMSGSIZE) {
					//サイズ不足なのでサイズを倍にして再試行する
					recvBufferSize *= 2;
					free(buffer);
					buffer = static_cast<char*>(malloc(recvBufferSize));
				}
				else {
					//切断？
					assert(false);
					break;
				}
			}
		}

		//最終的なバッファの削除
		free(buffer);

		{
			LockScope ls(connectionLock);

			//切断処理
			closesocket(clientSocket);
			clientSocket = INVALID_SOCKET;	//TODO: ここも終了指示の方法を排他制御つきでやること

			//ブレーク中なら再開
			ClearState();

			sendThread->join();
			listenThread->join();

			//リッスンに戻す
			//TODO: 繰り返すとキャプチャのチェーンが長くなりそうなのが気になる。キャプチャなしで開始する方法をけんとうすべきかも
			listenThread.reset(new std::thread(
				[this]() {ListenThread(); }
			));
			SetThreadDescription(listenThread->native_handle(), L"Aosora Debug Listen");
		}
	}


	//デバッグフック
	void Debugger::NotifyASTExecute(const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext) {
		if (DebugIO::Get() != nullptr) {
			//ブレークポイント要求に対して反応する想定
			DebugIO::Get()->NotifyASTExecute(executingNode, executeContext);
		}
	}

	void Debugger::Create() {
		DebugIO::Create();
	}

	void Debugger::Destroy() {
		DebugIO::Destroy();
	}
}