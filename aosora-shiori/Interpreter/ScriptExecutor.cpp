#include <filesystem>

#include "AST/AST.h"
#include "Interpreter/ScriptVariable.h"
#include "Interpreter/ScriptExecutor.h"
#include "CoreLibrary/CoreLibrary.h"
#include "CommonLibrary/CommonClasses.h"
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

		//無限ループ対策の実行ステップ制限
		if (executeContext.GetInterpreter().GetLimitScriptSteps() > 0) {
			if (executeContext.GetInterpreter().IncrementScriptStep() > executeContext.GetInterpreter().GetLimitScriptSteps()) {
				executeContext.ThrowRuntimeError<RuntimeError>(node, TextSystem::Find("AOSORA_BUILTIN_ERROR_001"), executeContext)->SetCanCatch(false);
				return ScriptValue::Null;
			}
		}

		//ノードタイプごとに実行
		switch (node.GetType()) {
		case ASTNodeType::CodeBlock:
			return ExecuteCodeBlock(static_cast<const ASTNodeCodeBlock&>(node), executeContext);
		case ASTNodeType::FormatString:
			return ExecuteFormatString(static_cast<const ASTNodeFormatString&>(node), executeContext);
		case ASTNodeType::StringLiteral:
			return ExecuteStringLiteral(static_cast<const ASTNodeStringLiteral&>(node), executeContext);
		case ASTNodeType::NumberLiteral:
			return ExecuteNumberLiteral(static_cast<const ASTNodeNumberLiteral&>(node), executeContext);
		case ASTNodeType::BooleanLiteral:
			return ExecuteBooleanLiteral(static_cast<const ASTNodeBooleanLiteral&>(node), executeContext);
		case ASTNodeType::ResolveSymbol:
			return ExecuteResolveSymbol(static_cast<const ASTNodeResolveSymbol&>(node), executeContext);
		case ASTNodeType::AssignSymbol:
			return ExecuteAssignSymbol(static_cast<const ASTNodeAssignSymbol&>(node), executeContext);
		case ASTNodeType::ArrayInitializer:
			return ExecuteArrayInitializer(static_cast<const ASTNodeArrayInitializer&>(node), executeContext);
		case ASTNodeType::ObjectInitializer:
			return ExecuteObjectInitializer(static_cast<const ASTNodeObjectInitializer&>(node), executeContext);
		case ASTNodeType::FunctionStatement:
			return ExecuteFunctionStatement(static_cast<const ASTNodeFunctionStatement&>(node), executeContext);
		case ASTNodeType::FunctionInitializer:
			return ExecuteFunctionInitializer(static_cast<const ASTNodeFunctionInitializer&>(node), executeContext);
		case ASTNodeType::LocalVariableDeclaration:
			return ExecuteLocalVariableDeclaration(static_cast<const ASTNodeLocalVariableDeclaration&>(node), executeContext);
		case ASTNodeType::LocalVariableDeclarationList:
			return ExecuteLocalVariableDeclarationList(static_cast<const ASTNodeLocalVariableDeclarationList&>(node), executeContext);
		case ASTNodeType::For:
			return ExecuteFor(static_cast<const ASTNodeFor&>(node), executeContext);
		case ASTNodeType::While:
			return ExecuteWhile(static_cast<const ASTNodeWhile&>(node), executeContext);
		case ASTNodeType::If:
			return ExecuteIf(static_cast<const ASTNodeIf&>(node), executeContext);
		case ASTNodeType::Break:
			return ExecuteBreak(static_cast<const ASTNodeBreak&>(node), executeContext);
		case ASTNodeType::Continue:
			return ExecuteContinue(static_cast<const ASTNodeContinue&>(node), executeContext);
		case ASTNodeType::Return:
			return ExecuteReturn(static_cast<const ASTNodeReturn&>(node), executeContext);
		case ASTNodeType::Operator2:
			return ExecuteOperator2(static_cast<const ASTNodeEvalOperator2&>(node), executeContext);
		case ASTNodeType::Operator1:
			return ExecuteOperator1(static_cast<const ASTNodeEvalOperator1&>(node), executeContext);
		case ASTNodeType::NewClassInstance:
			return ExecuteNewClassInstance(static_cast<const ASTNodeNewClassInstance&>(node), executeContext);
		case ASTNodeType::FunctionCall:
			return ExecuteFunctionCall(static_cast<const ASTNodeFunctionCall&>(node), executeContext);
		case ASTNodeType::ResolveMember:
			return ExecuteResolveMember(static_cast<const ASTNodeResolveMember&>(node), executeContext);
		case ASTNodeType::AssignMember:
			return ExecuteAssignMember(static_cast<const ASTNodeAssignMember&>(node), executeContext);
		case ASTNodeType::Try:
			return ExecuteTry(static_cast<const ASTNodeTry&>(node), executeContext);
		case ASTNodeType::Throw:
			return ExecuteThrow(static_cast<const ASTNodeThrow&>(node), executeContext);
		case ASTNodeType::TalkJump:
			return ExecuteTalkJump(static_cast<const ASTNodeTalkJump&>(node), executeContext);
		case ASTNodeType::TalkSpeak:
			return ExecuteTalkSpeak(static_cast<const ASTNodeTalkSpeak&>(node), executeContext);
		case ASTNodeType::TalkSetSpeaker:
			return ExecuteTalkSetSpeaker(static_cast<const ASTNodeTalkSetSpeaker&>(node), executeContext);
		default:
			assert(false);
			return ScriptValue::Null;
		}
	}

	//コードブロック
	ScriptValueRef ScriptExecutor::ExecuteCodeBlock(const ASTNodeCodeBlock& node, ScriptExecuteContext& executeContext) {

		//NOTE: C++のように任意でスコープを組むことはできないこともあり（オブジェクトイニシャライザと競合）呼び出し側でブロックスコープを形成させる
		//たとえばforやcatchのようなローカル変数を用意するものがあるので呼び出し側でブロックスコープをつくり、その中で変数をいれてから呼び出す

		for (const ConstASTNodeRef& stmt : node.GetStatements()) {

			//ここがステートメントになるはずなので、ブレークポイントヒットとしては適切なはず⋯
			Debugger::NotifyASTExecute(*stmt.get(), executeContext);

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
				auto str = v->ToStringWithFunctionCall(executeContext);
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

	//シンボル解決
	ScriptValueRef ScriptExecutor::ExecuteResolveSymbol(const ASTNodeResolveSymbol& node, ScriptExecuteContext& executeContext) {
		return executeContext.GetSymbol(node.GetSymbolName());
	}

	//シンボル設定
	ScriptValueRef ScriptExecutor::ExecuteAssignSymbol(const ASTNodeAssignSymbol& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef v = ExecuteInternal(*node.GetValueNode(), executeContext);

		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		executeContext.SetSymbol(node.GetSymbolName(), v);
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
					if (loopScopeContext.GetStack().IsLeave()) {
						return ScriptValue::Null;
					}

					//break対応
					if (loopScopeContext.GetStack().IsBreak()) {
						return ScriptValue::Null;
					}

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
					if (loopScopeContext.GetStack().IsLeave()) {
						return ScriptValue::Null;
					}
					//break対応
					if (loopScopeContext.GetStack().IsBreak()) {
						return ScriptValue::Null;
					}
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

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//javascriptにならって左がtrueなら左をかえし、そうでないなら右をかえす
		if (left->ToBoolean()) {
			return left;
		}
		else {
			return right;
		}
	}

	//論理and
	ScriptValueRef ScriptExecutor::ExecuteOpLogicalAnd(const ASTNodeEvalOperator2& node, ScriptExecuteContext& executeContext) {
		ScriptValueRef left = ExecuteInternal(*node.GetOperandLeft(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		ScriptValueRef right = ExecuteInternal(*node.GetOperandRight(), executeContext);
		if (executeContext.RequireLeave()) {
			return ScriptValue::Null;
		}

		//javascriptにならって左がtureなら右をかえす
		if (left->ToBoolean()) {
			return right;
		}
		else {
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

			//グローバルから探す(ローカル指定も可能にできると良い?)
			ScriptValueRef item = executeContext.GetInterpreter().GetGlobalVariable(funcName);
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
				executeContext.GetInterpreter().SetGlobalVariable(funcName, ScriptValue::Make(funcList));
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

		return ScriptValue::Make(executeContext.GetInterpreter().NewClassInstance(node, classData, callArgs, executeContext));
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
			executeContext.ThrowRuntimeError<RuntimeError>(node, TextSystem::Find("AOSORA_BUILTIN_ERROR_002"), executeContext);
			return ScriptValue::Null;
		}

		FunctionResponse response;
		executeContext.GetInterpreter().CallFunction(*function, response, callArgs, executeContext, &node);
		if (response.IsThrew()) {
			executeContext.ThrowError(node, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName(), response.GetThrewError());
			return ScriptValue::Null;
		}
		else {
			return response.GetReturnValue();
		}
	}

	//メンバ取得
	ScriptValueRef ScriptExecutor::ExecuteResolveMember(const ASTNodeResolveMember& node, ScriptExecuteContext& executeContext) {

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
			auto result = r->GetObjectRef()->Get(r->GetObjectRef(), key, executeContext);
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

		//汎用メソッド類
		auto generalResult = PrimitiveMethod::GetGeneralMember(r, key, executeContext);
		if (generalResult != nullptr) {
			return generalResult;
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

			r->GetObjectRef()->Set(r->GetObjectRef(), k->ToString(), v, executeContext);
		}
		return ScriptValue::Null;
	}

	//try
	ScriptValueRef ScriptExecutor::ExecuteTry(const ASTNodeTry& node, ScriptExecuteContext& executeContext) {

		//まずtryブロックを実行
		auto tryContext = executeContext.CreateChildBlockScopeContext();
		ExecuteInternal(*node.GetTryBlock(), tryContext);

		//throwされているかを確認
		if (executeContext.GetStack().IsThrew()) {
			
			ObjectRef err = executeContext.GetStack().GetThrewError();
			RuntimeError* errObj = executeContext.GetInterpreter().InstanceAs<RuntimeError>(err);
			bool canCatch = true;
			if (errObj != nullptr) {
				canCatch = errObj->CanCatch();
			}

			const ASTNodeBase* catchBlock = nullptr;

			if (canCatch) {
				//catchブロックを選択する
				for (size_t i = 0; i < node.GetCatchBlocks().size(); i++) {
					bool isMatch = false;
					const auto& cb = node.GetCatchBlocks()[i];

					if (cb.catchClasses.size() > 0) {
						//catchブロックに一致するクラスがあるか調べる
						for (const std::string& className : cb.catchClasses) {

							//クラスが合うか
							uint32_t classId = executeContext.GetInterpreter().GetClassId(className);
							if (executeContext.GetInterpreter().InstanceIs(err, classId)) {
								isMatch = true;
								break;
							}
						}
					}
					else {
						//クラス指定のないcatchブロックなので絶対ヒットする
						isMatch = true;
					}

					if (isMatch) {
						catchBlock = node.GetCatchBlocks()[i].catchBlock.get();
						break;
					}
				}

				//エラーを退避
				executeContext.GetStack().PendingLeaveMode();
			}

			//catchブロックが選択されていれば実行する
			if (catchBlock != nullptr) {
				auto catchContext = executeContext.CreateChildBlockScopeContext();
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
			executeContext.ThrowError(node, executeContext.GetBlockScope(), executeContext.GetStack().GetFunctionName(), r->GetObjectRef());
		}
		else {
			executeContext.ThrowRuntimeError<RuntimeError>(node, "throwできるのはErrorオブジェクトだけです。", executeContext);
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
			executeContext.GetInterpreter().CallFunction(*jumpTarget, res, args, executeContext, &node);

			if (res.IsThrew()) {
				executeContext.GetStack().Throw(res.GetThrewError());
				return ScriptValue::Null;
			}
			else {

				//関数の出力に貼り付ける
				auto str = res.GetReturnValue()->ToStringWithFunctionCall(executeContext);
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

	//クラス取得
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
	ToStringFunctionCallResult ScriptInterpreter::Execute(const ConstASTNodeRef& node, bool toStringResult) {
		ScriptInterpreterStack rootStack;
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
	ScriptValueRef ScriptInterpreter::Eval(const std::string& expr) {

		//TODO: とりあえずいけそう⋯というところまで。続きの実装を。

		//その場で解析を行い、実行する		
		auto tokens = TokensParser::Parse(expr, SourceFilePath("eval", "eval"));	//一時的なファイル名として入力しておく
		auto ast = ASTParser::Parse(tokens);

		ScriptInterpreterStack rootStack;
		Reference<BlockScope> rootBlock = CreateNativeObject<BlockScope>(nullptr);
		ScriptExecuteContext executeContext(*this, rootStack, rootBlock);
		ScriptExecutor::ExecuteASTNode(*ast->root, executeContext);

		//結果をかえす(失敗した場合とかはどうする？）
		return nullptr;
	}

	ScriptInterpreter::ScriptInterpreter() :
		scriptSteps(0),
		limitScriptSteps(100 * 10000),	//100万ステップでエラーにしておく
		scriptClassCount(0),
		debugOutputStream(nullptr)
	{
		//組み込みのクラスを登録
		RegisterNativeFunction("print", &ScriptInterpreter::Print);

		//TODO: 無名の登録が必要かも、グローバル空間をあまり汚染したくないものもあったり
		ImportClass(NativeClass::Make<ScriptArray>("Array"));
		ImportClass(NativeClass::Make<ScriptObject>("Map"));
		ImportClass(NativeClass::Make<ClassData>("ClassData"));
		ImportClass(NativeClass::Make<Reflection>("Reflection"));
		ImportClass(NativeClass::Make<Delegate>("Delegate"));
		ImportClass(NativeClass::Make<RuntimeError>("Error", &RuntimeError::CreateObject));
		ImportClass(NativeClass::Make<Time>("Time"));
		ImportClass(NativeClass::Make<SaveData>("Save"));
		ImportClass(NativeClass::Make<OverloadedFunctionList>("OverloadedFunctionList"));
		ImportClass(NativeClass::Make<InstancedOverloadFunctionList>("InstancedOverloadFunctionList"));
		ImportClass(NativeClass::Make<TalkTimer>("TalkTimer"));
		ImportClass(NativeClass::Make<Random>("Random"));
		ImportClass(NativeClass::Make<SaoriManager>("Saori"));
		ImportClass(NativeClass::Make<SaoriModule>("SaoriModule"));
		ImportClass(NativeClass::Make<TalkBuilder>("TalkBuilder"));
		ImportClass(NativeClass::Make<TalkBuilderSettings>("TalkBuilderSettings"));
		ImportClass(NativeClass::Make<ScriptDebug>("Debug"));
	}

	ScriptInterpreter::~ScriptInterpreter() {

		//各クラスの終了処理
		for (auto item : classMap) {
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

	void ScriptInterpreter::ImportClass(const std::shared_ptr<const ClassBase>& cls){
		//スクリプトクラスの場合はこのタイミングでID発行
		uint32_t classId = cls->GetTypeId();
		if (cls->IsScriptClass()) {
			classId = ObjectTypeIdGenerator::GenerateScriptClassId(scriptClassCount);
			scriptClassCount++;
		}

		//登録
		auto classData = CreateNativeObject<ClassData>(cls, classId, this);
		classMap[cls->GetName()] = classData;
		classIdMap[classId] = classData;
	}

	//クラスのリレーションシップ構築
	void ScriptInterpreter::CommitClasses() {
		for (auto item : classMap) {
			if (item.second->GetMetadata().HasParentClass()) {
				auto found = classMap.find(item.second->GetMetadata().GetParentClassName());
				assert(found != classMap.end());

				//双方登録
				found->second->AddChildClass(item.second);
				item.second->SetParentClass(found->second);
			}

			//ネイティブクラスのstatic情報初期化
			if (!item.second->GetMetadata().IsScriptClass()) {
				static_cast<const NativeClass&>(item.second->GetMetadata()).GetStaticInitFunc()(*this);
			}
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

	//クラスインスタンス生成
	ObjectRef ScriptInterpreter::NewClassInstance(const ASTNodeBase& callingNode, const ScriptValueRef& classData, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& context) {

		ClassData* c = context.GetInterpreter().InstanceAs<ClassData>(classData);
		if (c != nullptr) {
			//まずクラスを取得
			return NewClassInstance(callingNode, c, args, context, nullptr);
		}
		else {
			return nullptr;
		}
	}

	//クラスインスタンス生成
	ObjectRef ScriptInterpreter::NewClassInstance(const ASTNodeBase& callingNode, const Reference<ClassData>& classData, const std::vector<ScriptValueRef>& args, ScriptExecuteContext& context, Reference<ScriptObject> scriptObjInstance) {

		//スクリプトかどうかで分岐してしまう
		if (classData->GetMetadata().IsScriptClass()) {
			//スクリプトクラスの場合は継承元をもてる
			//ネイティブはC++側で継承関係をもてばいいからいらない？

			const ScriptClass& scriptClassData = static_cast<const ScriptClass&>(classData->GetMetadata());

			Reference<ClassData> parentClass = classData->GetParentClass();
			ScriptFunctionRef initFunc = scriptClassData.GetInitFunc();

			//コンストラクタ用のスタックフレームとブロックスコープを作成して引数を登録
			ScriptInterpreterStack initStack = context.GetStack().CreateChildStackFrame(&callingNode, context.GetBlockScope(), "init");
			Reference<BlockScope> initScope = context.GetInterpreter().CreateNativeObject<BlockScope>(context.GetBlockScope());

			ScriptExecuteContext funcContext(*this, initStack, initScope);

			//初期化関数があれば引数を作成
			//TODO: なくても評価が必要かも
			if (initFunc != nullptr) {
				for (size_t i = 0; i < initFunc->GetArguments().size(); i++) {

					ScriptValueRef a = ScriptValue::Null;
					if (i < args.size()) {
						//引数を設定
						a = args[i];
					}
					initScope->SetLocalVariable(initFunc->GetArguments()[i], a);
				}
			}

			Reference<ScriptObject> createdObject = scriptObjInstance;
			if (createdObject == nullptr) {
				//呼び出し元からオブジェクトを渡されていたらそれを使う、なければ新規
				createdObject = CreateObject();
			}

			if (parentClass != nullptr) {

				//親クラスがある場合、親クラスの初期化に投げる引数を設定
				std::vector<ScriptValueRef> parentArgs;

				for (size_t i = 0; i < scriptClassData.GetParentClassInitArgumentCount(); i++) {
					ScriptValueRef v = ScriptExecutor::ExecuteASTNode(*scriptClassData.GetParentClassInitArgument(i), funcContext);
					parentArgs.push_back(v);
				}

				//親クラスを初期化、その際スクリプトオブジェクトを渡してそこに書き込んでもらう
				ObjectRef r = NewClassInstance(callingNode, parentClass, parentArgs, funcContext, createdObject);

				//戻り値と同じインスタンスを示しているはず
				assert(r.Get() == createdObject.Get());

				//TODO: NewClassInstanceが例外を出した場合に初期化を中断して離脱する
			}

			//クラス情報を割り当て
			createdObject->SetClassInfo(classData);

			//ここからコンストラクタ

			//まずメンバ初期化子
			for (size_t i = 0; i < scriptClassData.GetMemberCount(); i++) {
				//コンストラクタの空間使っていいかがちょっと気になるけどそのままいく
				ScriptClassMemberRef m = scriptClassData.GetMember(i);
				ASTNodeRef init = m->GetInitializer();
				ScriptValueRef a = ScriptValue::Null;
				if (init != nullptr) {
					a = ScriptExecutor::ExecuteASTNode(*init, funcContext);
				}
				createdObject->RawSet(m->GetName(), a);
			}

			//初期化子まで実行するとthisアクセスが可能になるのでスタックフレームにthisを設定する
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
					context.ThrowError(callingNode, context.GetBlockScope(), "init", response.GetThrewError());
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
				context.ThrowRuntimeError<RuntimeError>(callingNode, "このクラスはインスタンス化できません。", context);
				return nullptr;
			}
		}
	}

	bool ScriptInterpreter::InstanceIs(const ObjectRef& obj, uint32_t classId) {
		return classIdMap[obj->GetInstanceTypeId()]->InstanceIs(classId);
	}

	void ScriptInterpreter::CollectObjects() {
		//ルート空間の情報を収集してGCを起動する
		std::vector<CollectableBase*> rootCollectables;

		for (auto kv : systemRegistry) {
			if (kv.second->IsObject()) {
				rootCollectables.push_back(kv.second->GetObjectRef().Get());
			}
		}

		for (auto kv : globalVariables) {
			if (kv.second->IsObject()) {
				rootCollectables.push_back(kv.second->GetObjectRef().Get());
			}
		}

		for (auto kv : classMap) {
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
	ScriptValueRef ScriptExecuteContext::GetSymbol(const std::string& name) {

		//優先順位にしたがってシンボルを検索する
		ScriptValueRef result = nullptr;

		//this
		ScriptValueRef thisValue = blockScope->GetThisValue();
		if (thisValue != nullptr) {
			if (thisValue->IsObject()) {
				ObjectRef thisObj = thisValue->GetObjectRef();
				result = thisObj->Get(thisObj, name, *this);
				if (result != nullptr) {
					return result;
				}
			}
		}

		//ローカル
		result = blockScope->GetLocalVariable(name);
		if (result != nullptr) {
			return result;
		}

		//クラス
		result = interpreter.GetClass(name);
		if (result != nullptr) {
			return result;
		}

		//グローバルよりシステムレジストリが先
		result = interpreter.GetSystemRegistryValue(name);
		if (result != nullptr) {
			return result;
		}

		//グローバル
		result = interpreter.GetGlobalVariable(name);
		if (result != nullptr) {
			return result;
		}

		//最終的に何も見つからなかったらnullが帰る
		return ScriptValue::Null;
	}

	//シンボルの書き込み
	void ScriptExecuteContext::SetSymbol(const std::string& name, const ScriptValueRef& value) {
		//ローカルにあれば書き込み
		if (blockScope->SetLocalVariable(name, value)) {
			return;
		}

		if (interpreter.ContainsSystemRegistry(name)) {
			//システムレジストリは書き込み不可
			assert(false);
			return;
		}

		//グローバル
		interpreter.SetGlobalVariable(name, value);
	}

	std::vector<CallStackInfo> ScriptExecuteContext::MakeStackTrace(const ASTNodeBase& currentAstNode, const Reference<BlockScope>& callingBlockScope, const std::string & currentFuncName) {
		//現在実行中のスタックフレームは引数から位置を取得
		std::vector<CallStackInfo> stackInfo;
		{
			CallStackInfo stackFrame;
			stackFrame.hasSourceRange = true;
			stackFrame.sourceRange = currentAstNode.GetSourceRange();
			stackFrame.funcName = currentFuncName;
			stackFrame.blockScope = callingBlockScope;
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
				stackFrame.blockScope = st->GetCallingBlockScope();
			}
			else {
				stackFrame.hasSourceRange = false;
			}
			stackFrame.funcName = st->GetFunctionName();

			stackInfo.push_back(stackFrame);
			st = st->GetParentStackFrame();
		}

		return stackInfo;
	}

	void ScriptExecuteContext::ThrowError(const ASTNodeBase& throwAstNode, const Reference<BlockScope>& callingBlockScope, const std::string& funcName, const ObjectRef& err) {

		//エラーオブジェクトしかスローできないのでチェック
		RuntimeError* e = interpreter.InstanceAs<RuntimeError>(err);
		if (e == nullptr) {
			ThrowError(throwAstNode, callingBlockScope, funcName, interpreter.CreateNativeObject<RuntimeError>("throwできるのはErrorオブジェクトのみです。"));
			return;
		}

		//コールスタック情報が未設定の場合にのみ設定する
		if (!e->HasCallstackInfo()) {
			e->SetCallstackInfo(MakeStackTrace(throwAstNode, callingBlockScope, funcName));
		}
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
				talkBody = TalkStringCombiner::CombineTalk(talkBody, talkHead, interpreter, &speakedCache, false);
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
