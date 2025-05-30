﻿#include <random>
#include "CoreLibrary/CoreClasses.h"
#include "Interpreter/Interpreter.h"
#include "Misc/Utility.h"

namespace sakura {

	//ブロックスコープ
	void BlockScope::FetchReferencedItems(std::list<CollectableBase*>& result) {
		//thisとローカル変数を全部返す
		for (const auto& item : localVariables) {
			if (item.second->IsObject()) {
				result.push_back(item.second->GetObjectRef().Get());
			}
		}

		if (thisValue != nullptr && thisValue->IsObject()) {
			result.push_back(thisValue->GetObjectRef().Get());
		}

		result.push_back(parentScope.Get());
	}

	void Delegate::FetchReferencedItems(std::list<CollectableBase*>& result) {
		if (thisValue->IsObject()) {
			result.push_back(thisValue->GetObjectRef().Get());
		}
		result.push_back(blockScope.Get());
	}

	//クラス情報
	ClassData::ClassData(const std::shared_ptr<const ClassBase>& meta, uint32_t classTypeId, ScriptInterpreter* interpreter) :
		metadata(meta),
		classId(classTypeId)
	{
		//スクリプトクラスのIDはインタプリタが決定するためmetaの情報を使わない
		assert(classId != ObjectTypeIdGenerator::INVALID_ID);
		upcastTypes.insert(classId);

		//関数データのインポート
		if (metadata->IsScriptClass()) {
			const ScriptClass& scriptMetadata = static_cast<const ScriptClass&>(*metadata);
			for (size_t i = 0; i < scriptMetadata.GetFunctionCount(); i++) {
				const ScriptFunctionDef& d = scriptMetadata.GetFunction(i);
				for (const std::string& name : d.names) {
					if (!methods.contains(name)) {
						methods[name] = interpreter->CreateNativeObject<OverloadedFunctionList>();
					}
					methods[name]->Add(d.func, d.condition);
				}
			}
		}
	}

	void ClassData::SetToInstance(const std::string& key, const ScriptValueRef& value, ScriptObject& instance, ScriptExecuteContext& executeContext) {

		if (metadata->IsScriptClass()) {
			//スクリプトクラスを示している場合はそちらから検索する
			const ScriptClass& scriptMetadata = static_cast<const ScriptClass&>(*metadata);
			if (scriptMetadata.ContainsMember(key)) {
				//キーが有効であれば中身をつかってよい
				instance.RawSet(key, value);
				return;
			}
			else if (methods.contains(key)) {
				//メソッドとして存在していたら書き込み無効
				return;
			}

		}
		else {
			//ネイティブクラスの場合はインスタンス内のネイティブオブジェクトに問い合わせを回す
			assert(instance.GetNativeBaseInstance() != nullptr);
			instance.GetNativeBaseInstance()->Set(instance.GetNativeBaseInstance(), key, value, executeContext);
			return;
		}

		//見つからない場合さらに親を見る
		if (parentClass != nullptr) {
			parentClass->SetToInstance(key, value, instance, executeContext);
		}
	}

	ScriptValueRef ClassData::GetFromInstance(const std::string& key, ScriptObject& instance, ScriptExecuteContext& executeContext) {

		if (metadata->IsScriptClass()) {
			//スクリプトクラスを示している場合はそちらから検索する
			const ScriptClass& scriptMetadata = static_cast<const ScriptClass&>(*metadata);
			if (scriptMetadata.ContainsMember(key)) {
				//キーが有効であれば中身をつかってよい
				return instance.RawGet(key);
			}
			else if (methods.contains(key)) {
				//WARN: ObjectRefを作り直しているのがまずそう
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<InstancedOverloadFunctionList>(methods[key], 
					ScriptValue::Make(ObjectRef(&instance))));
			}
		}
		else {
			//ネイティブクラスの場合はインスタンス内のネイティブオブジェクトに問い合わせを回す
			assert(instance.GetNativeBaseInstance() != nullptr);
			return instance.GetNativeBaseInstance()->Get(instance.GetNativeBaseInstance(), key, executeContext);
		}

		//見つからない場合さらに親を見る
		if (parentClass != nullptr) {
			return parentClass->GetFromInstance(key, instance, executeContext);
		}

		return nullptr;
	}

	void ClassData::FetchReferencedItems(std::list<CollectableBase*>& result) {
		result.push_back(parentClass.Get());
		result.push_back(scriptStaticData.Get());

		for (auto kv : methods) {
			result.push_back(kv.second.Get());
		}

		for (auto kv : staticMethods) {
			result.push_back(kv.second.Get());
		}
	}

	void ClassData::Set(const ObjectRef& self, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		if (metadata->IsScriptClass()) {
			//いまのところスクリプトクラスにstaticがない
		}
		else {
			auto nativeClass = std::static_pointer_cast<const NativeClass>(metadata);
			auto* staticSetter = nativeClass->GetStaticSetFunc();
			if (staticSetter != nullptr) {
				staticSetter(key, value, executeContext);
			}
		}
	}

	ScriptValueRef ClassData::Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) {
		if (metadata->IsScriptClass()) {
			//いまのところない
		}
		else {
			auto nativeClass = std::static_pointer_cast<const NativeClass>(metadata);
			auto* staticGetter = nativeClass->GetStaticGetFunc();
			if (staticGetter != nullptr) {
				return staticGetter(key, executeContext);
			}
		}

		return nullptr;
	}

	//エラー基底
	void RuntimeError::FetchReferencedItems(std::list<CollectableBase*>& result) {
		//なし
	}

	void RuntimeError::CreateObject(const FunctionRequest& req, FunctionResponse& res) {
		std::string message = "<no message>";
		if (req.GetArgumentCount() > 0) {
			message = req.GetArgument(0)->ToString();
		}
		res.SetReturnValue(ScriptValue::Make(req.GetContext().GetInterpreter().CreateNativeObject<RuntimeError>(message)));
	}

	//呼出順のリストを作成
	//あらかじめシャッフルしておいて上から順に見ることで重複回避ということにする
	void OverloadedFunctionList::MakeCallorder() {
		
		callOrder.resize(functions.size());
		for (size_t i = 0; i < callOrder.size(); i++) {
			callOrder[i] = i;
		}

		//シャッフル
		std::shuffle(callOrder.begin(), callOrder.end(), GetInternalRandom());
	}

	//関数オーバーロードオブジェクト
	const OverloadedFunctionList::FunctionItem* OverloadedFunctionList::SelectItemInternal(const FunctionRequest& request, FunctionResponse& response) {

		//アイテムが無い
		if (functions.empty()) {
			return nullptr;
		}

		bool isSuffled = false;

		while (true)
		{
			//重複回避リストの初期化
			if (callOrder.empty()) {
				if (!isSuffled) {
					MakeCallorder();
					isSuffled = true;
				}
				else {
					//重複回避リストを再生成したのに結局見つからなかったので、条件一致なしとして打ち切り
					break;
				}
			}

			//あらかじめシャッフルしたリストから１つ選択する
			const size_t index = *callOrder.rbegin();
			callOrder.pop_back();
			auto& item = functions[index];

			//選択したものに条件がついてなければ決定
			if (item.condition == nullptr) {
				return &item;
			}
			else {
				//条件評価
				//TODO: 評価する場合新しいスタックフレームを使う必要があるかもしれない？
				auto conditionResult = ScriptExecutor::ExecuteASTNode(*item.condition, request.GetContext());
				assert(conditionResult != nullptr);

				if (conditionResult->ToBoolean()) {
					return &item;
				}
			}
		}

		return nullptr;
	}

	void OverloadedFunctionList::Call(const FunctionRequest& request, FunctionResponse& response) {
		ThisCall(request, response, nullptr);
	}

	void OverloadedFunctionList::ThisCall(const FunctionRequest& request, FunctionResponse& response, const ScriptValueRef& thisValue) {
		//TODO: thisの考慮が必要な場合の検討
		auto selectedItem = SelectItem(request.GetContext(), nullptr);
		if (selectedItem->IsObject()) {
			//関数呼び出しを実行、そのままレスポンスをもらって帰る
			std::vector<ScriptValueRef> args = request.GetArgumentCollection();
			request.GetContext().GetInterpreter().CallFunction(*selectedItem, response, args, request.GetContext(), nullptr, funcName);
		}
	}

	ScriptValueRef OverloadedFunctionList::Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "length") {
			return ScriptValue::Make(static_cast<number>(functions.size()));
		}
		return nullptr;
	}

	ScriptValueRef OverloadedFunctionList::SelectItem(ScriptExecuteContext& executeContext, const ScriptValueRef& thisValue) {
		FunctionRequest request(executeContext);
		FunctionResponse response;

		const FunctionItem* item = SelectItemInternal(request, response);
		if (item == nullptr) {
			//条件が一致しなかった場合など見つからなかった場合
			return ScriptValue::Null;
		}

		//デリゲートを返す
		if (item->nativeFunc == nullptr)
		{
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(item->scriptFunc, thisValue, item->blockScope));
		}
		else
		{
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(item->nativeFunc, thisValue, item->blockScope));
		}
	}

	void OverloadedFunctionList::FetchReferencedItems(std::list<CollectableBase*>& result) {
		for (FunctionItem& item : functions) {
			result.push_back(item.blockScope.Get());
		}
	}


	void InstancedOverloadFunctionList::Call(const FunctionRequest& request, FunctionResponse& response) {
		func->ThisCall(request, response, thisValue);
	}

	void InstancedOverloadFunctionList::FetchReferencedItems(std::list<CollectableBase*>& result) {
		if (thisValue->IsObject()) {
			result.push_back(thisValue->GetObjectRef().Get());
		}
		result.push_back(func.Get());
	}

	//リフレクション
	void Reflection::ScopeGet(const FunctionRequest& request, FunctionResponse& response) {
		//スコープから指定された文字列で検索する
		if (request.GetArgumentCount() >= 1) {
			response.SetReturnValue(request.GetContext().GetSymbol(request.GetArgument(0)->ToString()));
		}
		else {
			//TODO: 例外投げるべき？
			response.SetReturnValue(ScriptValue::Null);
		}
	}

	void Reflection::ScopeSet(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 2) {
			request.GetContext().SetSymbol(request.GetArgument(0)->ToString(), request.GetArgument(1));
		}
	}

	ScriptValueRef Reflection::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "Get") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Reflection::ScopeGet));
		}
		else if (key == "Set") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Reflection::ScopeSet));
		}
		return nullptr;
	}


	void ScriptArray::FetchReferencedItems(std::list<CollectableBase*>& result) {
		for (auto& item : members) {
			if (item->IsObject()) {
				result.push_back(item->GetObjectRef().Get());
			}
		}
	}

	std::string ScriptArray::DebugToString(ScriptExecuteContext& executeContext, DebugOutputContext& debugOutputContext) {
		if (members.empty()) {
			//からっぽ
			return "[]";
		}

		//ディクショナリ形式で文字列化
		std::string result("[");
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
				result.append(item->DebugToString(executeContext, debugOutputContext));
			}
		}
		debugOutputContext.AppendNewLine(result);
		result.append("]");
		return result;
	}

	void TalkBuilderSettings::Set(const ObjectRef& self, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		if (key == "AutoLineBreak") {
			autoLineBreak = value->ToString();
		}
		else if (key == "ScopeChangeLineBreak") {
			scopeChangeLineBreak = value->ToString();
		}
		else if (key == "Head") {
			scriptHead = value->ToString();
		}
	}

	ScriptValueRef TalkBuilderSettings::Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "AutoLineBreak") {
			return ScriptValue::Make(autoLineBreak);
		}
		else if (key == "ScopeChangeLineBreak") {
			return ScriptValue::Make(scopeChangeLineBreak);
		}
		else if (key == "Head") {
			return ScriptValue::Make(scriptHead);
		}
		return nullptr;
	}

	const char* TalkBuilder::NAME_DEFAULT_SETTINGS = "Default";
	const char* TalkBuilder::NAME_CURRENT_SETTINGS = "Current";

	ScriptValueRef TalkBuilder::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == NAME_DEFAULT_SETTINGS) {
			return executeContext.GetInterpreter().StaticStore<TalkBuilder>()->RawGet(NAME_DEFAULT_SETTINGS);
		}
		else if (key == NAME_CURRENT_SETTINGS) {
			return executeContext.GetInterpreter().StaticStore<TalkBuilder>()->RawGet(NAME_CURRENT_SETTINGS);
		}
		return nullptr;
	}

	void TalkBuilder::StaticInit(ScriptInterpreter& interpreter) {
		//デフォルト設定用オブジェクトを追加
		auto staticStore = interpreter.StaticStore<TalkBuilder>();
		staticStore->RawSet(NAME_DEFAULT_SETTINGS, ScriptValue::Make(interpreter.CreateNativeObject<TalkBuilderSettings>()));
	}

	void TalkBuilder::Prepare(ScriptInterpreter& interpreter) {
		//デフォルトからクローンを作成
		auto staticStore = interpreter.StaticStore<TalkBuilder>();
		auto* globalObj = interpreter.InstanceAs<TalkBuilderSettings>(staticStore->RawGet(NAME_DEFAULT_SETTINGS));
		auto currentObj = interpreter.CreateNativeObject<TalkBuilderSettings>(*globalObj);
		staticStore->RawSet(NAME_CURRENT_SETTINGS, ScriptValue::Make(currentObj));
	}

	TalkBuilderSettings& TalkBuilder::GetCurrentSettings(ScriptInterpreter& interpreter) {
		return *interpreter.InstanceAs<TalkBuilderSettings>(interpreter.StaticStore<TalkBuilder>()->RawGet(NAME_CURRENT_SETTINGS));
	}

	const std::string& TalkBuilder::GetAutoLineBreak(ScriptInterpreter& interpreter) {
		return GetCurrentSettings(interpreter).GetLineBreak();
	}

	const std::string& TalkBuilder::GetScopeChangeLineBreak(ScriptInterpreter& interpreter) {
		return GetCurrentSettings(interpreter).GetScopeChangeLineBreak();
	}

	const std::string& TalkBuilder::GetScriptHead(ScriptInterpreter& interpreter) {
		return GetCurrentSettings(interpreter).GetScriptHead();
	}
}