#include "Misc/Utility.h"
#include "Interpreter/ScriptVariable.h"
#include "Interpreter/ScriptExecutor.h"
#include "CoreLibrary/CoreLibrary.h"

namespace sakura {
	const ScriptValueRef ScriptValue::Null(new ScriptValue(ScriptValueType::Null));
	const ScriptValueRef ScriptValue::Undefined(new ScriptValue(ScriptValueType::Undefined));
	const ScriptValueRef ScriptValue::NaN(new ScriptValue(NAN));
	const ScriptValueRef ScriptValue::True(new ScriptValue(true));
	const ScriptValueRef ScriptValue::False(new ScriptValue(false));

	ScriptValueRef ObjectBase::Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) {
		return nullptr;
	}

	void ObjectBase::Set(const ObjectRef& self, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
	}

	std::string ObjectBase::DebugToString(ScriptExecuteContext& executeContext, DebugOutputContext& debugOutputContext) {
		std::string result("[");
		result.append(executeContext.GetInterpreter().GetClassTypeName(GetInstanceTypeId()));
		result.append("]");
		return result;
	}

	void ScriptObject::ScriptAdd(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 2) {
			std::string key = request.GetArgument(0)->ToString();
			ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());
			obj->Add(key, request.GetArgument(1));
		}
	}

	void ScriptObject::ScriptContains(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 1) {
			std::string key = request.GetArgument(0)->ToString();
			ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());
			response.SetReturnValue(ScriptValue::Make(obj->Contains(key)));
		}
	}

	void ScriptObject::ScriptClear(const FunctionRequest& request, FunctionResponse& response) {
		ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());
		obj->Clear();
	}

	void ScriptObject::ScriptKeys(const FunctionRequest& request, FunctionResponse& response) {
		ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());
		auto result = request.GetContext().GetInterpreter().CreateNativeObject<ScriptArray>();

		for (auto item : obj->members) {
			result->Add(ScriptValue::Make(item.first));
		}
		response.SetReturnValue(ScriptValue::Make(result));
	}

	void ScriptObject::ScriptRemove(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() > 0) {
			ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());
			std::string k = request.GetArgument(0)->ToString();
			obj->Remove(k);
		}
	}

	void ScriptObject::RawSet(const std::string& key, const ScriptValueRef& value) {
		members[key] = value;
	}

	ScriptValueRef ScriptObject::RawGet(const std::string& key) {
		auto it = members.find(key);
		if (it != members.end()) {
			return it->second;
		}
		else {
			//undefined相当としてnullptrを返してみる
			return nullptr;
		}
	}

	void ScriptObject::Add(const std::string& key, const ScriptValueRef& value) {
		if (!Contains(key)) {
			RawSet(key, value);
		}
	}

	bool ScriptObject::Contains(const std::string& key) {
		return members.contains(key);
	}

	void ScriptObject::Set(const ObjectRef& self, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext)  {

		//クラスデータが設定されている場合そちらに設定する
		members[key] = value;
	}

	ScriptValueRef ScriptObject::Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) {

		//クラスデータが設定されている場合そちら経由で取得する
		auto rawgetResult = RawGet(key);
		if (rawgetResult != nullptr) {
			return rawgetResult;
		}
			
		//組み込みフィールド
		if (key == "length") {
			return ScriptValue::Make(static_cast<number>(members.size()));
		}
		else if (key == "Add") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptAdd, self));
		}
		else if (key == "Contains") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptContains, self));
		}
		else if (key == "Clear") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptClear, self));
		}
		else if (key == "Remove") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptRemove, self));
		}
		else if (key == "Keys") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptKeys, self));
		}

		return nullptr;
	}

	std::string ScriptObject::DebugToString(ScriptExecuteContext& executeContext, DebugOutputContext& debugOutputContext) {

		if (members.empty()) {
			//からっぽ
			return "{}";
		}

		//ディクショナリ形式で文字列化
		std::string result("{");
		{
			DebugOutputContext::IndentScope indentScope(debugOutputContext);
			bool isFirst = true;
			for (auto item : members) {
				if (!isFirst) {
					result.append(",");
				}
				else {
					isFirst = false;
				}
				debugOutputContext.AppendNewLine(result);
				result.append(item.first);
				result.append(": ");
				result.append(item.second->DebugToString(executeContext, debugOutputContext));
			}
		}
		debugOutputContext.AppendNewLine(result);
		result.append("}");
		return result;
	}

	void ScriptObject::FetchReferencedItems(std::list<CollectableBase*>& result) {
		for (auto item : members) {
			if (item.second->GetValueType() == ScriptValueType::Object) {
				result.push_back(item.second->GetObjectRef().Get());
			}
		}
	}

	ClassInstance::ClassInstance(const Reference<ClassData>& classType, ScriptExecuteContext& executeContext)
	{
		//内部ストレージを生成
		//TODO: ネイティブクラスの場合は不要
		scriptStore = executeContext.GetInterpreter().CreateObject();

		//タイプIDを指定
		classData = classType;
		SetInstanceTypeId(classData->GetClassTypeId());
	}

	void ClassInstance::Set(const ObjectRef& self, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		
		if (classData->GetMetadata().IsScriptClass()) {
			//スクリプトクラスの場合、ストレージを直接書き換える
			//NOTE: 機能的にこれをつかうのは悪手だけど、シンプルなつくりになるように？ ふさぎたいかも？
			const ScriptClass& classInfo = static_cast<const ScriptClass&>(classData->GetMetadata());
		}
		else {
			//ネイティブなら型領域へセット
			//NOTE: この制約により継承を許容するネイティブクラスはプロパティ書き込みは不可
			return classData->SetToInstance(key, value, self.Cast<ClassInstance>(), executeContext);
		}
	}

	ScriptValueRef ClassInstance::Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) {

		//スクリプトクラスであれば内部ストレージから検索
		if (classData->GetMetadata().IsScriptClass()) {
			if (scriptStore->Contains(key)) {
				return scriptStore->RawGet(key);
			}
		}

		//見つかってなければ型領域から検索
		return classData->GetFromInstance(key, self.Cast<ClassInstance>(), executeContext);
	}

	void ClassInstance::FetchReferencedItems(std::list<CollectableBase*>& result) {
		if (classData != nullptr) {
			result.push_back(classData.Get());
		}
		if (scriptStore != nullptr) {
			result.push_back(scriptStore.Get());
		}
		if (nativeClassInstance != nullptr) {
			result.push_back(nativeClassInstance.Get());
		}
	}

	ScriptValueRef ScriptValue::MakeObject(ScriptInterpreter& interpreter) {
		return ScriptValueRef(new ScriptValue(interpreter.CreateObject()));
	}

	std::string ScriptValue::ToStringWithFunctionCall(ScriptExecuteContext& executeContext, const ASTNodeBase* callingAstNode) {

		if (IsObject()) {
			//もしオブジェクトなら呼び出し可能かどうかを評価、可能なら引数なしで呼んだ結果を返す
			auto objRef = GetObjectRef();
			if (objRef->CanCall()) {
				std::vector<ScriptValueRef> args;
				FunctionResponse res;
				executeContext.GetInterpreter().CallFunction(*this, res, args, executeContext, callingAstNode);

				if (res.IsThrew()) {
					//例外がスローされていればエラーで打ち切る
					executeContext.GetStack().Throw(res.GetThrewError());
					return "";
				}

				if (res.GetReturnValue() != nullptr) {
					return res.GetReturnValue()->ToStringWithFunctionCall(executeContext, callingAstNode);
				}
			}

			//呼び出せないため結果無効
			return "";
		}
		else {
			//オブジェクト遺体なので関数呼び出しにかかわらずToStringを返す
			return ToString();
		}
	}

	//内部でコンテキストを作るヘルパ
	ToStringFunctionCallResult ScriptValue::ToStringWithFunctionCall(ScriptInterpreter& interpreter) {
		ScriptInterpreterStack rootStack;
		Reference<BlockScope> rootBlock = interpreter.CreateNativeObject<BlockScope>(nullptr);
		ScriptExecuteContext executeContext(interpreter, rootStack, rootBlock);

		//例外が出ている場合も考慮する
		std::string str = ToStringWithFunctionCall(executeContext, nullptr);
		ToStringFunctionCallResult result;
		if (executeContext.GetStack().IsThrew()) {
			result.error = executeContext.GetStack().GetThrewError();
			result.success = false;
		}
		else {
			result.result = str;
			result.success = true;
		}
		return result;
	}

	ScriptValueRef FunctionRequest::GetThisValue() const {
		return GetContext().GetBlockScope()->GetThisValue();
	}
}