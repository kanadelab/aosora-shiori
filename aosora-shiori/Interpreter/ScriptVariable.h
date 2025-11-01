#pragma once
#include <cmath>
#include <string>
#include <map>
#include <memory>
#include <sstream>
#include <set>
#include <deque>
#include <iostream>
#include <iomanip>
#include <limits>

#include "AST/ASTNodeBase.h"
#include "Interpreter/ObjectSystem.h"

namespace sakura {

	class ClassData;
	class ScriptObject;
	class ScriptArray;
	class ScriptInterpreter;
	class ScriptExecuteContext;
	class ObjectBase;
	class ScriptIterator;

	//オブジェクト型
	enum class ScriptValueType {
		Undefined,	//使わないかも・・・
		Null,
		Number,
		String,
		Boolean,
		Object
	};

	//デバッグ出力用の情報
	class DebugOutputContext
	{
	private:
		bool enableIndent;	//改行およびインデントの適用
		int32_t indentLevel;
		char indentChar;
		std::vector<std::set<ObjectBase*>> loopCheckSet;	//TODO: 再帰書き出し検出のためのセット。全世代でみないといけないのですこしややこしいかも？

	public:

		//インデント区間管理用のスコープオブジェクト
		class IndentScope {
			DebugOutputContext& context;
		public:
			IndentScope(DebugOutputContext& context) :
				context(context)
			{
				context.indentLevel++;
			}

			~IndentScope()
			{
				context.indentLevel--;
			}
		};

		//再帰出力回避管理用のスコープオブジェクト
		class LoopCheckScope {
			DebugOutputContext& context;
		public:
			LoopCheckScope(DebugOutputContext& context) :
				context(context)
			{
				context.loopCheckSet.push_back(std::set<ObjectBase*>());
			}

			~LoopCheckScope()
			{
				context.loopCheckSet.pop_back();
			}
		};

		DebugOutputContext() :
			enableIndent(true),
			indentLevel(0),
			indentChar('\t')
		{ }

		bool IsEnableIndent() { return enableIndent; }
		int32_t GetIndentLevel() { return indentLevel; }
		char GetIndentChar() { return indentChar; }
		std::string MakeIndent() { return std::string(indentLevel, indentChar); }

		void AppendIndent(std::string& str) {
			if (enableIndent && indentLevel > 0) {
				str.append(MakeIndent());
			}
		}

		void AppendNewLine(std::string& str) {
			if (enableIndent) {
				str.append("\n");
				AppendIndent(str);
			}
		}

		//再帰ループしてないかの検証
		bool ValidateLoopedObject(ObjectBase* ptr) {

			//要素がないので問題なし
			if (loopCheckSet.empty()) {
				return true;
			}

			//重複する要素がないか調べる
			for (auto& map : loopCheckSet) {
				if (map.contains(ptr)) {
					return false;	//重複
				}
			}

			//追加
			loopCheckSet.rbegin()->insert(ptr);
			return true;
		}
	};

	//オブジェクト
	class ObjectBase : public CollectableBase {
	private:
		uint32_t typeId;				//スクリプト側に見せる型のID
		uint32_t nativeObjectTypeId;	//C++側の実際の型ID

	protected:
		//スクリプトに渡す型を変更する
		//主にClassInstanceを表現するためのもの
		void SetInstanceTypeId(uint32_t id) { typeId = id; }

		//C++継承で型変換が発生する場合のためのもの
		void SetNativeOverrideInstanceId(uint32_t id) { typeId = id; nativeObjectTypeId = id; }

	public:
		ObjectBase(uint32_t objectTypeId):
			typeId(objectTypeId),
			nativeObjectTypeId(objectTypeId)
		{}

		//オブジェクトタイプIDの取得
		uint32_t GetInstanceTypeId() const { return typeId; }
		uint32_t GetNativeInstanceTypeId() const { return nativeObjectTypeId; }
		std::string GetClassTypeName(ScriptInterpreter& interpreter);

		//ゲッタとセッタ
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext);
		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext);

		//関数呼び出しスタイルの使用が可能かどうか
		virtual bool CanCall() const { return false; }
		virtual void Call(const FunctionRequest& request, FunctionResponse& response) {}

		//デバッグ向けの文字列化
		virtual std::string DebugToString(ScriptExecuteContext& executeContext, DebugOutputContext& debugOutputContext);

		//リファレンスオブジェクト作成
		Reference<ObjectBase> GetRef() { return Reference<ObjectBase>(this); }
	};

	using ObjectRef = Reference<ObjectBase>;

	//ToStringFunctionCallの結果オブジェクト
	struct ToStringFunctionCallResult {
		std::string result;
		ObjectRef error;
		bool success;
	};


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
		static ScriptValueRef MakeArray(ScriptInterpreter& interpreter);

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
			case ScriptValueType::Object:		//オブジェクトは常にtrue
				return true;
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
				//doubleが正確に示せる整数範囲では指数表記にならないようにする
				ost << std::setprecision(std::numeric_limits<double>::digits10) << numberValue;
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

		//デバッグ用に文字列化する
		std::string DebugToString(ScriptExecuteContext& executeContext, DebugOutputContext& debugOutputContext) const {
			switch (valueType) {
			case ScriptValueType::String: {
				//デバッグ出力のためにダブルクォートをつける
				return std::string() + '"' + stringValue + '"';
			}
			case ScriptValueType::Null:
				//null値も表示する
				return "null";
			case ScriptValueType::Object: {
				DebugOutputContext::LoopCheckScope loopCheckScope(debugOutputContext);
				if (debugOutputContext.ValidateLoopedObject(GetObjectRef().Get())) {
					return GetObjectRef()->DebugToString(executeContext, debugOutputContext);
				}
				else {
					//オブジェクトが再帰しているので、出力しないようにする
					return "{...repeat...}";
				}
			}
			default:
				//ほかは通常通り
				return ToString();
			}
		}

		//文字列として評価する、デリゲートも呼び出して戻り値を評価する
		std::string ToStringWithFunctionCall(ScriptExecuteContext& executeContext, const ASTNodeBase* callingAstNode);
		ToStringFunctionCallResult ToStringWithFunctionCall(ScriptInterpreter& interpreter);

		//インデックス数値として評価する（主に内部向け）
		bool ToIndex(size_t& result) const {
			number m = ToNumber();
			if (std::isnan(m)) {
				return false;
			}
			if (m < 0.0) {
				return false;
			}
			result = static_cast<size_t>(m);
			return true;
		}

		//数値として評価する
		number ToNumber() const {
			switch (valueType) {
			case ScriptValueType::Number:
				return numberValue;
			case ScriptValueType::Null:
				return 0.0;
			case ScriptValueType::String:
				try {
					return std::stod(stringValue);
				}
				catch (const std::exception&) {
					return NAN;
				}
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

				//それ以外は数値比較できるなら数値で、不可能なら文字列で比較
				const number leftNumber = ToNumber();
				const number rightNumber = target->ToNumber();

				//数値比較できそうなら数値比較して、だめなら文字列的に比較する
				if (!std::isnan(leftNumber) && !std::isnan(rightNumber)) {
					if (leftNumber == rightNumber) {
						return true;
					}
					else {
						return false;
					}
				}
				else {
					if (ToString() == target->ToString()) {
						return true;
					}
					else {
						return false;
					}
				}
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
	//さらに継承する必要がある場合は素直にいかないので注意･･･（TypeIdを直接定義したうえでコンストラクタでIDの上書きが必要）
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

		static void StaticDestruct(ScriptInterpreter& interpreter) {

		}

		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<T>();
		}

		Object() : ObjectBase(ObjectTypeIdGenerator::Id<T>()) {
		}
	};

	//イテレート可能オブジェクト
	class ScriptIterable : public Object<ScriptIterable>{
	private:
		//有効なイテレータ数(これがある状態でコレクションを変更するとエラーとなる)
		int32_t iteratorCheckoutCounter;

	public:
		virtual Reference<ScriptIterator> CreateIterator(ScriptExecuteContext& executeContext) = 0;
		bool ValidateCollectionLock(const FunctionRequest& request, FunctionResponse& response);
		bool ValidateCollectionLock(ScriptExecuteContext& executeContext);

		void CheckoutIterator() {
			iteratorCheckoutCounter++;
		}

		void CheckinIterator() {
			iteratorCheckoutCounter--;
			assert(iteratorCheckoutCounter >= 0);
		}

		bool IsCollectionLocked() const {
			//イテレータが有効な間はコレクションの変更は許容できないので止める
			return (iteratorCheckoutCounter > 0);
		}

		//これ自体は参照をもたない
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result){}
	};

	//イテレータ
	class ScriptIterator : public Object<ScriptIterator> {
	private:
		Reference<ScriptIterable> target;

	public:
		//生成時、破棄時で元オブジェクトに対して checkout/checkinをかけてイテレータの有効範囲を通知する
		ScriptIterator(const Reference<ScriptIterable>& iterable):
			target(iterable) {
			target->CheckoutIterator();
		}
		
		virtual ~ScriptIterator() {
			assert(target == nullptr);
		}

		void Dispose() {
			assert(target != nullptr);
			if (target != nullptr) {
				target->CheckinIterator();
				target = nullptr;
			}
		}

		virtual ScriptValueRef GetValue() = 0;
		virtual ScriptValueRef GetKey() = 0;
		virtual void FetchNext() = 0;
		virtual bool IsEnd() = 0;

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result);
	};

	//スクリプト連想配列
	class ScriptObject : public ScriptIterable {
	public:
		using InternalCollectionType = std::map<std::string, ScriptValueRef>;

	private:
		//連想配列の実体
		InternalCollectionType members;

	public:

		ScriptObject() {
			//スクリプト公開型継承なのでインスタンスIDを再指定しないといけない
			SetNativeOverrideInstanceId(TypeId());
		}

		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<ScriptObject>();
		}

		static void ScriptAdd(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptContains(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptClear(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptKeys(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptRemove(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptGet(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptSet(const FunctionRequest& request, FunctionResponse& response);

		//内部オブジェクトの調節操作
		void RawSet(const std::string& key, const ScriptValueRef& value);
		ScriptValueRef RawGet(const std::string& key);
		
		void Add(const std::string& key, const ScriptValueRef& value);
		bool Contains(const std::string& key);

		void Clear() {
			members.clear();
		}

		void Remove(const std::string& key) {
			members.erase(key);
		}

		uint32_t GetLength() const {
			return static_cast<uint32_t>(members.size());
		}

		const InternalCollectionType& GetInternalCollection() const { return members; }

		//操作
		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;
		virtual std::string DebugToString(ScriptExecuteContext& executeContext, DebugOutputContext& debugOutputContext) override;

		virtual Reference<ScriptIterator> CreateIterator(ScriptExecuteContext& executeContext) override;
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result);
	};

	//連想配列に対するイテレータ
	class ScriptObjectIterator : public ScriptIterator {
	private:
		Reference<ScriptObject> targetObject;
		ScriptObject::InternalCollectionType::const_iterator internalIterator;

	public:
		ScriptObjectIterator(const Reference<ScriptObject>& target) : ScriptIterator(target),
			targetObject(target) {
			SetNativeOverrideInstanceId(TypeId());
			internalIterator = targetObject->GetInternalCollection().begin();
		}

		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<ScriptObjectIterator>();
		}

		virtual ScriptValueRef GetValue() override {
			return internalIterator->second;
		}

		virtual ScriptValueRef GetKey() override {
			return ScriptValue::Make(internalIterator->first);
		}

		virtual void FetchNext() {
			++internalIterator;
		}

		virtual bool IsEnd() override {
			if (internalIterator == targetObject->GetInternalCollection().end()) {
			return true;
			}
			return false;
		}

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result);
	};

	//クラスインスタンス
	class UpcastClassInstance;
	class ClassInstance : public Object<ClassInstance> {
		friend class UpcastClassInstance;
	private:
		//スクリプトクラス型
		Reference<ClassData> classData;

		//スクリプト向けの領域、thisに相当するキーバリューストア
		Reference<ScriptObject> scriptStore;

		//ネイティブクラスインスタンス。スクリプトの基底型から１つ継承したネイティブ型
		ObjectRef nativeClassInstance;

	private:
		void SetInternal(const Reference<ClassInstance>& self, const Reference<ClassData>& classType, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext);
		ScriptValueRef GetInternal(const Reference<ClassInstance>& self, const Reference<ClassData>& classType, const std::string& key, ScriptExecuteContext& executeContext);

	public:

		ClassInstance(const Reference<ClassData>& classType, ScriptExecuteContext& executeContext);

		//継承元のネイティブオブジェクトを設定
		void SetNativeBaseInstance(const ObjectRef& nativeInstance) {
			nativeClassInstance = nativeInstance;
		}
		const ObjectRef& GetNativeBaseInstance() const { return nativeClassInstance; }
		const Reference<ScriptObject>& GetScriptStore() const { return scriptStore; }
		
		Reference<UpcastClassInstance> MakeBase(const Reference<ClassInstance>& self, const ScriptClassRef& contextClass, ScriptExecuteContext& executeContext);
		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
	};

	//クラスインスタンスのアップキャスト体
	class UpcastClassInstance : public Object<UpcastClassInstance> {
	private:
		//参照先
		Reference<ClassInstance> classInstance;
		
		//参照型
		Reference<ClassData> upcastedClassData;

	public:
		UpcastClassInstance(const Reference<ClassInstance>& instance, const Reference<ClassData> upcastedType):
			classInstance(instance),
			upcastedClassData(upcastedType) 
		{}

		const Reference<ClassInstance>& GetClassInstance() const { return classInstance; }
		const Reference<ClassData>& GetUpcastedClassData() const { return upcastedClassData; }

		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
	};

	//関数リクエスト
	class FunctionRequest {
	private:
		std::vector<ScriptValueRef> args;
		ScriptExecuteContext& executeContext;

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

		//インタプリタ取得
		ScriptInterpreter& GetInterpreter() const;

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

		//this取得ヘルパ
		ScriptValueRef GetThisValue() const;
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
			if (v != nullptr) {
				returnValue = v;
			}
			else {
				//emptyを不許容
				returnValue = ScriptValue::Null;
			}
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
