//#include <ctime>
#include "Shiori.h"
#include "AST/AST.h"
#include "Interpreter/Interpreter.h"
#include "Misc/Json.h"
#include "Misc/Utility.h"

namespace sakura {

	
	
	//ScriptObjectのシリアライザ
	class ObjectSerializer {
	public:
		//json->objref
		static ScriptValueRef Deserialize(const std::shared_ptr<JsonTokenBase>& token, ScriptInterpreter& interpreter);
		static Reference<ScriptObject> DeserializeObject(const std::shared_ptr<JsonTokenBase>& token, ScriptInterpreter& interpreter);
		static Reference<ScriptArray> DeserializeArray(const std::shared_ptr<JsonTokenBase>& token, ScriptInterpreter& interpreter);
		static ScriptValueRef Deserialize(const std::string& json, ScriptInterpreter& interpreter);

		//obj->json
		static std::shared_ptr<JsonTokenBase> Serialize(const ScriptValueRef& value);
		static std::shared_ptr<JsonArray> SerializeArray(const Reference<ScriptArray>& obj);
		static std::shared_ptr<JsonObject> SerializeObject(const ObjectRef& obj);
		static std::string Serialize(const ObjectRef& obj);
	};

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
		bool called = false;

		if (canCallRandomTalk && interval > 0.0 && seconds > interval) {

			//トークをリクエストする
			auto talkVal = staticStore->RawGet(KeyRandomTalk);

			if (talkVal != nullptr) {
				//ランダムトーク呼出
				std::vector<ScriptValueRef> args;
				interpreter.CallFunction(*talkVal, response, args);

				//秒数をリセット
				seconds = 0.0;
				called = true;
			}
		}
		
		//経過秒を書き戻す
		staticStore->RawSet(KeyRandomTalkElapsedSeconds, ScriptValue::Make(seconds));

		//呼び出しを実行したかどうかを返す
		return called;
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
		if (canCallTalk && threshold > 0.0 && count > threshold) {

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
		interpreter.StaticStore<TalkTimer>()->RawSet(KeyNadenadeMoveThreshold, ScriptValue::Make(50.0));
	}

	void TalkTimer::StaticSet(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		//存在してるキーにだけアクセスを許容する
		if (
			key == KeyRandomTalk ||
			key == KeyRandomTalkElapsedSeconds ||
			key == KeyRandomTalkIntervalSeconds ||
			key == KeyNadenadeActiveCollision ||
			key == KeyNadenadeMoveCount ||
			key == KeyNadenadeMoveThreshold ||
			key == KeyNadenadeTalk
			) {
			executeContext.GetInterpreter().StaticStore<TalkTimer>()->RawSet(key, value);
		}
	}

	ScriptValueRef TalkTimer::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		//存在してるキーにだけアクセスを許容する
		if (
			key == KeyRandomTalk ||
			key == KeyRandomTalkElapsedSeconds ||
			key == KeyRandomTalkIntervalSeconds ||
			key == KeyNadenadeActiveCollision ||
			key == KeyNadenadeMoveCount ||
			key == KeyNadenadeMoveThreshold ||
			key == KeyNadenadeTalk
			) {
			executeContext.GetInterpreter().StaticStore<TalkTimer>()->RawGet(key);
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
		else if (key == "GetNowYear") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowYear));
		}
		else if (key == "GetNowMonth") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowMonth));
		}
		else if (key == "GetNowDate") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Time::GetNowDate));
		}

		return nullptr;
	}


	//ランダム
	ScriptValueRef Random::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "Select") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Random::Select));
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
		
		//9オリジンでアイテムが入っている前提で、ランダムに１つ選択する
		Reference<ScriptArray> sobj = obj.template Cast<ScriptArray>();
		const size_t size = sobj->Count();
		const int32_t result = Rand(0, static_cast<int32_t>(size));

		ScriptValueRef item = sobj->At(result);
		response.SetReturnValue(item != nullptr ? item : ScriptValue::Null);
	}
}
