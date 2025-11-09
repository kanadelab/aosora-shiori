#pragma once

#include "Tokens/Tokens.h"
#include "AST/ASTNodes.h"

namespace sakura {

	class ASTParseContext;
	class ScriptToken;
	class ScriptVariableDef;


	//パースリザルト
	struct ASTParseResult {
		bool success;
		ConstASTNodeRef root;
		std::map<std::string, ScriptClassRef> classMap;

		//デバッグツールに提供するブレーク可能な行インデックスの一覧
		std::vector<uint32_t> breakableLines;

		//パースエラー: 発生時、その場で解析を打ち切るのでエラーは必ず１個だけ
		std::shared_ptr<ScriptParseError> error;
	};

	//ソースコードパーサ
	class ASTParser {
	private:
		enum class BlockType {
			Function,
			Talk
		};

		//変数定義
		struct ScriptVariableDef {
			std::string name;
			ASTNodeRef initializer;
			SourceCodeRange range;
		};

	public:
		static const OperatorInformation* TokenToOperator(const ScriptToken& token, bool isRequireOperand);
		static ASTNodeRef ParseASTStatement(ASTParseContext& parseContext, bool isRootBlock);
		static ASTNodeRef ParseASTExpression(ASTParseContext& parseContext, uint32_t sequenceEndFlags);

		static void ParseASTExpressionList(ASTParseContext& parseContext, std::vector<ConstASTNodeRef>& result, uint32_t sequenceEndFlags);
		static void ParseASTArgumentList(ASTParseContext& parseContext, std::vector<std::string>& result, uint32_t sequenceEndFlags);
		static bool IsSequenceEnd(ASTParseContext& parseContext, uint32_t sequenceEndFlags);

		static ASTNodeRef ParseASTSourceRoot(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTCodeBlock(ASTParseContext& parseContext, bool isBlacketEnd = false);
		static ASTNodeRef ParseASTTalkBlock(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTLocalVariable(ASTParseContext& parseContext);
		static bool ParseVariableDef(ASTParseContext& parseContext, std::vector<ScriptVariableDef>& defs);
		static ASTNodeRef ParseASTWhile(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTForeach(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTFor(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTIf(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTReturn(ASTParseContext& parseContext, bool isLambdaSyntaxSugar = false);
		static ASTNodeRef ParseASTBreak(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTContinue(ASTParseContext& parseContext);

		static ASTNodeRef ParseASTNumberLiteral(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTString(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTStringLiteral(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTContextValue(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTSymbol(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTArrayInitializer(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTObjectInitializer(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTFunctionInitializer(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTFunctionStatement(ASTParseContext& parseContext, bool isRootBlockStatement);
		static ASTNodeRef ParseASTTalkInitializer(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTTalkStatement(ASTParseContext& parseContext, bool isRootBlockStatement);

		static ScriptFunctionDef ParseFunctionDef(ASTParseContext& parseContext, BlockType blockType);

		static ASTNodeRef ParseASTSet(ASTParseContext& parseContext, const ASTNodeRef& target, const ASTNodeRef& value);
		static ASTNodeRef ParseASTNew(ASTParseContext& parseContext, const ScriptToken& operatorToken, const ASTNodeRef& target);

		//クラスとuse宣言はASTを返さない
		static ASTNodeRef ParseASTClass(ASTParseContext& parseContext);
		static void ParseASTUse(ASTParseContext& parseContext);

		//unitは文脈で宣言かオブジェクトかが変わる
		static void ParseASTUnitDef(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTUnit(ASTParseContext& parseContext);

		//例外系
		static ASTNodeRef ParseASTTry(ASTParseContext& parseContext);
		static ASTNodeRef ParseASTThrow(ASTParseContext& parseContext);

	public:
		//ソースコードとしてパース
		static std::shared_ptr<const ASTParseResult> Parse(const std::shared_ptr<const TokensParseResult>& tokens);

		//式としてパース
		static std::shared_ptr<const ASTParseResult> ParseExpression(const std::shared_ptr<const TokensParseResult>& tokens, const ScriptSourceMetadataRef* importSourceMeta, bool disableParseAssert = false);
	};

}
