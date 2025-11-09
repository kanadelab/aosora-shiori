//#include <ctime>
#include "Shiori.h"
#include "AST/AST.h"
#include "Interpreter/Interpreter.h"
#include "Misc/Json.h"
#include "Misc/Utility.h"
#include "Misc/SaoriLoader.h"
#include "Misc/PluginLoader.h"
#include "CommonLibrary/PluginContextManager.h"
#include "Debugger/Debugger.h"

namespace sakura {

	ScriptValueRef ObjectSerializer::Deserialize(const std::string& json, ScriptInterpreter& interpreter) {
		auto jsonResult = JsonSerializer::Deserialize(json);
		if (!jsonResult.success) {
			//jsonデシリアライズ失敗
			return nullptr;
		}
		return Deserialize(jsonResult.token, interpreter);
	}

	ScriptValueRef ObjectSerializer::Deserialize(const std::shared_ptr<JsonTokenBase>& token, ScriptInterpreter& interpreter) {

		switch (token->GetType())
		{
			case JsonTokenType::Number:
				return ScriptValue::Make(std::static_pointer_cast<JsonPrimitive>(token)->GetNumber());
			case JsonTokenType::Boolean:
				return ScriptValue::Make(std::static_pointer_cast<JsonPrimitive>(token)->GetBoolean());
			case JsonTokenType::String:
				return ScriptValue::Make(std::static_pointer_cast<JsonString>(token)->GetString());
			case JsonTokenType::Null:
				return ScriptValue::Null;
			case JsonTokenType::Object:
				return ScriptValue::Make(DeserializeObject(token, interpreter));
			case JsonTokenType::Array:
				return ScriptValue::Make(DeserializeArray(token, interpreter));
			default:
				assert(false);
				return ScriptValue::Null;
		}
	}

	//json->objref
	Reference<ScriptObject> ObjectSerializer::DeserializeObject(const std::shared_ptr<JsonTokenBase>& token, ScriptInterpreter& interpreter) {

		//Objectを作って渡す
		Reference<ScriptObject> result = interpreter.CreateObject();

		auto obj = std::static_pointer_cast<JsonObject>(token);
		for (const auto& item : obj->GetCollection()) {
			result->RawSet(item.first, Deserialize(item.second, interpreter));
		}

		return result;
	}

	Reference<ScriptArray> ObjectSerializer::DeserializeArray(const std::shared_ptr<JsonTokenBase>& token, ScriptInterpreter& interpreter) {
		
		Reference<ScriptArray> result = interpreter.CreateNativeObject<ScriptArray>();

		auto obj = std::static_pointer_cast<JsonArray>(token);
		for (const auto& item : obj->GetCollection()) {
			result->Add(Deserialize(item, interpreter));
		}

		return result;
	}

	//obj->json
	std::string ObjectSerializer::Serialize(const ObjectRef& obj) {
		auto jsonObj = SerializeObject(obj);
		return JsonSerializer::Serialize(jsonObj);
	}

	std::shared_ptr<JsonTokenBase> ObjectSerializer::Serialize(const ScriptValueRef& value) {

		switch (value->GetValueType()) {
			case ScriptValueType::Number:
				return std::shared_ptr<JsonTokenBase>(new JsonPrimitive(value->ToNumber()));
			case ScriptValueType::Boolean:
				return std::shared_ptr<JsonTokenBase>(new JsonPrimitive(value->ToBoolean()));
			case ScriptValueType::Null:
				return std::shared_ptr<JsonTokenBase>(new JsonPrimitive());
			case ScriptValueType::String:
				return std::shared_ptr<JsonTokenBase>(new JsonString(value->ToString()));
			case ScriptValueType::Object:
				if (value->GetObjectInstanceTypeId() == ScriptArray::TypeId()) {
					return SerializeArray(value->GetObjectRef().template Cast<ScriptArray>());
				}
				else {
					return SerializeObject(value->GetObjectRef());
				}
		}
		assert(false);
		return nullptr;
	}

	std::shared_ptr<JsonArray> ObjectSerializer::SerializeArray(const Reference<ScriptArray>& obj) {
		std::shared_ptr<JsonArray> result(new JsonArray());

		//ScriptArrayをjsonシリアライズ
		for (size_t i = 0; i < obj->Count(); i++) {
			result->Add(Serialize(obj->At(i)));
		}
		return result;
	}

	std::shared_ptr<JsonObject> ObjectSerializer::SerializeObject(const ObjectRef& obj) {

		std::shared_ptr<JsonObject> result(new JsonObject());

		//ScriptObjectのみシリアライズ可能とする
		//クラス自体の復元が困難なので、単純なキーバリューストアとしてのみやりとりする目的
		if (obj->GetInstanceTypeId() == ScriptObject::TypeId()) {
			Reference<ScriptObject> sobj = obj.template Cast<ScriptObject>();
			for (const auto& item : sobj->GetInternalCollection()) {
				result->Add(item.first, Serialize(item.second));
			}
		}

		return result;
	}

	

	//セーブデータ読み込み書き込み系

	void SaveData::StaticSet(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		if (key == "Data") {
			//セーブデータオブジェクトを上書き
			if (value->GetObjectInstanceTypeId() == ScriptObject::TypeId()) {
				executeContext.GetInterpreter().SetStaticStore<SaveData>(value->GetObjectRef().template Cast<ScriptObject>());
			}
		}
	}

	ScriptValueRef SaveData::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "Data") {
			return ScriptValue::Make(executeContext.GetInterpreter().StaticStore<SaveData>());
		}
		return nullptr;
	}

	void SaveData::Load(ScriptInterpreter& interpreter) {
		
		//"savedata.asv" から読み込むなど
		auto saveObject = interpreter.StaticStore<SaveData>();

		std::string fileBody;
		File::ReadAllText(interpreter.GetFileName("savedata.asv"), fileBody);

		//jsonをデシリアライズ
		ScriptValueRef deserialized = ObjectSerializer::Deserialize(fileBody, interpreter);

		//ScriptObjectであればそれを使う
		if (deserialized != nullptr && deserialized->GetObjectInstanceTypeId() == ScriptObject::TypeId()) {
			Reference<ScriptObject> saveData = deserialized->GetObjectRef().template Cast<ScriptObject>();
			for (auto item : saveData->GetInternalCollection()) {
				saveObject->RawSet(item.first, item.second);
			}
		}
	}

	void SaveData::Save(ScriptInterpreter& interpreter) {

		auto saveObject = interpreter.StaticStore<SaveData>();

		//jsonシリアライズ
		std::string json;
		json = ObjectSerializer::Serialize(saveObject);

		//savedata.asv に保存
		File::WriteAllText(interpreter.GetFileName("savedata.asv"), json);
	}


	//トークタイマー
	const char* TalkTimer::KeyRandomTalk = "RandomTalk";
	const char* TalkTimer::KeyRandomTalkIntervalSeconds = "RandomTalkIntervalSeconds";
	const char* TalkTimer::KeyRandomTalkElapsedSeconds = "RandomTalkElapsedSeconds";
	const char* TalkTimer::KeyRandomTalkQueue = "RandomTalkQueue";

	const char* TalkTimer::KeyNadenadeTalk = "NadenadeTalk";
	const char* TalkTimer::KeyNadenadeMoveCount = "NadenadeMoveCount";
	const char* TalkTimer::KeyNadenadeMoveThreshold = "NadenadeMoveThreshold";
	const char* TalkTimer::KeyNadenadeActiveCollision = "NadenadeActiveCollision";

	
	bool TalkTimer::HandleEvent(ScriptInterpreter& interpreter, FunctionResponse& response, const ShioriRequest& shioriRequest, bool hasResponseTalk) {
		//レスポンスがすでに設定されている場合はトークを呼ばない
		bool canTalk = !hasResponseTalk;

		if (shioriRequest.GetEventId() == "OnSecondChange") {
			return OnSecondChange(interpreter, response, shioriRequest, canTalk);
		}
		else if (shioriRequest.GetEventId() == "OnMouseMove") {
			return OnMouseMove(interpreter, response, shioriRequest, canTalk);
		}
		else if (shioriRequest.GetEventId() == "OnMouseLeave") {
			ClearMouseMove(interpreter);
			return false;
		}
		else if (shioriRequest.GetEventId() == "OnMouseHover") {
			//hoverが来たらいリセットにする
			ClearMouseMove(interpreter);
			return false;
		}

		return false;
	}

	bool TalkTimer::OnSecondChange(ScriptInterpreter& interpreter, FunctionResponse& response, const ShioriRequest& shioriRequest, bool canCallRandomTalk) {
		auto staticStore = interpreter.StaticStore<TalkTimer>();

		//発話不可能な場合にはReference3に0が入る
		if (shioriRequest.GetReferenceCount() >= 4 && shioriRequest.GetReference(3) == "0") {
			canCallRandomTalk = false;
		}

		//記録した経過時間を取得、数値として不正なら0相当として読む
		auto secondsVal = staticStore->RawGet(KeyRandomTalkElapsedSeconds);
		number seconds = secondsVal != nullptr ? secondsVal->ToNumber() : 0.0;

		//数値評価できないものがきていればリセットにする
		if (std::isnan(seconds)) {
			seconds = 0.0;
		}

		//加算
		seconds += 1.0;

		//ランダムトーク発動秒を取得
		auto intervalVal = staticStore->RawGet(KeyRandomTalkIntervalSeconds);
		number interval = intervalVal != nullptr ? intervalVal->ToNumber() : 0.0;
		if (std::isnan(interval)) {
			interval = 0.0;
		}

		//必要秒数を超えていればトークを発生
		if (canCallRandomTalk && interval > 0.0 && seconds >= interval) {
			//呼び出しを実行したかどうかを返す
			return CallRandomTalk(interpreter, response);
		}
		else {
			//経過秒を書き戻す
			staticStore->RawSet(KeyRandomTalkElapsedSeconds, ScriptValue::Make(seconds));
			return false;
		}
	}

	//ランダムトークの呼び出し要求
	bool TalkTimer::CallRandomTalk(ScriptInterpreter& interpreter, FunctionResponse& response) {
		auto staticStore = interpreter.StaticStore<TalkTimer>();
		ScriptValueRef talkVal = nullptr;

		//トークキューにトークがあるかを確認
		auto talkQueue = staticStore->RawGet(KeyRandomTalkQueue);
		ScriptArray* queue = interpreter.InstanceAs<ScriptArray>(talkQueue);
		if (queue && queue->Count() > 0) {

			//トークを取得してキューから取り除く
			talkVal = queue->At(0);
			queue->Remove(0);
		}

		if (talkVal == nullptr) {
			//トークをリクエストする
			talkVal = staticStore->RawGet(KeyRandomTalk);
		}

		if (talkVal != nullptr) {
			//ランダムトーク呼出
			std::vector<ScriptValueRef> args;
			interpreter.CallFunction(*talkVal, response, args);

			//秒数をリセット
			ClearRandomTalkInterval(interpreter);
			return true;
		}
		return false;
	}

	void TalkTimer::ClearRandomTalkInterval(ScriptInterpreter& interpreter) {
		//間隔をクリアする
		interpreter.StaticStore<TalkTimer>()->RawSet(KeyRandomTalkElapsedSeconds, ScriptValue::Make(0.0));
	}

	//ランダムトークを起動する
	void TalkTimer::ScriptCallRandomTalk(const FunctionRequest& request, FunctionResponse& response) {
		CallRandomTalk(request.GetContext().GetInterpreter(), response);
	}

	//ランダムトーク間隔のクリア
	void TalkTimer::ScriptClearTalkInterval(const FunctionRequest& request, FunctionResponse& response) {
		ClearRandomTalkInterval(request.GetContext().GetInterpreter());
	}

	bool TalkTimer::OnMouseMove(ScriptInterpreter& interpreter, FunctionResponse& response, const ShioriRequest& shioriRequest, bool canCallTalk) {
		if (shioriRequest.GetReferenceCount() < 5) {
			return false;
		}

		auto staticStore = interpreter.StaticStore<TalkTimer>();
		bool called = false;

		//コリジョン名
		std::string collisionName = shioriRequest.GetReference(4);
		auto activeCollision = staticStore->RawGet(KeyNadenadeActiveCollision);
		if (activeCollision == nullptr) {
			activeCollision = ScriptValue::Null;
		}

		//押した判定位置が変わっていたら最初から判定をとる
		auto collisionNameValue = ScriptValue::Make(collisionName);
		if (!activeCollision->IsEquals(collisionNameValue)) {
			//なでなで回数をリセット
			staticStore->RawSet(KeyNadenadeActiveCollision, collisionNameValue);
			staticStore->RawSet(KeyNadenadeMoveCount, ScriptValue::Make(0.0));
		}
		
		//なでなで回数を更新
		auto nadenadeCount = staticStore->RawGet(KeyNadenadeMoveCount);
		number count = nadenadeCount != nullptr ? nadenadeCount->ToNumber() : 0.0;
		if (std::isnan(count)) {
			count = 0.0;
		}
		count += 1.0;

		//なでなで回数のしきい値を超えているか確認
		auto nadenadeThreshold = staticStore->RawGet(KeyNadenadeMoveThreshold);
		number threshold = nadenadeThreshold != nullptr ? nadenadeThreshold->ToNumber() : 0.0;
		if (std::isnan(threshold)) {
			threshold = 0.0;
		}

		//トーク可能であればイベントを呼び出す
		if (canCallTalk && threshold > 0.0 && count >= threshold) {

			//トークをリクエストする
			auto talkVal = staticStore->RawGet(KeyNadenadeTalk);

			if (talkVal != nullptr && talkVal->IsObject()) {
				//ランダムトーク呼出
				std::vector<ScriptValueRef> args;
				interpreter.CallFunction(*talkVal, response, args);

				//カウントをリセット
				count = 0.0;
				called = true;
			}

			count = 0.0;
			called = true;
		}

		//カウントの書き戻し
		staticStore->RawSet(KeyNadenadeMoveCount, ScriptValue::Make(count));
		
		return called;
	}

	void TalkTimer::ClearMouseMove(ScriptInterpreter& interpreter) {
		//なでなでの状態をリセット
		interpreter.StaticStore<TalkTimer>()->RawSet(KeyNadenadeActiveCollision, ScriptValue::Null);
	}

	void TalkTimer::StaticInit(ScriptInterpreter& interpreter) {
		//デフォルト値として適当に
		auto staticStore = interpreter.StaticStore<TalkTimer>();
		staticStore->RawSet(KeyNadenadeMoveThreshold, ScriptValue::Make(50.0));
		staticStore->RawSet(KeyRandomTalkQueue, ScriptValue::Make(interpreter.CreateNativeObject<ScriptArray>()));
	}

	void TalkTimer::StaticSet(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		//存在してるキーにだけアクセスを許容する
		if (
			key == KeyRandomTalk ||
			key == KeyRandomTalkElapsedSeconds ||
			key == KeyRandomTalkIntervalSeconds ||
			key == KeyRandomTalkQueue ||
			key == KeyNadenadeActiveCollision ||
			key == KeyNadenadeMoveCount ||
			key == KeyNadenadeMoveThreshold ||
			key == KeyNadenadeTalk
			) {
			executeContext.GetInterpreter().StaticStore<TalkTimer>()->RawSet(key, value);
		}
	}

	ScriptValueRef TalkTimer::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {

		//ランダムトーク要求
		if (key == "CallRandomTalk") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&TalkTimer::ScriptCallRandomTalk));
		}
		else if (key == "ClearTalkInterval") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&TalkTimer::ScriptCallRandomTalk));
		}

		//存在してるキーにだけアクセスを許容する
		if (
			key == KeyRandomTalk ||
			key == KeyRandomTalkElapsedSeconds ||
			key == KeyRandomTalkIntervalSeconds ||
			key == KeyRandomTalkQueue ||
			key == KeyNadenadeActiveCollision ||
			key == KeyNadenadeMoveCount ||
			key == KeyNadenadeMoveThreshold ||
			key == KeyNadenadeTalk
			) {
			return executeContext.GetInterpreter().StaticStore<TalkTimer>()->RawGet(key);
		}

		return nullptr;
	}


	//日付と時刻
	ScriptValueRef Time::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		
		//各種のパラメータを取得
		if (key == "GetNowHour") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowHour));
		}
		else if (key == "GetNowMinute") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowMinute));
		}
		else if (key == "GetNowSecond") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowSecond));
		}
		else if (key == "GetNowYear") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowYear));
		}
		else if (key == "GetNowMonth") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowMonth));
		}
		else if (key == "GetNowDate") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowDate));
		}
		else if (key == "GetNowDayOfWeek") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowDayOfWeek));
		}
		else if (key == "GetNowUnixEpoch") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowUnixEpoch));
		}

		return nullptr;
	}


	//ランダム
	ScriptValueRef Random::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "Select") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Random::Select));
		}
		else if (key == "GetNumber") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Random::GetNumber));
		}
		else if (key == "GetIndex") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Random::GetIndex));
		}
		else if (key == "CreateSelector") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Random::CreateSelector));
		}

		return nullptr;
	}

	void Random::Select(const FunctionRequest& request, FunctionResponse& response) {

		if (request.GetArgumentCount() <= 0) {
			response.SetReturnValue(ScriptValue::Null);
			return;
		}

		//配列から１つ選ぶ
		ScriptValueRef target = request.GetArgument(0);

		if (!target->IsObject()) {
			//配列でなければ打ち切り
			response.SetReturnValue(ScriptValue::Null);
			return;
		}

		//配列かどうかを確認
		ObjectRef obj = target->GetObjectRef();

		if (obj->GetInstanceTypeId() != ScriptArray::TypeId()) {
			response.SetReturnValue(ScriptValue::Null);
			return;
		}
		
		//配列からアイテムを1つランダム選択
		Reference<ScriptArray> sobj = obj.template Cast<ScriptArray>();
		const size_t size = sobj->Count();
		const int32_t result = Rand(0, static_cast<int32_t>(size));

		ScriptValueRef item = sobj->At(result);
		response.SetReturnValue(item != nullptr ? item : ScriptValue::Null);
	}

	void Random::GetIndex(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 2) {
			size_t randMin, randMax;
			if (request.GetArgument(0)->ToIndex(randMin) && request.GetArgument(1)->ToIndex(randMax)) {
				if (randMin > randMax) {
					std::swap(randMin, randMax);
				}
				else if (randMin == randMax) {
					response.SetReturnValue(ScriptValue::Make(static_cast<number>(randMin)));
					return;
				}

				response.SetReturnValue(ScriptValue::Make(static_cast<number>(Rand(randMin, randMax))));
			}
		}
	}

	void Random::GetNumber(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(RandNum()));
	}

	void Random::CreateSelector(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() > 0) {
			auto* items = request.GetInterpreter().InstanceAs<ScriptArray>(request.GetArgument(0));
			if (items != nullptr) {
				auto selector = request.GetInterpreter().CreateNativeObject<OverloadedFunctionList>();
				selector->SetName("(RandomSelector)");
				
				for (size_t i = 0; i < items->Count(); i++) {
					selector->Add(items->At(i));
				}

				response.SetReturnValue(ScriptValue::Make(selector));
			}
		}
		else {
			//引数がない場合は空のセレクタを返す
			auto selector = request.GetInterpreter().CreateNativeObject<OverloadedFunctionList>();
			selector->SetName("(RandomSelector)");
			response.SetReturnValue(ScriptValue::Make(selector));
		}
	}


	void SaoriManager::Load(const FunctionRequest& request, FunctionResponse& response) {
#if defined(AOSORA_ENABLE_SAORI_LOADER)
		if (request.GetArgumentCount() > 0) {
			//指定パスでSAORIをロードして、SAORIオブジェクトを返す
			std::string saoriRelativePath = request.GetArgument(0)->ToString();

			//ロード済みのSAORIをチェックする、ロード済みならそのまま返す
			auto staticStore = request.GetContext().GetInterpreter().StaticStore<SaoriManager>();
			auto loadedSaori = staticStore->RawGet(saoriRelativePath);
			if (loadedSaori != nullptr) {
				response.SetReturnValue(loadedSaori);
				return;
			}

			std::string saoriPath = request.GetContext().GetInterpreter().GetWorkingDirectory() + saoriRelativePath;

			//SAORIのロードを試みる
			auto loadResult = LoadSaori(saoriPath);
			if (loadResult.type == SaoriResultType::SUCCESS) {
				//ロード成功
				auto loadObj = ScriptValue::Make(request.GetContext().GetInterpreter().CreateNativeObject<SaoriModule>(loadResult.saori));
				staticStore->RawSet(saoriRelativePath, loadObj);
				response.SetReturnValue(loadObj);
			}
			else {
				//ロード失敗
				response.SetThrewError(request.GetContext().GetInterpreter().CreateNativeObject<RuntimeError>(
					SaoriResultTypeToString(loadResult.type)
				));
			}
		}
#else
		response.SetThrewError(request.GetContext().GetInterpreter().CreateNativeObject<RuntimeError>(
			"SAORIに対応していない環境です"
		));
#endif	//#if defined(AOSORA_ENABLE_SAORI_LOADER)
	}

	ScriptValueRef SaoriManager::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "Load") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&SaoriManager::Load));
		}
		return nullptr;
	}

	void SaoriModule::Request(const FunctionRequest& request, FunctionResponse& response) {
#if defined(AOSORA_ENABLE_SAORI_LOADER)
		SaoriModule* self = request.GetContext().GetInterpreter().InstanceAs<SaoriModule>(request.GetThisValue());

		//呼出引数を列挙
		SaoriRequestResult saoriResponse;
		std::vector<std::string> inputArgs;
		for (const auto& a : request.GetArgumentCollection()) {
			inputArgs.push_back(a->ToString());
		}

		//呼出
		RequestSaori(self->loadedModule, request.GetContext().GetInterpreter().GetSecurityLevel(), inputArgs, saoriResponse);

		//返答オブジェクトを作成
		Reference<ScriptObject> responseObj = request.GetContext().GetInterpreter().CreateObject();
		Reference<ScriptArray> responseValues = request.GetContext().GetInterpreter().CreateNativeObject<ScriptArray>();
		responseObj->RawSet("Result", ScriptValue::Make(saoriResponse.result));

		if (saoriResponse.type != SaoriResultType::SUCCESS) {
			responseObj->RawSet("Error", ScriptValue::Make(SaoriResultTypeToString(saoriResponse.type)));
		}

		for (size_t i = 1; i < saoriResponse.values.size(); i++) {
			responseValues->Add(ScriptValue::Make(saoriResponse.values[i]));
		}
		responseObj->RawSet("Values", ScriptValue::Make(responseValues));
		response.SetReturnValue(ScriptValue::Make(responseObj));
#endif	// #if defined(AOSORA_ENABLE_SAORI_LOADER)
	}

	ScriptValueRef SaoriModule::Get(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "Request") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&SaoriModule::Request, GetRef()));
		}
		return nullptr;
	}

	SaoriModule::~SaoriModule() {
#if defined(AOSORA_ENABLE_SAORI_LOADER)
		UnloadSaori(loadedModule);
#endif // #if defined(AOSORA_ENABLE_SAORI_LOADER)
	}

	void PluginManager::Load(const FunctionRequest& request, FunctionResponse& response) {
#if defined(AOSORA_ENABLE_PLUGIN_LOADER)
		if (request.GetArgumentCount() > 0) {
			//指定パスでプラグインDLLをロードしてオブジェクトを返す
			std::string pluginRelativePath = request.GetArgument(0)->ToString();

			//ロード済みのSAORIをチェックする、ロード済みならそのまま返す
			auto staticStore = request.GetContext().GetInterpreter().StaticStore<PluginManager>();
			auto loadedPlugin = staticStore->RawGet(pluginRelativePath);
			auto loadedPluginModule = request.GetInterpreter().InstanceAs<PluginModule>(loadedPlugin);
			if (loadedPluginModule != nullptr) {
				response.SetReturnValue(loadedPluginModule->GetPluginBody());
				return;
			}

			//ここでプラグイン呼び出しコンテキストが必要になる
			std::string pluginPath = request.GetContext().GetInterpreter().GetWorkingDirectory() + pluginRelativePath;

			//SAORIのロードを試みる
			auto loadResult = LoadPlugin(pluginPath);
			if (loadResult.type == PluginResultType::SUCCESS) {

				//プラグイン初期化実行
				auto loadObj = PluginContextManager::ExecuteModuleLoadFunction(*loadResult.plugin, request.GetContext());
				auto moduleData = request.GetInterpreter().CreateNativeObject<PluginModule>(loadObj, loadResult.plugin);
				staticStore->RawSet(pluginRelativePath, ScriptValue::Make(moduleData));
				response.SetReturnValue(loadObj);
			}
			else {
				//ロード失敗
				response.SetThrewError(request.GetContext().GetInterpreter().CreateNativeObject<RuntimeError>(
					PluginResultTypeToString(loadResult.type)
				));
			}
		}
#else
		response.SetThrewError(request.GetContext().GetInterpreter().CreateNativeObject<RuntimeError>(
			"プラグインシステムに対応していない環境です"
		));
#endif
	}

	ScriptValueRef PluginManager::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "Load") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&PluginManager::Load));
		}
		return nullptr;
	}

	PluginModule::~PluginModule() {
#if defined(AOSORA_ENABLE_PLUGIN_LOADER)
		PluginContextManager::UnregisterPlugin(loadedModule);
		UnloadPlugin(loadedModule);
#endif
	}

	void PluginModule::FetchReferencedItems(std::list<CollectableBase*>& result) {
		if (pluginBody != nullptr && pluginBody->IsObject()) {
			result.push_back(pluginBody->GetObjectRef().Get());
		}

#if defined(AOSORA_ENABLE_PLUGIN_LOADER)
		//プラグイン管理側が持っている参照も取り込む
		PluginContextManager::FetchReferencedItems(loadedModule, result);
#endif
	}

	void PluginDelegate::FetchReferencedItems(std::list<CollectableBase*>& result) {
		result.push_back(thisValue->GetObjectRef().Get());
	}

	void PluginDelegate::Call(const FunctionRequest& request, FunctionResponse& response) {
		//プラグインに対応してない場合はインスタンスを作る方法が実質無いので無視してよいはず
#if defined(AOSORA_ENABLE_PLUGIN_LOADER)
		//プラグイン関数を呼び出す
		PluginContextManager::ExecutePluginFunction(*pluginModule, functionPtr, thisValue, request, response);
#endif
	}

	void ScriptDebug::WriteLine(const FunctionRequest& request, FunctionResponse& response) {
		if(request.GetArgumentCount() == 0){
			return;
		}

		DebugOutputContext debugOutputContext;
		std::string data = request.GetArgument(0)->DebugToString(request.GetContext(), debugOutputContext);
		const ASTNodeBase* node = request.GetContext().GetLatestASTNode();

		std::ofstream* const stream = request.GetContext().GetInterpreter().GetDebugOutputStream();
		if (stream != nullptr) {
			if (node != nullptr) {
				//出力元のスクリプト位置をとっておく
				*stream << node->GetSourceRange().ToString() << " ";
			}
			*stream << data << std::endl;
			stream->flush();
		}

		if (node != nullptr) {
			//デバッグ出力
			Debugger::NotifyLog(data, *node, false);
		}
	}

	void ScriptDebug::Assert(const FunctionRequest& request, FunctionResponse& response) {
		if (!Debugger::IsCreated()) {
			return;
		}

		if (request.GetArgumentCount() >= 1) {
			//式評価
			bool v = request.GetArgument(0)->ToBoolean();
			if (!v) {

				//エラーメッセージがあれば使う
				std::string errorMessage;
				if (request.GetArgumentCount() >= 2) {
					errorMessage = request.GetArgument(1)->ToString();
				}

				//エラー発生
				response.SetThrewError(request.GetContext().GetInterpreter().CreateNativeObject<AssertError>(
					!errorMessage.empty() ? errorMessage : "Assertion failed"
				));
			}
		}
		else {
			//引数不足もエラー扱い
			response.SetThrewError(request.GetContext().GetInterpreter().CreateNativeObject<AssertError>(
				"Assertion failed"
			));
		}
	}

	ScriptValueRef ScriptDebug::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "isEnabled") {
			return ScriptValue::Make(Debugger::IsCreated());
		}
		else if (key == "WriteLine") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptDebug::WriteLine));
		}
		else if (key == "Assert") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptDebug::Assert));
		}
		return nullptr;
	}

}
