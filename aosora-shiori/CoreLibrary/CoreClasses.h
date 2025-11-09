#pragma once
#include "Interpreter/Interpreter.h"
#include <set>

//スクリプトコアライブラリ。
//スクリプト実行系が生成するなど必須のクラス
namespace sakura {


	class OverloadedFunctionList;

	//ブロックスコープ
	class BlockScope : public Object<BlockScope> {
	private:
		//ローカル変数スロット
		std::map<std::string, ScriptValueRef> localVariables;

		//親スコープ
		Reference<BlockScope> parentScope;

		//this
		ScriptValueRef thisValue;

	public:
		BlockScope(const Reference<BlockScope>& parent):
		parentScope(parent)
		{}

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;

		void RegisterLocalVariable(const std::string& name, const ScriptValueRef& variable) {
			if (localVariables.contains(name)) {
				//多重宣言は不可なのでできれば例外発出か何かしたいけど
				assert(false);
			}

			//追加
			localVariables.insert(std::map<std::string, ScriptValueRef>::value_type(name, variable));
		}

		//ローカル変数の取得
		ScriptValueRef GetLocalVariable(const std::string& name) {
			//値を探して返す
			auto it = localVariables.find(name);
			if (it != localVariables.end()) {
				return it->second;
			}
			else if(parentScope != nullptr) {
				//見つからなかった場合は親ブロックに再帰的に問い合わせる
				return parentScope->GetLocalVariable(name);
			}
			else {
				//見つからない場合、nullptrを返す
				return nullptr;
			}
		}

		//ローカル変数の設定
		bool SetLocalVariable(const std::string& name, const ScriptValueRef& value) {
			auto it = localVariables.find(name);
			if (it != localVariables.end()) {
				it->second = value;
				return true;
			}
			else if(parentScope != nullptr) {
				//見つからなかった場合親ブロックに再帰的に問い合わせる
				return parentScope->SetLocalVariable(name, value);
			}
			else {
				//宣言されてない
				return false;
			}
		}

		//ローカル変数のコレクションを直接取得
		const std::map<std::string, ScriptValueRef>& GetLocalVariableCollection() const {
			return localVariables;
		}

		//thisの設定
		void SetThisValue(const ScriptValueRef& value) {
			thisValue = value;
		}

		ScriptValueRef GetThisValue() const {
			if (thisValue != nullptr) {
				return thisValue;
			}
			else if(parentScope != nullptr) {
				//見つからなかった場合親ブロックに再帰的に問い合わせる
				return parentScope->GetThisValue();
			}
			else {
				return nullptr;
			}
		}

		//親スコープの取得
		const Reference<BlockScope>& GetParentScope() const {
			return parentScope;
		}
	};

	//デリゲートオブジェクト
	class Delegate : public Object<Delegate> {
	private:
		ConstScriptFunctionRef scriptFunc;
		ScriptNativeFunction nativeFunc;
		ScriptValueRef thisValue;				//取得元のオブジェクト this
		Reference<BlockScope> blockScope;		//関数定義時のブロックスコープ。ローカル変数キャプチャ領域。

	public:
		Delegate(const ConstScriptFunctionRef& func, const ScriptValueRef& thisVal = ScriptValue::Null, const Reference<BlockScope>& scope = nullptr) :
			scriptFunc(func),
			nativeFunc(nullptr),
			thisValue(thisVal),
			blockScope(scope) {
			assert(func != nullptr);
		}

		Delegate(ScriptNativeFunction func, const ScriptValueRef& thisVal = ScriptValue::Null, const Reference<BlockScope>& scope = nullptr) :
			scriptFunc(nullptr),
			nativeFunc(func),
			thisValue(thisVal),
			blockScope(scope) {
			assert(func != nullptr);
		}

		Delegate(ScriptNativeFunction func, const ObjectRef& thisVal, const Reference<BlockScope>& scope = nullptr) :
			scriptFunc(nullptr),
			nativeFunc(func),
			thisValue(ScriptValue::Make(thisVal)),
			blockScope(scope) {
			assert(func != nullptr);
		}

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;

		//呼び出し可能だが直接Callを呼ぶのは禁止。インタプリタの呼び出し手続きで特殊な対応が入る。
		virtual bool CanCall() const { return true; }
		virtual void Call(const FunctionRequest& request, FunctionResponse& response) { assert(false); }

		ConstScriptFunctionRef GetScriptFunction() const {
			return scriptFunc;
		}

		ScriptNativeFunction GetNativeFunction() const {
			return nativeFunc;
		}

		ScriptValueRef GetThisValue() const {
			return thisValue;
		}

		const Reference<BlockScope>& GetBlockScope() const {
			return blockScope;
		}

		bool IsScriptFunction() const {
			return scriptFunc != nullptr;
		}
	};

	//スクリプトインタプリタ上のクラス表現
	class ClassData : public Object<ClassData> {
	private:
		//メタデータ
		std::shared_ptr<const ClassBase> metadata;

		//型のid
		const uint32_t classId;

		//親クラス
		Reference<ClassData> parentClass;

		//static領域
		Reference<ScriptObject> scriptStaticData;

		//メソッド情報
		std::map<std::string, Reference<OverloadedFunctionList>>  methods;

		//staticメソッド情報
		std::map<std::string, Reference<OverloadedFunctionList>>  staticMethods;

		//instanceofで一致できるクラス（自分と継承先)のID
		std::set<uint32_t> upcastTypes;

	public:
		ClassData(const std::shared_ptr<const ClassBase>& meta, uint32_t classTypeId, ScriptInterpreter* interpreter);

		//メタデータ取得
		const ClassBase& GetMetadata() const {
			return *metadata;
		}

		//型ID取得
		uint32_t GetClassTypeId() const {
			return classId;
		}

		//親クラス登録
		void SetParentClass(const Reference<ClassData>& parent) {
			parentClass = parent;
		}

		const Reference<ClassData>& GetParentClass() {
			return parentClass;
		}

		//子以降のアップキャスト関係にある型を追加
		void AddUpcastType(const Reference<ClassData>& child) {
			const uint32_t typeId = child->GetClassTypeId();
			assert(typeId != ObjectTypeIdGenerator::INVALID_ID);
			upcastTypes.insert(typeId);
		}

		//インスタンス判定
		bool InstanceIs(uint32_t objectClassId) {
			return upcastTypes.contains(objectClassId);
		}

		void SetToInstance(const std::string& key, const ScriptValueRef& value, const Reference<ClassInstance>& instance, ScriptExecuteContext& executeContext);
		ScriptValueRef GetFromInstance(const std::string& key, const Reference<ClassInstance>& instance, ScriptExecuteContext& executeContext);

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;

		
	};

	//エラーオブジェクト
	class ScriptError : public Object<ScriptError> {
	private:
		bool canCatch;
		bool hasCallstackInfo;
		int32_t errorCode;
		std::string message;
		std::vector<CallStackInfo> callStackInfo;

	public:
		ScriptError(const std::string& errorMessage):
			canCatch(true),
			hasCallstackInfo(false),
			message(errorMessage)
		{}
		
		void SetCallstackInfo(const std::vector<CallStackInfo>& info) {
			callStackInfo = info;
			hasCallstackInfo = true;
		}

		const std::vector<CallStackInfo>& GetCallStackInfo() const {
			return callStackInfo;
		}

		bool HasCallstackInfo() const {
			return hasCallstackInfo;
		}

		void SetErrorCode(int32_t code) {
			errorCode = code;
		}

		int32_t GetErrorCode() const {
			return errorCode;
		}

		//メッセージ取得
		const std::string& GetErrorMessage() const {
			return message;
		}

		const std::string ToString() const {
			std::string r;
			for (const auto& info : callStackInfo) {
				if (info.hasSourceRange) {
					r += info.sourceRange.ToString() + "\n";
				}
			}
			r += message;
			return r;
		}

		//キャッチ可能フラグの設定（パニック相当のものを同じ仕組みをつかいつつキャッチさせない目的）
		void SetCanCatch(bool can) {
			canCatch = can;
		}

		bool CanCatch() const {
			return canCatch;
		}

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;
		static void CreateObject(const FunctionRequest& req, FunctionResponse& res);

		//スクリプト向け実装
		static void ScriptToString(const FunctionRequest& request, FunctionResponse& response) {
			ScriptError* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptError>(request.GetContext().GetBlockScope()->GetThisValue());
			//スタックはなしでエラーメッセージだけにしておく
			response.SetReturnValue(ScriptValue::Make(obj->message));
		}
	};

	//ランタイム側のエラー
	class RuntimeError : public ScriptError {
	public:
		RuntimeError(const std::string& errorMessage) :ScriptError(errorMessage)
		{
			SetNativeOverrideInstanceId(TypeId());
		}

		//Object<>の継承からさらに継承なので別途TypeIdを定義しないといけない
		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<RuntimeError>();
		}
	};

	//アサートエラー
	class AssertError : public ScriptError {
	public:
		AssertError(const std::string& errorMessage) :ScriptError(errorMessage)
		{
			SetNativeOverrideInstanceId(TypeId());

			//catch不可エラーとして実現する
			SetCanCatch(false);
		}

		//Object<>の継承からさらに継承なので別途TypeIdを定義しないといけない
		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<AssertError>();
		}
	};

	//プラグインエラー、aosoraプラグインが標準で発生させるエラー
	class PluginError : public ScriptError {
	public:
		PluginError(const std::string& errorMessage) : ScriptError(errorMessage) {
			SetNativeOverrideInstanceId(TypeId());
		}

		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<PluginError>();
		}
	};


	//オーバーロードを許容する関数の集合、ただしスクリプトから単語群想定で使用できるように非関数も一応サポート
	//重複回避つきのランダム選択機能
	class OverloadedFunctionList : public Object<OverloadedFunctionList> {
	private:
		struct FunctionItem {
			ConstScriptFunctionRef scriptFunc;
			ScriptNativeFunction nativeFunc;
			Reference<BlockScope> blockScope;		//関数定義時のブロックスコープ。ローカル変数キャプチャ領域。
			ConstASTNodeRef condition;

			//後付けでスクリプト上で追加した場合のもの
			ScriptValueRef scriptCondition;
			ScriptValueRef scriptItem;
		};

		enum class SelectorMode {
			NoOverwrappedRandom,	//重複回避ランダム
			OverwrappedRandom		//純粋なランダム
		};

	private:
		SelectorMode selectorMode;
		std::vector<FunctionItem> functions;
		std::vector<size_t> callOrder;
		std::string funcName;

	private:
		const FunctionItem* SelectItemInternal(const FunctionRequest& request, FunctionResponse& response);
		const FunctionItem* SelectItemInternalNoOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response);
		const FunctionItem* SelectItemInternalOverwrapedRandom(const FunctionRequest& request, FunctionResponse& response);
		bool ValidateItemCondition(const FunctionRequest& request, FunctionResponse& response, const FunctionItem& item);

		void MakeCallorder();

	public:
		OverloadedFunctionList() :
			selectorMode(SelectorMode::NoOverwrappedRandom) {
		}

		void Add(const ConstScriptFunctionRef& func, const ConstASTNodeRef& condition, const Reference<BlockScope>& scope = nullptr) {
			FunctionItem item;
			item.scriptFunc = func;
			item.condition = condition;
			item.nativeFunc = nullptr;
			item.blockScope = scope;
			functions.push_back(item);
		}

		void Add(ScriptNativeFunction func, const ConstASTNodeRef& condition, const Reference<BlockScope>& scope = nullptr) {
			FunctionItem item;
			item.scriptFunc = nullptr;
			item.condition = condition;
			item.nativeFunc = func;
			item.blockScope = scope;
			functions.push_back(item);
		}

		void Add(const ScriptValueRef& value, const ScriptValueRef& condition = ScriptValue::Null) {
			FunctionItem item;
			item.scriptItem = value;
			item.scriptCondition = condition;
			functions.push_back(item);
		}

		//定義時の関数名。別変数に代入できてしまうので、デバッグ用に登録時の名前をとっておく。
		void SetName(const std::string& name) {
			funcName = name;
		}

		const std::string& GetName() const {
			return funcName;
		}

		//セレクタモード
		void SetSelectorMode(SelectorMode mode) {
			if (selectorMode != mode) {
				//変更時は呼び出し情報をクリアしてリセットをかける
				callOrder.clear();
				selectorMode = mode;
			}
		}

		SelectorMode GetSelectorMode() const {
			return selectorMode;
		}

		static void ScriptReturnThisFunc(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptUseNoOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptUseOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptIsNoOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptIsOverwrappedRandom(const FunctionRequest& request, FunctionResponse& response);
		static void ShuffleOverwrap(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptClear(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptAdd(const FunctionRequest& request, FunctionResponse& response);
		static void ScriptAddRange(const FunctionRequest& request, FunctionResponse& response);

		ScriptValueRef SelectItem(ScriptExecuteContext& executeContext, const ScriptValueRef& thisValue);
		void ThisCall(const FunctionRequest& request, FunctionResponse& response, const ScriptValueRef& thisValue);

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
		virtual bool CanCall() const override { return true; }
		virtual void Call(const FunctionRequest& request, FunctionResponse& response);
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;
	};

	//インスタンス付きの関数オーバーロードオブジェクト
	//クラスから取得した場合に、デリゲートのようにインスタンスとセットにして取得されるもの
	//クラス側へのビュー
	class InstancedOverloadFunctionList : public Object<InstancedOverloadFunctionList> {
	private:
		ScriptValueRef thisValue;
		Reference<OverloadedFunctionList> func;

	public:

		InstancedOverloadFunctionList(const Reference<OverloadedFunctionList>& function, const ScriptValueRef& instance):
			thisValue(instance),
			func(function)
		{}

		void SetName(const std::string& name) {
			func->SetName(name);
		}

		const std::string& GetName() const {
			return func->GetName();
		}

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
		virtual bool CanCall() const override { return true; }
		virtual void Call(const FunctionRequest& request, FunctionResponse& response);
	};

	//リフレクション
	class Reflection : public Object<Reflection> {
	private:
		static ScriptSourceMetadataRef GetCallingSourceMetadata(const FunctionRequest& request);

	public:
		static void ScopeGet(const FunctionRequest& request, FunctionResponse& response);
		static void ScopeSet(const FunctionRequest& request, FunctionResponse& response);

		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);
	};

	//スクリプト配列
	//配列イニシャライザ記法ではこちらのオブジェクトを生成する形
	class ScriptArray : public ScriptIterable {
	private:
		//線形配列の実体
		std::vector<ScriptValueRef> members;

	public:

		ScriptArray() {
			SetNativeOverrideInstanceId(TypeId());
		}

		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<ScriptArray>();
		}

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;

		void Add(const ScriptValueRef& item) {
			members.push_back(item);
		}

		void Insert(const ScriptValueRef& item, size_t index) {
			members.insert(members.begin() + index, item);
		}

		size_t Count() const {
			return members.size();
		}

		void Remove(size_t index) {
			if (index < Count()) {
				members.erase(members.begin() + index);
			}
		}

		ScriptValueRef At(size_t index) const {
			if (index < Count()) {
				return members.at(index);
			}
			return ScriptValue::Null;
		}

		void SetAt(size_t index, const ScriptValueRef& value) {
			if (index < Count()) {
				members[index] = value;
			}
		}

		void Clear() {
			members.clear();
		}

		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override {
			
			//通常の対応にあわせてnumberに変換してからsize_t にする
			number indexNumber = NAN;
			try {
				indexNumber = std::stod(key);
			}
			catch (const std::exception&) {
				indexNumber = NAN;
			}

			if (std::isnan(indexNumber)) {
				return;
			}

			size_t index = static_cast<size_t>(indexNumber);
			if (index < Count()) {
				members[index] = value;
			}
		}

		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override {

			if (key == "Add") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptAdd, GetRef()));
			}
			else if (key == "AddRange") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptAddRange, GetRef()));
			}
			else if (key == "Insert") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptInsert, GetRef()));
			}
			else if (key == "Remove") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptRemove, GetRef()));
			}
			else if (key == "Clear") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptClear, GetRef()));
			}
			else if (key == "length") {
				return ScriptValue::Make(static_cast<number>(members.size()));
			}
			else{
				//index

				//通常の対応にあわせてnumberに変換してからsize_t にする
				number indexNumber = NAN;
				try {
					indexNumber = std::stod(key);
				}
				catch(const std::exception&) {
					indexNumber = NAN;
				}

				if (!std::isnan(indexNumber)) {
					size_t index = static_cast<size_t>(indexNumber);
					if (index < Count()) {
						return members[index];
					}
				}
			}

			return nullptr;
		}

		//スクリプト向け実装
		static void ScriptAdd(const FunctionRequest& request, FunctionResponse& response) {
			if (request.GetArgumentCount() >= 1) {
				ScriptArray* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetContext().GetBlockScope()->GetThisValue());

				if (!obj->ValidateCollectionLock(request, response)) {
					return;
				}

				obj->Add(request.GetArgument(0));
			}
		}

		static void ScriptAddRange(const FunctionRequest& request, FunctionResponse& response) {
			if (request.GetArgumentCount() >= 1) {
				ScriptArray* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetContext().GetBlockScope()->GetThisValue());

				if (!obj->ValidateCollectionLock(request, response)) {
					return;
				}

				ScriptArray* arg = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetArgument(0));

				if (arg != nullptr) {
					for (size_t i = 0; i < arg->Count(); i++) {
						obj->Add(arg->At(i));
					}
				}
			}
		}

		static void ScriptRemove(const FunctionRequest& request, FunctionResponse& response) {
			if (request.GetArgumentCount() >= 1) {
				ScriptArray* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetContext().GetBlockScope()->GetThisValue());

				if (!obj->ValidateCollectionLock(request, response)) {
					return;
				}

				size_t n = static_cast<size_t>(request.GetArgument(0)->ToNumber());
				if (n < obj->Count()) {
					obj->Remove(n);
				}
			}
		}

		static void ScriptClear(const FunctionRequest& request, FunctionResponse& response) {
			ScriptArray* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetContext().GetBlockScope()->GetThisValue());

			if (!obj->ValidateCollectionLock(request, response)) {
				return;
			}

			obj->Clear();
		}

		static void ScriptInsert(const FunctionRequest& request, FunctionResponse& response) {
			if (request.GetArgumentCount() >= 2) {
				ScriptArray* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetContext().GetBlockScope()->GetThisValue());

				if (!obj->ValidateCollectionLock(request, response)) {
					return;
				}

				size_t index;
				if (request.GetArgument(1)->ToIndex(index) && index <= obj->Count()) {
					obj->Insert(request.GetArgument(0), index);
				}
			}
		}


		virtual Reference<ScriptIterator> CreateIterator(ScriptExecuteContext& executeContext) override;
		virtual std::string DebugToString(ScriptExecuteContext& executeContext, DebugOutputContext& debugOutputContext) override;

	};

	class ScriptArrayIterator : public ScriptIterator {
	private:
		Reference<ScriptArray> targetArray;
		size_t currentIndex;

	public:
		ScriptArrayIterator(const Reference<ScriptArray>& target) :ScriptIterator(target),
			targetArray(target),
			currentIndex(0) {
			SetNativeOverrideInstanceId(TypeId());
		}

		static uint32_t TypeId() {
			return ObjectTypeIdGenerator::Id<ScriptArrayIterator>();
		}

		virtual ScriptValueRef GetValue() {
			return targetArray->At(currentIndex);
		}

		virtual ScriptValueRef GetKey() {
			return ScriptValue::Make(static_cast<number>(currentIndex));
		}

		virtual void FetchNext() {
			currentIndex++;
		}

		virtual bool IsEnd() {
			return !(currentIndex < targetArray->Count());
		}

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result);
	};

	//トーク内にタグを自動挿入する関係の設定オブジェクト
	class TalkBuilderSettings : public Object<TalkBuilderSettings> {
	private:
		std::string autoLineBreak;
		std::string scopeChangeLineBreak;
		std::string scriptHead;

	public:
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override {};
		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;

		TalkBuilderSettings() :
			autoLineBreak("\\n"),
			scopeChangeLineBreak("\\n\\n[half]"),
			scriptHead("")
		{}

		const std::string& GetLineBreak() { return autoLineBreak; }
		const std::string& GetScopeChangeLineBreak() { return scopeChangeLineBreak; }
		const std::string& GetScriptHead() { return scriptHead; }
	};

	//TalkBuildSettingsを格納するためのもの
	class TalkBuilder : public Object<TalkBuilder> {
	private:
		static const char* NAME_DEFAULT_SETTINGS;
		static const char* NAME_CURRENT_SETTINGS;

	public:
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override {};
		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);
		static void StaticInit(ScriptInterpreter& interpreter);
		static void Prepare(ScriptInterpreter& interpreter);

		static TalkBuilderSettings& GetCurrentSettings(ScriptInterpreter& interpreter);

		//自動改行につかう文字列を取得
		static const std::string& GetAutoLineBreak(ScriptInterpreter& interpreter);

		//スコープ切替時の改行に使う文字列を取得
		static const std::string& GetScopeChangeLineBreak(ScriptInterpreter& interpreter);

		//トークの先頭に付与するスクリプトを取得
		static const std::string& GetScriptHead(ScriptInterpreter& interpreter);
	};

	//ユニットオブジェクト
	//ScriptUnit参照用の専用オブジェクト
	class UnitObject : public Object<UnitObject> {
	private:
		std::string path;

	public:
		UnitObject(const std::string& unitPath):
			path(unitPath)
		{ }

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override {}
		virtual ScriptValueRef Get(const std::string& key, ScriptExecuteContext& executeContext) override;
		virtual void Set(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;

		ScriptValueRef Get(const std::string& key, ScriptInterpreter& interpreter);
		void Set(const std::string& ket, const ScriptValueRef& value, ScriptInterpreter& interpreter);
	};

}
