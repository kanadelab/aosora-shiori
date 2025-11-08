#pragma once
#include <fstream>
#include <filesystem>
#include "AST/AST.h"
#include "Interpreter/ScriptVariable.h"
#include "Misc/Utility.h"

namespace sakura {

	class ScriptExecuteContext;
	class ClassData;
	class BlockScope;
	class ScriptError;
	class ScriptInterpreterStack;
	class UnitObject;

	class ScriptExecutor {
	private:
		static ScriptValueRef ExecuteInternal(const ASTNodeBase& node, ScriptExecuteContext& context);

		//各種実行
		static ScriptValueRef ExecuteCodeBlock(const ASTNodeCodeBlock& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteFormatString(const ASTNodeFormatString& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteStringLiteral(const ASTNodeStringLiteral& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteNumberLiteral(const ASTNodeNumberLiteral& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteBooleanLiteral(const ASTNodeBooleanLiteral& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteNull(const ASTNodeNull& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteContextValue(const ASTNodeContextValue& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteUnitRoot(const ASTNodeUnitRoot& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteResolveSymbol(const ASTNodeResolveSymbol& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteAssignSymbol(const ASTNodeAssignSymbol& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteArrayInitializer(const ASTNodeArrayInitializer& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteObjectInitializer(const ASTNodeObjectInitializer& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteFunctionStatement(const ASTNodeFunctionStatement& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteFunctionInitializer(const ASTNodeFunctionInitializer& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteLocalVariableDeclaration(const ASTNodeLocalVariableDeclaration& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteLocalVariableDeclarationList(const ASTNodeLocalVariableDeclarationList& node, ScriptExecuteContext& executeContext);
		static ScriptValueRef ExecuteForeach(const ASTNodeForeach& node, ScriptExecuteContext& executeContext);
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
		static ScriptValueRef ExecuteOpNullCoalescing(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext);

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

	struct UnitData {
		std::map<std::string, ScriptValueRef> unitVariables;	//ユニット変数
	};

	struct EvaluateExpressionResult {
		//パースエラー類(パースエラーで実行失敗している場合に入る)
		std::shared_ptr<ScriptParseError> error;
		//実行結果
		ScriptValueRef value;
	};

	//スクリプト実行インタプリタ
	class ScriptInterpreter {
	public:
		// デフォルト100万ステップでエラーにしておく
		static const size_t DEFAULT_EXECUTE_LIMIT_STEPS = 100 * 1000;

		//デバッガ処理の実行中を示す
		class DebuggerScope {
		private:
			ScriptInterpreter& interpreter;

		public:
			DebuggerScope(ScriptInterpreter& interpreter):
				interpreter(interpreter) {
				assert(!interpreter.isDebuggerScope);
				interpreter.isDebuggerScope = true;
			}

			~DebuggerScope() {
				interpreter.isDebuggerScope = false;
			}
		};

	private:
		size_t scriptSteps;
		size_t debuggerScriptSteps;
		size_t limitScriptSteps;
		uint32_t scriptClassCount;
		SecurityLevel securityLevel;

		//システムレジストリ(書き込み禁止)
		std::map<std::string, ScriptValueRef> systemRegistry;
		//std::map<std::string, ScriptValueRef> globalVariables;
		std::map<std::string, UnitData> units;

		//クラス型情報
		//std::map<std::string, Reference<ClassData>> classMap;
		std::map<uint32_t, Reference<ClassData>> classIdMap;

		//ネイティブクラス用の、staticな情報ストア(クラスごとにScriptObject１個)
		//(インタプリタを複数作っても分離できるようにするため）
		std::map<uint32_t, ObjectRef> nativeStaticStore;

		//オブジェクト管理
		ObjectSystem objectManager;

		//作業ディレクトリ(末尾にパス区切りを含む)
		std::string workingDirectory;

		//デバッグ出力
		std::ofstream* debugOutputStream;

		//デバッガ処理中
		bool isDebuggerScope;

	private:
		void CallFunctionInternal(const ScriptValue& funcVariable, const std::vector<ScriptValueRef>& args, ScriptInterpreterStack& funcStack, FunctionResponse& response);

	public:

		//テスト関数
		static void Print(const FunctionRequest& request, FunctionResponse& response) {

			//コンソール表示はsjisにしておく
 			printf("[Print] %s\n", Utf8ToSjis(request.GetArgument(0)->ToString()).c_str());
		}

		ScriptInterpreter();
		~ScriptInterpreter();

		//ステップ数を計測、無限ループの強制脱出用
		uint64_t IncrementScriptStep() {
			if (isDebuggerScope) {
				debuggerScriptSteps++;
				return debuggerScriptSteps;
			}
			scriptSteps++;
			return scriptSteps;
		}

		void ResetScriptStep() {
			if (isDebuggerScope) {
				debuggerScriptSteps = 0;
				return;
			}

			scriptSteps = 0;
		}

		void SetLimitScriptSteps(size_t steps) {
			limitScriptSteps = steps;
		}

		size_t GetLimitScriptSteps() const {
			if (isDebuggerScope) {
				//デバッガスコープににいる場合、ステップ制限は必ずデフォルトで適用される
				//デバッガ側によって無限ループに入らないように
				return DEFAULT_EXECUTE_LIMIT_STEPS;
			}
			return limitScriptSteps;
		}

		//セキュリティレベル
		void SetSecurityLevel(SecurityLevel level) {
			securityLevel = level;
		}

		SecurityLevel GetSecurityLevel() const {
			return securityLevel;
		}

		//セキュリティレベル的にローカルマシンデータにアクセスしていいかを確認
		bool IsAllowLocalAccess() const {
			//local時のみ
			return GetSecurityLevel() == SecurityLevel::LOCAL;
		}

		//ワーキングディレクトリ(パス区切り文字終端)
		void SetWorkingDirectory(const std::string& dir) {
			workingDirectory = dir;
		}

		const std::string& GetWorkingDirectory() const {
			return workingDirectory;
		}

		//TODO: relativePathといいつつ絶対パスを考慮してるので考える⋯
		std::string GetFileName(const std::string& relativePath) const {
			if (std::filesystem::path(relativePath).is_absolute()) {
				//絶対パスだったらなにもしない
				return relativePath;
			}
			return workingDirectory + relativePath;
		}


		//クラスの登録
		void ImportClasses(const std::map<std::string, ScriptClassRef>& classMap);
		void ImportClass(const std::shared_ptr<ClassBase>& nativeClass);
		std::shared_ptr<ScriptParseError> CommitClasses();
		ClassData* FindClass(const ClassPath& classPath, const ScriptUnitAlias* alias);

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

		//現在のユニット変数を取得
		ScriptValueRef GetUnitVariable(const std::string& name, const std::string& scriptUnit);

		//現在のユニット変数を設定
		void SetUnitVariable(const std::string& name, const ScriptValueRef& value, const std::string& scriptUnit);

		//ユニットを登録
		void RegisterUnit(const std::string& unitName);

		//ユニット情報を取得
		const UnitData* FindUnitData(const std::string& unitName);

		//ユニットの内部コレクション取得
		const std::map<std::string, UnitData>& GetUnitCollection() const {
			return units;
		}

		//ルート空間からユニットを取得
		Reference<UnitObject> GetUnit(const std::string& unitName);
		Reference<UnitObject> FindUnit(const std::string& unitNmae);

		//エイリアス指定群から値の参照
		ScriptValueRef GetFromAlias(const ScriptUnitAlias& alias, const std::string& name);

		//クラス取得
		ScriptValueRef GetClass(const uint32_t typeId);

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

		//クラス名取得
		std::string GetClassTypeName(uint32_t typeId);

		//ASTをインタプリタに渡して実行
		ToStringFunctionCallResult Execute(const ConstASTNodeRef& node, bool toStringResult, bool isRootCall = false);

		//文字列をスクリプト式として評価
		EvaluateExpressionResult Eval(const std::string& expr, ScriptExecuteContext& executeContext, const ScriptSourceMetadataRef& importSourceMeta);

		//関数実行
		void CallFunction(const ScriptValue& funcVariable, FunctionResponse& response, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& executeContext, const ASTNodeBase* callingAstNode, const std::string& funcName = "");
		void CallFunction(const ScriptValue& funcVariable, FunctionResponse& response, const std::vector<ScriptValueRef>& args);

		//オブジェクト生成
		Reference<ScriptObject> CreateObject() {
			return objectManager.CreateObject<ScriptObject>();
		}

		//配列生成
		Reference<ScriptArray> CreateArray();
		//ネイティブオブジェクト作成
		template<typename T, typename... Args>
		Reference<T> CreateNativeObject(Args... args) {
			return objectManager.CreateObject<T>(args...);
		}

		//クラスインスタンス生成
		ObjectRef NewClassInstance(const ScriptValueRef& classData, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& context);
		ObjectRef NewClassInstance(const Reference<ClassData>& classData, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& context, Reference<ClassInstance> scriptObjInstance);


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
			if (obj == nullptr) {
				return false;
			}
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
		//WARN: 結果を他に渡さないこと。スクリプトで継承したネイティブオブジェクトをもってくるとスクリプト部分が見えなくなってしまうため
		template<typename T>
		T* InstanceAs(const ObjectRef& obj) {
			if (InstanceIs<T>(obj)) {
				if (obj->GetNativeInstanceTypeId() == ClassInstance::TypeId()) {
					//スクリプトクラスインスタンスをC++クラスにキャストする場合、内部型を得てからキャストする。
					return obj.Cast<ClassInstance>()->GetNativeBaseInstance().template Cast<T>().Get();
				}
				else {
					return obj.template Cast<T>().Get();
				}
			}
			else {
				return nullptr;
			}
		}

		template<typename T>
		T* InstanceAs(const ScriptValueRef& obj) {
			if (InstanceIs<T>(obj)) {
				if (obj->GetObjectInstanceTypeId() == ClassInstance::TypeId()) {
					//スクリプトクラスインスタンスをC++クラスにキャストする場合、内部型を得てからキャストする。
					return obj->GetObjectRef().Cast<ClassInstance>()->GetNativeBaseInstance().template Cast<T>().Get();
				}
				else {
					return obj->GetObjectRef().template Cast<T>().Get();
				}
			}
			else {
				return nullptr;
			}
		}

		template<typename T>
		T* InstanceAs(const ScriptValue& obj) {
			if (InstanceIs<T>(obj)) {
				if (obj.GetObjectInstanceTypeId() == ClassInstance::TypeId()) {
					return obj.GetObjectRef().Cast<ClassInstance>()->GetNativeBaseInstance().template Cast<T>().Get();
				}
				else {
					return obj.GetObjectRef().template Cast<T>().Get();
				}
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
				return item->second.template Cast<typename T::StaticStoreType>();
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

		//デバッグ情報書き出し
		void OpenDebugOutputStream(const std::string& filename);
		std::ofstream* GetDebugOutputStream();
		void CloseDebugOutputStream();

		//デバッガ処理スコープか？
		bool IsDebuggerScope() const { return isDebuggerScope; }

	};

	//トーク結合システム。トーク向けのルールで文字列を結合する
	class TalkStringCombiner {
	public:
		static const int32_t TALK_SPEAKER_INDEX_DEFAULT = -1;

		//話者選択情報
		struct SpeakerSelector {
			int32_t speakerIndex;
			std::string_view selectorTag;
		};

		//話者仕様履歴情報
		struct SpeakedSpeakers {
			std::set<int32_t> usedSpeaker;
			int32_t lastSpeakerIndex;
		};

		//先頭のタグから話者情報を取得
		static SpeakerSelector FetchFirstSpeaker(const std::string& str);

		//最後に来る話者情報と、トーク内で使われた話者を取得
		static SpeakedSpeakers FetchLastSpeaker(const std::string& str);

		//トークの結合
		//SpeakedSpeakersを取ってある場合はそちらを使ってleftと結果に対するSpeakedSpeakerをキャッシュとして更新する
		//disableSpeakerChangeLineBreak がtrueなら、結合ルールを無視するが、SpeakedSpeakerの更新は行う
		static std::string CombineTalk(const std::string& left, const std::string& right, ScriptInterpreter& interpreter, SpeakedSpeakers* speakedCache, bool disableSpeakerChangeLineBreak = false);

		//SpeakedScopesをleftにrightを追加する形でマージ
		static void MergeSpeakedScopes(SpeakedSpeakers& left, const SpeakedSpeakers& right);
	};

	//スクリプト実行スタック
	class ScriptInterpreterStack {
	private:

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
				assert(st.loopDepth >= 0);
			}
		};

		class TryScope {
		public:
			ScriptInterpreterStack& st;
			TryScope(ScriptInterpreterStack& stack) :
				st(stack) {
				st.tryDepth++;
			}
			~TryScope() {
				st.tryDepth--;
				assert(st.tryDepth >= 0);
			}
		};

	private:
		ScriptValueRef returnValue;
		ObjectRef threwError;
		const ASTNodeBase* callingAstNode;
		Reference<BlockScope> callingBlockScope;

		ScriptInterpreterStack* parent;
		LeaveMode leaveMode;

		//ループ
		int32_t loopDepth;
		LoopMode loopMode;

		//try
		int32_t tryDepth;

		//トーク
		TalkStringCombiner::SpeakedSpeakers speakedCache;
		std::string talkBody;
		bool isTalkLineEnd;
		bool isTalkJump;

		//ルート呼び出し（スクリプトファイルそのものの実行中か）
		bool isRootCall;

		//このスタック位置の関数名
		std::string funcName;

	private:
		ScriptInterpreterStack(ScriptInterpreterStack* parent) :
			returnValue(nullptr),
			threwError(nullptr),
			callingAstNode(nullptr),
			callingBlockScope(nullptr),
			parent(parent),
			leaveMode(LeaveMode::None),
			loopDepth(0),
			loopMode(LoopMode::Normal),
			tryDepth(0),
			isTalkLineEnd(false),
			isTalkJump(false),
			isRootCall(false)
		{
			speakedCache.lastSpeakerIndex = TalkStringCombiner::TALK_SPEAKER_INDEX_DEFAULT;
		}

	public:
		ScriptInterpreterStack(bool isRootCall = false) :
			returnValue(nullptr),
			threwError(nullptr),
			callingAstNode(nullptr),
			callingBlockScope(nullptr),
			parent(nullptr),
			leaveMode(LeaveMode::None),
			isTalkLineEnd(false),
			isTalkJump(false),
			isRootCall(isRootCall)
		{
			speakedCache.lastSpeakerIndex = TalkStringCombiner::TALK_SPEAKER_INDEX_DEFAULT;
		}

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

		//例外をクリア
		void ClearError() {
			threwError = nullptr;
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

		//loopmodeを初期状態に戻す
		void ClearLoopMode() {
			loopMode = LoopMode::Normal;
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

		//スタックフレームのブロックスコープ
		const Reference<BlockScope>& GetCallingBlockScope() const {
			return callingBlockScope;
		}

		//tryブロック内かどうか(例外を投げた場合catchされるかどうか)
		bool IsTryBlock() const {
			if (tryDepth > 0) {
				return true;
			}

			//スタックをさかのぼってcatchスコープにあるかを確認する
			if (parent != nullptr) {
				return parent->IsTryBlock();
			}
			else {
				return false;
			}
		}

		//子スタックフレームの作成
		ScriptInterpreterStack CreateChildStackFrame(const ASTNodeBase* callingNode, const Reference<BlockScope>& callingScope, const std::string& targetFunctionName) {

			//スタックに入る前に今実行しているノードを記録しておく(throwされたときにエラー表示するために）
			callingBlockScope = callingScope;
			callingAstNode = callingNode;
			ScriptInterpreterStack childStack(this);
			childStack.SetFunctionName(targetFunctionName);
			return childStack;
		}

		//親スタックフレームのメタデータを取得
		ScriptSourceMetadataRef GetParentStackSourceMetadata() const {
			if (parent == nullptr) {
				return nullptr;
			}
			if (parent->GetCallingASTNode() == nullptr) {
				return nullptr;
			}
			return parent->GetCallingASTNode()->GetSourceMetadata();
		}

		//TalkBuilderで設定されたTalkHeaderを必要に応じて追加
		void AppendTalkHeadIfNeed(ScriptInterpreter& interpreter);

		//トーク内容を追加
		void AppendTalkBody(const std::string& str, ScriptInterpreter& interpreter);

		//ジャンプによるトーク内容の追加
		void AppendJumpedTalkBody(const std::string& str, ScriptInterpreter& interpreter);

		//話者を指定
		void SetTalkSpeakerIndex(int32_t speakerIndex, ScriptInterpreter& interpreter);

		void TalkLineEnd() {
			isTalkLineEnd = true;
		}

		//フレームがトークジャンプによって呼び出されているか
		void SetTalkJump(bool isJump) {
			isTalkJump = isJump;
		}

		bool IsTalkJump() const { return isTalkJump; }

		//スクリプトルートの実行中か
		bool IsRootCall() const { return isRootCall; }

		//話者交替タグ
		void SwitchTalkSpeakerIndex(ScriptInterpreter& interpreter) {
			//0と1の間で変更
			if (speakedCache.lastSpeakerIndex != 0) {
				SetTalkSpeakerIndex(0, interpreter);
			}
			else {
				SetTalkSpeakerIndex(1, interpreter);
			}
		}

		void SetFunctionName(const std::string& name) {
			funcName = name;
		}

		const std::string& GetFunctionName() const {
			return funcName;
		}
	};

	//出力用のコールスタック情報
	struct CallStackInfo {
		const ASTNodeBase* executingAstNode;
		SourceCodeRange sourceRange;
		Reference<BlockScope> blockScope;
		std::string funcName;
		bool hasSourceRange;
		bool isJumping;
	};

	//スクリプト実行コンテキスト
	class ScriptExecuteContext {
	private:
		ScriptInterpreter& interpreter;
		ScriptInterpreterStack& stack;
		Reference<BlockScope> blockScope;
		const ASTNodeBase* currentNode;

	public:
		ScriptExecuteContext(ScriptInterpreter& vm, ScriptInterpreterStack& st, const Reference<BlockScope>& scope) :
			interpreter(vm),
			stack(st),
			blockScope(scope),
			currentNode(stack.GetCallingASTNode())
		{}

		ScriptInterpreterStack& GetStack() { return stack; }
		ScriptInterpreter& GetInterpreter() { return interpreter; }
		const Reference<BlockScope>& GetBlockScope() { return blockScope; }
		const ASTNodeBase* GetCurrentASTNode() { return currentNode; }
		void SetCurrentASTNode(const ASTNodeBase* node) { currentNode = node; }

		//新しいブロックスコープのコンテキストを作る
		ScriptExecuteContext CreateChildBlockScopeContext();

		ScriptValueRef GetSymbol(const std::string& name, const ScriptSourceMetadata& metadata);
		void SetSymbol(const std::string& name, const ScriptValueRef& value, const ScriptSourceMetadata& metadata);

		//スタックトレースの取得
		std::vector<CallStackInfo> MakeStackTrace(const ASTNodeBase& currentAstNode, const Reference<BlockScope>& callingBlockScope, const std::string& currentFuncName);

		//エラーオブジェクトのスロー
		void ThrowError(const ASTNodeBase& throwAstNode, const Reference<BlockScope>& callingBlockScope, const std::string& funcName, const ObjectRef& err, ScriptExecuteContext& executeContext);

		//エラーのスローヘルパ
		template<typename T>
		Reference<ScriptError> ThrowRuntimeError(const std::string& message, ScriptExecuteContext& context) {
			Reference<ScriptError> err = interpreter.CreateNativeObject<T>(message);
			ThrowError(*context.GetCurrentASTNode(), context.GetBlockScope(), GetStack().GetFunctionName(), err, context);
			return err;
		}

		//即時離脱が必要かどうか
		bool RequireLeave() const {
			return stack.IsLeave();
		}

		//直近で取得できるASTノードを取得(呼び出し元の取得)
		const ASTNodeBase* GetLatestASTNode() {
			const ScriptInterpreterStack* node = &stack;
			while (node != nullptr) {
				if (node->GetCallingASTNode() != nullptr) {
					return node->GetCallingASTNode();
				}
				node = node->GetParentStackFrame();
			}
			return nullptr;
		}
		
	};

}
