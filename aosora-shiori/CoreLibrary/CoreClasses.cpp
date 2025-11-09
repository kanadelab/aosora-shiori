#include <random>
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
	
	
	void ClassData::SetToInstance(const std::string& key, const ScriptValueRef& value, const Reference<ClassInstance>& instance, ScriptExecuteContext& executeContext) {

		if (metadata->IsScriptClass()) {
			//スクリプトクラスではこないはず（キーバリューストアへのアクセスになるため）
			assert(false);
			return;
		}
		else {
			//ネイティブクラスの場合はインスタンス内のネイティブオブジェクトに問い合わせを回す
			assert(instance->GetNativeBaseInstance() != nullptr);
			instance->GetNativeBaseInstance()->Set(key, value, executeContext);
			return;
		}

		//見つからない場合さらに親を見る
		if (parentClass != nullptr) {
			parentClass->SetToInstance(key, value, instance, executeContext);
		}
	}

	ScriptValueRef ClassData::GetFromInstance(const std::string& key, const Reference<ClassInstance>& instance, ScriptExecuteContext& executeContext) {

		if (metadata->IsScriptClass()) {
			if (methods.contains(key)) {
				Reference< InstancedOverloadFunctionList> res = executeContext.GetInterpreter().CreateNativeObject<InstancedOverloadFunctionList>(methods[key], ScriptValue::Make(instance));
				res->SetName(key);
				return ScriptValue::Make(res);
			}
		}
		else {
			//ネイティブクラスの場合はインスタンス内のネイティブオブジェクトに問い合わせを回す
			assert(instance->GetNativeBaseInstance() != nullptr);
			return instance->GetNativeBaseInstance()->Get(key, executeContext);
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

	void ClassData::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
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

	ScriptValueRef ClassData::Get(const std::string& key, ScriptExecuteContext& executeContext) {
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
	void ScriptError::FetchReferencedItems(std::list<CollectableBase*>& result) {
		//なし
	}

	ScriptValueRef ScriptError::Get(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "ToString") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptError::ScriptToString, GetRef()));
		}
		return ScriptValue::Null;
	}

	void ScriptError::CreateObject(const FunctionRequest& req, FunctionResponse& res) {
		std::string message = "<no message>";
		if (req.GetArgumentCount() > 0) {
			message = req.GetArgument(0)->ToString();
		}
		res.SetReturnValue(ScriptValue::Make(req.GetContext().GetInterpreter().CreateNativeObject<ScriptError>(message)));
	}

	//呼出順のリストを作成
	//あらかじめシャッフルしておいて上から順に見ることで重複回避ということにする

	void OverloadedFunctionList::ScriptReturnThisFunc(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(request.GetThisValue());
	}

	void OverloadedFunctionList::ScriptUseNoOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response) {
		auto* self = request.GetInterpreter().InstanceAs<OverloadedFunctionList>(request.GetThisValue());
		if (self != nullptr) {
			self->SetSelectorMode(SelectorMode::NoOverwrappedRandom);
		}
	}

	void OverloadedFunctionList::ScriptUseOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response) {
		auto* self = request.GetInterpreter().InstanceAs<OverloadedFunctionList>(request.GetThisValue());
		if (self != nullptr) {
			self->SetSelectorMode(SelectorMode::OverwrappedRandom);
		}
	}

	void OverloadedFunctionList::ScriptIsNoOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response) {
		auto* self = request.GetInterpreter().InstanceAs<OverloadedFunctionList>(request.GetThisValue());
		if (self != nullptr) {
			response.SetReturnValue(ScriptValue::Make(self->GetSelectorMode() != SelectorMode::NoOverwrappedRandom));
		}
		response.SetReturnValue(ScriptValue::False);
	}

	void OverloadedFunctionList::ScriptIsOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response) {
		auto* self = request.GetInterpreter().InstanceAs<OverloadedFunctionList>(request.GetThisValue());
		if (self != nullptr) {
			response.SetReturnValue(ScriptValue::Make(self->GetSelectorMode() != SelectorMode::OverwrappedRandom));
		}
		response.SetReturnValue(ScriptValue::False);
	}

	void OverloadedFunctionList::ShuffleOverwrap(const FunctionRequest& request, FunctionResponse& response) {
		auto* self = request.GetInterpreter().InstanceAs<OverloadedFunctionList>(request.GetThisValue());
		if (self != nullptr) {
			//callOrderがクリアされて呼び出し順が再生成されるようにする
			self->callOrder.clear();
		}
	}

	void OverloadedFunctionList::ScriptClear(const FunctionRequest& request, FunctionResponse& response) {
		auto* self = request.GetInterpreter().InstanceAs<OverloadedFunctionList>(request.GetThisValue());
		if (self != nullptr) {
			//内部リストをクリア
			self->callOrder.clear();
			self->functions.clear();
		}
	}

	void OverloadedFunctionList::ScriptAdd(const FunctionRequest& request, FunctionResponse& response) {

		//内容を追加
		if (request.GetArgumentCount() < 1) {
			return;
		}

		auto* self = request.GetInterpreter().InstanceAs<OverloadedFunctionList>(request.GetThisValue());
		if (self != nullptr) {
			ScriptValueRef value = request.GetArgument(0);
			ScriptValueRef cond = nullptr;
			if (request.GetArgumentCount() >= 2) {
				cond = request.GetArgument(1);
			}

			self->Add(value, cond);
		}
	}

	void OverloadedFunctionList::ScriptAddRange(const FunctionRequest& request, FunctionResponse& response) {

		//内容を追加
		if (request.GetArgumentCount() < 1) {
			return;
		}

		auto* self = request.GetInterpreter().InstanceAs<OverloadedFunctionList>(request.GetThisValue());
		if (self != nullptr) {
			auto* value = request.GetInterpreter().InstanceAs<ScriptArray>(request.GetArgument(0));
			if (value == nullptr) {
				return;
			}

			ScriptValueRef cond = nullptr;
			if (request.GetArgumentCount() >= 2) {
				cond = request.GetArgument(1);
			}

			for (size_t i = 0; i < value->Count(); i++) {
				self->Add(value->At(i), cond);
			}
		}
	}

	void OverloadedFunctionList::MakeCallorder() {
		
		callOrder.resize(functions.size());
		for (size_t i = 0; i < callOrder.size(); i++) {
			callOrder[i] = i;
		}

		//シャッフル
		std::shuffle(callOrder.begin(), callOrder.end(), GetInternalRandom());
	}

	const OverloadedFunctionList::FunctionItem* OverloadedFunctionList::SelectItemInternal(const FunctionRequest& request, FunctionResponse& response) {
		//アイテムが無い
		if (functions.empty()) {
			return nullptr;
		}

		switch (selectorMode)
		{
			case sakura::OverloadedFunctionList::SelectorMode::NoOverwrappedRandom:
				return SelectItemInternalNoOverwrappedRandom(request, response);
			case sakura::OverloadedFunctionList::SelectorMode::OverwrappedRandom:
				return SelectItemInternalOverwrapedRandom(request, response);
			default:
				break;
		}
		return nullptr;
	}
	
	const OverloadedFunctionList::FunctionItem* OverloadedFunctionList::SelectItemInternalNoOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response) {

		bool isSuffled = false;

		while (true) {
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

			//条件評価
			if (ValidateItemCondition(request, response, item)) {
				return &item;
			}

			//例外が出ている場合キャンセル
			if (response.IsThrew()) {
				return nullptr;
			}
		}

		return nullptr;
	}

	const OverloadedFunctionList::FunctionItem* OverloadedFunctionList::SelectItemInternalOverwrapedRandom(const FunctionRequest& request, FunctionResponse& response) {
		MakeCallorder();

		while (!callOrder.empty()) {
			//あらかじめシャッフルしたリストから１つ選択する
			const size_t index = *callOrder.rbegin();
			callOrder.pop_back();
			auto& item = functions[index];

			//条件評価
			if (ValidateItemCondition(request, response, item)) {
				return &item;
			}

			//例外が出ている場合キャンセル
			if (response.IsThrew()) {
				return nullptr;
			}
		}
		return nullptr;
	}

	bool OverloadedFunctionList::ValidateItemCondition(const FunctionRequest& request, FunctionResponse& response, const FunctionItem& item) {
		if (item.condition != nullptr) {
			//条件評価
			//TODO: 評価する場合新しいスタックフレームを使う必要があるかもしれない？
			auto conditionResult = ScriptExecutor::ExecuteASTNode(*item.condition, request.GetContext());
			assert(conditionResult != nullptr);

			if (conditionResult->ToBoolean()) {
				return true;
			}
		}
		else if (item.scriptCondition != nullptr && item.scriptCondition->IsObject() && item.scriptCondition->GetObjectRef()->CanCall()) {
			//条件デリゲートを使用する場合は関数よびだしを実行
			std::vector<ScriptValueRef> args;
			FunctionResponse res;
			request.GetInterpreter().CallFunction(*item.scriptCondition, res, args, request.GetContext(), request.GetContext().GetStack().GetCallingASTNode());

			if (res.IsThrew()) {
				return false;
			}
			else if (res.GetReturnValue() != nullptr && res.GetReturnValue()->ToBoolean()) {
				return true;
			}
		}
		else {
			//選択したものに条件がついてなければ決定
			return true;
		}
		return false;
	}

	void OverloadedFunctionList::Call(const FunctionRequest& request, FunctionResponse& response) {
		ThisCall(request, response, nullptr);
	}

	void OverloadedFunctionList::ThisCall(const FunctionRequest& request, FunctionResponse& response, const ScriptValueRef& thisValue) {
		auto selectedItem = SelectItem(request.GetContext(), thisValue);
		if (selectedItem->IsObject()) {
			//関数呼び出しを実行、そのままレスポンスをもらって帰る
			std::vector<ScriptValueRef> args = request.GetArgumentCollection();
			request.GetContext().GetInterpreter().CallFunction(*selectedItem, response, args, request.GetContext(), nullptr, funcName);
		}
	}

	ScriptValueRef OverloadedFunctionList::Get(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "length") {
			return ScriptValue::Make(static_cast<number>(functions.size()));
		}

		else if (key == "UseNoOverwrappedRandom") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ScriptUseNoOverwrappedRandom, GetRef()));
		}
		else if (key == "UseOverwrappedRandom") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ScriptUseOverwrappedRandom, GetRef()));
		}
		else if (key == "IsNoOverwrappedRandom") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ScriptIsNoOverwrappedRandom, GetRef()));
		}
		else if (key == "IsOverwrappedRandom") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ScriptIsOverwrappedRandom, GetRef()));
		}
		else if (key == "ShuffleOverwrap") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ShuffleOverwrap, GetRef()));
		}
		else if (key == "Clear") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ScriptClear, GetRef()));
		}
		else if (key == "Add") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ScriptAdd, GetRef()));
		}
		else if (key == "AddRange") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ScriptAddRange, GetRef()));
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
		if (item->nativeFunc == nullptr) {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(item->scriptFunc, thisValue, item->blockScope));
		}
		else if(item->scriptFunc != nullptr) {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(item->nativeFunc, thisValue, item->blockScope));
		}
		else if (item->scriptItem != nullptr) {
			if (item->scriptItem->IsObject() && item->scriptItem->GetObjectRef()->CanCall()) {
				return item->scriptItem;
			}
			else {
				//呼び出し可能オブジェクトでない場合は、それ自体を戻り値として返す特殊なデリゲートを作る
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&OverloadedFunctionList::ScriptReturnThisFunc, item->scriptItem));
			}
		}

		return nullptr;
	}

	void OverloadedFunctionList::FetchReferencedItems(std::list<CollectableBase*>& result) {
		for (FunctionItem& item : functions) {
			result.push_back(item.blockScope.Get());

			if (item.scriptCondition != nullptr) {
				result.push_back(item.scriptCondition->GetObjectRef().Get());
			}

			if (item.scriptItem != nullptr) {
				result.push_back(item.scriptItem->GetObjectRef().Get());
			}
			
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
	ScriptSourceMetadataRef Reflection::GetCallingSourceMetadata(const FunctionRequest& request) {
		//スタックの1段上を参照して呼び出し元のデータを取得
		return request.GetContext().GetStack().GetParentStackSourceMetadata();
	}

	void Reflection::ScopeGet(const FunctionRequest& request, FunctionResponse& response) {
		//スクリプト呼び出し元のユニットを取得
		auto sourcemeta = GetCallingSourceMetadata(request);
		if (sourcemeta == nullptr) {
			assert(false);
			response.SetReturnValue(ScriptValue::Null);
			return;
		}

		//スコープから指定された文字列で検索する
		if (request.GetArgumentCount() >= 1) {
			response.SetReturnValue(request.GetContext().GetSymbol(request.GetArgument(0)->ToString(), *sourcemeta));
		}
		else {
			//TODO: 例外投げるべき？
			response.SetReturnValue(ScriptValue::Null);
		}
	}

	void Reflection::ScopeSet(const FunctionRequest& request, FunctionResponse& response) {
		//スクリプト呼び出し元のユニットを取得
		auto sourcemeta = GetCallingSourceMetadata(request);
		if (sourcemeta == nullptr) {
			assert(false);
			return;
		}

		if (request.GetArgumentCount() >= 2) {
			request.GetContext().SetSymbol(request.GetArgument(0)->ToString(), request.GetArgument(1), *sourcemeta);
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

	Reference<ScriptIterator> ScriptArray::CreateIterator(ScriptExecuteContext& executeContext) {
		return executeContext.GetInterpreter().CreateNativeObject<ScriptArrayIterator>(Reference<ScriptArray>(this));
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

	void ScriptArrayIterator::FetchReferencedItems(std::list<CollectableBase*>& result) {
		ScriptIterator::FetchReferencedItems(result);
		result.push_back(targetArray.Get());
	}

	void TalkBuilderSettings::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
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

	ScriptValueRef TalkBuilderSettings::Get(const std::string& key, ScriptExecuteContext& executeContext) {
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

	//ユニットオブジェクト
	ScriptValueRef UnitObject::Get(const std::string& key, ScriptExecuteContext& executeContext) {
		if (!path.empty()) {
			return executeContext.GetInterpreter().GetUnitVariable(key, path);
		}
		else {
			//ルートからユニット取得
			return ScriptValue::Make(executeContext.GetInterpreter().GetUnit(key));
		}
	}

	void UnitObject::Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) {
		if (!path.empty()) {
			executeContext.GetInterpreter().SetUnitVariable(key, value, path);
		}
		else {
			//ルートへの書き込み禁止
		}
	}

	ScriptValueRef UnitObject::Get(const std::string& key, ScriptInterpreter& interpreter) {
		if (!path.empty()) {
			return interpreter.GetUnitVariable(key, path);
		}
		else {
			//ルートからユニット取得
			return ScriptValue::Make(interpreter.GetUnit(key));
		}
	}

	void UnitObject::Set(const std::string& key, const ScriptValueRef& value, ScriptInterpreter& interpreter) {
		if (!path.empty()) {
			interpreter.SetUnitVariable(key, value, path);
		}
		else {
			//ルートへの書き込み禁止
		}
	}


}