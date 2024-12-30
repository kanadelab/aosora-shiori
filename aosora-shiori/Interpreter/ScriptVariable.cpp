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

	ScriptValueRef ObjectBase::Get(const std::string& key, ScriptExecuteContext& executeContext) {
		return nullptr;
	}

	void ObjectBase::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
	}


	void ScriptObject::ScriptPush(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() > 0) {
			ScriptObject* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptObject>(request.GetContext().GetBlockScope()->GetThisValue());
			obj->Push(request.GetArgument(0));
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

	void ScriptObject::Push(const ScriptValueRef& val) {
		//順当に配列ならば最後の要素に設定する形になるはず
		members[ToString(members.size())] = val;
	}

	void ScriptObject::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext)  {

		//クラスデータが設定されている場合そちらに設定する
		if (scriptClass != nullptr) {
			scriptClass->SetToInstance(key, value, *this, executeContext);
		}
		else {
			members[key] = value;
		}
	}

	ScriptValueRef ScriptObject::Get(const std::string& key, ScriptExecuteContext& executeContext) {

		//クラスデータが設定されている場合そちら経由で取得する
		if (scriptClass != nullptr) {
			return scriptClass->GetFromInstance(key, *this, executeContext);
		}
		else {
			auto rawgetResult = RawGet(key);
			if (rawgetResult != nullptr) {
				return rawgetResult;
			}
			
			//組み込みフィールド
			if (key == "length") {
				return ScriptValue::Make(static_cast<number>(members.size()));
			}
			else if (key == "Push") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptObject::ScriptPush, executeContext.GetBlockScope()->GetThisValue()));
			}
			return nullptr;
		}
	}

	void ScriptObject::FetchReferencedItems(std::list<CollectableBase*>& result) {
		for (auto item : members) {
			if (item.second->GetValueType() == ScriptValueType::Object) {
				result.push_back(item.second->GetObjectRef().Get());
			}
		}
	}

	void ScriptObject::SetClassInfo(const Reference<ClassData>& classData) {
		scriptClass = classData;

		//タイプIDをScriptObjectからクラスへ上書き
		SetInstanceTypeId(scriptClass->GetClassTypeId());
	}


	ScriptValueRef ScriptValue::MakeObject(ScriptInterpreter& interpreter) {
		return ScriptValueRef(new ScriptValue(interpreter.CreateObject()));
	}

	std::string ScriptValue::ToStringWithFunctionCall(ScriptExecuteContext& executeContext) {
		if (IsObject()) {
			//もしオブジェクトなら呼び出し可能かどうかを評価、可能なら引数なしで呼んだ結果を返す
			auto objRef = GetObjectRef();
			if (objRef->CanCall()) {
				std::vector<ScriptValueRef> args;
				FunctionResponse res;
				executeContext.GetInterpreter().CallFunction(*this, res, args, executeContext, nullptr);
				if (res.GetReturnValue() != nullptr) {
					return res.GetReturnValue()->ToStringWithFunctionCall(executeContext);
				}
			}

			//呼び出せないため結果無効
			return "";
		}
		else {
			return ToString();
		}
	}

	//内部でコンテキストを作るヘルパ
	std::string ScriptValue::ToStringWithFunctionCall(ScriptInterpreter& interpreter) {
		//TODO: 例外処理とかも気にする必要あるかも･･･
		ScriptInterpreterStack rootStack;
		Reference<BlockScope> rootBlock = interpreter.CreateNativeObject<BlockScope>(nullptr);
		ScriptExecuteContext executeContext(interpreter, rootStack, rootBlock);
		return ToStringWithFunctionCall(executeContext);
	}
}