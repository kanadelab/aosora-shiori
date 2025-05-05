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

	enum class ExecutingState {
		Execute,
		Break
	};

	enum class ConnectionState {
		Disconnected,
		Listen,
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

	//デバッグ通信系
	//TODO: ソケット通信系とデバッグ処理系をきりはなしたほうがいいかも
	class DebugIO {
	private:
		static DebugIO* instance;

	private:
		std::shared_ptr<std::thread> listenThread;
		std::shared_ptr<std::thread> sendThread;
		std::shared_ptr<std::thread> recvThread;
		ExecutingState executingState;
		SOCKET clientSocket;
		CriticalSection connectionLock;

		//送信キュー
		std::list<std::shared_ptr<JsonObject>> sendQueue;
		CriticalSection sendQueueLock;

		//ブレークポイント
		BreakPointCollection breakPoints;
		SyncEvent breakingEvent;

		//現在ブレーク中の位置
		std::string lastBreakFullPath;
		uint32_t lastBreakLineIndex;
		ScriptExecuteContext* breakingContext;

	private:
		void ListenThread();
		void SendThread();
		void RecvThread();
		void ClearState();

	public:
		//AST実行時の通知およびトラップ
		void NotifyASTExecute(const ASTNodeBase& executingNode, ScriptExecuteContext& executeContext);
		void NotifyRequest(const JsonObject& jsonObject);
		
		static void Create() { assert(instance == nullptr); if (instance == nullptr) { instance = new DebugIO(); } }
		static void Destroy() { assert(instance != nullptr); if (instance != nullptr) { delete instance; instance = nullptr; } }
		static DebugIO* Get() { return instance; }

		void Send(const std::shared_ptr<JsonObject>& sendObj);
		JsonObjectRef RequestScopes(const JsonObjectRef& request);

		DebugIO();
		~DebugIO();
	};

	DebugIO* DebugIO::instance = nullptr;

	


	DebugIO::DebugIO()
	{
		executingState = ExecutingState::Execute;
		breakingContext = nullptr;

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
		executingState = ExecutingState::Execute;
		breakingContext = nullptr;
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

		//ブレークポイントに引っかかるかを検証
		if (breakPoints.QueryBreakPoint(node.GetSourceRange().GetSourceFileFullPath(), node.GetSourceRange().GetBeginLineIndex() )) {

			const std::string fullPath = node.GetSourceRange().GetSourceFileFullPath();
			const uint32_t line = node.GetSourceRange().GetBeginLineIndex();

			//同じ位置では連続で停止しない
			if (fullPath == lastBreakFullPath && line == lastBreakLineIndex) {
				return;
			}

			lastBreakFullPath = fullPath;
			lastBreakLineIndex = line;

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
			auto stackFrame = executeContext.MakeStackTrace(node, executeContext.GetStack().GetFunctionName());
			uint32_t stackLevel = 0;
			for (auto& frame : stackFrame) {
				if (!frame.hasSourceRange) {
					continue;	//スクリプトのスタックフレームのみ
				}

				//行番号等を格納
				auto f = JsonSerializer::MakeObject();
				f->Add("index", JsonSerializer::From(stackLevel));
				f->Add("name", JsonSerializer::From(frame.funcName));
				f->Add("filename", JsonSerializer::From(frame.sourceRange.GetSourceFileFullPath()));
				f->Add("line", JsonSerializer::From(frame.sourceRange.GetBeginLineIndex()));
				stackArray->Add(f);
				stackLevel++;
			}

			body->Add("stackTrace", stackArray);

			Send(data);

			//ブレーク停止中はスレッドをそこで止める
			executingState = ExecutingState::Break;
			breakingContext = &executeContext;
			breakingEvent.Wait();
			breakingContext = nullptr;
			executingState = ExecutingState::Execute;
		}
		else {
			//通過位置を記憶
			const std::string fullPath = node.GetSourceRange().GetSourceFileFullPath();
			const uint32_t line = node.GetSourceRange().GetBeginLineIndex();

			lastBreakFullPath = fullPath;
			lastBreakLineIndex = line;
		}
	}

	JsonObjectRef MakeVariableInfo(const std::string& name, const ScriptValueRef& value, ScriptInterpreter& interpreter) {
		auto variableInfo = JsonSerializer::MakeObject();
		variableInfo->Add("key", JsonSerializer::From(name));
		std::string primitiveType;
		std::string objectType;

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
			variableInfo->Add("value", JsonSerializer::MakeNull());
		}

		variableInfo->Add("primitiveType", JsonSerializer::From(primitiveType));
		variableInfo->Add("objectType", JsonSerializer::From(objectType));
		return variableInfo;
	}

	JsonArrayRef EnumLocalVariables(ScriptExecuteContext& context) {
		ScriptInterpreterStack& stackFrame = context.GetStack();

		//TODO: ここでブロックスコープが取得できるものの、スタックフレームには紐づいてないので追跡手段が必要。
		//デバッグ情報のような形でもいいのかも
		std::set<std::string> addedSet;
		JsonArrayRef result = JsonSerializer::MakeArray();

		Reference<BlockScope> scope = context.GetBlockScope();
		while (scope != nullptr) {
			for (auto& kvp : scope->GetLocalVariableCollection()) {
				if (!addedSet.contains(kvp.first)) {
					result->Add(MakeVariableInfo(kvp.first, kvp.second, context.GetInterpreter()));
					addedSet.insert(kvp.first);
				}
			}
			scope = scope->GetParentScope();
		}

		return result;
	}

	JsonArrayRef EnumShioriRequests(ScriptExecuteContext& context) {
		//TODO: nullチェックを
		auto shioriVariable = context.GetInterpreter().GetGlobalVariable("Shiori");
		ScriptObject* shioriObj = context.GetInterpreter().InstanceAs<ScriptObject>(shioriVariable);
		
		auto referenceVariable = shioriObj->RawGet("Reference");
		ScriptArray* referenceArray = context.GetInterpreter().InstanceAs<ScriptArray>(referenceVariable);

		//Referenceを優先で収集
		std::set<std::string> addedSet;
		JsonArrayRef result = JsonSerializer::MakeArray();
		for (size_t i = 0; i < referenceArray->Count(); i++) {
			std::string key = std::string("Reference") + std::to_string(i);
			auto elem = referenceArray->At(i);
			result->Add(MakeVariableInfo(key, elem, context.GetInterpreter()));
			addedSet.insert(key);
		}

		//そのあと把握してないヘッダを取り込む
		auto headersVariable = shioriObj->RawGet("Headers");
		ScriptObject* headersObj = context.GetInterpreter().InstanceAs<ScriptObject>(headersVariable);
		for (auto kvp : headersObj->GetInternalCollection()) {
			if (!addedSet.contains(kvp.first)) {
				result->Add(MakeVariableInfo(kvp.first, kvp.second, context.GetInterpreter()));
			}
		}

		return result;
	}

	void DebugIO::NotifyRequest(const JsonObject& json) {
		std::string requestType;

		if (!JsonSerializer::As(json.Get("type"), requestType)) {
			//対応なし
			assert(false);
			return;
		}

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
		}
		else if (requestType == "continue") {
			//続行指示
			breakingEvent.Raise();
		}
		else if (requestType == "request_variable_scope") {
			//スコープ情報
			JsonObjectRef response = JsonSerializer::MakeObject();
			response->Add("type", JsonSerializer::From("response"));
			response->Add("responseId", json.Get("id"));
			response->Add("body", RequestScopes(body));
			Send(response);
		}


		//このほか、ステップインやオーバー、ウォッチが考えられる
	}

	JsonObjectRef DebugIO::RequestScopes(const JsonObjectRef& body) {

		std::string scopeName;
		JsonObjectRef responseBody = JsonSerializer::MakeObject();
		if (JsonSerializer::As(body->Get("scope"), scopeName)) {
			if (scopeName == "shiori_request") {
				responseBody->Add("variables", EnumShioriRequests(*breakingContext));
			}
			else if (scopeName == "locals") {
				responseBody->Add("variables", EnumLocalVariables(*breakingContext));
			}
			else {
				//把握してないリクエスト
				assert(false);
				responseBody->Add("variables", JsonSerializer::MakeArray());
			}
		}
		return responseBody;
	}

	void DebugIO::Send(const std::shared_ptr<JsonObject>& obj) {
		LockScope ls(sendQueueLock);
		sendQueue.push_back(obj);
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
			//TODO: 排他制御周辺は要確認
			if (executingState == ExecutingState::Break) {
				ClearState();
				breakingEvent.Raise();
			}
			else {
				ClearState();
			}

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