#include <filesystem>

#include "AST/AST.h"
#include "Interpreter/ScriptVariable.h"
#include "Interpreter/ScriptExecutor.h"
#include "CoreLibrary/CoreLibrary.h"
#include "CommonLibrary/CommonClasses.h"
#include "CommonLibrary/StandardLibrary.h"
#include "Misc/Message.h"
#include "Debugger/Debugger.h"

namespace sakura {

	const std::regex PATTERN_PTAG(R"(\\p(\[([0-9]+)\]|([0-9])))");

	//ASTノード実行系
	ScriptValueRef ScriptExecutor::ExecuteASTNode(const ASTNodeBase& node, ScriptExecuteContext& executorContext) {
		return ExecuteInternal(node, executorContext);
	}


	//ASTふりわけ
	ScriptValueRef ScriptExecutor::ExecuteInternal(const ASTNodeBase& node, ScriptExecuteContext& executeContext) {

		//非実行ノードはスルー
		if (!node.IsExecutable()) {
			return ScriptValue::Null;
		}

		//実行結果をもっておく
		ScriptValueRef result = ScriptValue::Null;

		//コンテキストに実行中ノードをセット
		const ASTNodeBase* callingNode = executeContext.GetCurrentASTNode();
		executeContext.SetCurrentASTNode(&node);

		//無限ループ対策の実行ステップ制限
		if (executeContext.GetInterpreter().GetLimitScriptSteps() > 0) {
			if (executeContext.GetInterpreter().IncrementScriptStep() > executeContext.GetInterpreter().GetLimitScriptSteps()) {
				executeContext.ThrowRuntimeError<RuntimeError>(TextSystem::Find("AOSORA_BUILTIN_ERROR_001"), executeContext)->SetCanCatch(false);

				//実行中ノードを戻して終わり
				executeContext.SetCurrentASTNode(callingNode);
				return ScriptValue::Null;
			}
		}

		//ノードタイプごとに実行
		switch (node.GetType()) {
		case ASTNodeType::CodeBlock:
			result = ExecuteCodeBlock(static_cast<const ASTNodeCodeBlock&>(node), executeContext);
			break;
		case ASTNodeType::FormatString:
			result = ExecuteFormatString(static_cast<const ASTNodeFormatString&>(node), executeContext);
			break;
		case ASTNodeType::StringLiteral:
			result = ExecuteStringLiteral(static_cast<const ASTNodeStringLiteral&>(node), executeContext);
			break;
		case ASTNodeType::NumberLiteral:
			result = ExecuteNumberLiteral(static_cast<const ASTNodeNumberLiteral&>(node), executeContext);
			break;
		case ASTNodeType::BooleanLiteral:
			result = ExecuteBooleanLiteral(static_cast<const ASTNodeBooleanLiteral&>(node), executeContext);
			break;
		case ASTNodeType::Null:
			result = ExecuteNull(static_cast<const ASTNodeNull&>(node), executeContext);
			break;
		case ASTNodeType::ResolveSymbol:
			result = ExecuteResolveSymbol(static_cast<const ASTNodeResolveSymbol&>(node), executeContext);
			break;
		case ASTNodeType::AssignSymbol:
			result = ExecuteAssignSymbol(static_cast<const ASTNodeAssignSymbol&>(node), executeContext);
			break;
		case ASTNodeType::ArrayInitializer:
			result = ExecuteArrayInitializer(static_cast<const ASTNodeArrayInitializer&>(node), executeContext);
			break;
		case ASTNodeType::ObjectInitializer:
			result = ExecuteObjectInitializer(static_cast<const ASTNodeObjectInitializer&>(node), executeContext);
			break;
		case ASTNodeType::FunctionStatement:
			result = ExecuteFunctionStatement(static_cast<const ASTNodeFunctionStatement&>(node), executeContext);
			break;
		case ASTNodeType::FunctionInitializer:
			result = ExecuteFunctionInitializer(static_cast<const ASTNodeFunctionInitializer&>(node), executeContext);
			break;
		case ASTNodeType::LocalVariableDeclaration:
			result = ExecuteLocalVariableDeclaration(static_cast<const ASTNodeLocalVariableDeclaration&>(node), executeContext);
			break;
		case ASTNodeType::LocalVariableDeclarationList:
			result = ExecuteLocalVariableDeclarationList(static_cast<const ASTNodeLocalVariableDeclarationList&>(node), executeContext);
			break;
		case ASTNodeType::Foreach:
			result = ExecuteForeach(static_cast<const ASTNodeForeach&>(node), executeContext);
			break;
		case ASTNodeType::For:
			result = ExecuteFor(static_cast<const ASTNodeFor&>(node), executeContext);
			break;
		case ASTNodeType::While:
			result = ExecuteWhile(static_cast<const ASTNodeWhile&>(node), executeContext);
			break;
		case ASTNodeType::If:
			result = ExecuteIf(static_cast<const ASTNodeIf&>(node), executeContext);
			break;
		case ASTNodeType::Break:
			result = ExecuteBreak(static_cast<const ASTNodeBreak&>(node), executeContext);
			break;
		case ASTNodeType::Continue:
			result = ExecuteContinue(static_cast<const ASTNodeContinue&>(node), executeContext);
			break;
		case ASTNodeType::Return:
			result = ExecuteReturn(static_cast<const ASTNodeReturn&>(node), executeContext);
			break;
		case ASTNodeType::Operator2:
			result = ExecuteOperator2(static_cast<const ASTNodeEvalOperator2&>(node), executeContext);
			break;
		case ASTNodeType::Operator1:
			result = ExecuteOperator1(static_cast<const ASTNodeEvalOperator1&>(node), executeContext);
			break;
		case ASTNodeType::NewClassInstance:
			result = ExecuteNewClassInstance(static_cast<const ASTNodeNewClassInstance&>(node), executeContext);
			break;
		case ASTNodeType::FunctionCall:
			result = ExecuteFunctionCall(static_cast<const ASTNodeFunctionCall&>(node), executeContext);
			break;
		case ASTNodeType::ContextValue:
			result = ExecuteContextValue(static_cast<const ASTNodeContextValue&>(node), executeContext);
			break;
		case ASTNodeType::ResolveMember:
			result = ExecuteResolveMember(static_cast<const ASTNodeResolveMember&>(node), executeContext);
			break;
		case ASTNodeType::AssignMember:
			result = ExecuteAssignMember(static_cast<const ASTNodeAssignMember&>(node), executeContext);
			break;
		case ASTNodeType::Try:
			result = ExecuteTry(static_cast<const ASTNodeTry&>(node), executeContext);
			break;
		case ASTNodeType::Throw:
			result = ExecuteThrow(static_cast<const ASTNodeThrow&>(node), executeContext);
			break;
		case ASTNodeType::TalkJump:
			result = ExecuteTalkJump(static_cast<const ASTNodeTalkJump&>(node), executeContext);
			break;
		case ASTNodeType::TalkSpeak:
			result = ExecuteTalkSpeak(static_cast<const ASTNodeTalkSpeak&>(node), executeContext);
			break;
		case ASTNodeType::TalkSetSpeaker:
			result = ExecuteTalkSetSpeaker(static_cast<const ASTNodeTalkSetSpeaker&>(node), executeContext);
			break;
		case ASTNodeType::UnitRoot:
			result = ExecuteUnitRoot(static_cast<const ASTNodeUnitRoot&>(node), executeContext);
			break;

		default:
			assert(false);
		}

		//実行中ノードを戻して終わり
		executeContext.SetCurrentASTNode(callingNode);
		return result;
	}

	//コードブロック
	ScriptValueRef ScriptExecutor::ExecuteCodeBlock(const ASTNodeCodeBlock& node, ScriptExecuteContext& executeContext) {

		//NOTE: C++のように任意でスコープを組むことはできないこともあり（オブジェクトイニシャライザと競合）呼び出し側でブロックスコープを形成させる
		//たとえばforやcatchのようなローカル変数を用意するものがあるので呼び出し側でブロックスコープをつくり、その中で変数をいれてから呼び出す

		for (const ConstASTNodeRef& stmt : node.GetStatements()) {

			if (stmt->IsExecutable()) {
				//ここがステートメントになるはずなので、ブレークポイントヒットとしては適切なはず⋯
				Debugger::NotifyASTExecute(*stmt.get(), executeContext);
			}

			//実行
			ExecuteInternal(*stmt, executeContext);

			//離脱要求があれば即時終了
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
		}


		//最後まで実行し、それがトークブロックであれば戻り値を設定する
		if (node.IsTalkBlock()) {
			executeContext.GetStack().ReturnTalk();
		}

		return ScriptValue::Null;
	}

	//フォーマット文字列
	ScriptValueRef ScriptExecutor::ExecuteFormatString(const ASTNodeFormatString& node, ScriptExecuteContext& executeContext) {

		//文字列をくみたてていく
		std::string result;
		for (const auto& item : node.GetItems()) {
			ScriptValueRef v = ExecuteInternal(*item.node, executeContext);
			
			//離脱要求があれば終了
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			//フォーマット指定が有効なものだけくっつける
			if (item.isFormatExpression) {
				auto str = v->ToStringWithFunctionCall(executeContext, &node);
				if (executeContext.RequireLeave()) {
					return ScriptValue::Null;
				}
				result += str;
			}
		}

		return ScriptValue::Make(result);
	}

	//文字列リテラル
	ScriptValueRef ScriptExecutor::ExecuteStringLiteral(const ASTNodeStringLiteral& node, ScriptExecuteContext& executeContext) {
		return ScriptValue::Make(node.GetValue());
	}

	//数値リテラル
	ScriptValueRef ScriptExecutor::ExecuteNumberLiteral(const ASTNodeNumberLiteral& node, ScriptExecuteContext& executeContext) {
		return ScriptValue::Make(node.GetValue());
	}

	//boolリテラル
	ScriptValueRef ScriptExecutor::ExecuteBooleanLiteral(const ASTNodeBooleanLiteral& node, ScriptExecuteContext& executeContext) {
		return node.GetValue() ? ScriptValue::True : ScriptValue::False;
	}

	//null
	ScriptValueRef ScriptExecutor::ExecuteNull(const ASTNodeNull& node, ScriptExecuteContext& executeContext) {
		return ScriptValue::Null;
	}

	//コンテキスト値
	ScriptValueRef ScriptExecutor::ExecuteContextValue(const ASTNodeContextValue& node, ScriptExecuteContext& executeContext) {
		switch (node.GetValueType()) {
			case ASTNodeContextValue::ValueType::This:
				//そのままthisをかえす
				return executeContext.GetBlockScope()->GetThisValue();
				
			case ASTNodeContextValue::ValueType::Base:
				{
					//実行中のthis、ASTノードのクラスを使用してbaseを返す
					ScriptValueRef self = executeContext.GetBlockScope()->GetThisValue();
					if (self->IsObject()) {
						ObjectRef obj = self->GetObjectRef();
						if (obj->GetNativeInstanceTypeId() == ClassInstance::TypeId()) {
							return ScriptValue::Make(obj.Cast<ClassInstance>()->MakeBase(obj.Cast<ClassInstance>(), node.GetClass(), executeContext));
						}
					}
					//相当するオブジェクトが存在しない場合
					return ScriptValue::Null;
				}	 
				break;
		}

		//こないはず
		assert(false);
		return ScriptValue::Null;
	}

	//ユニットルート
	ScriptValueRef ScriptExecutor::ExecuteUnitRoot(const ASTNodeUnitRoot& node, ScriptExecuteContext& executeContext) {
		//ルート空間を示す
		return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<UnitObject>(""));
	}

	//シンボル解決
	ScriptValueRef ScriptExecutor::ExecuteResolveSymbol(const ASTNodeResolveSymbol& node, ScriptExecuteContext& executeContext) {
		return executeContext.GetSymbol(node.GetSymbolName(), *node.GetSourceMetadata());
	}

	//シンボル設定
	ScriptValueRef ScriptExecutor::ExecuteAssignSymbol(const ASTNodeAssignSymbol& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef v = ExecuteInternal(*node.GetValueNode(), executeContext);

		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		executeContext.SetSymbol(node.GetSymbolName(), v, *node.GetSourceMetadata());
		return v;
	}

	//ローカル変数宣言
	ScriptValueRef ScriptExecutor::ExecuteLocalVariableDeclaration(const ASTNodeLocalVariableDeclaration& node, ScriptExecuteContext& executeContext) {
		//スタックローカルに変数を宣言してかきこむ
		if (node.GetValueNode() != nullptr) {
			ScriptValueRef init = ExecuteInternal(*node.GetValueNode(), executeContext);

			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			executeContext.GetBlockScope()->RegisterLocalVariable(node.GetName(), init);
		}
		else {
			executeContext.GetBlockScope()->RegisterLocalVariable(node.GetName(), ScriptValue::Null);
		}
		return ScriptValue::Null;
	}

	//ローカル変数宣言リスト
	ScriptValueRef ScriptExecutor::ExecuteLocalVariableDeclarationList(const ASTNodeLocalVariableDeclarationList& node, ScriptExecuteContext& executeContext) {
		for (const std::shared_ptr<const ASTNodeLocalVariableDeclaration>& item : node.GetVariables()) {
			ExecuteLocalVariableDeclaration(*item, executeContext);

			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
		}
		return ScriptValue::Null;
	}

	ScriptValueRef ScriptExecutor::ExecuteForeach(const ASTNodeForeach& node, ScriptExecuteContext& executeContext) {
		
		//変数定義部分のスコープを作成
		ScriptExecuteContext initScopeContext = executeContext.CreateChildBlockScopeContext();

		//変数宣言に応じて設定
		if (node.IsRegisterLocalVariable()) {
			initScopeContext.GetBlockScope()->RegisterLocalVariable(node.GetLoopValueName(), ScriptValue::Null);
			initScopeContext.GetBlockScope()->RegisterLocalVariable(node.GetLoopKeyName(), ScriptValue::Null);
		}

		//ターゲット式の実行
		ScriptValueRef target = ExecuteInternal(*node.GetTargetExpression(), executeContext);

		//ターゲットが反復可能基底に基づいている必要がある
		ScriptIterable* iterable = executeContext.GetInterpreter().InstanceAs<ScriptIterable>(target);
		if (iterable == nullptr) {
			//error: 反復可能ではないので例外送出
			executeContext.ThrowRuntimeError<RuntimeError>(TextSystem::Find("AOSORA_ITERATOR_ERROR_002"), executeContext);
			return ScriptValue::Null;
		}

		//イテレータオブジェクトの作成
		Reference<ScriptIterator> it = iterable->CreateIterator(executeContext);

		while (true) {
			{
				//ループ毎にブロックスコープを作り直す
				ScriptExecuteContext loopScopeContext = initScopeContext.CreateChildBlockScopeContext();

				//スープスコープとしてマーク
				ScriptInterpreterStack::LoopScope loopScole(loopScopeContext.GetStack());

				//イテレータ有効性をチェック
				if (it->IsEnd()) {
					it->Dispose();
					return ScriptValue::Null;
				}

				//ループ変数にイテレータが示す値を代入
				loopScopeContext.SetSymbol(node.GetLoopValueName(), it->GetValue(), *node.GetSourceMetadata());
				if (!node.GetLoopKeyName().empty()) {
					loopScopeContext.SetSymbol(node.GetLoopKeyName(), it->GetKey(), *node.GetSourceMetadata());
				}

				//コードブロック本体の実行
				ExecuteInternal(*node.GetLoopStatement(), loopScopeContext);

				//return/throwの脱出の場合は最優先で終わり
				if (loopScopeContext.GetStack().IsStackLeave()) {
					it->Dispose();
					return ScriptValue::Null;
				}

				//break対応
				if (loopScopeContext.GetStack().IsBreak()) {
					it->Dispose();
					return ScriptValue::Null;
				}

				//continue指示で離脱してきている可能性があるので指示をクリア
				loopScopeContext.GetStack().ClearLoopMode();

				//イテレータを次に進める
				it->FetchNext();
			}
		}

		return ScriptValue::Null;
	}

	ScriptValueRef ScriptExecutor::ExecuteFor(const ASTNodeFor& node, ScriptExecuteContext& executeContext) {
		
		//初期化部分のスコープを作成
		ScriptExecuteContext initScopeContext = executeContext.CreateChildBlockScopeContext();

		//初期化式の実行
		if (node.GetInitExpression() != nullptr) {
			ExecuteInternal(*node.GetInitExpression(), initScopeContext);
		}

		while (true) {

			{
				//ループ毎にブロックスコープを作り直す
				ScriptExecuteContext loopScopeContext = initScopeContext.CreateChildBlockScopeContext();

				//ループスコープとしてマーク
				ScriptInterpreterStack::LoopScope loopScope(loopScopeContext.GetStack());

				//条件部を実行
				ScriptValueRef expressionResult = ScriptValue::True;
				if (node.GetIfExpression() != nullptr) {
					expressionResult = ExecuteInternal(*node.GetIfExpression(), loopScopeContext);
					if (loopScopeContext.RequireLeave()) {
						return ScriptValue::Null;
					}
				}

				if (expressionResult->ToBoolean()) {
					//コードブロック本体を実行
					ExecuteInternal(*node.GetLoopStatement(), loopScopeContext);

					//return/throwの脱出の場合は最優先で終わり
					if (loopScopeContext.GetStack().IsStackLeave()) {
						return ScriptValue::Null;
					}

					//break対応
					if (loopScopeContext.GetStack().IsBreak()) {
						return ScriptValue::Null;
					}

					//continue指示で離脱してきている可能性があるので指示をクリア
					loopScopeContext.GetStack().ClearLoopMode();

					//continueか通常の離脱の場合、インクリメント部を実行
					if (node.GetIncrementExpression() != nullptr) {
						ExecuteInternal(*node.GetIncrementExpression(), loopScopeContext);
						if (loopScopeContext.RequireLeave()) {
							return ScriptValue::Null;
						}
					}
				}
				else {
					//条件不一致のため終了
					return ScriptValue::Null;
				}
			}
		}
	}

	ScriptValueRef ScriptExecutor::ExecuteWhile(const ASTNodeWhile& node, ScriptExecuteContext& executeContext) {

		while (true) {
			//判定処理
			ScriptValueRef expressionResult = ExecuteInternal(*node.GetIfExpression(), executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			//trueステートメント実行
			if (expressionResult->ToBoolean()) {

				{
					//ループ毎にブロックスコープを作り直す
					ScriptExecuteContext loopScopeContext = executeContext.CreateChildBlockScopeContext();

					//ループスコープとしてマーク
					ScriptInterpreterStack::LoopScope loopScope(loopScopeContext.GetStack());

					//コードブロック本体を実行
					ExecuteInternal(*node.GetTrueStatement(), loopScopeContext);
					
					//return/throwの脱出の場合は最優先で終わり
					if (loopScopeContext.GetStack().IsStackLeave()) {
						return ScriptValue::Null;
					}
					//break対応
					if (loopScopeContext.GetStack().IsBreak()) {
						return ScriptValue::Null;
					}

					//continue指示で離脱してきている可能性があるので指示をクリア
					loopScopeContext.GetStack().ClearLoopMode();
				}
			}
			else {
				//falseなのでループをブレークする
				return ScriptValue::Null;
			}
		}
	}

	//if文
	ScriptValueRef ScriptExecutor::ExecuteIf(const ASTNodeIf& node, ScriptExecuteContext& executeContext) {

		//判定処理
		ScriptValueRef expressionResult = ExecuteInternal(*node.GetIfExpression(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//ブロックスコープを作成
		ScriptExecuteContext childScopeContext = executeContext.CreateChildBlockScopeContext();

		//結果に応じてそれぞれのステートメントを実行
		if (expressionResult->ToBoolean()) {
			ExecuteInternal(*node.GetTrueStatement(), childScopeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
		}
		else if (node.GetFalseStatement() != nullptr) {
			ExecuteInternal(*node.GetFalseStatement(), childScopeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
		}
		return ScriptValue::Null;
	}

	//break文
	ScriptValueRef ScriptExecutor::ExecuteBreak(const ASTNodeBreak& node, ScriptExecuteContext& executeContext) {
		executeContext.GetStack().Break();
		return ScriptValue::Null;
	}

	//continue文
	ScriptValueRef ScriptExecutor::ExecuteContinue(const ASTNodeContinue& node, ScriptExecuteContext& executeContext) {
		executeContext.GetStack().Continue();
		return ScriptValue::Null;
	}

	//return文
	ScriptValueRef ScriptExecutor::ExecuteReturn(const ASTNodeReturn& node, ScriptExecuteContext& executeContext) {
		if (node.GetValueNode() != nullptr) {
			ScriptValueRef v = ExecuteInternal(*node.GetValueNode(), executeContext);

			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			//スタックフレームに脱出を指示
			executeContext.GetStack().Return(v);
		}
		else {
			//戻り値がなければnullを設定しておく
			executeContext.GetStack().Return(ScriptValue::Null);
		}
		return ScriptValue::Null;
	}

	//2項演算子評価
	ScriptValueRef ScriptExecutor::ExecuteOperator2(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		switch (node.GetOperator().type) {
		case OperatorType::Add:
			return ExecuteOpAdd(node, executeContext);
		case OperatorType::Sub:
			return ExecuteOpSub(node, executeContext);
		case OperatorType::Mul:
			return ExecuteOpMul(node, executeContext);
		case OperatorType::Div:
			return ExecuteOpDiv(node, executeContext);
		case OperatorType::Mod:
			return ExecuteOpMod(node, executeContext);
		case OperatorType::Eq:
			return ExecuteOpEq(node, executeContext);
		case OperatorType::Ne:
			return ExecuteOpNe(node, executeContext);
		case OperatorType::Gt:
			return ExecuteOpGt(node, executeContext);
		case OperatorType::Lt:
			return ExecuteOpLt(node, executeContext);
		case OperatorType::Ge:
			return ExecuteOpGe(node, executeContext);
		case OperatorType::Le:
			return ExecuteOpLe(node, executeContext);
		case OperatorType::LogicalOr:
			return ExecuteOpLogicalOr(node, executeContext);
		case OperatorType::LogicalAnd:
			return ExecuteOpLogicalAnd(node, executeContext);
		case OperatorType::NullCoalescing:
			return ExecuteOpNullCoalescing(node, executeContext);
		default:
			assert(false);
			return ScriptValue::Null;
		}
	}

	//足し算
	ScriptValueRef ScriptExecutor::ExecuteOpAdd(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		//両方のオペランドを評価
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		if (left->IsNumber() && right->IsNumber()) {
			//数値同士なら計算
			return ScriptValue::Make(left->ToNumber() + right->ToNumber());
		}
		else if (left->IsString() || right->IsString()) {
			//片方が文字列なら文字列結合
			return ScriptValue::Make(left->ToString() + right->ToString());
		}
		else {
			//それ以外は計算不可、nan
			return ScriptValue::NaN;
		}
	}

	//引き算
	ScriptValueRef ScriptExecutor::ExecuteOpSub(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		//両方のオペランドを評価
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//両方数値のみ使用可能
		if (left->IsNumber() && right->IsNumber()) {
			return ScriptValue::Make(left->ToNumber() - right->ToNumber());
		}
		else {
			return ScriptValue::NaN;
		}
	}

	//掛け算
	ScriptValueRef ScriptExecutor::ExecuteOpMul(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//両方数値のみ
		if (left->IsNumber() && right->IsNumber()) {
			return ScriptValue::Make(left->ToNumber() * right->ToNumber());
		}
		else {
			return ScriptValue::NaN;
		}
	}

	//割り算
	ScriptValueRef ScriptExecutor::ExecuteOpDiv(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//両方数値のみ
		if (left->IsNumber() && right->IsNumber()) {
			return ScriptValue::Make(left->ToNumber() / right->ToNumber());
		}
		else {
			return ScriptValue::NaN;
		}
	}

	//剰余
	ScriptValueRef ScriptExecutor::ExecuteOpMod(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//両方数値のみ
		if (left->IsNumber() && right->IsNumber()) {
			return ScriptValue::Make(std::fmod(left->ToNumber(), right->ToNumber()));
		}
		else {
			return ScriptValue::NaN;
		}
	}

	//一致
	ScriptValueRef ScriptExecutor::ExecuteOpEq(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//一致比較
		return ScriptValue::Make(left->IsEquals(right));
	}

	//不一致
	ScriptValueRef ScriptExecutor::ExecuteOpNe(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//一致比較の反転
		return ScriptValue::Make(!left->IsEquals(right));
	}

	// >
	ScriptValueRef ScriptExecutor::ExecuteOpGt(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//比較
		const number leftNumber = left->ToNumber();
		const number rightNumber = right->ToNumber();

		//数値比較できそうなら数値比較して、だめなら文字列的に比較する
		if (!std::isnan(leftNumber) && !std::isnan(rightNumber)) {
			if (leftNumber > rightNumber) {
				return ScriptValue::True;
			}
			else {
				return ScriptValue::False;
			}
		}
		else {
			//数値として評価できない場合は文字列的な大小比較をする
			if (left->ToString() > right->ToString()) {
				return ScriptValue::True;
			}
			else {
				return ScriptValue::False;
			}
		}
	}

	// <
	ScriptValueRef ScriptExecutor::ExecuteOpLt(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//比較
		const number leftNumber = left->ToNumber();
		const number rightNumber = right->ToNumber();

		//数値比較できそうなら数値比較して、だめなら文字列的に比較する
		if (!std::isnan(leftNumber) && !std::isnan(rightNumber)) {
			if (leftNumber < rightNumber) {
				return ScriptValue::True;
			}
			else {
				return ScriptValue::False;
			}
		}
		else {
			//数値として評価できない場合は文字列的な大小比較をする
			if (left->ToString() < right->ToString()) {
				return ScriptValue::True;
			}
			else {
				return ScriptValue::False;
			}
		}
	}

	// >=
	ScriptValueRef ScriptExecutor::ExecuteOpGe(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//比較
		const number leftNumber = left->ToNumber();
		const number rightNumber = right->ToNumber();

		//数値比較できそうなら数値比較して、だめなら文字列的に比較する
		if (!std::isnan(leftNumber) && !std::isnan(rightNumber)) {
			if (leftNumber >= rightNumber) {
				return ScriptValue::True;
			}
			else {
				return ScriptValue::False;
			}
		}
		else {
			//数値として評価できない場合は文字列的な大小比較をする
			if (left->ToString() >= right->ToString()) {
				return ScriptValue::True;
			}
			else {
				return ScriptValue::False;
			}
		}
	}

	// <=
	ScriptValueRef ScriptExecutor::ExecuteOpLe(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//比較
		const number leftNumber = left->ToNumber();
		const number rightNumber = right->ToNumber();

		//数値比較できそうなら数値比較して、だめなら文字列的に比較する
		if (!std::isnan(leftNumber) && !std::isnan(rightNumber)) {
			if (leftNumber <= rightNumber) {
				return ScriptValue::True;
			}
			else {
				return ScriptValue::False;
			}
		}
		else {
			//数値として評価できない場合は文字列的な大小比較をする
			if (left->ToString() <= right->ToString()) {
				return ScriptValue::True;
			}
			else {
				return ScriptValue::False;
			}
		}
	}

	//論理or
	ScriptValueRef ScriptExecutor::ExecuteOpLogicalOr(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//javascriptにならって左がtrueなら左をかえし、そうでないなら右をかえす
		if (left->ToBoolean()) {
			return left;
		}
		else {
			ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
			return right;
		}
	}

	//論理and
	ScriptValueRef ScriptExecutor::ExecuteOpLogicalAnd(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//javascriptにならって左がtrueなら右をかえす
		if (left->ToBoolean()) {
			ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
			return right;
		}
		else {
			return left;
		}
	}

	//null合体
	ScriptValueRef ScriptExecutor::ExecuteOpNullCoalescing(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		if (left->IsNull()) {
			//leftがnullと評価された場合rightを返す

			ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			return right;
		}
		else {
			//非nullなら左
			return left;
		}
	}

	//単項演算子
	ScriptValueRef ScriptExecutor::ExecuteOperator1(const ASTNodeEvalOperator1& node, ScriptExecuteContext& executeContext) {
		switch (node.GetOperator().type) {
		case OperatorType::Plus:
			return ExecuteOpPlus(node, executeContext);
		case OperatorType::Minus:
			return ExecuteOpMinus(node, executeContext);
		case OperatorType::LogicalNot:
			return ExecuteOpLogicalNot(node, executeContext);
		}
		
		assert(false);
		return ScriptValue::Null;
	}

	//単項マイナス
	ScriptValueRef ScriptExecutor::ExecuteOpMinus(const ASTNodeEvalOperator1& node, ScriptExecuteContext& executeContext) {
		// number として解決し、マイナスにする
		ScriptValueRef v = ExecuteInternal(*node.GetOperand(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}
		const number numberValue = v->ToNumber();
		return ScriptValue::Make(-numberValue);
	}

	//単項プラス
	ScriptValueRef ScriptExecutor::ExecuteOpPlus(const ASTNodeEvalOperator1& node, ScriptExecuteContext& executeContext) {
		// number として解決するだけでよさそう
		ScriptValueRef v = ExecuteInternal(*node.GetOperand(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}
		const number numberValue = v->ToNumber();
		return ScriptValue::Make(numberValue);

	}

	// !
	ScriptValueRef ScriptExecutor::ExecuteOpLogicalNot(const ASTNodeEvalOperator1& node, ScriptExecuteContext& executeContext) {
		// boolに変換して反転する
		ScriptValueRef v = ExecuteInternal(*node.GetOperand(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}
		const bool value = v->ToBoolean();
		return ScriptValue::Make(!value);
	}

	//配列生成
	ScriptValueRef ScriptExecutor::ExecuteArrayInitializer(const ASTNodeArrayInitializer& node, ScriptExecuteContext& executeContext) {
		Reference<ScriptArray> obj = executeContext.GetInterpreter().CreateNativeObject<ScriptArray>();

		for (size_t i = 0; i < node.GetValues().size(); i++) {
			ScriptValueRef value = ExecuteInternal(*node.GetValues()[i], executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			obj->Add(value);
		}

		return ScriptValue::Make(obj);
	}

	//オブジェクト生成
	ScriptValueRef ScriptExecutor::ExecuteObjectInitializer(const ASTNodeObjectInitializer& node, ScriptExecuteContext& executeContext) {
		Reference<ScriptObject> obj = executeContext.GetInterpreter().CreateObject();;

		for (size_t i = 0; i < node.GetItems().size(); i++) {
			ScriptValueRef value = ExecuteInternal(*node.GetItems()[i].value, executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			obj->RawSet(node.GetItems()[i].key, value);
		}
		return ScriptValue::Make(obj);
	}

	//関数生成
	ScriptValueRef ScriptExecutor::ExecuteFunctionInitializer(const ASTNodeFunctionInitializer& node, ScriptExecuteContext& executeContext) {
		return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(node.GetFunction(), ScriptValue::Null));
	}

	//関数ステートメント。関数をランダムオーバーロード用オブジェクトにつっこんで渡す
	ScriptValueRef ScriptExecutor::ExecuteFunctionStatement(const ASTNodeFunctionStatement& node, ScriptExecuteContext& executeContext) {

		for (const std::string& funcName : node.GetNames()) {

			ScriptValueRef item = nullptr;
			if (executeContext.GetStack().IsRootCall()) {
				//ルート空間を実行している間はユニット変数を対象にする
				item = executeContext.GetInterpreter().GetUnitVariable(funcName, node.GetSourceMetadata()->GetScriptUnit()->GetUnit());
			}
			else {
				//そうでない場合は現在ブロックのローカル変数を対象にする
				item = executeContext.GetBlockScope()->GetLocalVariable(funcName);
			}
			
			OverloadedFunctionList* functionList = nullptr;
			
			if (item != nullptr) {
				functionList = executeContext.GetInterpreter().InstanceAs<OverloadedFunctionList>(item->GetObjectRef());
			}

			if (functionList != nullptr) {
				//すでに関数リストオブジェクトがある場合はそこにデリゲートを追加する
				functionList->Add(node.GetFunction(), node.GetConditionNode(), executeContext.GetBlockScope());
			}
			else {
				//関数リストではない場合は新規に作成する
				Reference<OverloadedFunctionList> funcList = executeContext.GetInterpreter().CreateNativeObject<OverloadedFunctionList>();
				funcList->SetName(funcName);
				funcList->Add(node.GetFunction(), node.GetConditionNode(), executeContext.GetBlockScope());

				if (executeContext.GetStack().IsRootCall()) {
					executeContext.GetInterpreter().SetUnitVariable(funcName, ScriptValue::Make(funcList), node.GetSourceMetadata()->GetScriptUnit()->GetUnit());
				}
				else {
					executeContext.GetBlockScope()->SetLocalVariable(funcName, ScriptValue::Make(funcList));
				}
			}
		}

		return ScriptValue::Null;
	}

	//インスタンス作成
	ScriptValueRef ScriptExecutor::ExecuteNewClassInstance(const ASTNodeNewClassInstance& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef classData = ExecuteInternal(*node.GetClassDataNode(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		std::vector<ScriptValueRef> callArgs;

		for (auto a : node.GetArgumentNodes()) {
			callArgs.push_back(ExecuteInternal(*a, executeContext));
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
		}

		return ScriptValue::Make(executeContext.GetInterpreter().NewClassInstance(classData, callArgs, executeContext));
	}

	//関数呼び出し
	ScriptValueRef ScriptExecutor::ExecuteFunctionCall(const ASTNodeFunctionCall& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef function = ExecuteInternal(*node.GetFunctionNode(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		std::vector<ScriptValueRef> callArgs;

		for (auto a : node.GetArgumentNodes()) {
			callArgs.push_back(ExecuteInternal(*a, executeContext));
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
		}

		//呼び出し不可時、エラーを生成する
		if (!function->IsObject() || !function->GetObjectRef()->CanCall()) {
			executeContext.ThrowRuntimeError<RuntimeError>(TextSystem::Find("AOSORA_BUILTIN_ERROR_002"), executeContext);
			return ScriptValue::Null;
		}

		FunctionResponse response;
		executeContext.GetInterpreter().CallFunction(*function, response, callArgs, executeContext, &node);
		if (response.IsThrew()) {
			executeContext.ThrowError(node, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName(), response.GetThrewError(), executeContext);
			return ScriptValue::Null;
		}
		else {
			return response.GetReturnValue();
		}
	}

	//メンバ取得
	ScriptValueRef ScriptExecutor::ExecuteResolveMember(const ASTNodeResolveMember& node, ScriptExecuteContext& executeContext) {

		//TODO: ASTで基本型メソッドを検索してしまうとリフレクション経由でこのあたりの情報にたどりつけないので基本形型にGet相当を呼び出した時をサポートしたほうがよい

		//thisオブジェクトの取り出し
		ScriptValueRef r = ExecuteInternal(*node.GetThisNode(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//キーの取り出し
		ScriptValueRef k = ExecuteInternal(*node.GetKeyNode(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}
		const std::string key = k->ToString();

		//Objectの場合は内部参照、それ以外の場合は組み込みメソッドの呼出等となる
		if (r->GetValueType() == ScriptValueType::Object) {
			auto result = r->GetObjectRef()->Get(key, executeContext);
			if (result != nullptr) {
				return result;
			}
		}
		else if (r->GetValueType() == ScriptValueType::Number) {
			auto result = PrimitiveMethod::GetNumberMember(r, key, executeContext);
			if (result != nullptr) {
				return result;
			}
		}
		else if (r->GetValueType() == ScriptValueType::Boolean) {
			auto result = PrimitiveMethod::GetBooleanMember(r, key, executeContext);
			if (result != nullptr) {
				return result;
			}
		}
		else if (r->GetValueType() == ScriptValueType::String) {
			auto result = PrimitiveMethod::GetStringMember(r, key, executeContext);
			if (result != nullptr) {
				return result;
			}
		}

		//TODO: ScriptObjectは内部でこれを検索しているので省略している。もっといい方法をとりたい。
		if (!executeContext.GetInterpreter().InstanceIs<ScriptObject>(r)) {
			//汎用メソッド類
			auto generalResult = PrimitiveMethod::GetGeneralMember(r, key, executeContext);
			if (generalResult != nullptr) {
				return generalResult;
			}
		}

		return ScriptValue::Null;
	}

	//メンバ設定
	ScriptValueRef ScriptExecutor::ExecuteAssignMember(const ASTNodeAssignMember& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef r = ExecuteInternal(*node.GetThisNode(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		if (r->GetValueType() == ScriptValueType::Object) {
			ScriptValueRef v = ExecuteInternal(*node.GetValueNode(), executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			ScriptValueRef k = ExecuteInternal(*node.GetKeyNode(), executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			r->GetObjectRef()->Set(k->ToString(), v, executeContext);
		}
		return ScriptValue::Null;
	}

	//try
	ScriptValueRef ScriptExecutor::ExecuteTry(const ASTNodeTry& node, ScriptExecuteContext& executeContext) {

		//まずtryブロックを実行
		{
			auto tryContext = executeContext.CreateChildBlockScopeContext();
			ScriptInterpreterStack::TryScope tryScope(tryContext.GetStack());
			ExecuteInternal(*node.GetTryBlock(), tryContext);
		}

		//throwされているかを確認
		if (executeContext.GetStack().IsThrew()) {
			
			ObjectRef err = executeContext.GetStack().GetThrewError();
			ScriptError* errObj = executeContext.GetInterpreter().InstanceAs<ScriptError>(err);
			bool canCatch = true;
			if (errObj != nullptr) {
				canCatch = errObj->CanCatch();
			}

			const ASTNodeBase* catchBlock = nullptr;
			const std::string* catchVariableName = nullptr;

			if (canCatch) {
				//catchブロックを選択する
				for (size_t i = 0; i < node.GetCatchBlocks().size(); i++) {
					bool isMatch = false;
					const auto& cb = node.GetCatchBlocks()[i];

					//TODO: 多段catchを想定していたときのなごり。javascriptのようにcatch型指定をなくすのでcatchブロックは1つのみの許容になるはず
					isMatch = true;

					if (isMatch) {
						catchBlock = node.GetCatchBlocks()[i].catchBlock.get();
						catchVariableName = &node.GetCatchBlocks()[i].catchVariable;
						break;
					}
				}

				//エラーを退避
				executeContext.GetStack().PendingLeaveMode();
			}

			//catchブロックが選択されていれば実行する
			if (catchBlock != nullptr) {
				auto catchContext = executeContext.CreateChildBlockScopeContext();

				//エラー変数があればそれを使う
				if (!catchVariableName->empty()) {
					catchContext.GetBlockScope()->RegisterLocalVariable(*catchVariableName, ScriptValue::Make(err));
				}

				ExecuteInternal(*catchBlock, catchContext);
			}

			//例外が再スローされてなければ例外を破棄する(catchによって握りつぶされた形)
			if (!executeContext.GetStack().IsThrew()) {
				executeContext.GetStack().ClearPendingError();
			}
		}

		//returnや再throwが発生していれば一旦退避
		executeContext.GetStack().PendingLeaveMode();

		//finallyブロックがあれば実行する
		if (node.GetFinallyBlock() != nullptr) {
			//TODO: finallyでreutrnできない制約を設ける必要あり
			auto finallyContext = executeContext.CreateChildBlockScopeContext();
			ExecuteInternal(*node.GetFinallyBlock(), finallyContext);
		}

		//returnや再throwの状態を復元
		executeContext.GetStack().RestorePendingLeaveMode();
		return ScriptValue::Null;
	}

	//throw
	ScriptValueRef ScriptExecutor::ExecuteThrow(const ASTNodeThrow& node, ScriptExecuteContext& executeContext) {
		//投げるエラーを取得
		//TODO: 再スローを検討
		ScriptValueRef r = ExecuteInternal(*node.GetValueNode(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}
		
		if (r->IsObject()) {
			executeContext.ThrowError(node, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName(), r->GetObjectRef(), executeContext);
		}
		else {
			executeContext.ThrowRuntimeError<RuntimeError>("throwできるのはErrorオブジェクトだけです。", executeContext);
		}
		
		return ScriptValue::Null;
	}

	ScriptValueRef ScriptExecutor::ExecuteTalkSpeak(const ASTNodeTalkSpeak& node, ScriptExecuteContext& executeContext) {
		//計算結果をトーク本文に追加する
		ScriptValueRef r = ExecuteInternal(*node.GetBody(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		if (executeContext.RequireLeave())
			return ScriptValue::Null;

		if (node.GetBody()->GetType() == ASTNodeType::FormatString) {
			//フォーマット文字列の場合、ステートメントブロックだけだったらトークの一部とはみなさない（無駄な改行をさけたいので)
			auto fmt = std::static_pointer_cast<const ASTNodeFormatString>(node.GetBody());
			if (fmt->IsFuncStatementBlockOnly()) {
				return ScriptValue::Null;
			}
		}

		executeContext.GetStack().AppendTalkBody(r->ToString(), executeContext.GetInterpreter());
		executeContext.GetStack().TalkLineEnd();
		return ScriptValue::Null;
	}

	ScriptValueRef ScriptExecutor::ExecuteTalkSetSpeaker(const ASTNodeTalkSetSpeaker& node, ScriptExecuteContext& executeContext) {

		//トークの先頭部分の指定があればそれを行ってから話者指定に入る
		executeContext.GetStack().AppendTalkHeadIfNeed(executeContext.GetInterpreter());

		//話者指定を作成
		if (node.GetSpeakerIndex() == ASTNodeTalkSetSpeaker::SPEAKER_INDEX_SWITCH) {
			executeContext.GetStack().SwitchTalkSpeakerIndex(executeContext.GetInterpreter());
		}
		else {
			executeContext.GetStack().SetTalkSpeakerIndex(node.GetSpeakerIndex(), executeContext.GetInterpreter());
		}
		return ScriptValue::Null;
	}

	ScriptValueRef ScriptExecutor::ExecuteTalkJump(const ASTNodeTalkJump& node, ScriptExecuteContext& executeContext) {

		//ジャンプ条件を評価
		ScriptValueRef conditionResult = ScriptValue::True;

		if (node.GetConditionNode() != nullptr) {
			conditionResult = ExecuteInternal(*node.GetConditionNode(), executeContext);
			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}
		}

		if (executeContext.RequireLeave())
			return ScriptValue::Null;

		if (!conditionResult->ToBoolean()) {
			//不成立
			return ScriptValue::Null;
		}

		//ジャンプ先を取得
		ScriptValueRef jumpTarget = ExecuteInternal(*node.GetFunctionNode(), executeContext);
		if (jumpTarget->IsObject() && jumpTarget->GetObjectRef()->CanCall()) {

			//呼び出し可能であれば引数の作成にはいる
			std::vector<ScriptValueRef> args;
			ResolveArguments(node.GetArgumentNodes(), args, executeContext);

			if (executeContext.RequireLeave()) {
				return ScriptValue::Null;
			}

			//呼び出し
			FunctionResponse res;
			executeContext.GetStack().SetTalkJump(true);
			executeContext.GetInterpreter().CallFunction(*jumpTarget, res, args, executeContext, &node);
			executeContext.GetStack().SetTalkJump(false);

			if (res.IsThrew()) {
				executeContext.GetStack().Throw(res.GetThrewError());
				return ScriptValue::Null;
			}
			else {

				//関数の出力に貼り付ける
				executeContext.GetStack().SetTalkJump(true);
				auto str = res.GetReturnValue()->ToStringWithFunctionCall(executeContext, &node);
				executeContext.GetStack().SetTalkJump(false);

				if (executeContext.RequireLeave()) {
					return ScriptValue::Null;
				}
				executeContext.GetStack().AppendJumpedTalkBody(str, executeContext.GetInterpreter());

				//ここまでのトーク内容を関数の戻り値として返す
				executeContext.GetStack().ReturnTalk();

				return ScriptValue::Null;
			}
		}

		//ジャンプ不可
		return ScriptValue::Null;
	}

	bool ScriptExecutor::ResolveArguments(const std::vector<ConstASTNodeRef>& argumentNodes, std::vector<ScriptValueRef>& argumentValues, ScriptExecuteContext& executeContext) {

		//それぞれ解決
		for (auto a : argumentNodes) {
			auto v = ExecuteInternal(*a, executeContext);
			if (executeContext.RequireLeave()) {
				//打ち切り
				return false;
			}
			argumentValues.push_back(v);
		}

		return true;
	}

	//現在のユニット変数を取得
	ScriptValueRef ScriptInterpreter::GetUnitVariable(const std::string& name, const std::string& scriptUnit) {

		//unitがない場合は追加
		//TODO: 追加を許容しなくなるかも?
		if (!units.contains(scriptUnit)) {
			RegisterUnit(scriptUnit);
		}

		auto& variables = units.find(scriptUnit)->second.unitVariables;
		auto it = variables.find(name);
		if (it != variables.end()) {
			return it->second;
		}
		else {

			//子ユニット参照
			std::string childUnitKey = scriptUnit + "." + name;
			auto childUnit = units.find(childUnitKey);
			if (childUnit != units.end()) {
				return ScriptValue::Make(GetUnit(childUnitKey));
			}

			return nullptr;
		}
	}

	//現在のユニット変数を設定
	void ScriptInterpreter::SetUnitVariable(const std::string& name, const ScriptValueRef& value, const std::string& scriptUnit) {

		//unitがない場合は追加
		//TODO: 追加を許容しなくなるかも?
		if (!units.contains(scriptUnit)) {
			RegisterUnit(scriptUnit);
		}

		auto& variables = units.find(scriptUnit)->second.unitVariables;
		auto it = variables.find(name);
		if (it != variables.end()) {
			it->second = value;
		}
		else {
			variables.insert(std::map<std::string, ScriptValueRef>::value_type(name, value));
		}
	}

	//ユニットを登録
	void ScriptInterpreter::RegisterUnit(const std::string& unitName) {
		units.insert(decltype(units)::value_type(unitName, UnitData()));
	}

	//ユニット情報を取得
	const UnitData* ScriptInterpreter::FindUnitData(const std::string& unitName) {
		auto it = units.find(unitName);
		if (it != units.end()) {
			return &it->second;
		}
		return nullptr;
	}

	//ユニット取得
	Reference<UnitObject> ScriptInterpreter::GetUnit(const std::string& unitName) {

		if (unitName.empty()) {
			//ユニットルートを示す
			return CreateNativeObject<UnitObject>("");
		}

		//unitがない場合は追加
		//TODO: 追加を許容しなくなるかも?
		if (!units.contains(unitName)) {
			RegisterUnit(unitName);
		}

		return CreateNativeObject<UnitObject>(unitName);
	}

	Reference<UnitObject> ScriptInterpreter::FindUnit(const std::string& unitName) {
		if (unitName.empty()) {
			//ユニットルートを示す
			return CreateNativeObject<UnitObject>("");
		}

		//Find版は見つからなければnullを返す
		if (!units.contains(unitName)) {
			return nullptr;
		}

		return CreateNativeObject<UnitObject>(unitName);
	}

	ScriptValueRef ScriptInterpreter::GetFromAlias(const ScriptUnitAlias& alias, const std::string& name) {

		//エイリアス
		const AliasItem* unitAlias = alias.FindAlias(name);
		if (unitAlias) {
			//ユニットエイリアスを解決してその中身を取得
			Reference<UnitObject> unit = GetUnit(unitAlias->parentUnit);
			if (unit != nullptr) {
				ScriptValueRef result = unit->Get(unitAlias->targetName, *this);
				if (result != nullptr) {
					return result;
				}
			}
		}

		//ワイルドカードエイリアスを検索
		for (size_t i = 0; i < alias.GetWildcardAliasCount(); i++) {
			Reference<UnitObject> unit = GetUnit(alias.GetWildcardAlias(i));
			if (unit != nullptr) {
				//ユニット名から名前検索
				ScriptValueRef result = unit->Get(name, *this);
				if (result != nullptr) {
					return result;
				}
			}
		}

		return ScriptValue::Null;
	}

	//クラス取得
	ScriptValueRef ScriptInterpreter::GetClass(uint32_t typeId) {
		auto it = classIdMap.find(typeId);
		if (it != classIdMap.end()) {
			return ScriptValue::Make(it->second);
		}
		else {
			return nullptr;
		}
	}

	/*
	ScriptValueRef ScriptInterpreter::GetClass(const std::string& name) {
		auto it = classMap.find(name);
		if (it != classMap.end()) {
			return ScriptValue::Make(it->second);
		}
		else {
			return nullptr;
		}
	}

	uint32_t ScriptInterpreter::GetClassId(const std::string& name) {
		auto it = classMap.find(name);
		if (it != classMap.end()) {
			return it->second->GetClassTypeId();
		}
		else {
			return ObjectTypeIdGenerator::INVALID_ID;
		}
	}
	*/

	//クラス参照の検索
	ScriptValueRef FindClass(const std::string& classPath, const ScriptSourceMetadataRef& sourcemeta, bool isAbsolutePath) {
		//TODO: 検索
		return nullptr;
	}

	std::string ScriptInterpreter::GetClassTypeName(uint32_t typeId) {
		auto item = classIdMap.find(typeId);
		if (item != classIdMap.end()) {
			return item->second->GetMetadata().GetName();
		}
		else {
			assert(false);
			return "";	//登録してないクラスはないはず⋯ 入力IDが誤りなので、C++側に問題がありそう
		}
	}

	//ルートステートメント実行
	ToStringFunctionCallResult ScriptInterpreter::Execute(const ConstASTNodeRef& node, bool toStringResult, bool isRootCall) {

		//ユニットを登録
		RegisterUnit(node->GetScriptUnit()->GetUnit());

		ScriptInterpreterStack rootStack(isRootCall);
		Reference<BlockScope> rootBlock = CreateNativeObject<BlockScope>(nullptr);
		ScriptExecuteContext executeContext(*this, rootStack, rootBlock);
		ScriptExecutor::ExecuteASTNode(*node, executeContext);

		ToStringFunctionCallResult result;
		if (rootStack.IsThrew()) {
			result.success = false;
			result.error = rootStack.GetThrewError();
		}
		else if(toStringResult) {
			return rootStack.GetReturnValue()->ToStringWithFunctionCall(*this);
		}
		else {
			//成功だけ返す
			result.success = true;
		}
		return result;
	}

	//文字列を式として評価
	EvaluateExpressionResult ScriptInterpreter::Eval(const std::string& expr, ScriptExecuteContext& executeContext, const ScriptSourceMetadataRef& importSourceMeta) {

		EvaluateExpressionResult result;

		//その場で解析を行い、実行する		
		auto tokens = TokensParser::Parse(expr, SourceFilePath("__eval__", "__eval__"));	//一時的なファイル名として入力しておく
		if (tokens->error != nullptr) {
			result.error = tokens->error;
			return result;
		}

		//ASTにユニットエイリアスをインポートしないといけない
		auto ast = ASTParser::ParseExpression(tokens, &importSourceMeta, true);

		if (ast->error != nullptr) {
			result.error = ast->error;
			return result;
		}

		result.value = ScriptExecutor::ExecuteASTNode(*ast->root, executeContext);
		return result;
	}

	ScriptInterpreter::ScriptInterpreter() :
		scriptSteps(0),
		debuggerScriptSteps(0),
		limitScriptSteps(DEFAULT_EXECUTE_LIMIT_STEPS),
		scriptClassCount(0),
		debugOutputStream(nullptr),
		isDebuggerScope(false)
	{
		//組み込みのクラスを登録
		RegisterNativeFunction("print", &ScriptInterpreter::Print);

		ImportClass(NativeClass::Make<ScriptIterable>("Iterable"));
		ImportClass(NativeClass::Make<ScriptIterator>("Iterator"));
		ImportClass(NativeClass::Make<ScriptArray>("Array", ClassPath("Iterable")));
		ImportClass(NativeClass::Make<ScriptArrayIterator>("Array", ClassPath("Iterator")));
		ImportClass(NativeClass::Make<ScriptObject>("Map", ClassPath("Iterable")));
		ImportClass(NativeClass::Make<ScriptObjectIterator>("MapIterator", ClassPath("Iterator")));
		ImportClass(NativeClass::Make<ClassData>("ClassData"));
		ImportClass(NativeClass::Make<Reflection>("Reflection"));
		ImportClass(NativeClass::Make<Delegate>("Delegate"));
		ImportClass(NativeClass::Make<ScriptError>("Error", &ScriptError::CreateObject));
		ImportClass(NativeClass::Make<RuntimeError>("RuntimeError", ClassPath("Error")));
		ImportClass(NativeClass::Make<PluginError>("PluginError", ClassPath("Error")));
		ImportClass(NativeClass::Make<AssertError>("AssertError", ClassPath("Error")));
		ImportClass(NativeClass::Make<Time>("Time"));
		ImportClass(NativeClass::Make<SaveData>("Save"));
		ImportClass(NativeClass::Make<OverloadedFunctionList>("OverloadedFunctionList"));
		ImportClass(NativeClass::Make<InstancedOverloadFunctionList>("InstancedOverloadFunctionList"));
		ImportClass(NativeClass::Make<TalkTimer>("TalkTimer"));
		ImportClass(NativeClass::Make<Random>("Random"));
		ImportClass(NativeClass::Make<SaoriManager>("Saori"));
		ImportClass(NativeClass::Make<SaoriModule>("SaoriModule"));
		ImportClass(NativeClass::Make<PluginManager>("PluginManager"));
		ImportClass(NativeClass::Make<PluginModule>("PluginModule"));
		ImportClass(NativeClass::Make<PluginDelegate>("PluginDelegate"));
		ImportClass(NativeClass::Make<TalkBuilder>("TalkBuilder"));
		ImportClass(NativeClass::Make<TalkBuilderSettings>("TalkBuilderSettings"));
		ImportClass(NativeClass::Make<ScriptDebug>("Debug"));
		ImportClass(NativeClass::Make<UnitObject>("ScriptUnit"));
		ImportClass(NativeClass::Make<MemoryBuffer>("MemoryBuffer"));

		ImportClass(NativeClass::Make<ScriptJsonSerializer>("JsonSerializer", nullptr, "std"));
		ImportClass(NativeClass::Make<ScriptFileAccess>("File", nullptr, "std"));
	}

	ScriptInterpreter::~ScriptInterpreter() {

		//各クラスの終了処理
		for (auto item : classIdMap) {
			//ネイティブクラスのstatic情報解放
			if (!item.second->GetMetadata().IsScriptClass()) {
				static_cast<const NativeClass&>(item.second->GetMetadata()).GetStaticDestructFunc()(*this);
			}
		}

		//デバッグストリームの削除
		CloseDebugOutputStream();
	}

	//クラスのインポート
	void ScriptInterpreter::ImportClasses(const std::map<std::string, ScriptClassRef>& classMap) {
		for (auto item : classMap) {
			ImportClass(item.second);
		}
	}

	void ScriptInterpreter::ImportClass(const std::shared_ptr<ClassBase>& cls){
		//スクリプトクラスの場合はこのタイミングでID発行
		uint32_t classId = cls->GetTypeId();
		if (cls->IsScriptClass()) {
			classId = ObjectTypeIdGenerator::GenerateScriptClassId(scriptClassCount);
			cls->SetTypeId(classId);
			scriptClassCount++;
		}

		//登録
		auto classData = CreateNativeObject<ClassData>(cls, classId, this);
		//classMap[cls->GetName()] = classData;
		classIdMap[classId] = classData;

		//ユニット空間に登録
		SetUnitVariable(cls->GetName(), ScriptValue::Make(classData), cls->GetUnitName());
	}

	//クラスのリレーションシップ構築
	std::shared_ptr<ScriptParseError> ScriptInterpreter::CommitClasses() {
		for (auto item : classIdMap) {
			if (item.second->GetMetadata().HasParentClass()) {

				//親クラス検索
				ClassData* parentClass = nullptr;

				const ClassBase& classMeta = item.second->GetMetadata();
				if (classMeta.IsScriptClass()) {
					//スクリプトクラスであればエイリアスから継承検索をかける
					parentClass = FindClass(item.second->GetMetadata().GetParentClassPath(), &static_cast<const ScriptClass&>(classMeta).GetSourceMetadata()->GetAlias());
				}
				else {
					parentClass = FindClass(item.second->GetMetadata().GetParentClassPath(), nullptr);
				}

				if (parentClass == nullptr) {
					//パースエラーの一部ということにしておく
					ScriptParseErrorData errData;
					errData.errorCode = "A048";
					errData.message = TextSystem::Find("ERROR_MESSAGEA048");
					errData.hint = TextSystem::Find("ERROR_HINTA048");
					if (classMeta.IsScriptClass()) {
						return std::shared_ptr<ScriptParseError>(new ScriptParseError(errData, static_cast<const ScriptClass&>(classMeta).GetDeclareSourceRange()));
					}
					else {
						return std::shared_ptr<ScriptParseError>(new ScriptParseError(errData, SourceCodeRange()));
					}
				}

				//双方登録
				//WARN: 参照が再作成されているので注意かも
				item.second->SetParentClass(parentClass);

				while (parentClass != nullptr) {
					parentClass->AddUpcastType(item.second);
					parentClass = parentClass->GetParentClass().Get();
				}
			}

			//ネイティブクラスのstatic情報初期化
			if (!item.second->GetMetadata().IsScriptClass()) {
				static_cast<const NativeClass&>(item.second->GetMetadata()).GetStaticInitFunc()(*this);
			}
		}

		return nullptr;
	}

	//クラスの検索
	ClassData* ScriptInterpreter::FindClass(const ClassPath& classPath, const ScriptUnitAlias* alias) {
		if (!classPath.IsValid()) {
			return nullptr;
		}

		if (classPath.IsFullPath()) {

			//パス指定形式に問題がある（ユニット指定なので最小でもユニット名＋クラス名の２要素以上にならないといけない）
			if (classPath.GetPathNodeCount() < 2) {
				return nullptr;
			}

			//unit. で指定するフルパスなのでユニットバスと中身に分離して検索
			const std::string unitPath = JoinString(classPath.GetPathNodeCollection(), 0, classPath.GetPathNodeCount() - 1, ".");
			return InstanceAs<ClassData>(
				GetUnitVariable(classPath.GetPathNode(classPath.GetPathNodeCount() - 1), unitPath)
			);
		}
		else {

			//フルパスではない場合エイリアス指定が必須
			if (alias == nullptr) {
				assert(false);
				return nullptr;
			}

			//エイリアスで検索
			const std::string& firstNode = classPath.GetPathNode(0);
			auto node = GetFromAlias(*alias, firstNode);

			if (node == nullptr || !node->IsObject()) {
				return nullptr;
			}

			auto nodeObj = node->GetObjectRef();

			//ユニット階層をたどる
			for (size_t i = 1; i < classPath.GetPathNodeCount(); i++) {
				UnitObject* unit = InstanceAs<UnitObject>(nodeObj);
				if (unit != nullptr) {
					auto v = unit->Get(classPath.GetPathNode(i), *this);
					if (v == nullptr || !v->IsObject()) {
						return nullptr;
					}
					else {
						nodeObj = v->GetObjectRef();
					}
				}
				else {
					return nullptr;
				}
			}

			//最終的にclassDataが手に入るはず
			return InstanceAs<ClassData>(nodeObj);
		}
	}

	//関数登録
	void ScriptInterpreter::RegisterNativeFunction(const std::string& name, ScriptNativeFunction func) {

		RegisterSystem(name, ScriptValue::Make(CreateNativeObject<Delegate>(func)));
	}

	//スクリプトの呼出

	void ScriptInterpreter::CallFunctionInternal(const ScriptValue& funcVariable, const std::vector<ScriptValueRef>& args, ScriptInterpreterStack& funcStack, FunctionResponse& response) {
		assert(funcVariable.IsObject());

		if (!funcVariable.IsObject()) {
			//オブジェクトではないので呼出不可
			return;
		}

		ObjectRef funcObj = funcVariable.GetObjectRef();

		if (funcObj->CanCall()) {
			//コールスタックを作成
			std::shared_ptr<ScriptExecuteContext> funcContext;
			Reference<BlockScope> callScope;

			//デリゲートオブジェクトに対する呼び出しであればデリゲートからthisとブロックスコープを得て呼び出す
			Delegate* dele = InstanceAs<Delegate>(funcVariable);
			if (dele != nullptr) {
				callScope = CreateNativeObject<BlockScope>(dele->GetBlockScope());
				callScope->SetThisValue(dele->GetThisValue());

				//関数実行のコンテキストとリクエストを作成
				funcContext = std::shared_ptr<ScriptExecuteContext>(new ScriptExecuteContext(*this, funcStack, callScope));
				FunctionRequest req(args, *funcContext);

				if (dele->IsScriptFunction()) {
					//スクリプト関数の場合はブロックスコープに引数を展開
					auto scriptFunc = dele->GetScriptFunction();

					for (size_t i = 0; i < scriptFunc->GetArguments().size(); i++) {
						ScriptValueRef a = ScriptValue::Null;
						if (i < args.size()) {
							//引数を設定
							a = args[i];
						}
						callScope->RegisterLocalVariable(scriptFunc->GetArguments()[i], a);
					}

					//呼出
					ScriptExecutor::ExecuteASTNode(*scriptFunc->GetFunctionBody(), *funcContext);

					//結果のかきもどし
					if (funcStack.IsThrew()) {
						//関数から例外が返された
						response.SetThrewError(funcStack.GetThrewError());
					}
					else {
						//戻り値を返して終わり
						response.SetReturnValue(funcStack.GetReturnValue());
					}

				}
				else {
					//ネイティブ関数
					auto nativeFunc = dele->GetNativeFunction();
					nativeFunc(req, response);
				}
			}
			else {

				//独立したブロックスコープ作成
				callScope = CreateNativeObject<BlockScope>(nullptr);

				//関数実行のコンテキストとリクエストを作成
				funcContext = std::shared_ptr<ScriptExecuteContext>(new ScriptExecuteContext(*this, funcStack, callScope));
				FunctionRequest req(args, *funcContext);

				//オブジェクトを起動
				funcObj->Call(req, response);
			}
		}
		else {
			//呼出不可オブジェクト
		}
	}

	//実行コンテキストから新しいスタックフレームで関数を実行
	void ScriptInterpreter::CallFunction(const ScriptValue& funcVariable, FunctionResponse& response, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& executeContext, const ASTNodeBase* callingAstNode, const std::string& funcName) {
		ScriptInterpreterStack funcStack = executeContext.GetStack().CreateChildStackFrame(callingAstNode, executeContext.GetBlockScope(), funcName);
		CallFunctionInternal(funcVariable, args, funcStack, response);
	}

	//最上位のスタックフレームで関数を実行
	void ScriptInterpreter::CallFunction(const ScriptValue& funcVariable, FunctionResponse& response, const std::vector<ScriptValueRef>& args) {

		ScriptInterpreterStack funcStack;
		CallFunctionInternal(funcVariable, args, funcStack, response);
	}

	Reference<ScriptArray> ScriptInterpreter::CreateArray() {
		return objectManager.CreateObject<ScriptArray>();
	}

	//クラスインスタンス生成
	ObjectRef ScriptInterpreter::NewClassInstance(const ScriptValueRef& classData, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& context) {

		ClassData* c = context.GetInterpreter().InstanceAs<ClassData>(classData);
		if (c != nullptr) {
			//まずクラスを取得
			return NewClassInstance(c, args, context, nullptr);
		}
		else {
			return nullptr;
		}
	}

	//クラスインスタンス生成
	ObjectRef ScriptInterpreter::NewClassInstance(const Reference<ClassData>& classData, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& context, Reference<ClassInstance> scriptObjInstance) {

		//スクリプトクラスとネイティブクラスで初期化が異なる
		if (classData->GetMetadata().IsScriptClass()) {
			const ScriptClass& scriptClassData = static_cast<const ScriptClass&>(classData->GetMetadata());

			Reference<ClassData> parentClass = classData->GetParentClass();
			ScriptFunctionRef initFunc = scriptClassData.GetInitFunc();

			//コンストラクタ用のスタックフレームとブロックスコープを作成して引数を登録
			ScriptInterpreterStack initStack = context.GetStack().CreateChildStackFrame(context.GetCurrentASTNode(), context.GetBlockScope(), "init");
			Reference<BlockScope> initScope = context.GetInterpreter().CreateNativeObject<BlockScope>(context.GetBlockScope());

			ScriptExecuteContext funcContext(*this, initStack, initScope);

			//初期化関数があれば引数を作成
			//TODO: なくても評価が必要かも
			//引数評価の仕組みを統一する必要ありそう
			if (initFunc != nullptr) {
				for (size_t i = 0; i < initFunc->GetArguments().size(); i++) {

					ScriptValueRef a = ScriptValue::Null;
					if (i < args.size()) {
						//引数を設定
						a = args[i];
					}
					initScope->RegisterLocalVariable(initFunc->GetArguments()[i], a);
				}
			}

			Reference<ClassInstance> createdObject = scriptObjInstance;
			if (createdObject == nullptr) {
				//呼び出し元からオブジェクトを渡されていたらそれを使う、なければ新規
				createdObject = CreateNativeObject<ClassInstance>(classData, context);
			}

			if (parentClass != nullptr) {

				//親クラスがある場合、親クラスの初期化に投げる引数を設定
				std::vector<ScriptValueRef> parentArgs;

				for (size_t i = 0; i < scriptClassData.GetParentClassInitArgumentCount(); i++) {
					ScriptValueRef v = ScriptExecutor::ExecuteASTNode(*scriptClassData.GetParentClassInitArgument(i), funcContext);
					parentArgs.push_back(v);
				}

				//親クラスを初期化、その際スクリプトオブジェクトを渡してそこに書き込んでもらう
				ObjectRef r = NewClassInstance(parentClass, parentArgs, funcContext, createdObject);

				//戻り値と同じインスタンスを示しているはず
				assert(r.Get() == createdObject.Get());

				//TODO: NewClassInstanceが例外を出した場合に初期化を中断して離脱する
			}

			//thisを指定
			initScope->SetThisValue(ScriptValue::Make(createdObject));

			//コンストラクタ実行
			if (initFunc != nullptr) {
				ScriptExecutor::ExecuteASTNode(*initFunc->GetFunctionBody(), funcContext);
			}

			return createdObject;
		}
		else {
			//ネイティブクラス対応
			FunctionRequest request(args, context);
			FunctionResponse response;
			const NativeClass& nativeClass = static_cast<const NativeClass&>(classData->GetMetadata());

			if (nativeClass.GetInitFunc() != nullptr) {
				nativeClass.GetInitFunc()(request, response);
				
				if (response.IsThrew()) {
					context.ThrowError(*context.GetCurrentASTNode(), context.GetBlockScope(), "init", response.GetThrewError(), context);
					return nullptr;
				}
				else {
					//初期化成功時
					if (scriptObjInstance != nullptr) {
						//スクリプトオブジェクトの継承として初期化要求されている場合、結果をそのオブジェクトに格納する
						scriptObjInstance->SetNativeBaseInstance(response.GetReturnValue()->GetObjectRef());
						return scriptObjInstance;
					}
					else {
						//ネイティブオブジェクトそのものを初期化要求されている場合、それ自体を返す
						return response.GetReturnValue()->GetObjectRef();
					}
				}
			}
			else {
				context.ThrowRuntimeError<RuntimeError>("このクラスはインスタンス化できません。", context);
				return nullptr;
			}
		}
	}

	bool ScriptInterpreter::InstanceIs(const ObjectRef& obj, uint32_t classId) {
		return classIdMap[classId]->InstanceIs(obj->GetInstanceTypeId());
	}

	void ScriptInterpreter::CollectObjects() {
		//ルート空間の情報を収集してGCを起動する
		std::vector<CollectableBase*> rootCollectables;

		for (auto kv : systemRegistry) {
			if (kv.second->IsObject()) {
				rootCollectables.push_back(kv.second->GetObjectRef().Get());
			}
		}

		for (auto u : units) {
			for (auto kv : u.second.unitVariables) {
				if (kv.second->IsObject()) {
					rootCollectables.push_back(kv.second->GetObjectRef().Get());
				}
			}
		}

		for (auto kv : classIdMap) {
			rootCollectables.push_back(kv.second.Get());
		}

		for (auto kv : classIdMap) {
			//念の為
			rootCollectables.push_back(kv.second.Get());
		}

		for (auto kv : nativeStaticStore) {
			rootCollectables.push_back(kv.second.Get());
		}

		objectManager.CollectObjects(rootCollectables);
	}

	//デバッグ出力
	void ScriptInterpreter::OpenDebugOutputStream(const std::string& filename) {
		//場合によって追記モード
		assert(debugOutputStream == nullptr);
		if (debugOutputStream == nullptr) {
			debugOutputStream = new std::ofstream(workingDirectory + filename, std::ofstream::out | std::ofstream::app);
		}
	}

	std::ofstream* ScriptInterpreter::GetDebugOutputStream() {
		return debugOutputStream;
	}

	void ScriptInterpreter::CloseDebugOutputStream() {
		if (debugOutputStream != nullptr) {
			delete debugOutputStream;
			debugOutputStream = nullptr;
		}
	}

	//子ブロックスコープ作成
	ScriptExecuteContext ScriptExecuteContext::CreateChildBlockScopeContext() {
		return ScriptExecuteContext(interpreter, stack, interpreter.CreateNativeObject<BlockScope>(blockScope));
	}

	//実行
	ScriptValueRef ScriptExecuteContext::GetSymbol(const std::string& name, const ScriptSourceMetadata& sourcemeta) {

		//優先順位にしたがってシンボルを検索する
		ScriptValueRef result = nullptr;

		//ローカル
		result = blockScope->GetLocalVariable(name);
		if (result != nullptr) {
			return result;
		}

		//システム
		result = interpreter.GetSystemRegistryValue(name);
		if (result != nullptr) {
			return result;
		}

		//最後はユニット空間から探す
		return interpreter.GetFromAlias(sourcemeta.GetAlias(), name);
	}

	//シンボルの書き込み
	void ScriptExecuteContext::SetSymbol(const std::string& name, const ScriptValueRef& value, const ScriptSourceMetadata& sourcemeta) {
		//ローカルにあれば書き込み
		if (blockScope->SetLocalVariable(name, value)) {
			return;
		}

		if (interpreter.ContainsSystemRegistry(name)) {
			//システムレジストリは書き込み不可
			assert(false);
			return;
		}

		//エイリアス
		const AliasItem* unitAlias = sourcemeta.GetAlias().FindAlias(name);
		if (unitAlias) {
			//ユニットエイリアスを解決してその中身を取得
			Reference<UnitObject> unit = interpreter.GetUnit(unitAlias->targetName);
			if (unit != nullptr) {
				auto result = unit->Get(unitAlias->targetName, *this);
				if (result != nullptr) {
					//参照先がある場合その名前で書きこむ
					unit->Set(unitAlias->targetName, value, *this);
					return;
				}
			}
		}

		//ワイルドカードエイリアスを検索
		for (size_t i = 0; i < sourcemeta.GetAlias().GetWildcardAliasCount(); i++) {
			Reference<UnitObject> unit = interpreter.GetUnit(sourcemeta.GetAlias().GetWildcardAlias(i));
			if (unit != nullptr) {
				//ユニット名から名前検索
				auto result = unit->Get(name, *this);
				if (result != nullptr) {
					unit->Set(name, value, *this);
					return;
				}
			}
		}

		//最終的に無ければ現在ユニット空間に書き込む
		Reference<UnitObject> currentUnit = interpreter.GetUnit(sourcemeta.GetScriptUnit()->GetUnit());
		assert(currentUnit != nullptr);
		if (currentUnit != nullptr) {
			currentUnit->Set(name, value, *this);
		}
	}

	std::vector<CallStackInfo> ScriptExecuteContext::MakeStackTrace(const ASTNodeBase& currentAstNode, const Reference<BlockScope>& callingBlockScope, const std::string& currentFuncName) {
		//現在実行中のスタックフレームは引数から位置を取得
		std::vector<CallStackInfo> stackInfo;
		{
			CallStackInfo stackFrame;
			stackFrame.hasSourceRange = true;
			stackFrame.executingAstNode = &currentAstNode;
			stackFrame.sourceRange = currentAstNode.GetSourceRange();
			stackFrame.funcName = currentFuncName;
			stackFrame.blockScope = callingBlockScope;
			stackFrame.isJumping = false;
			stackInfo.push_back(stackFrame);
		}

		//親以降は呼び出し時に格納されてる値を使う
		const ScriptInterpreterStack* st = stack.GetParentStackFrame();
		while (st != nullptr)
		{
			CallStackInfo stackFrame;
			if (st->GetCallingASTNode() != nullptr) {
				stackFrame.hasSourceRange = true;
				stackFrame.sourceRange = st->GetCallingASTNode()->GetSourceRange();
				stackFrame.executingAstNode = st->GetCallingASTNode();
				stackFrame.blockScope = st->GetCallingBlockScope();
			}
			else {
				stackFrame.hasSourceRange = false;
				stackFrame.executingAstNode = nullptr;
			}
			stackFrame.funcName = st->GetFunctionName();
			stackFrame.isJumping = st->IsTalkJump();

			stackInfo.push_back(stackFrame);
			st = st->GetParentStackFrame();
		}

		return stackInfo;
	}

	void ScriptExecuteContext::ThrowError(const ASTNodeBase& throwAstNode, const Reference<BlockScope>& callingBlockScope, const std::string& funcName, const ObjectRef& err, ScriptExecuteContext& executeContext) {

		//エラーオブジェクトしかスローできないのでチェック
		ScriptError* e = interpreter.InstanceAs<ScriptError>(err);
		if (e == nullptr) {
			ThrowError(throwAstNode, callingBlockScope, funcName, interpreter.CreateNativeObject<RuntimeError>("throwできるのはErrorオブジェクトのみです。"), executeContext);
			return;
		}

		//コールスタック情報が未設定の場合にのみ設定する
		if (!e->HasCallstackInfo()) {
			e->SetCallstackInfo(MakeStackTrace(throwAstNode, callingBlockScope, funcName));
		}

		//この時点でデバッガに例外を通知する
		Debugger::NotifyError(*e, throwAstNode, executeContext);

		stack.Throw(e);
	}

	TalkStringCombiner::SpeakedSpeakers TalkStringCombiner::FetchLastSpeaker(const std::string& str) {
		//一旦\p[0]はおいといて、検索
		int32_t items[5];
		SpeakedSpeakers result;
		result.lastSpeakerIndex = 0;

		//NOTE: 見つからない場合~0になることを利用してマイナス化する
		items[0] = static_cast<int32_t>(str.rfind("\\0"));
		items[1] = static_cast<int32_t>(str.rfind("\\h"));

		if (items[0] >= 0 || items[1] >= 0) {
			result.usedSpeaker.insert(0);
		}

		items[2] = static_cast<int32_t>(str.rfind("\\1"));
		items[3] = static_cast<int32_t>(str.rfind("\\u"));

		if (items[2] >= 0 || items[3] >= 0) {
			result.usedSpeaker.insert(1);
		}

		items[4] = static_cast<int32_t>(str.rfind("\\p"));
		int32_t lastPTagIndex = 0;

		if (items[4] >= 0) {
			//\\pがある場合、すべての話者指定を抽出して処理
			std::string testing = str;
			std::smatch match{};

			//すべてのマッチをループ
			while (std::regex_search(testing, match, PATTERN_PTAG)) {

				//マッチの取り出し
				if (*(match[1].first) == '[') {
					size_t index;
					if (StringToIndex(match[2].str(), index)) {
						result.usedSpeaker.insert(static_cast<int32_t>(index));
						lastPTagIndex = static_cast<int32_t>(index);
					}
				}
				else {
					size_t index;
					if (StringToIndex(match[1].str(), index)) {
						result.usedSpeaker.insert(static_cast<int32_t>(index));
						lastPTagIndex = static_cast<int32_t>(index);
					}
				}

				testing = match.suffix();
			}
		}

		//最も最後に現れた話者を取得
		auto maxIterator = std::max_element(items, items + (sizeof(items)/sizeof(int32_t)));
		size_t maxIndex = std::distance(items, maxIterator);

		//1つも話者指定がない場合：\0扱いで終える
		if (items[maxIndex] == std::string::npos) {
			result.lastSpeakerIndex = 0;
			return result;
		}
		
		//最後のタグから話者の番号を決定
		switch (maxIndex) {
		case 0:
		case 1:
			result.lastSpeakerIndex = 0;
			return result;
		case 2:
		case 3:
			result.lastSpeakerIndex = 1;
			return result;
		case 4:
			result.lastSpeakerIndex = lastPTagIndex;
			return result;
		default:
			assert(false);
		}

		return result;
	}

	TalkStringCombiner::SpeakerSelector TalkStringCombiner::FetchFirstSpeaker(const std::string& str) {

		//先頭一致で話者を確認する
		if (str.starts_with("\\0") || str.starts_with("\\h")) {
			return { 0, std::string_view(str.c_str(), 2) };
		}
		else if (str.starts_with("\\1") || str.starts_with("\\u")) {
			return { 1, std::string_view(str.c_str(), 2) };
		}
		else if (str.starts_with("\\p[")) {
			size_t endBlacketIndex = str.find(']');
			if (endBlacketIndex != std::string::npos) {
				std::string n = str.substr(3, endBlacketIndex - 3);
				size_t speakerIndex;
				if (StringToIndex(n, speakerIndex)) {
					return { static_cast<int32_t>(speakerIndex), std::string_view(str.c_str(), endBlacketIndex) };
				}
			}
		}
		else if (str.starts_with("\\p")) {
			if (str.size() >= 3) {
				return { str[2] - '0', std::string_view(str.c_str(), 3) };
			}
		}
		
		return { -1, std::string_view(str.c_str(),0) };
	}

	std::string TalkStringCombiner::CombineTalk(const std::string& left, const std::string& right, ScriptInterpreter& interpreter, SpeakedSpeakers* speakedCache, bool disableSpeakerChangeLineBreak) {

		//leftの話者指定を検索
		auto selector = FetchFirstSpeaker(right);
		if (selector.speakerIndex == -1) {
			//話者指定が先頭にないのでそのままくっつける
			return left + right;
		}
		else {
			//話者指定があるので前半の最終の話者をチェック
			SpeakedSpeakers localSpeakers;
			SpeakedSpeakers* speakers = speakedCache;
			if (speakers == nullptr) {
				localSpeakers = FetchLastSpeaker(left);
				speakers = &localSpeakers;
			}

			std::string result;

			//話者変更、かつ使用済みの話者であれば改行+半改行を行う
			if (!disableSpeakerChangeLineBreak && speakers->lastSpeakerIndex != selector.speakerIndex && speakers->usedSpeaker.contains(selector.speakerIndex)) {
				result = left + std::string(selector.selectorTag) + TalkBuilder::GetScopeChangeLineBreak(interpreter) + right.substr(selector.selectorTag.size());
			}
			else {
				result = left + right;
			}

			//キャッシュが設定されてれば後半部を使用して更新する
			if (speakedCache != nullptr) {
				SpeakedSpeakers rightScopes = FetchLastSpeaker(right);
				MergeSpeakedScopes(*speakedCache, rightScopes);
			}

			return result;
		}
	}

	void TalkStringCombiner::MergeSpeakedScopes(SpeakedSpeakers& left, const SpeakedSpeakers& right) {
		if (right.lastSpeakerIndex != TALK_SPEAKER_INDEX_DEFAULT) {
			left.lastSpeakerIndex = right.lastSpeakerIndex;
		}

		for (uint32_t index : right.usedSpeaker) {
			left.usedSpeaker.insert(index);
		}
	}

	//必要があればトークの先頭部分をTalkBuilderから設定
	void ScriptInterpreterStack::AppendTalkHeadIfNeed(ScriptInterpreter& interpreter) {
		if (talkBody.empty()) {
			//最初のトーク内容を設定
			const std::string talkHead = TalkBuilder::GetScriptHead(interpreter);
			if (!talkHead.empty()) {
				talkBody = talkHead;
			}
		}
	}

	//トーク内容を追加
	void ScriptInterpreterStack::AppendTalkBody(const std::string& str, ScriptInterpreter& interpreter) {

		//先頭トークを追加
		AppendTalkHeadIfNeed(interpreter);

		//話者指定があるかをチェック
		auto firstSpeaker = TalkStringCombiner::FetchFirstSpeaker(str);

		if (speakedCache.lastSpeakerIndex == TalkStringCombiner::TALK_SPEAKER_INDEX_DEFAULT) {

			//最初の発話で話者指定が入ってなければ自動的に付与
			if (firstSpeaker.speakerIndex == TalkStringCombiner::TALK_SPEAKER_INDEX_DEFAULT) {
				talkBody.append("\\0");
				speakedCache.lastSpeakerIndex = 0;
				speakedCache.usedSpeaker.insert(0);
			}
		}

		//改行要求
		if (isTalkLineEnd) {
			talkBody.append(TalkBuilder::GetAutoLineBreak(interpreter));
			isTalkLineEnd = false;
		}

		//単純に文字列を結合
		talkBody = TalkStringCombiner::CombineTalk(talkBody, str, interpreter, &speakedCache, false);
	}

	void ScriptInterpreterStack::AppendJumpedTalkBody(const std::string& str, ScriptInterpreter& interpreter) {
		if (talkBody.empty()) {
			//発話してない場合（ジャンプ以外に反応しなかった場合）そのまま内容を受け入れる。先頭の話者指定などで冗長にしない
			talkBody = str;
		}
		else {
			//そうでない場合は通常の結合を行う
			AppendTalkBody(str, interpreter);
		}
	}

	//話者を指定
	void ScriptInterpreterStack::SetTalkSpeakerIndex(int32_t speakerIndex, ScriptInterpreter& interpreter) {

		if (speakedCache.lastSpeakerIndex != speakerIndex) {

			//話者が変更になる場合、変更先で改行が必要化の判断になるので改行を切る
			isTalkLineEnd = false;

			std::string selectorTag;

			switch (speakerIndex) {
			case 0:
				selectorTag = "\\0";
				break;
			case 1:
				selectorTag = "\\1";
				break;
			default:
				selectorTag = "\\p[" + std::to_string(speakerIndex) + "]";
				break;
			}

			//スコープ変更タグを結合して、必要に任せて改行
			talkBody = TalkStringCombiner::CombineTalk(talkBody, selectorTag, interpreter, &speakedCache);
		}
	}
	

}
