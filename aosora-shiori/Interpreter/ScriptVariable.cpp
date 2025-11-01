#include "Misc/Utility.h"
#include "Interpreter/ScriptVariable.h"
#include "Interpreter/ScriptExecutor.h"
#include "CoreLibrary/CoreLibrary.h"
#include "Misc/Message.h"

namespace sakura {
	const ScriptValueRef ScriptValue::Null(new ScriptValue(ScriptValueType::Null));
	const ScriptValueRef ScriptValue::Undefined(new ScriptValue(ScriptValueType::Undefined));
	const ScriptValueRef ScriptValue::NaN(new ScriptValue(NAN));
	const ScriptValueRef ScriptValue::True(new ScriptValue(true));
	const ScriptValueRef ScriptValue::False(new ScriptValue(false));

	ScriptValueRef ObjectBase::Get(const std::string& key, ScriptExecuteContext& executeContext) {
		return nullptr;
	}

	void ObjectBase::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
	}

	std::string ObjectBase::GetClassTypeName(ScriptInterpreter& interpreter) {
		return interpreter.GetClassTypeName(GetInstanceTypeId());
	}

	std::string ObjectBase::DebugToString(ScriptExecuteContext& executeContext, DebugOutputContext& debugOutputContext) {
		std::string result("[");
		result.append(executeContext.GetInterpreter().GetClassTypeName(GetInstanceTypeId()));
		result.append("]");
		return result;
	}

	bool ScriptIterable::ValidateCollectionLock(const FunctionRequest& request, FunctionResponse& response) {
		//コレクションがロックされているなら例外を投げてfalseを返す
		if (IsCollectionLocked()) {
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_ITERATOR_ERROR_001"))
			);
			return false;
		}
		return true;
	}

	bool ScriptIterable::ValidateCollectionLock(ScriptExecuteContext& executeContext) {
		if (IsCollectionLocked()) {
			executeContext.ThrowRuntimeError<RuntimeError>(TextSystem::Find("AOSORA_ITERATOR_ERROR_001"), executeContext);
			return false;
		}
		return true;
	}

	void ScriptIterator::FetchReferencedItems(std::list<CollectableBase*>& result) {
		if (target != nullptr) {
			result.push_back(target.Get());
		}
	}

	void ScriptObject::ScriptAdd(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 2) {

			std::string key = request.GetArgument(0)->ToString();
			ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());

			//要素が増えようとしている場合、foreach操作では不可
			if (!obj->ValidateCollectionLock(request, response)) {
				return;
			}

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

		//要素が増えようとしている場合、foreach操作では不可
		if (!obj->ValidateCollectionLock(request, response)) {
			return;
		}

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

			//要素が増えようとしている場合、foreach操作では不可
			if (!obj->ValidateCollectionLock(request, response)) {
				return;
			}

			std::string k = request.GetArgument(0)->ToString();
			obj->Remove(k);
		}
	}

	void ScriptObject::ScriptGet(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() > 0) {
			ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());

			std::string k = request.GetArgument(0)->ToString();
			auto rawgetResult = obj->RawGet(k);
			if (rawgetResult != nullptr) {
				response.SetReturnValue(rawgetResult);
			}
		}
	}

	void ScriptObject::ScriptSet(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() > 1) {
			ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());
			const std::string k = request.GetArgument(0)->ToString();

			if (!obj->Contains(k)) {
				//要素が増えようとしている場合、foreach操作では不可
				if (!obj->ValidateCollectionLock(request.GetContext())) {
					return;
				}
			}

			auto v = request.GetArgument(1);
			obj->RawSet(k, v);
		}
	}


	void ScriptObject::RawSet(const std::string& key, const ScriptValueRef& value) {
		assert(value != nullptr);
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

	void ScriptObject::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext)  {
		assert(value != nullptr);
		if (!Contains(key)) {
			//要素が増えようとしている場合、foreach操作では不可
			if (!ValidateCollectionLock(executeContext)) {
				return;
			}
		}
		members[key] = value;
	}

	ScriptValueRef ScriptObject::Get(const std::string& key, ScriptExecuteContext& executeContext) {
	
		//組み込みフィールド
		if (key == "length") {
			return ScriptValue::Make(static_cast<number>(members.size()));
		}
		else if (key == "Get") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptGet, GetRef()));
		}
		else if (key == "Set") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptSet, GetRef()));
		}
		else if (key == "Add") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptAdd, GetRef()));
		}
		else if (key == "Contains") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptContains, GetRef()));
		}
		else if (key == "Clear") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptClear, GetRef()));
		}
		else if (key == "Remove") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptRemove, GetRef()));
		}
		else if (key == "Keys") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptKeys, GetRef()));
		}

		auto genralGetResult = PrimitiveMethod::GetGeneralMember(ScriptValue::Make(GetRef()), key, executeContext);
		if (genralGetResult != nullptr) {
			return genralGetResult;
		}

		auto rawgetResult = RawGet(key);
		if (rawgetResult != nullptr) {
			return rawgetResult;
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

	Reference<ScriptIterator> ScriptObject::CreateIterator(ScriptExecuteContext& executeContext) {
		//NOTE: shared_ptr と違ってポインタからリファレンスを作成しても問題ないはず、カウンタをもつにしてもオブジェクト側がもつはずなので･･･
		return executeContext.GetInterpreter().CreateNativeObject<ScriptObjectIterator>(Reference<ScriptObject>(this));
	}

	void ScriptObjectIterator::FetchReferencedItems(std::list<CollectableBase*>& result) {
		ScriptIterator::FetchReferencedItems(result);
		result.push_back(targetObject.Get());
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
		scriptStore = executeContext.GetInterpreter().CreateObject();

		//タイプIDを指定
		classData = classType;
		SetInstanceTypeId(classData->GetClassTypeId());
	}

	void ClassInstance::SetInternal(const Reference<ClassInstance>& self, const Reference<ClassData>& classType, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		if (classType->GetMetadata().IsScriptClass()) {
			//スクリプトクラスの場合は中身を書き換える
			scriptStore->RawSet(key, value);
		}
		else {
			//ネイティブならインスタンス領域にセット
			classType->SetToInstance(key, value, self, executeContext);
		}
	}

	ScriptValueRef ClassInstance::GetInternal(const Reference<ClassInstance>& self, const Reference<ClassData>& classType, const std::string& key, ScriptExecuteContext& executeContext) {
		//スクリプトクラスであればストレージを検索
		//TODO: 検索順を反転させるべきかも？　クラスから検索すると多段検索がどうしても必要になるのでとりあえず効率からこうしているけれど･･･
		if (classType->GetMetadata().IsScriptClass()) {
			if (scriptStore->Contains(key)) {
				return scriptStore->RawGet(key);
			}
		}

		//見つかってなければ型領域から検索
		return classType->GetFromInstance(key, self, executeContext);
	}

	Reference<UpcastClassInstance> ClassInstance::MakeBase(const Reference<ClassInstance>& self, const ScriptClassRef& contextClass, ScriptExecuteContext& executeContext) {
		//baseキーワードで取得できるupcastedインスタンスを取得する
		//contextClassがbaseキーワードのASTノードが所属するクラスを示しているのでその親クラスを取得する
		//(classData->GetParentClass() を使うとthisに対するbaseとなるので多段継承で問題が出るためNG)

		//インタプリタからクラスデータを取得
		auto classRef = executeContext.GetInterpreter().GetClass(contextClass->GetTypeId());
		return executeContext.GetInterpreter().CreateNativeObject<UpcastClassInstance>(self, classRef->GetObjectRef().Cast<ClassData>()->GetParentClass());
	}

	void ClassInstance::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		SetInternal(GetRef().Cast<ClassInstance>(), classData, key, value, executeContext);
	}

	ScriptValueRef ClassInstance::Get(const std::string& key, ScriptExecuteContext& executeContext) {
		return GetInternal(GetRef().Cast<ClassInstance>(), classData, key, executeContext);
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

	void UpcastClassInstance::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		//ClassInstanceに対して型指定形式で問い合わせる
		classInstance->SetInternal(classInstance, upcastedClassData, key, value, executeContext);
	}

	ScriptValueRef UpcastClassInstance::Get(const std::string& key, ScriptExecuteContext& executeContext) {
		return classInstance->GetInternal(classInstance, upcastedClassData, key, executeContext);
	}

	void UpcastClassInstance::FetchReferencedItems(std::list<CollectableBase*>& result) {
		if (classInstance != nullptr) {
			result.push_back(classInstance.Get());
		}
		if (upcastedClassData != nullptr) {
			result.push_back(upcastedClassData.Get());
		}
	}

	ScriptValueRef ScriptValue::MakeObject(ScriptInterpreter& interpreter) {
		return Make(interpreter.CreateObject());
	}

	ScriptValueRef ScriptValue::MakeArray(ScriptInterpreter& interpreter) {
		return Make(interpreter.CreateArray());
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

	ScriptInterpreter& FunctionRequest::GetInterpreter() const {
		return executeContext.GetInterpreter();
	}
}