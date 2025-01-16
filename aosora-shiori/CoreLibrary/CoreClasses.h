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

		//子クラス追加
		void AddChildClass(const Reference<ClassData>& child) {
			const uint32_t typeId = GetClassTypeId();
			assert(typeId != ObjectTypeIdGenerator::INVALID_ID);
			upcastTypes.insert(typeId);
		}

		//インスタンス判定
		bool InstanceIs(uint32_t objectClassId) {
			return upcastTypes.contains(objectClassId);
		}

		void SetToInstance(const std::string& key, const ScriptValueRef& value, ScriptObject& instance, ScriptExecuteContext& executeContext);
		ScriptValueRef GetFromInstance(const std::string& key, ScriptObject& instance, ScriptExecuteContext& executeContext);

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
		virtual void Set(const ObjectRef& self, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;
		virtual ScriptValueRef Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) override;
	};

	//エラーオブジェクト
	class RuntimeError : public Object<RuntimeError> {
	public:
		//表示用のスタック情報
		struct CallStackInfo {
			SourceCodeRange sourceRange;
			std::string funcName;
			bool hasSourceRange;
		};

	private:
		bool canCatch;
		bool hasCallstackInfo;
		std::string message;
		std::vector<CallStackInfo> callStackInfo;

	public:
		RuntimeError(const std::string& errorMessage):
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

		//メッセージ取得
		const std::string& GetMessage() const {
			return message;
		}

		const std::string ToString() const {
			std::string r;
			for (const auto& info : callStackInfo) {
				r += info.sourceRange.ToString() + "\n";
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
		static void CreateObject(const FunctionRequest& req, FunctionResponse& res);
	};


	//オーバーロードを許容する関数の集合
	//重複回避つきのランダム選択機能
	class OverloadedFunctionList : public Object<OverloadedFunctionList> {
	private:
		struct FunctionItem {
			ConstScriptFunctionRef scriptFunc;
			ScriptNativeFunction nativeFunc;
			Reference<BlockScope> blockScope;		//関数定義時のブロックスコープ。ローカル変数キャプチャ領域。
			ConstASTNodeRef condition;
		};

	private:
		std::vector<FunctionItem> functions;
		std::vector<size_t> callOrder;
		std::string funcName;

	private:
		const FunctionItem* SelectItemInternal(const FunctionRequest& request, FunctionResponse& response);
		void MakeCallorder();

	public:
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

		//定義時の関数名。別変数に代入できてしまうので、デバッグ用に登録時の名前をとっておく。
		void SetName(const std::string& name) {
			funcName = name;
		}

		const std::string& GetName() const {
			return funcName;
		}

		ScriptValueRef SelectItem(ScriptExecuteContext& executeContext, const ScriptValueRef& thisValue);
		void ThisCall(const FunctionRequest& request, FunctionResponse& response, const ScriptValueRef& thisValue);

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
		virtual bool CanCall() const override { return true; }
		virtual void Call(const FunctionRequest& request, FunctionResponse& response);
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

		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override;
		virtual bool CanCall() const override { return true; }
		virtual void Call(const FunctionRequest& request, FunctionResponse& response);
	};

	//リフレクション
	class Reflection : public Object<Reflection> {
	public:
		static void ScopeGet(const FunctionRequest& request, FunctionResponse& response);
		static void ScopeSet(const FunctionRequest& request, FunctionResponse& response);

		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);
	};

	//スクリプト配列
	//配列イニシャライザ記法ではこちらのオブジェクトを生成する形
	class ScriptArray : public Object<ScriptArray> {
	private:
		//線形配列の実体
		std::vector<ScriptValueRef> members;

	public:

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
			assert(index < Count());
			members.erase(members.begin() + index);
		}

		ScriptValueRef At(size_t index) const {
			assert(index < Count());
			return members.at(index);
		}

		void Clear() {
			members.clear();
		}

		virtual void Set(const ObjectRef& self, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override {
			
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

		virtual ScriptValueRef Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) override {

			if (key == "Add") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptAdd, self));
			}
			else if (key == "Insert") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptInsert, self));
			}
			else if (key == "Remove") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptRemove, self));
			}
			else if (key == "Clear") {
				return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptArray::ScriptClear, self));
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
				obj->Add(request.GetArgument(0));
			}
		}

		static void ScriptRemove(const FunctionRequest& request, FunctionResponse& response) {
			if (request.GetArgumentCount() >= 1) {
				ScriptArray* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetContext().GetBlockScope()->GetThisValue());
				size_t n = static_cast<size_t>(request.GetArgument(0)->ToNumber());
				if (n < obj->Count()) {
					obj->Remove(n);
				}
			}
		}

		static void ScriptClear(const FunctionRequest& request, FunctionResponse& response) {
			ScriptArray* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetContext().GetBlockScope()->GetThisValue());
			obj->Clear();
		}

		static void ScriptInsert(const FunctionRequest& request, FunctionResponse& response) {
			if (request.GetArgumentCount() >= 2) {
				ScriptArray* obj = request.GetContext().GetInterpreter().InstanceAs<ScriptArray>(request.GetContext().GetBlockScope()->GetThisValue());
				size_t index;
				if (request.GetArgument(1)->ToIndex(index) && index <= obj->Count()) {
					obj->Insert(request.GetArgument(0), index);
				}
			}
		}

	};

	//トーク内にタグを自動挿入する関係の設定オブジェクト
	class TalkBuilderSettings : public Object<TalkBuilderSettings> {
	private:
		std::string autoLineBreak;
		std::string scopeChangeLineBreak;

	public:
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override {};
		virtual void Set(const ObjectRef& self, const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext) override;
		virtual ScriptValueRef Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) override;

		TalkBuilderSettings():
			autoLineBreak("\\n"),
			scopeChangeLineBreak("\\n\\n[half]")
		{}

		const std::string& GetLineBreak() { return autoLineBreak; }
		const std::string& GetScopeChangeLineBreak() { return scopeChangeLineBreak; }
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
	};

}
