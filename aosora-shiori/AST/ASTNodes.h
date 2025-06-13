#pragma once
#include "AST/ASTNodeBase.h"
#include <list>

namespace sakura {

	//解析エラーで脱出するためのダミー
	class ASTError : public ASTNodeBase {
	public:

		virtual const char* DebugName() const override { return "Error"; };
	};

	//一連のコードブロックを示す
	class ASTNodeCodeBlock : public ASTNodeBase {
	private:
		//ブロック内ステートメントの一覧
		std::list<ConstASTNodeRef> statements;

		//トークブロックかどうか
		bool isTalkBlock;

	public:
		ASTNodeCodeBlock():
			isTalkBlock(false)
		{}

		//ASTステートメントの追加
		void AddStatement(const ASTNodeRef& statement) {
			statements.push_back(statement);
		}

		//ASTステートメントの取得
		const std::list<ConstASTNodeRef>& GetStatements() const {
			return statements;
		}

		//トークブロックかどうか
		void SetTalkBlock(bool isTalk) {
			isTalkBlock = true;
		}

		bool IsTalkBlock() const {
			return isTalkBlock;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::CodeBlock; }

		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			for (const ConstASTNodeRef& r : statements) {
				nodes.push_back(r);
			}
		}

		virtual const char* DebugName() const override { return "CodeBlock"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			for (auto& item : statements) {
				item->DebugDump(indent + 1);
			}
		}
	};

	//フォーマット文字列
	class ASTNodeFormatString : public ASTNodeBase {
	public:
		struct Item {
			ConstASTNodeRef node;
			bool isFormatExpression;
		};

	private:
		std::vector<Item> items;

	public:
		ASTNodeFormatString(const std::vector<Item>& formatItems) :
			items(formatItems) {
		}

		const std::vector<Item>& GetItems() const {
			return items;
		}

		//中身がステートメントブロックのみのかを確認
		bool IsFuncStatementBlockOnly() const;

		virtual ASTNodeType GetType() const override { return ASTNodeType::FormatString; }

		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			for (const auto& r : items) {
				nodes.push_back(r.node);
			}
		}

		virtual const char* DebugName() const override { return "FormatString"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			for (auto& item : items) {
				item.node->DebugDump(indent + 1);
			}
		}
	};

	//文字列リテラル
	class ASTNodeStringLiteral : public ASTNodeBase {
	private:
		std::string value;

	public:
		ASTNodeStringLiteral(const std::string& stringValue) :
			value(stringValue)
		{}

		const std::string& GetValue() const{
			return value;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::StringLiteral; }
		virtual const char* DebugName() const override { return "StringLiteral"; }
		virtual std::string DebugToString() const override { return value; }
	};

	//数値リテラル
	class ASTNodeNumberLiteral : public ASTNodeBase {
	private:
		number value;

	public:
		ASTNodeNumberLiteral(number numberValue) :
			value(numberValue) {}

		number GetValue() const {
			return value;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::NumberLiteral; }
		virtual const char* DebugName() const override { return "NumberLiteral"; }
		virtual std::string DebugToString() const override { return std::to_string(value); }
	};

	//boolリテラル
	class ASTNodeBooleanLiteral : public ASTNodeBase {
	private:
		bool value;

	public:
		ASTNodeBooleanLiteral(bool booleanValue): 
			value(booleanValue)
		{}

		bool GetValue() const {
			return value;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::BooleanLiteral; }
		virtual const char* DebugName() const override { return "BooleanLiteral"; }
		virtual std::string DebugToString() const override { return std::to_string(value); }
	};

	//シンボルの解決
	class ASTNodeResolveSymbol : public ASTNodeBase {
	private:
		std::string name;

	public:
		ASTNodeResolveSymbol(const std::string& symbolName) :
			name(symbolName) {}

		const std::string& GetSymbolName() const { return name; }

		virtual bool CanConvertToSetter() const { return true; }
		virtual ASTNodeRef ConvertToSetter(const ASTNodeRef& valueNode) const;

		virtual ASTNodeType GetType() const override { return ASTNodeType::ResolveSymbol; }
		virtual const char* DebugName() const override { return "ResolveSymbol"; }
		virtual std::string DebugToString() const override { return name; }
	};

	//シンボルの割当
	class ASTNodeAssignSymbol : public ASTNodeBase {
	private:
		std::string name;
		ConstASTNodeRef value;

	public:
		ASTNodeAssignSymbol(const std::string& symbolName, const ConstASTNodeRef& valueNode) :
			name(symbolName),
			value(valueNode)
		{}

		const std::string& GetSymbolName() const { return name; }
		const ConstASTNodeRef& GetValueNode() const { return value; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::AssignSymbol; }

		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(value);
		}

		virtual const char* DebugName() const override { return "AssignSymbol"; }
		virtual std::string DebugToString() const override { return name; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			value->DebugDump(indent + 1);
		}
	};

	//配列イニシャライザ
	class ASTNodeArrayInitializer : public ASTNodeBase {
	private:
		std::vector<ConstASTNodeRef> values;

	public:

		ASTNodeArrayInitializer(const std::vector<ConstASTNodeRef>& items) :
			values(items)
		{}

		const std::vector<ConstASTNodeRef>& GetValues() const {
			return values;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::ArrayInitializer; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			for (const ConstASTNodeRef& r : values) {
				nodes.push_back(r);
			}
		}

		virtual const char* DebugName() const override { return "ArrayInitializer"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			for (auto& item : values) {
				item->DebugDump(indent + 1);
			}
		}
	};

	//オブジェクトイニシャライザ
	class ASTNodeObjectInitializer : public ASTNodeBase {
	public:
		struct Item {
			std::string key;
			ConstASTNodeRef value;
		};

	private:
		//初期化順を尊重するために単方向リストにする
		std::vector<Item> items;

	public:

		ASTNodeObjectInitializer(const std::vector<Item>& items) :
			items(items)
		{}

		const std::vector<Item> GetItems() const { return items; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::ObjectInitializer; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			for (const auto& r : items) {
				nodes.push_back(r.value);
			}
		}

		virtual const char* DebugName() const override { return "ObjectInitializer"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			for (auto& item : items) {
				item.value->DebugDump(indent + 1);
			}
		}

	};

	//関数ステートメント
	//ステートメント形式の場合はオーバーロードリストへの追加になる
	class ASTNodeFunctionStatement : public ASTNodeBase {
	private:
		std::vector<std::string> names;
		ConstScriptFunctionRef func;
		ConstASTNodeRef condition;

	public:
		ASTNodeFunctionStatement(const std::vector<std::string>& funcNames, const ConstScriptFunctionRef& function, const ConstASTNodeRef& conditionNode) :
			names(funcNames),
			func(function),
			condition(conditionNode)
		{}

		const std::vector<std::string>& GetNames() const { return names; }
		const ConstScriptFunctionRef& GetFunction() const { return func; }
		const ConstASTNodeRef& GetConditionNode() const { return condition; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::FunctionStatement; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(func->GetFunctionBody());
		}

		virtual const char* DebugName() const override { return "FunctionStatement"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			func->DebugDump(indent + 1);
		}
	};

	//関数イニシャライザ
	//関数イニシャライザ形式の場合、関数そのものを参照する
	class ASTNodeFunctionInitializer : public ASTNodeBase {
	private:
		ConstScriptFunctionRef func;

	public:
		ASTNodeFunctionInitializer(const ConstScriptFunctionRef& function) :
			func(function)
		{}

		const ConstScriptFunctionRef& GetFunction() const { return func; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::FunctionInitializer; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(func->GetFunctionBody());
		}

		virtual const char* DebugName() const override { return "FunctionInitializer"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			func->DebugDump(indent + 1);
		}
	};

	//ローカル変数宣言
	class ASTNodeLocalVariableDeclaration : public ASTNodeBase {
	private:
		std::string name;
		ConstASTNodeRef initialValue;

	public:
		ASTNodeLocalVariableDeclaration(const std::string& localVariableName, const ConstASTNodeRef& value) :
			name(localVariableName),
			initialValue(value) {
		}

		const std::string& GetName() const { return name; }
		const ConstASTNodeRef& GetValueNode() const { return initialValue; }
		
		virtual ASTNodeType GetType() const override { return ASTNodeType::LocalVariableDeclaration; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			if (initialValue != nullptr) {
				nodes.push_back(initialValue);
			}
		}

		virtual const char* DebugName() const override { return "LocalVariableDeclaration"; }
		virtual std::string DebugToString() const override { return name; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			if (initialValue != nullptr) {
				initialValue->DebugDump(indent + 1);
			}
		}
	};

	//ローカル変数宣言リスト
	//local でも複数つなげるため。もし１個でもリスト。
	class ASTNodeLocalVariableDeclarationList : public ASTNodeBase {
	private:
		std::list<std::shared_ptr<const ASTNodeLocalVariableDeclaration>> variables;

	public:
		ASTNodeLocalVariableDeclarationList() {}

		//変数の追加
		void AddVariable(const std::shared_ptr<const ASTNodeLocalVariableDeclaration>& item) {
			variables.push_back(item);
		}

		const std::list<std::shared_ptr<const ASTNodeLocalVariableDeclaration>>& GetVariables() const {
			return variables;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::LocalVariableDeclarationList; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			for (const ConstASTNodeRef& r : variables) {
				nodes.push_back(r);
			}
		}

		virtual const char* DebugName() const override { return "LocalVariableDeclarationList"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			for (const auto& v : variables) {
				v->DebugDump(indent + 1);
			}
		}
	};

	//for文
	class ASTNodeFor : public ASTNodeBase {
	private:
		ConstASTNodeRef initExpression;
		ConstASTNodeRef ifExpression;
		ConstASTNodeRef incrementExpression;
		ConstASTNodeRef loopStatement;

	public:
		ASTNodeFor(const ConstASTNodeRef& init, const ConstASTNodeRef& cond, const ConstASTNodeRef& inc, const ConstASTNodeRef& stmt):
			initExpression(init),
			ifExpression(cond),
			incrementExpression(inc),
			loopStatement(stmt)
		{}

		const ConstASTNodeRef& GetInitExpression() const { return initExpression; }
		const ConstASTNodeRef& GetIfExpression() const { return ifExpression; }
		const ConstASTNodeRef& GetIncrementExpression() const { return incrementExpression; }
		const ConstASTNodeRef& GetLoopStatement() const { return loopStatement; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::For; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			if (initExpression != nullptr) {
				nodes.push_back(initExpression);
			}

			if (ifExpression != nullptr) {
				nodes.push_back(ifExpression);
			}

			if (incrementExpression != nullptr) {
				nodes.push_back(incrementExpression);
			}
			
			nodes.push_back(loopStatement);
		}

		virtual const char* DebugName() const override { return "For"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			if (initExpression != nullptr) {
				initExpression->DebugDump(indent + 1);
			}

			if (ifExpression != nullptr) {
				ifExpression->DebugDump(indent + 1);
			}

			if (incrementExpression != nullptr) {
				incrementExpression->DebugDump(indent + 1);
			}
			loopStatement->DebugDump(indent + 1);
		}
	};

	//while文
	class ASTNodeWhile : public ASTNodeBase {
	private:
		ConstASTNodeRef ifExpression;
		ConstASTNodeRef trueStatement;

	public:
		ASTNodeWhile(const ConstASTNodeRef& expr, const ConstASTNodeRef& block):
			ifExpression(expr),
			trueStatement(block)
		{}

		const ConstASTNodeRef& GetIfExpression() const { return ifExpression; }
		const ConstASTNodeRef& GetTrueStatement() const { return trueStatement; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::While; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(ifExpression);
			nodes.push_back(trueStatement);
		}

		virtual const char* DebugName() const override { return "While"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			ifExpression->DebugDump(indent + 1);
			trueStatement->DebugDump(indent + 1);
		}
	};

	//if文
	class ASTNodeIf : public ASTNodeBase {
	private:
		ConstASTNodeRef ifExpression;
		ConstASTNodeRef trueStatement;
		ConstASTNodeRef falseStatement;

	public:

		ASTNodeIf(const ConstASTNodeRef& expr, const ConstASTNodeRef & t) :
			ifExpression(expr),
			trueStatement(t),
			falseStatement(nullptr)
		{}

		ASTNodeIf(const ConstASTNodeRef& expr, const ConstASTNodeRef& t, const ConstASTNodeRef& f) :
			ifExpression(expr),
			trueStatement(t),
			falseStatement(f)
		{}

		const ConstASTNodeRef& GetIfExpression() const { return ifExpression; }
		const ConstASTNodeRef& GetTrueStatement() const { return trueStatement; }
		const ConstASTNodeRef& GetFalseStatement() const { return falseStatement; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::If; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(ifExpression);
			nodes.push_back(trueStatement);
			if (falseStatement != nullptr) {
				nodes.push_back(falseStatement);
			}
		}

		virtual const char* DebugName() const override { return "if"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			ifExpression->DebugDump(indent + 1);
			trueStatement->DebugDump(indent + 1);
			if (falseStatement != nullptr) {
				falseStatement->DebugDump(indent + 1);
			}
		}
	};

	//break
	class ASTNodeBreak : public ASTNodeBase {
	public:
		ASTNodeBreak(){}
		virtual ASTNodeType GetType() const override { return ASTNodeType::Break; }
		virtual const char* DebugName() const override { return "Break"; }
	};

	//break
	class ASTNodeContinue : public ASTNodeBase {
	public:
		ASTNodeContinue() {}
		virtual ASTNodeType GetType() const override { return ASTNodeType::Continue; }
		virtual const char* DebugName() const override { return "Continue"; }
	};

	//return文
	class ASTNodeReturn : public ASTNodeBase {
	private:
		ConstASTNodeRef returnValueNode;

	public:
		ASTNodeReturn(const ConstASTNodeRef& returnValue) :
			returnValueNode(returnValue)
		{}

		ASTNodeReturn() :
			returnValueNode(nullptr)
		{}

		const ConstASTNodeRef& GetValueNode() const {
			return returnValueNode;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::Return; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(returnValueNode);
		}

		virtual const char* DebugName() const override { return "Return"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			if (returnValueNode != nullptr) {
				returnValueNode->DebugDump(indent + 1);
			}
		}
	};

	//２項演算子評価
	class ASTNodeEvalOperator2 : public ASTNodeBase {
	private:
		const OperatorInformation& operatorInfo;
		ConstASTNodeRef operandLeft;
		ConstASTNodeRef operandRight;

	public:
		ASTNodeEvalOperator2(const OperatorInformation& info, const ConstASTNodeRef& left, const ConstASTNodeRef& right) :
			operatorInfo(info),
			operandLeft(left),
			operandRight(right)
		{
			SetSourceRange(left->GetSourceRange(), right->GetSourceRange());
		}

		const OperatorInformation& GetOperator() const { return operatorInfo; }
		const ConstASTNodeRef& GetOperandLeft() const { return operandLeft; }
		const ConstASTNodeRef& GetOperandRight() const { return operandRight; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::Operator2; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(operandLeft);
			nodes.push_back(operandRight);
		}

		virtual const char* DebugName() const override { return "Operator2"; }
		virtual std::string DebugToString() const override { return operatorInfo.preview; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			operandLeft->DebugDump(indent + 1);
			operandRight->DebugDump(indent + 1);
		}
	};

	//１項演算子評価
	class ASTNodeEvalOperator1 : public ASTNodeBase {
	private:
		const OperatorInformation& operatorInfo;
		ConstASTNodeRef operand;

	public:
		ASTNodeEvalOperator1(const OperatorInformation& info, const ASTNodeRef& value) :
			operatorInfo(info),
			operand(value)
		{}

		const OperatorInformation& GetOperator() const { return operatorInfo; }
		const ConstASTNodeRef& GetOperand() const { return operand; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::Operator1; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(operand);
		}

		virtual const char* DebugName() const override { return "Operator1"; }
		virtual std::string DebugToString() const override { return operatorInfo.preview; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			operand->DebugDump(indent + 1);
		}
	};

	//new 演算子
	class ASTNodeNewClassInstance : public ASTNodeBase {
	private:
		ConstASTNodeRef classObj;
		std::vector<ConstASTNodeRef> args;

	public:
		ASTNodeNewClassInstance(const ConstASTNodeRef& classObject, const std::vector<ConstASTNodeRef>& arguments) :
			classObj(classObject),
			args(arguments) {
		}

		const ConstASTNodeRef& GetClassDataNode() const { return classObj; }
		const std::vector<ConstASTNodeRef>& GetArgumentNodes() const { return args; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::NewClassInstance; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(classObj);
			for (const ConstASTNodeRef& r : args) {
				nodes.push_back(r);
			}
		}

		virtual const char* DebugName() const override { return "New"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			classObj->DebugDump(indent + 1);
			for (auto a : args) {
				a->DebugDump(indent + 1);
			}
		}
	};

	//関数呼び出し
	class ASTNodeFunctionCall : public ASTNodeBase {
	private:
		ConstASTNodeRef func;
		std::vector<ConstASTNodeRef> args;

	public:
		ASTNodeFunctionCall(const ConstASTNodeRef& function, const std::vector<ConstASTNodeRef>& arguments) :
			func(function),
			args(arguments) {
		}

		const ConstASTNodeRef& GetFunctionNode() const { return func; }
		const std::vector<ConstASTNodeRef>& GetArgumentNodes() const { return args; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::FunctionCall; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(func);
			for (const ConstASTNodeRef& r : args) {
				nodes.push_back(r);
			}
		}

		virtual const char* DebugName() const override { return "FunctionCall"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			func->DebugDump(indent + 1);
			for (auto a : args) {
				a->DebugDump(indent + 1);
			}
		}
	};

	//メンバ展開
	class ASTNodeResolveMember : public ASTNodeBase {
	private:
		ConstASTNodeRef target;
		ConstASTNodeRef key;

	public:
		ASTNodeResolveMember(const ConstASTNodeRef& obj, const ConstASTNodeRef& member) :
			target(obj),
			key(member) {
			SetSourceRange(obj->GetSourceRange(), member->GetSourceRange());
		}

		const ConstASTNodeRef& GetThisNode() const { return target; }
		const ConstASTNodeRef& GetKeyNode() const { return key; }

		virtual bool CanConvertToSetter() const override { return true; }
		virtual ASTNodeRef ConvertToSetter(const ASTNodeRef& valueNode) const override;

		virtual ASTNodeType GetType() const override { return ASTNodeType::ResolveMember; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(target);
			nodes.push_back(key);
		}

		virtual const char* DebugName() const override { return "ResolveMember"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			target->DebugDump(indent + 1);
			key->DebugDump(indent + 1);
		}
	};

	//メンバ設定
	class ASTNodeAssignMember : public ASTNodeBase {
	private:
		ConstASTNodeRef target;
		ConstASTNodeRef value;
		ConstASTNodeRef key;

	public:
		ASTNodeAssignMember(const ConstASTNodeRef& obj, const ConstASTNodeRef& member, const ConstASTNodeRef& valueNode) :
			target(obj),
			value(valueNode),
			key(member)
		{}

		const ConstASTNodeRef& GetValueNode() const { return value; }
		const ConstASTNodeRef& GetThisNode() const { return target; }
		const ConstASTNodeRef& GetKeyNode() const { return key; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::AssignMember; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(target);
			nodes.push_back(key);
			nodes.push_back(value);
		}

		virtual const char* DebugName() const override { return "AssignMember"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			target->DebugDump(indent + 1);
			key->DebugDump(indent + 1);
			value->DebugDump(indent + 1);
		}
	};

	//try
	class ASTNodeTry : public ASTNodeBase {
	public:
		struct CatchItem {
			ConstASTNodeRef catchBlock;
			std::vector<std::string> catchClasses;	//catch対象のクラス
		};

	private:
		ConstASTNodeRef tryBlock;
		std::vector<CatchItem> catchBlocks;
		ConstASTNodeRef finallyBlock;

	public:
		ASTNodeTry(const ConstASTNodeRef& tryNode):
			tryBlock(tryNode),
			finallyBlock(nullptr)
		{}

		void AddCatchBlock(const ConstASTNodeRef& catchNode, const std::vector<std::string>& catchClasses) {
			catchBlocks.push_back({ catchNode, catchClasses });
		}

		void SetFinallyBlock(const ConstASTNodeRef& finallyNode) {
			finallyBlock = finallyNode;
		}

		const ConstASTNodeRef& GetTryBlock() const { return tryBlock; }
		const std::vector<CatchItem>& GetCatchBlocks() const { return catchBlocks; }
		const ConstASTNodeRef& GetFinallyBlock() const { return finallyBlock; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::Try; }
		virtual const char* DebugName() const override { return "Try"; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(tryBlock);
			for (const CatchItem& item : catchBlocks) {
				nodes.push_back(item.catchBlock);
			}
			if (finallyBlock != nullptr) {
				nodes.push_back(finallyBlock);
			}
		}

		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			tryBlock->DebugDump(indent + 1);
			for (auto item : catchBlocks) {
				item.catchBlock->DebugDump(indent + 1);
			}
			if (finallyBlock != nullptr) {
				finallyBlock->DebugDump(indent + 1);
			}
		}
	};

	//throw
	class ASTNodeThrow : public ASTNodeBase {
	private:
		ConstASTNodeRef throwValueNode;

	public:
		ASTNodeThrow(const ConstASTNodeRef& throwValue) :
			throwValueNode(throwValue)
		{}

		ASTNodeThrow() :
			throwValueNode(nullptr)
		{}

		const ConstASTNodeRef& GetValueNode() const {
			return throwValueNode;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::Throw; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			if (throwValueNode != nullptr) {
				nodes.push_back(throwValueNode);
			}
		}

		virtual const char* DebugName() const override { return "Throw"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			if (throwValueNode != nullptr) {
				throwValueNode->DebugDump(indent + 1);
			}
		}
	};

	//話者指定
	class ASTNodeTalkSetSpeaker : public ASTNodeBase {
	public:
		static const int32_t SPEAKER_INDEX_SWITCH = -1;	//話者交替用のインデックス

	private:
		int32_t speakerIndex;

	public:
		ASTNodeTalkSetSpeaker(int32_t speaker):
			speakerIndex(speaker)
		{}

		//話者番号
		const int32_t GetSpeakerIndex() const {
			return speakerIndex;
		}

		virtual ASTNodeType GetType() const override { return ASTNodeType::TalkSetSpeaker; }
		virtual const char* DebugName() const override { return "TalkSepaker"; }
		virtual std::string DebugToString() const override { return std::to_string(speakerIndex); }
	};

	//トークジャンプ
	class ASTNodeTalkJump : public ASTNodeBase {
	private:
		//呼出対象関数
		ConstASTNodeRef func;
		std::vector<ConstASTNodeRef> args;

		//ジャンプ条件
		ConstASTNodeRef condition;

	public:
		ASTNodeTalkJump(const ConstASTNodeRef& jumpTarget, const std::vector<ConstASTNodeRef>& arguments):
			func(jumpTarget),
			args(arguments),
			condition(nullptr)
		{}

		ASTNodeTalkJump(const ConstASTNodeRef& jumpTarget, const std::vector<ConstASTNodeRef>& arguments, const ConstASTNodeRef& jumpCondition):
			func(jumpTarget),
			args(arguments),
			condition(jumpCondition)
		{}

		const ConstASTNodeRef& GetFunctionNode() const { return func; }
		const std::vector<ConstASTNodeRef>& GetArgumentNodes() const { return args; }
		const ConstASTNodeRef& GetConditionNode() const { return condition; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::TalkJump; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(func);
			if (condition != nullptr) {
				nodes.push_back(condition);
			}
			for (const ConstASTNodeRef& r : args) {
				nodes.push_back(r);
			}
		}

		virtual const char* DebugName() const override { return "TalkJump"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			func->DebugDump(indent + 1);
			if (condition != nullptr) {
				condition->DebugDump(indent + 1);
			}
			for (auto a : args) {
				a->DebugDump(indent + 1);
			}
		}
	};

	//トーク発話
	class ASTNodeTalkSpeak : public ASTNodeBase {
	private:
		//発話内容
		ConstASTNodeRef body;

	public:
		ASTNodeTalkSpeak(const ConstASTNodeRef& talkBody):
			body(talkBody)
		{}

		const ConstASTNodeRef& GetBody() const { return body; }

		virtual ASTNodeType GetType() const override { return ASTNodeType::TalkSpeak; }
		virtual void GetChildren(std::vector<ConstASTNodeRef>& nodes) const override {
			nodes.push_back(body);
		}

		virtual const char* DebugName() const override { return "TalkSpeak"; }
		virtual void DebugDump(int32_t indent) const override {
			ASTNodeBase::DebugDump(indent);
			body->DebugDump(indent + 1);
		}
	};


	
	inline bool ASTNodeFormatString::IsFuncStatementBlockOnly() const {
		bool hasStatement = false;
		for (const Item& item : items) {
			if (item.isFormatExpression) {
				if (item.node->GetType() == ASTNodeType::StringLiteral) {
					auto str = std::static_pointer_cast<const ASTNodeStringLiteral>(item.node);
					if (!str->GetValue().empty()) {
						//有効な文字列がありからっぽでなければ有効な文字列と判断
						return false;
					}
				}
				else {
					//それ以外のフォーマット出力も有効
					return false;
				}
			}
			else {
				//１件でもないと、ステートメントブロックを持ってなければ「単なる空文字列」なので、ステートメントブロックのみとは言えない
				hasStatement = true;
			}
		}
		return hasStatement;
	}
}