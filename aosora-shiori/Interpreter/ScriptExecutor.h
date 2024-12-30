#pragma once

#include "AST/AST.h"
#include "Interpreter/ScriptVariable.h"
#include "Misc/Utility.h"

namespace sakura {

	class ScriptExecuteContext;
	class ClassData;
	class BlockScope;
	class RuntimeError;
	class ScriptInterpreterStack;

	class ScriptExecutor {
	private:
		static ScriptValueRef ExecuteInternal(const ASTNodeBase& node, ScriptExecuteContext& context);

		//各種実行
		static ScriptValueRef ExecuteCodeBlock(const ASTNodeCodeBlock& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteFormatString(const ASTNodeFormatString& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteStringLiteral(const ASTNodeStringLiteral& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteNumberLiteral(const ASTNodeNumberLiteral& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteBooleanLiteral(const ASTNodeBooleanLiteral& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteResolveSymbol(const ASTNodeResolveSymbol& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteAssignSymbol(const ASTNodeAssignSymbol& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteArrayInitializer(const ASTNodeArrayInitializer& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteObjectInitializer(const ASTNodeObjectInitializer& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteFunctionStatement(const ASTNodeFunctionStatement& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteFunctionInitializer(const ASTNodeFunctionInitializer& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteLocalVariableDeclaration(const ASTNodeLocalVariableDeclaration& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteLocalVariableDeclarationList(const ASTNodeLocalVariableDeclarationList& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteFor(const ASTNodeFor& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteWhile(const ASTNodeWhile& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteIf(const ASTNodeIf& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteBreak(const ASTNodeBreak& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteContinue(const ASTNodeContinue& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteReturn(const ASTNodeReturn& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOperator2(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOperator1(const ASTNodeEvalOperator1& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteNewClassInstance(const ASTNodeNewClassInstance& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteFunctionCall(const ASTNodeFunctionCall& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteResolveMember(const ASTNodeResolveMember& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteAssignMember(const ASTNodeAssignMember& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteTry(const ASTNodeTry& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteThrow(const ASTNodeThrow& node, ScriptExecuteContext& executeContext);

		//二項演算子
		static ScriptValueRef ExecuteOpAdd(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpSub(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpMul(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpDiv(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpMod(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpEq(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpNe(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpGt(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpLt(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpGe(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpLe(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpLogicalOr(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpLogicalAnd(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);

		//単項演算子
		static ScriptValueRef ExecuteOpMinus(const ASTNodeEvalOperator1& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpPlus(const ASTNodeEvalOperator1& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteOpLogicalNot(const ASTNodeEvalOperator1& node, ScriptExecuteContext& executeContext);

		//トークブロックノード
		static ScriptValueRef ExecuteTalkSpeak(const ASTNodeTalkSpeak& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteTalkSetSpeaker(const ASTNodeTalkSetSpeaker& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteTalkJump(const ASTNodeTalkJump& node, ScriptExecuteContext& executeContext);

		//引数解決
		static bool ResolveArguments(const std::vector<ConstASTNodeRef>& argumentNodes, std::vector<ScriptValueRef>& argumentValues, ScriptExecuteContext& executeContext);

	public:
		static ScriptValueRef ExecuteASTNode(const ASTNodeBase& node, ScriptExecuteContext& executeContext);
	};


	//スクリプト実行インタプリタ
	class ScriptInterpreter {
	private:
		uint32_t scriptClassCount;

		//システムレジストリ(書き込み禁止)
		std::map<std::string, ScriptValueRef> systemRegistry;
		std::map<std::string, ScriptValueRef> globalVariables;

		//クラス型情報
		std::map<std::string, Reference<ClassData>> classMap;
		std::map<uint32_t, Reference<ClassData>> classIdMap;

		//ネイティブクラス用の、staticな情報ストア(クラスごとにScriptObject１個)
		//(インタプリタを複数作っても分離できるようにするため）
		std::map<uint32_t, ObjectRef> nativeStaticStore;

		//オブジェクト管理
		ObjectSystem objectManager;

		//作業ディレクトリ
		std::string workingDirectory;

	private:
		void CallFunctionInternal(const ScriptValue& funcVariable, const std::vector<ScriptValueRef>& args, ScriptInterpreterStack& funcStack, FunctionResponse& response);

	public:

		//テスト関数
		static void Print(const FunctionRequest& request, FunctionResponse& response) {

			//コンソール表示はsjisにしておく
 			printf("[Print] %s\n", Utf8ToSjis(request.GetArgument(0)->ToString()).c_str());
		}

		ScriptInterpreter();

		//ワーキングディレクトリ
		void SetWorkingDirectory(const std::string& dir) {
			workingDirectory = dir;
		}

		const std::string& GetWorkingDirectory() const {
			return workingDirectory;
		}

		std::string GetFileName(const std::string& relativePath) const {
			return workingDirectory + "\\" + relativePath;
		}


		//クラスの登録
		void ImportClasses(const std::map<std::string, ScriptClassRef>& classMap);
		void ImportClass(const std::shared_ptr<const ClassBase>& nativeClass);
		void CommitClasses();

		//システムレジストリ値を追加
		void RegisterSystem(const std::string& name, const ScriptValueRef& value) {
			//TODO: 重複したときの対応
			systemRegistry.insert(std::map<std::string, ScriptValueRef>::value_type(name, value));
		}

		void RegisterNativeFunction(const std::string& name, ScriptNativeFunction func);

		//システムレジストリから取得
		ScriptValueRef GetSystemRegistryValue(const std::string& name) {
			//値を探して返す
			auto it = systemRegistry.find(name);
			if (it != systemRegistry.end()) {
				return it->second;
			}
			else {
				return nullptr;
			}
		}

		bool ContainsSystemRegistry(const std::string& name) {
			return systemRegistry.contains(name);
		}

		//グローバルから取得
		ScriptValueRef GetGlobalVariable(const std::string& name) {
			//値を探して返す
			auto it = globalVariables.find(name);
			if (it != globalVariables.end()) {
				return it->second;
			}
			else {
				return nullptr;
			}
		}

		//グローバルに設定
		void SetGlobalVariable(const std::string& name, const ScriptValueRef& value) {
			auto it = globalVariables.find(name);
			if (it != globalVariables.end()) {
				it->second = value;
			}
			else {
				globalVariables.insert(std::map<std::string, ScriptValueRef>::value_type(name, value));
			}
		}

		//クラス取得
		ScriptValueRef GetClass(const std::string& name);

		//クラス取得
		template<typename T>
		Reference<ClassData> GetClass() {
			auto item = classIdMap.find(T::TypeId());
			if (item != classIdMap.end()) {
				return item->second;
			}
			else {
				return nullptr;
			}
		}


		//クラスID取得
		uint32_t GetClassId(const std::string& name);

		//ASTをインタプリタに渡して実行
		void Execute(const ConstASTNodeRef& node);

		//ASTをインタプリタ側に渡して実行し、リザルトを得る
		void Execute(const ConstASTNodeRef& node, std::string& result);

		//関数実行
		void CallFunction(const ScriptValue& funcVariable, FunctionResponse& response, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& executeContext, const ASTNodeBase* callingAstNode, const std::string& funcName = "");
		void CallFunction(const ScriptValue& funcVariable, FunctionResponse& response, const std::vector<ScriptValueRef>& args);

		//オブジェクト生成
		Reference<ScriptObject> CreateObject() {
			return objectManager.CreateObject<ScriptObject>();
		}

		//ネイティブオブジェクト作成
		template<typename T, typename... Args>
		Reference<T> CreateNativeObject(Args... args) {
			return objectManager.CreateObject<T>(args...);
		}

		//クラスインスタンス生成
		ObjectRef NewClassInstance(const ASTNodeBase& callingNode, const ScriptValueRef& classData, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& context);
		ObjectRef NewClassInstance(const ASTNodeBase& callingNode, const Reference<ClassData>& classData, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& context, Reference<ScriptObject> scriptObjInstance);


		//オブジェクト型判定
		bool InstanceIs(const ObjectRef& obj, uint32_t classId);
		bool InstanceIs(const ScriptValue& obj, uint32_t classId) {
			if (obj.IsObject()) {
				return InstanceIs(obj.GetObjectRef(), classId);
			}
			else {
				return false;
			}
		}
		bool InstanceIs(const ScriptValueRef& obj, uint32_t classId) {
			return InstanceIs(*obj, classId);
		}

		template<typename T>
		bool InstanceIs(const ObjectRef& obj) {
			return InstanceIs(obj, T::TypeId());
		}

		template<typename T>
		bool InstanceIs(const ScriptValueRef& obj) {
			return InstanceIs(obj, T::TypeId());
		}

		template<typename T>
		bool InstanceIs(const ScriptValue& obj) {
			return InstanceIs(obj, T::TypeId());
		}


		//ネイティブクラスオブジェクトへのキャスト
		//TODO: ScriptObjectを渡した場合、中身のネイティブオブジェクトを参照できるようにする
		//WARN: 結果を他に渡さないこと。スクリプトで継承したネイティブオブジェクトをもってくるとスクリプト部分が見えなくなってしまうため
		template<typename T>
		T* InstanceAs(const ObjectRef& obj) {
			if (InstanceIs<T>(obj)) {
				return obj.Cast<T>().Get();
			}
			else {
				return nullptr;
			}
		}

		template<typename T>
		T* InstanceAs(const ScriptValueRef& obj) {
			if (InstanceIs<T>(obj)) {
				return obj->GetObjectRef().Cast<T>().Get();
			}
			else {
				return nullptr;
			}
		}

		template<typename T>
		T* InstanceAs(const ScriptValue& obj) {
			if (InstanceIs<T>(obj)) {
				return obj.GetObjectRef().Cast<T>().Get();
			}
			else {
				return nullptr;
			}
		}

		//ネイティブクラスのstatic領域を取得
		template<typename T>
		Reference<typename T::StaticStoreType> StaticStore() {
			auto item = nativeStaticStore.find(T::TypeId());
			if (item != nativeStaticStore.end()) {
				return item->second.Cast<typename T::StaticStoreType>();
			}
			else {
				auto newObj = CreateNativeObject<typename T::StaticStoreType>();
				nativeStaticStore[T::TypeId()] = newObj;
				return newObj;
			}
		}

		//ネイティブクラスのstatic領域を上書き
		template<typename T>
		void SetStaticStore(const Reference<typename T::StaticStoreType>& value) {
			nativeStaticStore[T::TypeId()] = value;
		}

		//GC
		void CollectObjects();

	};

	//スクリプト実行スタック
	class ScriptInterpreterStack {
	private:
		static const int32_t TALK_SPEAKER_INDEX_DEFAULT = -1;

		//関数離脱モード
		enum class LeaveMode {
			None,
			Return,				//returnによる脱出
			Throw,				//throwによる脱出
			PendingReturn,		//returnされていて、finallyブロックの実行中
			PendingThrow		//throwされていて、finallyブロックの実行中
		};

		//ループ処理モード
		enum class LoopMode {
			Normal,		//通常
			Break,		//break離脱中
			Continue	//continue離脱中
		};

	public:
		class LoopScope {
		public:
			ScriptInterpreterStack& st;
			LoopScope(ScriptInterpreterStack& stack) :
				st(stack) {
				st.loopDepth++;
			}

			~LoopScope() {
				//ループをひとつ離脱したのでステータスをもとに戻す
				st.loopDepth--;
				st.loopMode = LoopMode::Normal;
			}
		};

	private:
		ScriptValueRef returnValue;
		ObjectRef threwError;
		const ASTNodeBase* callingAstNode;

		ScriptInterpreterStack* parent;
		LeaveMode leaveMode;

		//ループ
		int32_t loopDepth;
		LoopMode loopMode;

		//トーク
		int32_t talkSpeakerIndex;
		std::string talkBody;
		bool isTalkLineEnd;

		//このスタック位置の関数名
		std::string funcName;

	private:
		ScriptInterpreterStack(ScriptInterpreterStack* parent) :
			returnValue(nullptr),
			threwError(nullptr),
			callingAstNode(nullptr),
			parent(parent),
			leaveMode(LeaveMode::None),
			loopDepth(0),
			loopMode(LoopMode::Normal),
			talkSpeakerIndex(TALK_SPEAKER_INDEX_DEFAULT),
			isTalkLineEnd(false)
		{}

	public:
		ScriptInterpreterStack() :
			returnValue(nullptr),
			threwError(nullptr),
			callingAstNode(nullptr),
			parent(nullptr),
			leaveMode(LeaveMode::None),
			talkSpeakerIndex(TALK_SPEAKER_INDEX_DEFAULT),
			isTalkLineEnd(false)
		{}

		//関数を抜ける設定
		void Return(const ScriptValueRef& value) {
			assert(leaveMode != LeaveMode::Return && leaveMode != LeaveMode::Throw);
			returnValue = value;
			leaveMode = LeaveMode::Return;
		}

		//talkbodyを使って関数を抜ける
		void ReturnTalk() {
			Return(ScriptValue::Make(talkBody));
		}

		//戻り値の取得
		const ScriptValueRef& GetReturnValue() const {
			return returnValue;
		}

		//returnを要求しているか
		bool IsReturned() const {
			return leaveMode == LeaveMode::Return;
		}

		//例外スロー(コールスタック設定済み)
		void Throw(const ObjectRef& err) {
			assert(leaveMode != LeaveMode::Return && leaveMode != LeaveMode::Throw);
			threwError = err;
			leaveMode = LeaveMode::Throw;
		}

		//スローされた例外を取得
		const ObjectRef& GetThrewError() const {
			return threwError;
		}

		//throwされているか
		bool IsThrew() const {
			return leaveMode == LeaveMode::Throw;
		}

		//breakループ離脱
		void Break() {
			if (loopDepth > 0) {
				loopMode = LoopMode::Break;
			}
		}

		//continueループ離脱
		void Continue() {
			if (loopDepth > 0) {
				loopMode = LoopMode::Continue;
			}
		}

		bool IsBreak() const {
			return loopMode == LoopMode::Break;
		}

		bool IsContinue() const {
			return loopMode == LoopMode::Continue;
		}


		//脱出モードをペンディング状態にする
		//catch, finallyブロック実行のための一時待機状態として
		void PendingLeaveMode() {
			if (leaveMode == LeaveMode::Return) {
				leaveMode = LeaveMode::PendingReturn;
			}
			else if (leaveMode == LeaveMode::Throw) {
				leaveMode = LeaveMode::PendingThrow;
			}
		}

		//ペンディング状態になった関数離脱を復元
		void RestorePendingLeaveMode() {
			assert(leaveMode != LeaveMode::Return);	//状態不正

			if (leaveMode == LeaveMode::PendingReturn) {
				leaveMode = LeaveMode::Return;
			}
			else if (leaveMode == LeaveMode::PendingThrow) {
				leaveMode = LeaveMode::Throw;
			}
			//throw モードの場合は例外が再度スローされたということで状態が上書きされているのでよい
		}

		//ペンディング状態の例外を破棄
		void ClearPendingError() {
			if (leaveMode == LeaveMode::PendingThrow) {
				leaveMode = LeaveMode::None;
				threwError = nullptr;
			}
		}

		//何かしらの理由で処理を中断すべきか
		bool IsLeave() const {
			return IsStackLeave() || IsLoopLeave();
		}

		//return/throwでスタックフレームの離脱を要求しているか
		bool IsStackLeave() const {
			return IsReturned() || IsThrew();
		}

		//break/continueでループブロックの離脱を要求しているか
		bool IsLoopLeave() const {
			return IsBreak() || IsContinue();
		}

		//親スタックフレームの取得
		const ScriptInterpreterStack* GetParentStackFrame() const {
			return parent;
		}

		//子スタックフレームを呼び出しているASTノード
		const ASTNodeBase* GetCallingASTNode() const {
			return callingAstNode;
		}

		//子スタックフレームの作成
		ScriptInterpreterStack CreateChildStackFrame(const ASTNodeBase* callingNode, const std::string& targetFunctionName) {

			//スタックに入る前に今実行しているノードを記録しておく(throwされたときにエラー表示するために）
			callingAstNode = callingNode;
			ScriptInterpreterStack childStack(this);
			childStack.SetFunctionName(targetFunctionName);
			return childStack;
		}

		//トーク内容を追加
		void AppendTalkBody(const std::string& str) {
			if (talkSpeakerIndex == TALK_SPEAKER_INDEX_DEFAULT) {
				//話者指定が入ってなければ\0を自動的に付与
				talkBody.append("\\0");
				talkSpeakerIndex = 0;
			}

			//改行要求
			if (isTalkLineEnd) {
				talkBody.append("\\n");
				isTalkLineEnd = false;
			}
			
			//単純に文字列を結合
			talkBody.append(str);
		}

		void TalkLineEnd() {
			isTalkLineEnd = true;
		}

		//話者を指定
		void SetTalkSpeakerIndex(int32_t speakerIndex) {

			if (talkSpeakerIndex != speakerIndex) {

				//話者が変更になる場合、SSP側での改行となるため改行を切る
				isTalkLineEnd = false;

				talkSpeakerIndex = speakerIndex;
				switch (talkSpeakerIndex) {
				case 0:
					talkBody.append("\\0");
					break;
				case 1:
					talkBody.append("\\1");
					break;
				default:
					talkBody.append("\\p[" + std::to_string(talkSpeakerIndex) + "]");
					break;
				}

				//話者変更時の改行がいりそう…
			}
		}

		//話者交替タグ
		void SwitchTalkSpeakerIndex() {
			//0と1の間で変更
			if (talkSpeakerIndex != 0) {
				SetTalkSpeakerIndex(0);
			}
			else {
				SetTalkSpeakerIndex(1);
			}
		}

		void SetFunctionName(const std::string& name) {
			funcName = name;
		}

		const std::string& GetFunctionName() const {
			return funcName;
		}
	};

	//スクリプト実行コンテキスト
	class ScriptExecuteContext {
	private:
		ScriptInterpreter& interpreter;
		ScriptInterpreterStack& stack;
		Reference<BlockScope> blockScope;

	public:
		ScriptExecuteContext(ScriptInterpreter& vm, ScriptInterpreterStack& st, const Reference<BlockScope>& scope) :
			interpreter(vm),
			stack(st),
			blockScope(scope)
		{}

		ScriptInterpreterStack& GetStack() { return stack; }
		ScriptInterpreter& GetInterpreter() { return interpreter; }
		const Reference<BlockScope>& GetBlockScope() { return blockScope; }

		//新しいブロックスコープのコンテキストを作る
		ScriptExecuteContext CreateChildBlockScopeContext();

		ScriptValueRef GetSymbol(const std::string& name);
		void SetSymbol(const std::string& name, const ScriptValueRef& value);

		//エラーオブジェクトのスロー
		void ThrowError(const ASTNodeBase& throwAstNode, const std::string& funcName, const ObjectRef& err);

		//エラーのスローヘルパ
		template<typename T>
		void ThrowRuntimeError(const ASTNodeBase& throwAstNode, const std::string& message) {
			Reference<RuntimeError> err = interpreter.CreateNativeObject<T>(message);
			ThrowError(throwAstNode, GetStack().GetFunctionName(), err);
		}

		//即時離脱が必要かどうか
		bool RequireLeave() const {
			return stack.IsLeave();
		}

	};

}