#pragma once
#include <string>
#include <map>
#include <memory>
#include <sstream>

#include "AST/ASTNodeBase.h"
#include "Interpreter/ObjectSystem.h"

namespace sakura {

	class ClassData;
	class ScriptObject;
	class ScriptInterpreter;
	class ScriptExecuteContext;

	//オブジェクト型
	enum class ScriptValueType {
		Undefined,	//使わないかも・・・
		Null,
		Number,
		String,
		Boolean,
		Object
	};


	//オブジェクト
	class ObjectBase : public CollectableBase {
	private:
		uint32_t typeId;

	protected:
		void SetInstanceTypeId(uint32_t id) { typeId = id; }

	public:
		ObjectBase(uint32_t objectTypeId):
			typeId(objectTypeId)
		{}

		//オブジェクトタイプIDの取得
		uint32_t GetInstanceTypeId() const { return typeId; }

		//ゲッタとセッタ
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext);
		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext);

		//関数呼び出しスタイルの使用が可能かどうか
		virtual bool CanCall() const { return false; }
		virtual void Call(const FunctionRequest& request, FunctionResponse& response) {}
	};

	using ObjectRef = Reference<ObjectBase>;

	//スクリプトの値型
	//ちょっとでかいけど、このままいく
	class ScriptValue {
	private:
		ScriptValueType valueType;
		number numberValue;
		bool boolValue;
		std::string stringValue;
		ObjectRef objectRefValue;

		ScriptValue(ScriptValueType type) :
			valueType(type),
			numberValue(0.0),
			boolValue(false),
			stringValue(),
			objectRefValue(nullptr)
		{}

		ScriptValue(bool boolean) :
			valueType(ScriptValueType::Boolean),
			numberValue(0.0),
			boolValue(boolean),
			stringValue(),
			objectRefValue(nullptr)
		{}

		ScriptValue(const std::string& str) :
			valueType(ScriptValueType::String),
			numberValue(0.0),
			boolValue(false),
			stringValue(str),
			objectRefValue(nullptr)
		{}

		ScriptValue(number num) :
			valueType(ScriptValueType::Number),
			numberValue(num),
			boolValue(false),
			stringValue(),
			objectRefValue(nullptr)
		{}

		ScriptValue(const ObjectRef& objRef) :
			valueType(ScriptValueType::Object),
			numberValue(0.0),
			boolValue(false),
			stringValue(),
			objectRefValue(objRef)
		{}

	public:
		static const ScriptValueRef Null;
		static const ScriptValueRef Undefined;
		static const ScriptValueRef NaN;
		static const ScriptValueRef True;
		static const ScriptValueRef False;

	public:

		static ScriptValueRef Make(const std::string& val) {
			return ScriptValueRef(new ScriptValue(val));
		}

		static ScriptValueRef Make(const char* val) {
			return ScriptValueRef(new ScriptValue(std::string(val)));
		}

		static ScriptValueRef Make(number val) {
			return ScriptValueRef(new ScriptValue(val));
		}

		static ScriptValueRef Make(bool val) {
			return val ? True : False;
		}

		static ScriptValueRef Make(const ObjectRef& val) {
			return ScriptValueRef(new ScriptValue(val));
		}

		static ScriptValueRef MakeObject(ScriptInterpreter& interpreter);

		static ScriptValueRef MakeNull() {
			return ScriptValueRef(new ScriptValue(ScriptValueType::Null));
		}

		//boolとして評価する
		bool ToBoolean() const {
			//型ごとにそれぞれ評価してく
			switch (valueType) {
			case ScriptValueType::Null:
				return false;	//nullなら false
			case ScriptValueType::Number:
				return (numberValue != 0.0);	//ゼロでなければ true
			case ScriptValueType::String:
				return !stringValue.empty();	//カラでなければ true
			case ScriptValueType::Boolean:
				return boolValue;
			default:
				//未知
				assert(false);
				return false;
			}
		}

		//文字列として評価する
		std::string ToString() const {
			switch (valueType) {
			case ScriptValueType::Null:
				return std::string();
			case ScriptValueType::Number:
			{
				std::ostringstream ost;
				ost << numberValue;
				return ost.str();
			}
			case ScriptValueType::String:
				return stringValue;
			case ScriptValueType::Boolean:
				return boolValue ? "true" : "false";
			default:
				//それ以外は文字列化を拒否する
				return "";
			}
		}

		//文字列として評価する、デリゲートも呼び出して戻り値を評価する
		std::string ToStringWithFunctionCall(ScriptExecuteContext& executeContext);
		std::string ToStringWithFunctionCall(ScriptInterpreter& interpreter);

		//数値として評価する
		number ToNumber() const {
			switch (valueType) {
			case ScriptValueType::Number:
				return numberValue;
			case ScriptValueType::Null:
				return 0.0;
			case ScriptValueType::String:
				return std::stod(stringValue);
			case ScriptValueType::Boolean:
				return boolValue ? 1.0 : 0.0;
			default:
				return NAN;
			}
		}

		//オブジェクト型を取得
		ObjectRef GetObjectRef() const { return objectRefValue; }

		//一致
		bool IsEquals(const ScriptValueRef& target) const {
			if (valueType == target->valueType) {
				//形一致の場合、それぞれの形で判別する
				switch (valueType) {
				case ScriptValueType::Null:
				case ScriptValueType::Undefined:
					//１個しかないので必ず同じ
					return true;
				case ScriptValueType::Number:
					//NaNを考えないとかも
					return numberValue == target->numberValue;
				case ScriptValueType::String:
					return stringValue == target->stringValue;
				case ScriptValueType::Boolean:
					return boolValue == target->boolValue;
				}
			}
			else {
				
				//片方がnullでもう片方がそうでないなら不一致
				if ((IsNull() && !target->IsNull()) || (!IsNull() && target->IsNull())) {
					return false;
				}

				//それ以外は文字列でチェックしてみる
				return ToString() == target->ToString();
			}

			//不一致の場合も対応がありそうだけどおいとく
			return false;
		}

		//型チェック
		ScriptValueType GetValueType() const {
			return valueType;
		}

		bool IsNull() const {
			return valueType == ScriptValueType::Null;
		}

		bool IsUndefined() const {
			return valueType == ScriptValueType::Undefined;
		}

		bool IsNumber() const {
			return valueType == ScriptValueType::Number;
		}

		bool IsString() const {
			return valueType == ScriptValueType::String;
		}

		bool IsBoolean() const {
			return valueType == ScriptValueType::Boolean;
		}

		bool IsObject() const {
			return valueType == ScriptValueType::Object;
		}
		
		//Objectを参照している場合にインスタンスの型IDを返す
		uint32_t GetObjectInstanceTypeId() const {
			if (!IsObject()) {
				return ObjectTypeIdGenerator::INVALID_ID;
			}
			else {
				return objectRefValue->GetInstanceTypeId();
			}
		}
	};
	
	//継承用のID発行付きのオブジェクト、オブジェクト型をC++で宣言するときはこれを継承する
	template<typename T>
	class Object : public ObjectBase {
	public:
		using StaticStoreType = ScriptObject;

		static void StaticSet(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {

		}

		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
			return nullptr;
		}

		static void StaticInit(ScriptInterpreter& interpreter) {

		}

		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<T>();
		}

		Object() : ObjectBase(ObjectTypeIdGenerator::Id<T>()) {
		}
	};

	//スクリプト連想配列
	class ScriptObject : public Object<ScriptObject> {
	private:
		//連想配列の実態
		std::map<std::string, ScriptValueRef> members;

		//スクリプトクラス型
		Reference<ClassData> scriptClass;

		//ネイティブクラスインスタンス。スクリプトの基底型から１つ継承したネイティブ型
		ObjectRef nativeClassInstance;

	public:

		ScriptObject() {
		}

		static void ScriptPush(const FunctionRequest& request, FunctionResponse& response);

		//内部オブジェクトの調節操作
		void RawSet(const std::string& key, const ScriptValueRef& value);
		ScriptValueRef RawGet(const std::string& key);
		void Push(const ScriptValueRef& val);

		void Clear() {
			members.clear();
		}
		const std::map<std::string, ScriptValueRef>& GetInternalCollection() const { return members; }

		//操作
		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result);

		//クラス情報の設定、初期化時にしか呼ばない
		void SetClassInfo(const Reference<ClassData>& classData);

		//継承元のネイティブオブジェクトを設定
		void SetNativeBaseInstance(const ObjectRef& nativeInstance) {
			nativeClassInstance = nativeInstance;
		}

		const ObjectRef& GetNativeBaseInstance() const {
			return nativeClassInstance;
		}

	};


	//関数リクエスト
	class FunctionRequest {
	private:
		std::vector<ScriptValueRef> args;
		ScriptExecuteContext& executeContext;
		const ASTNodeBase* callingNode;

	public:

		FunctionRequest(ScriptExecuteContext& context) :
			executeContext(context) {
		}

		FunctionRequest(const std::vector<ScriptValueRef>& argList, ScriptExecuteContext& context) :
			args(argList),
			executeContext(context) {
		}

		//コンテキスト取得
		ScriptExecuteContext& GetContext() const {
			return executeContext;
		}

		//引数の数
		size_t GetArgumentCount() const {
			return args.size();
		}

		//引数の取得
		const ScriptValueRef& GetArgument(size_t index) const {
			assert(index < GetArgumentCount());
			return args[index];
		}

		const std::vector<ScriptValueRef> GetArgumentCollection() const {
			return args;
		}
	};

	//関数実行結果
	class FunctionResponse {
	private:
		ScriptValueRef returnValue;
		ObjectRef threwError;
		bool isThrew;

	public:

		FunctionResponse() :
			returnValue(ScriptValue::Null),
			threwError(nullptr),
			isThrew(false){
		}

		//戻り値
		void SetReturnValue(const ScriptValueRef& v) {
			returnValue = v;
		}

		const ScriptValueRef& GetReturnValue() const {
			return returnValue;
		}

		//例外
		void SetThrewError(const ObjectRef& err) {
			threwError = err;
			isThrew = true;
		}

		const ObjectRef& GetThrewError() const {
			return threwError;
		}

		bool IsThrew() const {
			return isThrew;
		}
	};
}