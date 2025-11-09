#include <set>
#include "Misc/Utility.h"
#include "Tokens/Tokens.h"
#include "AST/ASTNodeBase.h"
#include "AST/ASTNodes.h"
#include "AST/ASTParser.h"
#include "Misc/Message.h"

//パースエラー発生時にassertで止める
#define AOSORA_ENABLE_PARSE_ERROR_ASSERT

namespace sakura {

	const std::string ERROR_AST_999 = "A999";
	const std::string ERROR_AST_000 = "A000";
	const std::string ERROR_AST_001 = "A001";
	const std::string ERROR_AST_002 = "A002";
	const std::string ERROR_AST_003 = "A003";
	const std::string ERROR_AST_004 = "A004";
	const std::string ERROR_AST_005 = "A005";
	const std::string ERROR_AST_006 = "A006";
	const std::string ERROR_AST_007 = "A007";
	const std::string ERROR_AST_008 = "A008";
	const std::string ERROR_AST_009 = "A009";
	const std::string ERROR_AST_010 = "A010";
	const std::string ERROR_AST_011 = "A011";
	const std::string ERROR_AST_012 = "A012";
	const std::string ERROR_AST_013 = "A013";
	const std::string ERROR_AST_014 = "A014";
	const std::string ERROR_AST_015 = "A015";
	const std::string ERROR_AST_016 = "A016";
	const std::string ERROR_AST_017 = "A017";
	const std::string ERROR_AST_018 = "A018";
	const std::string ERROR_AST_019 = "A019";
	const std::string ERROR_AST_020 = "A020";
	const std::string ERROR_AST_021 = "A021";
	const std::string ERROR_AST_022 = "A022";
	const std::string ERROR_AST_023 = "A023";
	const std::string ERROR_AST_024 = "A024";
	const std::string ERROR_AST_025 = "A025";
	const std::string ERROR_AST_026 = "A026";
	const std::string ERROR_AST_027 = "A027";
	const std::string ERROR_AST_028 = "A028";
	const std::string ERROR_AST_029 = "A029";
	const std::string ERROR_AST_030 = "A030";
	const std::string ERROR_AST_031 = "A031";
	const std::string ERROR_AST_032 = "A032";
	const std::string ERROR_AST_033 = "A033";
	const std::string ERROR_AST_034 = "A034";
	const std::string ERROR_AST_035 = "A035";
	const std::string ERROR_AST_036 = "A036";
	const std::string ERROR_AST_037 = "A037";
	const std::string ERROR_AST_038 = "A038";
	const std::string ERROR_AST_039 = "A039";
	const std::string ERROR_AST_040 = "A040";
	const std::string ERROR_AST_041 = "A041";
	const std::string ERROR_AST_042 = "A042";
	const std::string ERROR_AST_043 = "A043";
	const std::string ERROR_AST_044 = "A044";
	const std::string ERROR_AST_045 = "A045";
	const std::string ERROR_AST_046 = "A046";
	const std::string ERROR_AST_047 = "A047";
	const std::string ERROR_AST_049 = "A049";	//048はクラスリレーションエラーなのでここにはない
	const std::string ERROR_AST_050 = "A050";
	const std::string ERROR_AST_051 = "A051";
	const std::string ERROR_AST_052 = "A052";

	//２値演算
	const OperatorInformation OPERATOR_ADD = { OperatorType::Add, 6, 2, true, "+" };
	const OperatorInformation OPERATOR_SUB = { OperatorType::Sub, 6, 2, true, "-" };
	const OperatorInformation OPERATOR_MUL = { OperatorType::Mul, 5, 2, true, "*" };
	const OperatorInformation OPERATOR_DIV = { OperatorType::Div, 5, 2, true, "/" };
	const OperatorInformation OPERATOR_MOD = { OperatorType::Mod, 5, 2, true, "%" };
	const OperatorInformation OPERATOR_NULL_COALESCING = { OperatorType::NullCoalescing, 15, 2, true, "??" };

	//単項プラスマイナス
	const OperatorInformation OPERATOR_MINUS = { OperatorType::Minus, 3, 1, false, "-" };
	const OperatorInformation OPERATOR_PLUS = { OperatorType::Plus, 3, 1, false, "+" };

	//関係演算子
	const OperatorInformation OPERATOR_GT = { OperatorType::Gt, 8, 2, true, ">" };
	const OperatorInformation OPERATOR_LT = { OperatorType::Lt, 8, 2, true, "<" };
	const OperatorInformation OPERATOR_GE = { OperatorType::Ge, 8, 2, true, ">=" };
	const OperatorInformation OPERATOR_LE = { OperatorType::Le, 8, 2, true, "<=" };
	const OperatorInformation OPERATOR_EQ = { OperatorType::Eq, 9, 2, true, "==" };
	const OperatorInformation OPERATOR_NE = { OperatorType::Ne, 9, 2, true, "!=" };

	//論理演算子
	const OperatorInformation OPERATOR_LOGICAL_NOT = { OperatorType::LogicalNot, 3, 1, false, "!" };
	const OperatorInformation OPERATOR_LOGICAL_AND = { OperatorType::LogicalAnd, 13, 2, true, "&&" };
	const OperatorInformation OPERATOR_LOGICAL_OR = { OperatorType::LogicalOr, 14, 2, true, "||" };

	//呼び出し
	const OperatorInformation OPERATOR_CALL = { OperatorType::Call, 2, 2, true, "(call)" };

	//カッコ開(式解析用)
	const OperatorInformation OPERATOR_BRACKET = { OperatorType::Bracket, 0, 0, true, "(" };

	//メンバ
	const OperatorInformation OPERATOR_MEMBER = { OperatorType::Member, 2, 2, true, "." };

	//インデックス
	const OperatorInformation OPERATOR_INDEX = { OperatorType::Index, 2, 2, true, "[index]" };

	//new
	const OperatorInformation OPERATOR_NEW = { OperatorType::New, 3, 1, false, "new" };

	//代入
	const OperatorInformation OPERATOR_ASSIGN = { OperatorType::Assign, 16, 2, false, "="};
	const OperatorInformation OPERAOTR_ASSIGN_ADD = { OperatorType::AssignAdd, 16, 2, false, "+=" };
	const OperatorInformation OPERATOR_ASSIGN_SUB = { OperatorType::AssignSub, 16, 2, false, "-=" };
	const OperatorInformation OPERATOR_ASSIGN_MUL = { OperatorType::AssignMul, 16, 2, false, "*=" };
	const OperatorInformation OPERATOR_ASSIGN_DIV = { OperatorType::AssignDiv, 16, 2, false, "/=" };
	const OperatorInformation OPERATOR_ASSIGN_MOD = { OperatorType::AssignMod, 16, 2, false, "%=" };
	const OperatorInformation OPERATOR_ASSIGN_NULL_COALESCING = { OperatorType::AssignNullCoalescing, 16, 2, false, "??=" };

	//式やステートメントの終端として認めるもののフラグ
	const uint32_t SEQUENCE_END_FLAG_COMMA = 1u << 0u;
	const uint32_t SEQUENCE_END_FLAG_BLOCK_BLACKET = 1u << 1u;
	const uint32_t SEQUENCE_END_FLAG_ARRAY_BLACKET = 1u << 3u;
	const uint32_t SEQUENCE_END_FLAG_BLACKET = 1u << 4u;
	const uint32_t SEQUENCE_END_FLAG_SEMICOLON = 1u << 5u;
	const uint32_t SEQUENCE_END_FLAG_COLON = 1u << 6u;
	const uint32_t SEQUENCE_END_FALG_TALK_NEWLINE = 1u << 7u;
	const uint32_t SEQUENCE_END_FLAG_VERTICAL_BAR = 1u << 8u;
	const uint32_t SEQUENCE_END_FLAG_BLOCK_BLACKET_BEGIN = 1u << 9u;

	//ASTパース
	class ASTParseContext {
	public:
		//クラス解析中スコープ指定用
		class ClassScope {
		private:
			ASTParseContext& context;
		public:
			ClassScope(ASTParseContext& parseContext, const ScriptClassRef& classRef) :context(parseContext) {
				assert(context.scriptClass == nullptr);
				assert(classRef != nullptr);
				context.scriptClass = classRef;
			}
			~ClassScope() {
				assert(context.scriptClass != nullptr);
				context.scriptClass = nullptr;
			}
		};

	private:
		ASTParseResult& result;
		const std::list<ScriptToken>& tokens;
		std::list<ScriptToken>::const_iterator current;
		ScriptSourceMetadataRef sourceMetaData;
		ScriptClassRef scriptClass;	//現在解析中のクラス

		//アサート無効(vscode watchなどむけ）
		bool disableParseAsseert;

		//エラーが出ているかどうか、エラーがあればその場で解析を打ち切るので１つだけしか持たない
		bool hasError;
		ScriptParseErrorData errorData;
		SourceCodeRange errorSourceRange;

	public:
		ASTParseContext(const std::list<ScriptToken>& tokenList, ASTParseResult& parseResult) :
			result(parseResult),
			tokens(tokenList),
			disableParseAsseert(false),
			hasError(false)
		{
			//最初のアイテムをとる
			current = tokens.cbegin();

			//ユニット情報
			sourceMetaData.reset(new ScriptSourceMetadata());
		}

		void DisableASTAssert() {
			disableParseAsseert = true;
		}

		const ScriptToken& GetCurrent() {
			if (IsEOF()) {
				return TOKEN_EOF;
			}
			return *current;
		}

		//１つ前を取得
		const ScriptToken& GetPrev() {
			if (current == tokens.begin()) {
				//EOFじゃないけど一旦無効を返したいので
				return TOKEN_EOF;
			}
			auto it = current;
			it--;
			return *it;
		}

		//クラスのセット
		void AddClass(const ScriptClassRef& classData) {
			result.classMap[classData->GetName()] = classData;
		}

		//解析中のクラスを取得
		const ScriptClassRef& GetParsingClass() const {
			return scriptClass;
		}

		ASTNodeRef Error(const std::string& errorCode, const SourceCodeRange& sourceRange) {

#if defined(AOSORA_DEBUG)
			if (DEBUG_ENABLE_ASSERT_PARSE_ERROR) {
				if(!disableParseAsseert){
					//デバッグのためエラーだったら即止め
					assert(false);
				}
			}
#endif

			//最初に発生したエラーだけを記録(エラーでパースが崩れたのにあわせてまたエラーになるのを回避するため)
			if (!hasError) {
				errorData.errorCode = errorCode;
				errorData.message = TextSystem::Find(std::string("ERROR_MESSAGE") + errorCode);
				errorData.hint = TextSystem::Find(std::string("ERROR_HINT") + errorCode);
				errorSourceRange = sourceRange;
				hasError = true;
			}

			//エラー情報を返してそのまま脱出させる
			return ASTNodeRef(new ASTError());
		}

		//エラーのセット
		ASTNodeRef Error(const std::string& errorCode, const ScriptToken& token) {
			return Error(errorCode, token.sourceRange);
		}

		//次へ
		void FetchNext() {
			++current;
		}

		//戻る
		void FetchPrev() {
			--current;
		}

		bool IsEOF() const {
			return current == tokens.cend();
		}

		//解析を打ち切るべきかどうか
		//EOFのほか、解析エラーで脱出させるような目的も
		bool IsEnd() const {
			return IsEOF() || HasError();
		}

		bool HasError() const {
			return hasError;
		}

		std::string GetErrorMessage() const {
			return errorData.message;
		}

		const ScriptParseErrorData& GetErrorData() const {
			return errorData;
		}

		SourceCodeRange GetErrorSourceRange() const {
			return errorSourceRange;
		}

		const ScriptSourceMetadataRef& GetSourceMetadata() const {
			return sourceMetaData;
		}

		//メタデータを設定。デバッガなどの目的でほかのソースの一部としてエイリアス情報を共有するための用途
		void ImportScriptSourceMetadata(ScriptSourceMetadataRef sourceMeta) {
			sourceMetaData = sourceMeta;
		}

		const ScriptUnitRef& GetScriptUnit() const {
			return sourceMetaData->GetScriptUnit();
		}

		void CommitSourceMetadata() {
			sourceMetaData->CommitMetadata();
		}
	};

	//ソースコードとしてパース
	std::shared_ptr<const ASTParseResult> ASTParser::Parse(const std::shared_ptr<const TokensParseResult>& tokens) {
		std::shared_ptr<ASTParseResult> parseResult(new ASTParseResult());
		ASTParseContext parseContext(tokens->tokens, *parseResult);
		auto codeBlock = ParseASTSourceRoot(parseContext);
		parseResult->success = !parseContext.HasError();

#if 0
		printf("---AST---\n");
		codeBlock->DebugDump(0);

#endif
		if (parseContext.HasError()) {
#if 0
			printf("Error: %s\n", parseContext.GetErrorMessage().c_str());
			if (parseContext.GetErrorToken() != nullptr) {
				printf("Position: %s\n", parseContext.GetErrorToken()->sourceRange.ToString().c_str());
			}
#endif
			parseResult->error.reset(new ScriptParseError(parseContext.GetErrorData(), parseContext.GetErrorSourceRange()));
		}

		parseResult->root = codeBlock;

		//パースした結果からブレーク可能な行を作成
		std::vector<ConstASTNodeRef> allNodes;
		std::vector<ConstASTNodeRef> breakableNodes;
		std::set<uint32_t> breakableLinesSet;
		parseResult->root->GetChildrenRecursive(allNodes);

		//ブレークできるのはコードブロックの子として認識しているノードなので、それらをまとめて取得する
		for (size_t i = 0; i < allNodes.size(); i++) {
			if (allNodes[i]->GetType() == ASTNodeType::CodeBlock) {
				allNodes[i]->GetChildren(breakableNodes);
			}
		}

		//行数にまとめる
		for (size_t i = 0; i < breakableNodes.size(); i++) {
			breakableLinesSet.insert(breakableNodes[i]->GetSourceRange().GetBeginLineIndex());
		}

		//ソートしたうえで配列として持つ
		parseResult->breakableLines.reserve(breakableLinesSet.size());
		for (auto it = breakableLinesSet.begin(); it != breakableLinesSet.end(); it++) {
			parseResult->breakableLines.push_back(*it);
		}
		std::sort(parseResult->breakableLines.begin(), parseResult->breakableLines.end());

		parseContext.CommitSourceMetadata();
		return parseResult;
	}

	//式としてパース
	std::shared_ptr<const ASTParseResult> ASTParser::ParseExpression(const std::shared_ptr<const TokensParseResult>& tokens, const ScriptSourceMetadataRef* importSourceMeta, bool disableParseAssert) {
		std::shared_ptr<ASTParseResult> parseResult(new ASTParseResult());
		ASTParseContext parseContext(tokens->tokens, *parseResult);
		if (disableParseAssert) {
			parseContext.DisableASTAssert();
		}

		if (importSourceMeta != nullptr) {
			parseContext.ImportScriptSourceMetadata(*importSourceMeta);
		}

		auto expression = ParseASTExpression(parseContext, 0);
		parseResult->success = !parseContext.HasError();

		if (parseContext.HasError()) {
			parseResult->error.reset(new ScriptParseError(parseContext.GetErrorData(), parseContext.GetErrorSourceRange()));
		}
		parseResult->root = expression;
		return parseResult;
	}


	//ソースコードルートのパース
	ASTNodeRef ASTParser::ParseASTSourceRoot(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		std::shared_ptr<ASTNodeCodeBlock> result(new ASTNodeCodeBlock(parseContext.GetSourceMetadata()));

		while (!parseContext.IsEnd()) {

			//ASTノードとして扱わないクラスはルート空間のみということにしておく
			if (parseContext.GetCurrent().type == ScriptTokenType::Class) {
				result->AddStatement(ParseASTClass(parseContext));
				continue;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Unit) {
				parseContext.FetchNext();
				if (parseContext.GetCurrent().type == ScriptTokenType::Symbol) {
					parseContext.FetchPrev();
					ParseASTUnit(parseContext);
					continue;
				}
				
				//式文脈なので一旦戻してParseASTStatementにつなげる
				parseContext.FetchPrev();
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Use) {
				ParseASTUse(parseContext);
				continue;
			}

			//所属しているステートメントをブロック終了までパースする
			auto node = ParseASTStatement(parseContext, true);
			result->AddStatement(node);
		}

		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//コードブロックのパース
	ASTNodeRef ASTParser::ParseASTCodeBlock(ASTParseContext& parseContext, bool isBlacketEnd) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		std::shared_ptr<ASTNodeCodeBlock> result(new ASTNodeCodeBlock(parseContext.GetSourceMetadata()));

		while (!parseContext.IsEnd()) {

			//閉じ括弧があれば終了
			if (isBlacketEnd && parseContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
				parseContext.FetchNext();
				result->SetSourceRange(beginToken, parseContext.GetPrev());
				return result;
			}

			//所属しているステートメントをブロック終了までパースする
			auto node = ParseASTStatement(parseContext, false);
			result->AddStatement(node);
		}

		//閉じ括弧終了なのになければエラー
		if (isBlacketEnd) {
			return parseContext.Error(ERROR_AST_001, beginToken);
		}

		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//トークブロックのパース
	ASTNodeRef ASTParser::ParseASTTalkBlock(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		std::shared_ptr<ASTNodeCodeBlock> result(new ASTNodeCodeBlock(parseContext.GetSourceMetadata()));

		while (!parseContext.IsEnd()){

			//トークブロック特有のトークンが来た場合はトーク処理
			if (parseContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
				//閉じ括弧があれば終了
				parseContext.FetchNext();
				break;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::TalkLineEnd) {
				//読み飛ばし
				parseContext.FetchNext();
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::SpeakerIndex) {

				//話者指定
				ASTNodeRef si = ASTNodeRef(new ASTNodeTalkSetSpeaker(std::stoi(parseContext.GetCurrent().body), parseContext.GetSourceMetadata()));
				si->SetSourceRange(parseContext.GetCurrent());
				result->AddStatement(si);
				parseContext.FetchNext();

				//次に話者スイッチ命令が入ってるはずなのでこれを読み飛ばす(TokenParserが必ず追加する)
				assert(parseContext.GetCurrent().type == ScriptTokenType::SpeakerSwitch);
				if (parseContext.GetCurrent().type != ScriptTokenType::SpeakerSwitch) {
					return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
				}
				parseContext.FetchNext();
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::SpeakerSwitch) {
				//直接speakerが来たら話者スイッチ
				ASTNodeRef sw = ASTNodeRef(new ASTNodeTalkSetSpeaker(ASTNodeTalkSetSpeaker::SPEAKER_INDEX_SWITCH, parseContext.GetSourceMetadata()));
				sw->SetSourceRange(parseContext.GetCurrent());
				result->AddStatement(sw);
				parseContext.FetchNext();
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::TalkJump) {
				const ScriptToken& jumpBeginToken = parseContext.GetCurrent();

				//関数呼び出し式もしくは関数を示す式が来る
				parseContext.FetchNext();
				ConstASTNodeRef callNode = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_COLON | SEQUENCE_END_FALG_TALK_NEWLINE);
				ConstASTNodeRef condition(nullptr);
				std::vector<ConstASTNodeRef> args;

				//関数呼び出し式だった場合は条件部と本体を取り出す
				if (callNode->GetType() == ASTNodeType::FunctionCall) {
					std::shared_ptr<const ASTNodeFunctionCall> fc = std::static_pointer_cast<const ASTNodeFunctionCall>(callNode);
					args = fc->GetArgumentNodes();
					callNode = fc->GetFunctionNode();
				}

				if (parseContext.GetPrev().type == ScriptTokenType::Colon) {
					//コロンが指定されている場合は条件式があるのでとりこむ
					condition = ParseASTExpression(parseContext, SEQUENCE_END_FALG_TALK_NEWLINE);
				}

				ASTNodeRef talkJump = ASTNodeRef(new ASTNodeTalkJump(callNode, args, condition, parseContext.GetSourceMetadata()));
				talkJump->SetSourceRange(jumpBeginToken, parseContext.GetPrev());
				result->AddStatement(talkJump);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::SpeakBegin) {
				//発話
				const ScriptToken& talkBegin = parseContext.GetCurrent();
				ASTNodeRef talkBody = ParseASTString(parseContext);
				ASTNodeRef talk = ASTNodeRef(new ASTNodeTalkSpeak(talkBody, parseContext.GetSourceMetadata()));

				talk->SetSourceRange(talkBegin, parseContext.GetPrev());
				result->AddStatement(talk);
			}
			else
			{
				//それ以外の場合は通常の関数内ステートメントとして処理
				ASTNodeRef r = ParseASTStatement(parseContext, false);
				result->AddStatement(r);
			}
		}

		if (parseContext.HasError()) {
			return ASTNodeRef(new ASTError());
		}

		result->SetSourceRange(beginToken, parseContext.GetPrev());
		result->SetTalkBlock(true);
		return result;
	}

	//トークンとオペレータを交換
	const OperatorInformation* ASTParser::TokenToOperator(const ScriptToken& token, bool isRequireOperand) {

		if (isRequireOperand) {
			//前置演算子
			switch (token.type) {
			case ScriptTokenType::Plus:
				return &OPERATOR_PLUS;
			case ScriptTokenType::Minus:
				return &OPERATOR_MINUS;
			case ScriptTokenType::New:
				return &OPERATOR_NEW;
			case ScriptTokenType::LogicalNot:
				return &OPERATOR_LOGICAL_NOT;
			}
		}
		else {
			//後置演算子
			switch (token.type) {
			case ScriptTokenType::Plus:
				return &OPERATOR_ADD;
			case ScriptTokenType::Minus:
				return &OPERATOR_SUB;
			case ScriptTokenType::Slash:
				return &OPERATOR_DIV;
			case ScriptTokenType::Percent:
				return &OPERATOR_MOD;
			case ScriptTokenType::NullCoalescing:
				return &OPERATOR_NULL_COALESCING;
			case ScriptTokenType::Asterisk:
				return &OPERATOR_MUL;
			case ScriptTokenType::RelationalEq:
				return &OPERATOR_EQ;
			case ScriptTokenType::RelationalNe:
				return &OPERATOR_NE;
			case ScriptTokenType::RelationalGt:
				return &OPERATOR_GT;
			case ScriptTokenType::RelationalLt:
				return &OPERATOR_LT;
			case ScriptTokenType::RelationalGe:
				return &OPERATOR_GE;
			case ScriptTokenType::RelationalLe:
				return &OPERATOR_LE;
			case ScriptTokenType::LogicalAnd:
				return &OPERATOR_LOGICAL_AND;
			case ScriptTokenType::LogicalOr:
				return &OPERATOR_LOGICAL_OR;
			case ScriptTokenType::Equal:
				return &OPERATOR_ASSIGN;
			case ScriptTokenType::AssignAdd:
				return &OPERAOTR_ASSIGN_ADD;
			case ScriptTokenType::AssignSub:
				return &OPERATOR_ASSIGN_SUB;
			case ScriptTokenType::AssignMul:
				return &OPERATOR_ASSIGN_MUL;
			case ScriptTokenType::AssignDiv:
				return &OPERATOR_ASSIGN_DIV;
			case ScriptTokenType::AssignMod:
				return &OPERATOR_ASSIGN_MOD;
			case ScriptTokenType::AssignNullCoalescing:
				return &OPERATOR_ASSIGN_NULL_COALESCING;
			}
		}
		
		return nullptr;
	}

	//ASTステートメントのパースへの振り分け
	ASTNodeRef ASTParser::ParseASTStatement(ASTParseContext& parseContext, bool isRootBlock) {
		switch (parseContext.GetCurrent().type) {
		case ScriptTokenType::Local:
			return ParseASTLocalVariable(parseContext);
		case ScriptTokenType::Function:
			return ParseASTFunctionStatement(parseContext, isRootBlock);
		case ScriptTokenType::Talk:
			return ParseASTTalkStatement(parseContext, isRootBlock);
		case ScriptTokenType::Foreach:
			return ParseASTForeach(parseContext);
		case ScriptTokenType::For:
			return ParseASTFor(parseContext);
		case ScriptTokenType::While:
			return ParseASTWhile(parseContext);
		case ScriptTokenType::If:
			return ParseASTIf(parseContext);
		case ScriptTokenType::Break:
			return ParseASTBreak(parseContext);
		case ScriptTokenType::Continue:
			return ParseASTContinue(parseContext);
		case ScriptTokenType::Return:
			return ParseASTReturn(parseContext);
		case ScriptTokenType::Try:
			return ParseASTTry(parseContext);
		case ScriptTokenType::Throw:
			return ParseASTThrow(parseContext);
		}

		//ステートメントでなければその行は式として処理する
		//たとえば代入式や関数呼び出しなど
		return ParseASTExpression(parseContext, SEQUENCE_END_FLAG_SEMICOLON);
	}

	//式の解析用スタック
	class ExpressionParseStack {
	private:
	
		class StackItem {
		public:
			ASTNodeRef operandNode;
			ScriptToken operatorToken;
			const OperatorInformation* operatorInfo;

			StackItem(const ASTNodeRef& node) :
				operandNode(node),
				operatorToken(),
				operatorInfo(nullptr)
			{}

			StackItem(const ScriptToken& token, const OperatorInformation* info) :
				operandNode(nullptr),
				operatorToken(token),
				operatorInfo(info)
			{}
		};

	private:
		//解析用スタック
		std::vector<StackItem> expressionStack;

		//開カッコの数
		int32_t blacketCount;

	public:

		ExpressionParseStack():
			blacketCount(0)
		{}

		//代入系のオペレータかどうか
		static bool IsAssignOperator(OperatorType type) {
			switch (type) {
			case OperatorType::Assign:
			case OperatorType::AssignAdd:
			case OperatorType::AssignSub:
			case OperatorType::AssignMul:
			case OperatorType::AssignDiv:
			case OperatorType::AssignMod:
			case OperatorType::AssignNullCoalescing:
				return true;
			}
			return false;
		}

		//スタックトップから１個オペレータを解決する
		void ReduceOne(ASTParseContext& parseContext) {
			const OperatorInformation* operatorInfo = expressionStack[expressionStack.size() - 2].operatorInfo;
			const ScriptToken& operatorToken = expressionStack[expressionStack.size() - 2].operatorToken;

			if (IsAssignOperator(operatorInfo->type)) {

				//割当の場合、ゲット系のノードをセット系に変換する
				auto operandRight = expressionStack[expressionStack.size() - 1].operandNode;
				auto operandLeft = expressionStack[expressionStack.size() - 3].operandNode;
				expressionStack.pop_back();
				expressionStack.pop_back();
				expressionStack.pop_back();

				//加算代入等のオペレータの場合は右側を右と左の計算結果にする
				switch (operatorInfo->type)
				{
				case OperatorType::AssignAdd:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_ADD, operandLeft, operandRight, parseContext.GetSourceMetadata()));
					break;
				case OperatorType::AssignSub:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_SUB, operandLeft, operandRight, parseContext.GetSourceMetadata()));
					break;
				case OperatorType::AssignMul:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_MUL, operandLeft, operandRight, parseContext.GetSourceMetadata()));
					break;
				case OperatorType::AssignDiv:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_DIV, operandLeft, operandRight, parseContext.GetSourceMetadata()));
					break;
				case OperatorType::AssignMod:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_MOD, operandLeft, operandRight, parseContext.GetSourceMetadata()));
					break;
				case OperatorType::AssignNullCoalescing:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_NULL_COALESCING, operandLeft, operandRight, parseContext.GetSourceMetadata()));
					break;
				}

				expressionStack.push_back(StackItem(ASTParser::ParseASTSet(parseContext, operandLeft, operandRight)));

			}
			else if (operatorInfo->type == OperatorType::AssignAdd) {
				auto operandRight = expressionStack[expressionStack.size() - 1].operandNode;
				auto operandLeft = expressionStack[expressionStack.size() - 3].operandNode;
				expressionStack.pop_back();
				expressionStack.pop_back();
				expressionStack.pop_back();
				expressionStack.push_back(StackItem(ASTParser::ParseASTSet(parseContext, operandLeft, operandRight)));
			}
			else if (operatorInfo->type == OperatorType::New) {
				//newの場合、関数呼び出しのノードをnewに変換する
				auto operandTarget = expressionStack[expressionStack.size() - 1].operandNode;
				expressionStack.pop_back();
				expressionStack.pop_back();
				expressionStack.push_back(StackItem(ASTParser::ParseASTNew(parseContext, operatorToken, operandTarget)));
			}
			else if (operatorInfo->argCount == 2) {

				if (expressionStack.size() < 3) {
					//２項演算子なのにオペランドが足りない
					parseContext.Error(ERROR_AST_002, expressionStack[expressionStack.size() - 2].operatorToken);
					return;
				}

				//３つをポップして演算ノードをプッシュ
				auto operandRight = expressionStack[expressionStack.size() - 1].operandNode;
				auto operandLeft = expressionStack[expressionStack.size() - 3].operandNode;
				const auto* operatorInfo = expressionStack[expressionStack.size() - 2].operatorInfo;

				expressionStack.pop_back();
				expressionStack.pop_back();
				expressionStack.pop_back();

				expressionStack.push_back(StackItem(ASTNodeRef(new ASTNodeEvalOperator2(*operatorInfo, operandLeft, operandRight, parseContext.GetSourceMetadata()))));
			}
			else if (operatorInfo->argCount == 1) {

				//単項演算子、２つポップして演算ノードをプッシュ
				auto operand = expressionStack[expressionStack.size() - 1].operandNode;
				const auto* operatorInfo = expressionStack[expressionStack.size() - 2].operatorInfo;

				expressionStack.pop_back();
				expressionStack.pop_back();

				expressionStack.push_back(StackItem(ASTNodeRef(new ASTNodeEvalOperator1(*operatorInfo, operand, parseContext.GetSourceMetadata()))));
			}
			else if (operatorInfo->type == OperatorType::Bracket) {
				//カッコの対応関係がおかしい
				parseContext.Error(ERROR_AST_003, expressionStack[expressionStack.size() - 2].operatorToken);
				return;
			}
			else {
				//内部エラー
				assert(false);
				parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
				return;
			}
		}

		//現在までに積まれた式を解決する
		void Reduce(const OperatorInformation& nextOperator, ASTParseContext& parseContext) {

			while (expressionStack.size() > 2) {
				const OperatorInformation* operatorInfo = expressionStack[expressionStack.size() - 2].operatorInfo;

				if (operatorInfo == nullptr) {
					//オペレータでないなら処理しない
					//２項演算子のあとで単項演算子を入れようとしている場合など
					break;
				}

				if (operatorInfo->type == OperatorType::Bracket) {
					//開カッコは常に解決しない
					break;
				}

				if (nextOperator.priority < operatorInfo->priority) {
					//次のオペレータのほうが優先なら打ち切り
					break;
				}

				ReduceOne(parseContext);
			}
		}

		//最終的な解決
		ASTNodeRef ReduceAll(ASTParseContext& parseContext) {
			//アイテムが１個になるまで続ける
			while (expressionStack.size() > 1) {
				ReduceOne(parseContext);

				if (parseContext.HasError()) {
					//エラー発生済みなのでカラのエラーオブジェクトを返して解析を打ち切る
					return ASTNodeRef(new ASTError());
				}
			}

			return expressionStack[expressionStack.size() - 1].operandNode;
		}

		//スタックへのオペランド投入
		void PushOperand(const ASTNodeRef& operand) {
			expressionStack.push_back(StackItem(operand));
		}

		//オペランドをポップ
		ASTNodeRef PopOperand(ASTParseContext& parseContext) {
			if (expressionStack.empty()) {
				return parseContext.Error(ERROR_AST_000, parseContext.GetPrev());
			}

			if (expressionStack[expressionStack.size() - 1].operandNode == nullptr) {
				return parseContext.Error(ERROR_AST_000, expressionStack[expressionStack.size() - 1].operatorToken);
			}

			ASTNodeRef r = expressionStack[expressionStack.size() - 1].operandNode;
			expressionStack.pop_back();
			return r;
		}

		//スタックへのオペレータ投入
		void PushOperator(const ScriptToken& token, const OperatorInformation* operatorInfo) {
			expressionStack.push_back(StackItem(token, operatorInfo));
		}

		//スタックへのカッコ投入
		void PushBracket(const ScriptToken& token) {
			//カッコ開を示す演算子をプッシュ
			expressionStack.push_back(StackItem(token, &OPERATOR_BRACKET));
			blacketCount++;
		}

		//スタックからのカッコをポップ
		void PopBracket(ASTParseContext& parseContext, const ScriptToken& closeBracketToken) {

			if (blacketCount <= 0) {
				//１つもカッコがないのに要求された場合
				parseContext.Error(ERROR_AST_003, closeBracketToken);
				return;
			}

			//開カッコにたどり着くまで評価を続ける
			while (expressionStack.size() > 1) {
				const OperatorInformation* operatorInfo = expressionStack[expressionStack.size() - 2].operatorInfo;

				if (operatorInfo->type == OperatorType::Bracket)
				{
					//開カッコまできたら、結果、カッコの順に２つ取り除いて結果を再度プッシュする
					auto resultOperand = expressionStack[expressionStack.size() - 1];
					expressionStack.pop_back();
					expressionStack.pop_back();
					expressionStack.push_back(resultOperand);
					blacketCount--;
					return;
				}

				ReduceOne(parseContext);
			}

			//ここには来ないはず
			assert(false);
			parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
			return;
		}

		//スタックサイズ
		size_t Size() {
			return expressionStack.size();
		}

		//カッコを持っているか
		bool HasBlacket() {
			return blacketCount > 0;
		}

		//次にオペランドをプッシュすべきか
		bool IsRequireOperandNext() {
			return Size() == 0 || expressionStack.crbegin()->operandNode == nullptr;
		}

	};

	//AST式のパース
	ASTNodeRef ASTParser::ParseASTExpression(ASTParseContext& parseContext, uint32_t sequenceEndFlags) {

		ExpressionParseStack parseStack;

		//C++を考えるとスコープ解決演算子 :: が最速だが、今回それはないので
		//次に強いメンバ解決、関数、インデックスを最速扱いとして処理できる

		//アルゴリズムでスタックを確認
		while (!parseContext.IsEnd()) {
			const bool isRequireOperand = parseStack.IsRequireOperandNext();

			//オペレータを要求する場面でオペランドの役目しか無いトークンはエラーになる
			if (!isRequireOperand) {
				bool isLeave = false;

				switch (parseContext.GetCurrent().type) {
				case ScriptTokenType::Number:
				case ScriptTokenType::True:
				case ScriptTokenType::False:
				case ScriptTokenType::Null:
				case ScriptTokenType::Symbol:
				case ScriptTokenType::StringBegin:
				case ScriptTokenType::Function:
				case ScriptTokenType::Talk:
				case ScriptTokenType::Base:
				case ScriptTokenType::This:
				case ScriptTokenType::Unit:
					return parseContext.Error(ERROR_AST_004, parseContext.GetCurrent());
				case ScriptTokenType::BlockBegin:
					//BlockBeginで式を終えられるなら許容
					if ((sequenceEndFlags & SEQUENCE_END_FLAG_BLOCK_BLACKET_BEGIN) == 0) {
						return parseContext.Error(ERROR_AST_004, parseContext.GetCurrent());
					}
					else {
						parseContext.FetchNext();
						isLeave = true;
					}
					break;
				}

				//離脱して良い場合
				if (isLeave) {
					break;
				}
			}

			//それぞれのトークンを処理
			if (parseContext.GetCurrent().type == ScriptTokenType::Number) {
				parseStack.PushOperand(ParseASTNumberLiteral(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::True) {
				ASTNodeRef node = ASTNodeRef(new ASTNodeBooleanLiteral(true, parseContext.GetSourceMetadata()));
				node->SetSourceRange(parseContext.GetCurrent());
				parseContext.FetchNext();
				parseStack.PushOperand(node);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::False) {
				ASTNodeRef node = ASTNodeRef(new ASTNodeBooleanLiteral(false, parseContext.GetSourceMetadata()));
				node->SetSourceRange(parseContext.GetCurrent());
				parseContext.FetchNext();
				parseStack.PushOperand(node);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Null) {
				ASTNodeRef node = ASTNodeRef(new ASTNodeNull(parseContext.GetSourceMetadata()));
				node->SetSourceRange(parseContext.GetCurrent());
				parseContext.FetchNext();
				parseStack.PushOperand(node);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::StringBegin) {
				parseStack.PushOperand(ParseASTString(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::This || parseContext.GetCurrent().type == ScriptTokenType::Base) {
				//コンテキスト値
				parseStack.PushOperand(ParseASTContextValue(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Unit) {
				//解決しようとしている場合はユニットオブジェクトを返す形になる
				auto node = ASTNodeRef(new ASTNodeUnitRoot(parseContext.GetSourceMetadata()));
				node->SetSourceRange(parseContext.GetCurrent().sourceRange);
				parseContext.FetchNext();
				parseStack.PushOperand(node);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Symbol) {
				parseStack.PushOperand(ParseASTSymbol(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::ArrayBegin) {
				if (parseStack.IsRequireOperandNext()) {
					//オペランドなら配列
					parseStack.PushOperand(ParseASTArrayInitializer(parseContext));
				}
				else {
					//オペレータならインデックス
					parseContext.FetchNext();

					//呼び出し元を処理
					parseStack.Reduce(OPERATOR_INDEX, parseContext);

					//２要素を求める
					auto arrayExpression = parseStack.PopOperand(parseContext);
					if (parseContext.HasError()) {
						return arrayExpression;
					}
					auto indexExpression = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_ARRAY_BLACKET);

					//アクセスノードを作成
					parseStack.PushOperand(ASTNodeRef(new ASTNodeResolveMember(arrayExpression, indexExpression, parseContext.GetSourceMetadata())));
				}
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::BlockBegin) {
				parseStack.PushOperand(ParseASTObjectInitializer(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Function) {
				parseStack.PushOperand(ParseASTFunctionInitializer(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::VerticalBar) {
				parseStack.PushOperand(ParseASTFunctionInitializer(parseContext));
			}
			// オペレータが要求されない場合は
			// ||は引数無しの関数式として扱う
			else if (parseContext.GetCurrent().type == ScriptTokenType::LogicalOr && isRequireOperand) {
				parseStack.PushOperand(ParseASTFunctionInitializer(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Talk) {
				parseStack.PushOperand(ParseASTTalkInitializer(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Dot) {

				parseStack.Reduce(OPERATOR_MEMBER, parseContext);
				parseContext.FetchNext();
				if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
					return parseContext.Error(ERROR_AST_005, parseContext.GetCurrent());
				}

				//メンバ解決
				ASTNodeRef keyNode(new ASTNodeStringLiteral(parseContext.GetCurrent().body, parseContext.GetSourceMetadata()));
				keyNode->SetSourceRange(parseContext.GetCurrent());
				parseContext.FetchNext();

				ASTNodeRef target = parseStack.PopOperand(parseContext);
				if (parseContext.HasError()) {
					return target;
				}
				parseStack.PushOperand(ASTNodeRef(new ASTNodeResolveMember(target, keyNode, parseContext.GetSourceMetadata())));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Semicolon) {

				//セミコロンで終了可能なら終了
				if (!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_SEMICOLON)) {
					return parseContext.Error(ERROR_AST_006, parseContext.GetCurrent());
				}
				parseContext.FetchNext();
				break;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Colon) {
				if (!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COLON)) {
					return parseContext.Error(ERROR_AST_007, parseContext.GetCurrent());
				}
				parseContext.FetchNext();
				break;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
				// } で終了させられる場合は終了
				if (!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_BLOCK_BLACKET)) {
					return parseContext.Error(ERROR_AST_008, parseContext.GetCurrent());
				}
				parseContext.FetchNext();
				break;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::ArrayEnd) {
				if (!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_ARRAY_BLACKET)) {
					return parseContext.Error(ERROR_AST_009, parseContext.GetCurrent());
				}
				parseContext.FetchNext();
				break;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Comma) {
				if (!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COMMA)) {
					//タプルみたいなのがあってもいいかもだけど、今は考慮しない
					return parseContext.Error(ERROR_AST_010, parseContext.GetCurrent());
				}
				parseContext.FetchNext();
				break;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::TalkLineEnd) {
				if (!CheckFlags(sequenceEndFlags, SEQUENCE_END_FALG_TALK_NEWLINE)) {
					//発生するのがおそらくおかしい
					assert(false);
					return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
				}
				parseContext.FetchNext();
				break;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Increment && !isRequireOperand) {
				parseContext.FetchNext();

				//後置インクリメント
				auto operand = parseStack.PopOperand(parseContext);
				if (parseContext.HasError()) {
					return operand;
				}

				//1を足すように構成
				auto literalNode = ASTNodeRef(new ASTNodeNumberLiteral(1.0, parseContext.GetSourceMetadata()));
				literalNode->SetSourceRange(parseContext.GetCurrent());

				auto addNode = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_ADD, operand, literalNode, parseContext.GetSourceMetadata()));
				addNode->SetSourceRange(parseContext.GetCurrent());

				parseStack.PushOperand(ParseASTSet(parseContext, operand, addNode));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Decrement) {
				parseContext.FetchNext();

				//後置デクリメント
				auto operand = parseStack.PopOperand(parseContext);
				if (parseContext.HasError()) {
					return operand;
				}

				auto literalNode = ASTNodeRef(new ASTNodeNumberLiteral(1.0, parseContext.GetSourceMetadata()));
				literalNode->SetSourceRange(parseContext.GetCurrent());

				auto addNode = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_SUB, operand, literalNode, parseContext.GetSourceMetadata()));
				addNode->SetSourceRange(parseContext.GetCurrent());

				parseStack.PushOperand(ParseASTSet(parseContext, operand, addNode));
			}
			else {
				
				//オペランドが必要でカッコ開始であれば優先順位制御用のカッコ（オペレータが要求される場面では関数呼び出し）
				if (parseContext.GetCurrent().type == ScriptTokenType::BracketBegin) {
					if (!isRequireOperand) {
						auto rangeBegin = parseContext.GetCurrent();
						parseContext.FetchNext();

						//オペレータが必要な箇所で開カッコであれば関数呼び出し
						//スタックを処理したあと、トップをポップして関数呼び出しに置き換える
						parseStack.Reduce(OPERATOR_CALL, parseContext);

						//リスト解析
						std::vector<ConstASTNodeRef> args;
						ParseASTExpressionList(parseContext, args, SEQUENCE_END_FLAG_BLACKET);

						ASTNodeRef func = parseStack.PopOperand(parseContext);
						if (parseContext.HasError()) {
							return func;
						}
						std::shared_ptr<ASTNodeFunctionCall> call(new ASTNodeFunctionCall(func, args, parseContext.GetSourceMetadata()));
						call->SetSourceRange(rangeBegin, parseContext.GetPrev());
						parseStack.PushOperand(call);
						continue;
					}
					else {
						//オペランドが必要な箇所で開きカッコなら演算順制御のカッコ
						parseStack.PushBracket(parseContext.GetCurrent());
						parseContext.FetchNext();
						continue;
					}
				}
				else if (parseContext.GetCurrent().type == ScriptTokenType::BracketEnd) {
					if (!isRequireOperand) {

						if (CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_BLACKET) && !parseStack.HasBlacket()) {
							//もしカッコで終了する式でスタックにもカッコがない場合は終了
							parseContext.FetchNext();
							break;
						}

						//オペレータが必要な場合はカッコ閉じとして処理
						parseStack.PopBracket(parseContext, parseContext.GetCurrent());
						parseContext.FetchNext();
						continue;
					}
					else {
						//オペランドが必要なところではカッコ閉じは不可
						return parseContext.Error(ERROR_AST_011, parseContext.GetCurrent());
					}
				}
				else {

					const OperatorInformation* nextOperator = TokenToOperator(parseContext.GetCurrent(), isRequireOperand);

					if (nextOperator != nullptr) {
						//オペレータをプッシュ
						parseStack.Reduce(*nextOperator, parseContext);
						parseStack.PushOperator(parseContext.GetCurrent(), nextOperator);
						parseContext.FetchNext();
					}
					else {
						//使用できるオペレータが存在しないため不正
						return parseContext.Error(ERROR_AST_004, parseContext.GetCurrent());
					}
				}	
			}
		}

		//スタックサイズが0の場合、式がなくて終了しているので問題
		if (parseStack.Size() == 0) {
			return parseContext.Error(ERROR_AST_040, parseContext.GetPrev());
		}

		//最後がオペランドを要求しているなら打ち切ると問題になる
		if (parseStack.IsRequireOperandNext()) {
			return parseContext.Error(ERROR_AST_002, parseContext.GetPrev());
		}

		//出揃ったので最終的にまとめて終了
		return parseStack.ReduceAll(parseContext);
	}

	//カンマ区切りで任意の終端をもつ式をまとめる
	void ASTParser::ParseASTExpressionList(ASTParseContext& parseContext, std::vector<ConstASTNodeRef>& result, uint32_t sequenceEndFlags) {
		
		//カンマ終了はおかしいはず
		assert(!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COMMA));
		if (CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COMMA)) {
			parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
			return;
		}

		result.clear();

		//要素数だけループ
		while (!parseContext.IsEnd()) {
			//からっぽまたはカンマ後にExpressionが存在しない場合
			if (IsSequenceEnd(parseContext, sequenceEndFlags)) {
				parseContext.FetchNext();
				return;
			}
			ASTNodeRef node = ParseASTExpression(parseContext, sequenceEndFlags | SEQUENCE_END_FLAG_COMMA);
			result.push_back(node);
			if (parseContext.GetPrev().type != ScriptTokenType::Comma) {
				//カンマじゃなかったら終端となるので終了
				return;
			}
		}

		//終端がみつかってないのでエラー
		if (!parseContext.HasError()) {
			parseContext.Error(ERROR_AST_012, parseContext.GetCurrent());
		}
	}

	//仮引数リストを解析
	//TODO: 最後のトークンを捨てて終えるのかどうか定まってないので決める
	void ASTParser::ParseASTArgumentList(ASTParseContext& parseContext, std::vector<std::string>& result, uint32_t sequenceEndFlags) {

		//カンマ終了はおかしいはず
		assert(!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COMMA));
		if (CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COMMA)) {
			parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
			return;
		}

		result.clear();

		//からっぽの場合
		if (IsSequenceEnd(parseContext, sequenceEndFlags)) {
			return;
		}

		//要素数ぶんだけループ
		while (!parseContext.IsEnd()) {

			//まずシンボル
			if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
				parseContext.Error(ERROR_AST_013, parseContext.GetCurrent());
				return;
			}

			result.push_back(parseContext.GetCurrent().body);
			parseContext.FetchNext();

			//終端かカンマ
			if (IsSequenceEnd(parseContext, sequenceEndFlags)) {
				//終端
				return;
			}
			else if (IsSequenceEnd(parseContext, SEQUENCE_END_FLAG_COMMA)) {
				parseContext.FetchNext();
				continue;
			}

			parseContext.Error(ERROR_AST_014, parseContext.GetCurrent());
			return;
		}

		//終端がみつかってないのでエラー
		if (!parseContext.HasError()) {
			parseContext.Error(ERROR_AST_015, parseContext.GetCurrent());
		}
	}

	//シーケンス終了の文字にあてはまるか
	bool ASTParser::IsSequenceEnd(ASTParseContext& parseContext, uint32_t sequenceEndFlags) {
		switch (parseContext.GetCurrent().type) {
		case ScriptTokenType::Comma:
			return CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COMMA);
		case ScriptTokenType::Semicolon:
			return CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_SEMICOLON);
		case ScriptTokenType::Colon:
			return CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COLON);
		case ScriptTokenType::BracketEnd:
			return CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_BLACKET);
		case ScriptTokenType::ArrayEnd:
			return CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_ARRAY_BLACKET);
		case ScriptTokenType::BlockEnd:
			return CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_BLOCK_BLACKET);
		case ScriptTokenType::VerticalBar:
			return CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_VERTICAL_BAR);
		default:
			return false;
		}
	}

	//数値リテラル
	ASTNodeRef ASTParser::ParseASTNumberLiteral(ASTParseContext& parseContext) {

		//対象外の情報は内部エラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Number);
		if (parseContext.GetCurrent().type != ScriptTokenType::Number) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		//単純に string to double をかける
		const number value = std::stod(parseContext.GetCurrent().body);
		auto r = ASTNodeRef(new ASTNodeNumberLiteral(value, parseContext.GetSourceMetadata()));
		r->SetSourceRange(parseContext.GetCurrent());
		parseContext.FetchNext();
		return r;
	}

	//文字列リテラル / フォーマット文字列
	ASTNodeRef ASTParser::ParseASTString(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報は内部エラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::StringBegin || parseContext.GetCurrent().type == ScriptTokenType::SpeakBegin);
		if (parseContext.GetCurrent().type != ScriptTokenType::StringBegin && parseContext.GetCurrent().type != ScriptTokenType::SpeakBegin) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		std::vector<ASTNodeFormatString::Item> items;
		ASTNodeRef str(nullptr);

		//stringEndまで読む、string以外が来たら式扱いする
		while (!parseContext.IsEnd()) {
			if (parseContext.GetCurrent().type == ScriptTokenType::String) {
				str = ParseASTStringLiteral(parseContext);
				items.push_back({str, true});
			}
			else if(parseContext.GetCurrent().type == ScriptTokenType::ExpressionInString) {
				parseContext.FetchNext();
				auto item = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_BLOCK_BLACKET);
				items.push_back({item, true});
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::StatementInString) {
				parseContext.FetchNext();
				auto item = ParseASTCodeBlock(parseContext, true);
				items.push_back({item, false});
			}
			else {
				assert(false);
			}

			//終端
			if (parseContext.GetCurrent().type == ScriptTokenType::StringEnd || parseContext.GetCurrent().type == ScriptTokenType::SpeakEnd) {
				parseContext.FetchNext();
				break;
			}
		}

		if (items.size() == 1 && str != nullptr) {
			//もし文字列リテラル１個だけなら文字列リテラル自体に換える
			return str;
		}
		else {
			//それ以外ならフォーマット文字列ノードを作る
			ASTNodeRef r = ASTNodeRef(new ASTNodeFormatString(items, parseContext.GetSourceMetadata()));
			r->SetSourceRange(parseContext.GetCurrent());
			return r;
		}
	}

	//文字列リテラル
	ASTNodeRef ASTParser::ParseASTStringLiteral(ASTParseContext& parseContext) {

		//対象外の情報は内部エラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::String);
		if (parseContext.GetCurrent().type != ScriptTokenType::String) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		std::string value = parseContext.GetCurrent().body;
		
		auto r = ASTNodeRef(new ASTNodeStringLiteral(value, parseContext.GetSourceMetadata()));
		r->SetSourceRange(parseContext.GetCurrent());
		parseContext.FetchNext();
		return r;
	}

	//コンテキスト値取得
	ASTNodeRef ASTParser::ParseASTContextValue(ASTParseContext& parseContext) {

		//クラスパースのコンテキストでないと使用不可
		if (parseContext.GetParsingClass() == nullptr) {
			return parseContext.Error(ERROR_AST_044, parseContext.GetCurrent());
		}

		ASTNodeRef result = nullptr;
		if (parseContext.GetCurrent().type == ScriptTokenType::This) {
			result.reset(new ASTNodeContextValue(ASTNodeContextValue::ValueType::This, parseContext.GetParsingClass(), parseContext.GetSourceMetadata()));
		}
		else if (parseContext.GetCurrent().type == ScriptTokenType::Base) {
			result.reset(new ASTNodeContextValue(ASTNodeContextValue::ValueType::Base, parseContext.GetParsingClass(), parseContext.GetSourceMetadata()));
		}
		else {
			//存在しないタイプ
			assert(false);
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		result->SetSourceRange(parseContext.GetCurrent());
		parseContext.FetchNext();
		return result;
	}

	//シンボル解決
	ASTNodeRef ASTParser::ParseASTSymbol(ASTParseContext& parseContext) {

		//対象外の情報は内部エラーで返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Symbol);
		if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		std::string name = parseContext.GetCurrent().body;
		auto r = ASTNodeRef(new ASTNodeResolveSymbol(name, parseContext.GetSourceMetadata()));
		r->SetSourceRange(parseContext.GetCurrent());
		parseContext.FetchNext();
		return r;
	}

	//配列イニシャライザ
	ASTNodeRef ASTParser::ParseASTArrayInitializer(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報は内部エラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::ArrayBegin);
		if (parseContext.GetCurrent().type != ScriptTokenType::ArrayBegin) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		//各要素がexpressionとして存在していて、カンマもしくは終了カッコで閉じられるはずだ
		std::vector<ConstASTNodeRef> items;
		ParseASTExpressionList(parseContext, items, SEQUENCE_END_FLAG_ARRAY_BLACKET);
		auto r = ASTNodeRef(new ASTNodeArrayInitializer(items, parseContext.GetSourceMetadata()));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//オブジェクトイニシャライザ
	ASTNodeRef ASTParser::ParseASTObjectInitializer(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報は内部エラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::BlockBegin);
		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		std::vector<ASTNodeObjectInitializer::Item> items;

		//javascriptの記法を参考に
		while (!parseContext.IsEnd()) {
			//閉じ括弧で終わってれば終了
			if (parseContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
				parseContext.FetchNext();
				break;
			}

			//key
			std::string key;
			if (parseContext.GetCurrent().type == ScriptTokenType::Symbol) {
				//ダブルクォーテーションでかこまないシンボル形式
				key = parseContext.GetCurrent().body;
				parseContext.FetchNext();
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::String) {
				//ダブルクォーテーションでかこむ文字列形式
				key = parseContext.GetCurrent().body;
				parseContext.FetchNext();
			}
			else {
				return parseContext.Error(ERROR_AST_016, parseContext.GetCurrent());
			}

			//コロン
			if (parseContext.GetCurrent().type != ScriptTokenType::Colon) {
				return parseContext.Error(ERROR_AST_017, parseContext.GetCurrent());
			}
			parseContext.FetchNext();

			//value
			ASTNodeRef value = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_COMMA | SEQUENCE_END_FLAG_BLOCK_BLACKET);
			items.push_back({ key, value });

			//閉じ括弧で終わってれば終了
			if (parseContext.GetPrev().type == ScriptTokenType::BlockEnd) {
				break;
			}
		}

		auto r = ASTNodeRef(new ASTNodeObjectInitializer(items, parseContext.GetSourceMetadata()));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//関数イニシャライザ function(val) {  }
	ASTNodeRef ASTParser::ParseASTFunctionInitializer(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		switch (parseContext.GetCurrent().type) {
			case ScriptTokenType::Function:
			case ScriptTokenType::VerticalBar:
			// 引数無しの関数式とみなす。
			case ScriptTokenType::LogicalOr:
				break;
			default:
				assert(false);
				return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		if (parseContext.GetCurrent().type == ScriptTokenType::Function) {
			parseContext.FetchNext();
		}

		std::vector<std::string> argList;
		bool isSyntaxSugar = false;

		//開カッコがあれば引数リスト
		if (parseContext.GetCurrent().type == ScriptTokenType::BracketBegin) {
			parseContext.FetchNext();
			if (parseContext.GetCurrent().type != ScriptTokenType::BracketEnd) {
				ParseASTArgumentList(parseContext, argList, SEQUENCE_END_FLAG_BLACKET);
				parseContext.FetchNext();
			}
			else {
				parseContext.FetchNext();
			}
		}
		else if (parseContext.GetCurrent().type == ScriptTokenType::VerticalBar) {
			isSyntaxSugar = true;
			parseContext.FetchNext();
			if (parseContext.GetCurrent().type != ScriptTokenType::VerticalBar) {
				ParseASTArgumentList(parseContext, argList, SEQUENCE_END_FLAG_VERTICAL_BAR);
				parseContext.FetchNext();
			}
			else {
				parseContext.FetchNext();
			}
		}
		// 引数無しの関数式とみなす。
		else if (parseContext.GetCurrent().type == ScriptTokenType::LogicalOr) {
			isSyntaxSugar = true;
			parseContext.FetchNext();
		}

		//中括弧開
		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			return parseContext.Error(ERROR_AST_018, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		// 関数式の内部の式が1行で終わる場合は式の戻り値をreturnする
		if (isSyntaxSugar) {
			auto tentativeContext = parseContext;
			// 内部の式が1行で終わると仮定して読み進める
			// 戻り値はshared_ptrなので捨てて大丈夫
			ParseASTReturn(tentativeContext, true);

			// 成功するようなら改めてparseContextでやり直す
			if (!tentativeContext.IsEnd() && tentativeContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
				const ScriptToken& beginSyntaxSugarToken = parseContext.GetCurrent();
				std::shared_ptr<ASTNodeCodeBlock> funcBody(new ASTNodeCodeBlock(parseContext.GetSourceMetadata()));
				auto node = ParseASTReturn(parseContext, true);
				funcBody->AddStatement(node);
				parseContext.FetchNext();
				funcBody->SetSourceRange(beginSyntaxSugarToken, parseContext.GetPrev());

				auto r = ASTNodeRef(new ASTNodeFunctionInitializer(ScriptFunctionRef(new ScriptFunction(funcBody, argList)), parseContext.GetSourceMetadata()));
				r->SetSourceRange(beginToken, parseContext.GetPrev());
				return r;
			}
		}

		//コードブロックを取得
		ASTNodeRef funcBody = ParseASTCodeBlock(parseContext, true);
		auto r = ASTNodeRef(new ASTNodeFunctionInitializer(ScriptFunctionRef(new ScriptFunction(funcBody, argList)), parseContext.GetSourceMetadata()));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//関数ステートメント function FuncName:conditions(val) { }
	ASTNodeRef ASTParser::ParseASTFunctionStatement(ASTParseContext& parseContext, bool isRootBlockStatement) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Function);
		if (parseContext.GetCurrent().type != ScriptTokenType::Function) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		ScriptFunctionDef def = ParseFunctionDef(parseContext, BlockType::Function);
		if (def.func == nullptr) {
			return ASTNodeRef(new ASTError());
		}
		auto r = ASTNodeRef(new ASTNodeFunctionStatement(def.names, def.func, def.condition, isRootBlockStatement, parseContext.GetSourceMetadata()));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//トークイニシャライザ talk(val) { }
	ASTNodeRef ASTParser::ParseASTTalkInitializer(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Talk);
		if (parseContext.GetCurrent().type != ScriptTokenType::Talk) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();;

		std::vector<std::string> argList;

		//開カッコがれば引数リスト
		if (parseContext.GetCurrent().type == ScriptTokenType::BracketBegin) {
			parseContext.FetchNext();
			ParseASTArgumentList(parseContext, argList, SEQUENCE_END_FLAG_BLACKET);
			parseContext.FetchNext();
		}

		//中括弧開
		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			return parseContext.Error(ERROR_AST_019, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//トークブロック取得
		ASTNodeRef funcbody = ParseASTTalkBlock(parseContext);
		auto r = ASTNodeRef(new ASTNodeFunctionInitializer(ScriptFunctionRef(new ScriptFunction(funcbody, argList)), parseContext.GetSourceMetadata()));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//トークステートメント talk TalkName, TalkName2 : condition (args) {}
	ASTNodeRef ASTParser::ParseASTTalkStatement(ASTParseContext& parseContext, bool isRootBlockStatement) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Talk);
		if (parseContext.GetCurrent().type != ScriptTokenType::Talk) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		ScriptFunctionDef def = ParseFunctionDef(parseContext, BlockType::Talk);
		if (def.func == nullptr) {
			return ASTNodeRef(new ASTError());
		}
		auto r = ASTNodeRef(new ASTNodeFunctionStatement(def.names, def.func, def.condition, isRootBlockStatement, parseContext.GetSourceMetadata()));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	ScriptFunctionDef ASTParser::ParseFunctionDef(ASTParseContext& parseContext, BlockType blockType) {

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Function || blockType != BlockType::Function);
		assert(parseContext.GetCurrent().type == ScriptTokenType::Talk || blockType != BlockType::Talk);

		ScriptFunctionDef result;
		result.func = nullptr;		//funcがnullなら無効として戻り値を扱わせる

		if (blockType == BlockType::Function && parseContext.GetCurrent().type != ScriptTokenType::Function) {
			parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());

			//呼び出しもとでキャッチしてもらう必要がある。funcがnull
			return result;
		}

		if (blockType == BlockType::Talk && parseContext.GetCurrent().type != ScriptTokenType::Talk) {
			parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
			return result;
		}

		parseContext.FetchNext();

		//関数名
		while (true) {
			if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
				parseContext.Error(ERROR_AST_020, parseContext.GetCurrent());
				return result;
			}

			result.names.push_back(parseContext.GetCurrent().body);
			parseContext.FetchNext();

			//カンマがあればさらに名前を見る
			if (parseContext.GetCurrent().type == ScriptTokenType::Comma) {
				parseContext.FetchNext();
				continue;
			}
			break;
		}

		//開カッコがあれば引数リスト
		std::vector<std::string> argList;
		if (parseContext.GetCurrent().type == ScriptTokenType::BracketBegin) {
			parseContext.FetchNext();
			ParseASTArgumentList(parseContext, argList, SEQUENCE_END_FLAG_BLACKET);
			parseContext.FetchNext();
		}
		
		//条件部
		if (parseContext.GetCurrent().type == ScriptTokenType::If) {
			parseContext.FetchNext();
			if (parseContext.GetCurrent().type != ScriptTokenType::BracketBegin) {
				parseContext.Error(ERROR_AST_021, parseContext.GetCurrent());
				return result;
			}
			parseContext.FetchNext();
			result.condition = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_BLACKET);
		}
		
		//中括弧開
		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			parseContext.Error(ERROR_AST_022, parseContext.GetCurrent());
			return result;
		}
		parseContext.FetchNext();

		//コードブロックを取得
		ASTNodeRef funcBody;
		if (blockType == BlockType::Function) {
			funcBody = ParseASTCodeBlock(parseContext, true);
		}
		else if (blockType == BlockType::Talk) {
			funcBody = ParseASTTalkBlock(parseContext);
		}

		result.func = ScriptFunctionRef(new ScriptFunction(funcBody, argList));
		return result;
	}

	//セッタ
	ASTNodeRef ASTParser::ParseASTSet(ASTParseContext& parseContext, const ASTNodeRef& target, const ASTNodeRef& value) {

		//target側がゲット系のノードになっているはずなのでセット系のノードに変換する
		if (!target->CanConvertToSetter()) {
			return parseContext.Error(ERROR_AST_023, SourceCodeRange(target->GetSourceRange(), value->GetSourceRange()));
		}
		return target->ConvertToSetter(value);
	}

	//new
	ASTNodeRef ASTParser::ParseASTNew(ASTParseContext& parseContext, const ScriptToken& operatorToken, const ASTNodeRef& target) {
		if (target->GetType() != ASTNodeType::FunctionCall) {
			return parseContext.Error(ERROR_AST_024, parseContext.GetCurrent());
		}

		//FunctionCallとして呼出が解析されているのでコンバートする
		std::shared_ptr<ASTNodeFunctionCall> fc = std::static_pointer_cast<ASTNodeFunctionCall>(target);
		ConstASTNodeRef funcNode = fc->GetFunctionNode();

		auto r = ASTNodeRef(new ASTNodeNewClassInstance(fc->GetFunctionNode(), fc->GetArgumentNodes(), parseContext.GetSourceMetadata()));

		//ソース位置の特定、new演算子から最後の引数か関数までの間でとる
		SourceCodeRange last = fc->GetFunctionNode()->GetSourceRange();
		if (fc->GetArgumentNodes().size() > 0) {
			last = fc->GetArgumentNodes()[fc->GetArgumentNodes().size() - 1]->GetSourceRange();
		}
		
		SourceCodeRange range;
		range.SetRange(operatorToken.sourceRange, last);

		r->SetSourceRange(range);
		return r;
	}

	//クラス
	ASTNodeRef ASTParser::ParseASTClass(ASTParseContext& parseContext) {

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Class);
		if (parseContext.GetCurrent().type != ScriptTokenType::Class) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		const auto& beginToken = parseContext.GetCurrent();
		parseContext.FetchNext();
		ScriptClassRef result(new ScriptClass(parseContext.GetSourceMetadata()));
		ASTParseContext::ClassScope classScope(parseContext, result);

		//クラス名
		if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
			return parseContext.Error(ERROR_AST_025, parseContext.GetCurrent());
		}
		std::string className = parseContext.GetCurrent().body;
		result->SetName(className);
		parseContext.FetchNext();

		//継承
		if (parseContext.GetCurrent().type == ScriptTokenType::Colon) {
			parseContext.FetchNext();
			
			if (parseContext.GetCurrent().type == ScriptTokenType::Unit) {
				//ユニットルート参照
				result->GetParentClassPath().SetFullPath(true);
				parseContext.FetchNext();
			}

			while (!parseContext.IsEnd()) {
				if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
					return parseContext.Error(ERROR_AST_026, parseContext.GetCurrent());
				}

				result->GetParentClassPath().AddFullPath(parseContext.GetCurrent().body);
				parseContext.FetchNext();

				if (parseContext.GetCurrent().type == ScriptTokenType::Dot) {
					parseContext.FetchNext();
				}
				else {
					break;
				}
			}
		}

		//開カッコ
		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			return parseContext.Error(ERROR_AST_027, parseContext.GetCurrent());
		}

		//ここまで宣言部として記録する
		result->SetDeclareSourceRange(SourceCodeRange(beginToken.sourceRange, parseContext.GetCurrent().sourceRange));

		parseContext.FetchNext();

		//メンバを確認
		while (!parseContext.IsEnd()) {

			if (parseContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
				//終了
				parseContext.FetchNext();
				parseContext.AddClass(result);
				return ASTNodeRef(new ASTNodeClass(result, parseContext.GetSourceMetadata()));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Init) {
				//コンストラクタ
				parseContext.FetchNext();

				//引数リスト
				std::vector<std::string> argList;

				if (parseContext.GetCurrent().type == ScriptTokenType::BracketBegin) {
					//引数解析
					parseContext.FetchNext();
					ParseASTArgumentList(parseContext, argList, SEQUENCE_END_FLAG_BLACKET);
					parseContext.FetchNext();
				}

				//コロン・baseで親クラスコンストラクタ呼び出し
				if (parseContext.GetCurrent().type == ScriptTokenType::Colon) {
					parseContext.FetchNext();
					if (parseContext.GetCurrent().type != ScriptTokenType::Base) {
						return parseContext.Error(ERROR_AST_047, parseContext.GetCurrent());
					}

					//base呼び出し式を取得
					auto baseCall = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_BLOCK_BLACKET_BEGIN);
					if (baseCall->GetType() != ASTNodeType::FunctionCall) {
						return parseContext.Error(ERROR_AST_047, parseContext.GetCurrent());
					}

					auto baseInitCall = std::static_pointer_cast<ASTNodeFunctionCall>(baseCall);
					result->SetParentClassInitArguments(baseInitCall->GetArgumentNodes());
				}
				else {
					//開き中括弧(base呼び出しがある場合はBlockBeginまで処理して戻るので、そうでない場合のみ)
					if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
						return parseContext.Error(ERROR_AST_028, parseContext.GetCurrent());
					}
					parseContext.FetchNext();
				}

				//関数本体
				ASTNodeRef initBody = ParseASTCodeBlock(parseContext, true);

				auto functionRef = ScriptFunctionRef(new ScriptFunction(initBody, argList));
				result->SetInitFunc(functionRef);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Function) {
				//メンバ関数
				ScriptFunctionDef def = ParseFunctionDef(parseContext, BlockType::Function);
				if (def.func == nullptr) {
					return parseContext.Error(ERROR_AST_000, parseContext.GetCurrent());
				}

				result->AddFunction(def);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Talk) {
				//メンバトーク
				ScriptFunctionDef def = ParseFunctionDef(parseContext, BlockType::Talk);
				if (def.func == nullptr) {
					return parseContext.Error(ERROR_AST_000, parseContext.GetCurrent());
				}

				result->AddFunction(def);
			}
			else {
				//構文エラーか
				return parseContext.Error(ERROR_AST_000, parseContext.GetCurrent());
			}
		}

		if (parseContext.HasError()) {
			return parseContext.Error(ERROR_AST_029, parseContext.GetCurrent());
		}

		//ここでは戻れない（中括弧終わりにならないといけないので）
		return parseContext.Error(ERROR_AST_000, parseContext.GetCurrent());
	}

	//unitキーワードのパース
	ASTNodeRef ASTParser::ParseASTUnit(ASTParseContext& parseContext) {
		assert(parseContext.GetCurrent().type == ScriptTokenType::Unit);
		if (parseContext.GetCurrent().type != ScriptTokenType::Unit) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		//文脈によって宣言かオブジェクトかが変わる
		if (parseContext.GetCurrent().type == ScriptTokenType::Dot) {
			//このケースは式文脈なのでステートメント処理に流れてないといけない
			return parseContext.Error(ERROR_AST_000, parseContext.GetCurrent());
		}
		else if(parseContext.GetCurrent().type == ScriptTokenType::Symbol) {
			//シンボルが続く場合は宣言の形
			ParseASTUnitDef(parseContext);
			return nullptr;
		}
		else {
			//そのどれでもない場合は構文としておかしいのでエラー
			return parseContext.Error(ERROR_AST_049, parseContext.GetCurrent());
		}
	}

	//unit宣言のパース
	void ASTParser::ParseASTUnitDef(ASTParseContext& parseContext) {

		//ユニットを多重で設定不可
		if (parseContext.GetScriptUnit()->IsExplicitUnit()) {
			parseContext.Error(ERROR_AST_043, parseContext.GetCurrent());
			return;
		}

		//ユニット名
		std::string unitName = "";

		while (true) {
			if (parseContext.GetCurrent().type == ScriptTokenType::Symbol) {
				//ユニット名でないなにかがある
				unitName.append(parseContext.GetCurrent().body);
				parseContext.FetchNext();
			}
			else {
				parseContext.Error(ERROR_AST_042, parseContext.GetCurrent());
				return;
			}

			if (parseContext.GetCurrent().type == ScriptTokenType::Dot) {
				parseContext.FetchNext();
				unitName.append(".");
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Semicolon) {
				parseContext.FetchNext();
				break;
			}
			else {
				parseContext.Error(ERROR_AST_041, parseContext.GetCurrent());
				return;
			}
		}
		parseContext.GetScriptUnit()->SetUnit(unitName);
	}

	void ASTParser::ParseASTUse(ASTParseContext& parseContext) {
		assert(parseContext.GetCurrent().type == ScriptTokenType::Use);
		if (parseContext.GetCurrent().type != ScriptTokenType::Use) {
			parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
			return;
		}

		parseContext.FetchNext();

		//useキーワードの対象を取得
		std::string path;
		const ScriptToken* lastPathToken = nullptr;
		while (!parseContext.IsEnd()) {

			if (parseContext.GetCurrent().type == ScriptTokenType::Asterisk) {
				if (path.empty()) {
					//err
					assert(false);
				}

				//アスタリスクのワイルドカード参照
				parseContext.FetchNext();
				if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
					//err
					assert(false);
				}

				//ワイルドカード参照で登録
				parseContext.GetSourceMetadata()->GetAlias().RegisterWildcardAlias(path);
				parseContext.FetchNext();
				return;
			}

			if (parseContext.GetCurrent().type == ScriptTokenType::Symbol) {
				//シンボルをパスに追加
				if (!path.empty()) {
					path.append(".");
				}
				lastPathToken = &parseContext.GetCurrent();
				path.append(lastPathToken->body);
				parseContext.FetchNext();
			}
			else {
				//err
				return;
			}

			//セミコロンで終端するかピリオドで続ける
			if (parseContext.GetCurrent().type == ScriptTokenType::Semicolon) {
				//エイリアスを登録して終了
				if (!parseContext.GetSourceMetadata()->GetAlias().RegisterAlias(lastPathToken->body, path)) {
					//エイリアス重複のためエラー
				}
				parseContext.FetchNext();
				return;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Dot) {
				parseContext.FetchNext();
			}
			else {
				//err
				return;
			}
		}
	}

	//ローカル変数宣言のパース
	ASTNodeRef ASTParser::ParseASTLocalVariable(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//まずlocalを読み飛ばす
		assert(parseContext.GetCurrent().type == ScriptTokenType::Local);
		if (parseContext.GetCurrent().type != ScriptTokenType::Local) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		//結果リスト
		std::shared_ptr<ASTNodeLocalVariableDeclarationList> result(new ASTNodeLocalVariableDeclarationList(parseContext.GetSourceMetadata()));
		std::vector<ScriptVariableDef> defs;
		
		if (!ParseVariableDef(parseContext, defs)) {
			return ASTNodeRef(new ASTError());
		}

		for (size_t i = 0; i < defs.size(); i++) {
			auto val = std::shared_ptr<ASTNodeLocalVariableDeclaration>(new ASTNodeLocalVariableDeclaration(defs[i].name, defs[i].initializer, parseContext.GetSourceMetadata()));
			val->SetSourceRange(defs[i].range);
			result->AddVariable(val);
		}

		//解析範囲を格納
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	bool ASTParser::ParseVariableDef(ASTParseContext& parseContext, std::vector<ScriptVariableDef>& defs) {
	//変数リストのパース
		defs.clear();

		while (!parseContext.IsEnd()) {

			//シンボルでないなら構文エラー
			if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
				parseContext.Error(ERROR_AST_030, parseContext.GetCurrent());
				return false;
			}

			//シンボルならそれが名前になる
			const ScriptToken& beginToken = parseContext.GetCurrent();
			std::string variableName = parseContext.GetCurrent().body;
			parseContext.FetchNext();

			//もしイコールがあれば初期化式として扱う
			bool hasExpression = false;
			ASTNodeRef initialValue(nullptr);
			if (parseContext.GetCurrent().type == ScriptTokenType::Equal) {
				parseContext.FetchNext();
				initialValue = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_SEMICOLON | SEQUENCE_END_FLAG_COMMA);
				hasExpression = true;
			}

			//ここまでで決定
			ScriptVariableDef def;
			def.initializer = initialValue;
			def.name = variableName;
			def.range.SetRange(beginToken.sourceRange, parseContext.GetPrev().sourceRange);
			defs.push_back(def);

			if (hasExpression) {
				if (parseContext.GetPrev().type == ScriptTokenType::Comma) {
					//もしカンマがあれば連続宣言なのでループする
					continue;
				}
				else if (parseContext.GetPrev().type == ScriptTokenType::Semicolon) {
					//セミコロンなら終わり
					break;
				}
				else {
					//セミコロンでもカンマでもないとエラー
					parseContext.Error(ERROR_AST_000, parseContext.GetPrev());
					return false;
				}
			}
			else {
				if (parseContext.GetCurrent().type == ScriptTokenType::Comma) {
					//もしカンマがあれば連続宣言なのでループする
					parseContext.FetchNext();
					continue;
				}
				else if (parseContext.GetCurrent().type == ScriptTokenType::Semicolon) {
					//セミコロンなら終わり
					parseContext.FetchNext();
					break;
				}
				else {
					//セミコロンでもカンマでもないとエラー。ここは来ないはず
					parseContext.Error(ERROR_AST_039, parseContext.GetCurrent());
					return false;
				}
			}
		}

		
		return true;
	}

	//foreach文
	ASTNodeRef ASTParser::ParseASTForeach(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Foreach);
		if (parseContext.GetCurrent().type != ScriptTokenType::Foreach) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//開き括弧
		if (parseContext.GetCurrent().type != ScriptTokenType::BracketBegin) {
			return parseContext.Error(ERROR_AST_050, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//local(任意)
		bool isRegisterLoopVariable = false;
		if (parseContext.GetCurrent().type == ScriptTokenType::Local) {
			isRegisterLoopVariable = true;
			parseContext.FetchNext();
		}

		//変数シンボル
		if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
			return parseContext.Error(ERROR_AST_051, parseContext.GetCurrent());
		}
		std::string valueName = parseContext.GetCurrent().body;
		std::string keyName = "";
		parseContext.FetchNext();

		//カンマ（任意）
		if (parseContext.GetCurrent().type == ScriptTokenType::Comma) {
			parseContext.FetchNext();
			//キー変数シンボル
			if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
				return parseContext.Error(ERROR_AST_051, parseContext.GetCurrent());
			}
			keyName = parseContext.GetCurrent().body;
			parseContext.FetchNext();
		}

		//in
		if (parseContext.GetCurrent().type != ScriptTokenType::In) {
			return parseContext.Error(ERROR_AST_052, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//ターゲット式
		auto target = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_BLACKET);

		//ブロック開始のカッコがあるかを調べる
		ConstASTNodeRef loopStatement;
		if (parseContext.GetCurrent().type == ScriptTokenType::BlockBegin) {
			//ブロック開始であればtrue処理はコードブロックになる
			parseContext.FetchNext();
			loopStatement = ParseASTCodeBlock(parseContext, true);
		}
		else {
			//ブロック開始でなければ単体ステートメントになる
			loopStatement = ParseASTStatement(parseContext, true);
		}

		ASTNodeForeach* const node = new ASTNodeForeach(valueName, keyName, isRegisterLoopVariable, target, loopStatement, parseContext.GetSourceMetadata());
		node->SetSourceRange(beginToken, parseContext.GetPrev());
		return ASTNodeRef(node);
	}

	//for文
	ASTNodeRef ASTParser::ParseASTFor(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::For);
		if (parseContext.GetCurrent().type != ScriptTokenType::For) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}


		parseContext.FetchNext();

		//まず開きカッコが必要
		if (parseContext.GetCurrent().type != ScriptTokenType::BracketBegin) {
			return parseContext.Error(ERROR_AST_031, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		ASTNodeRef initExpression = nullptr;
		ASTNodeRef ifExpression = nullptr;
		ASTNodeRef incrementExpression = nullptr;
		ASTNodeRef loopStatement = nullptr;

		//初期化式
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			//原則として式のみ、例外でlocal だけ認める形
			if (parseContext.GetCurrent().type == ScriptTokenType::Local) {
				initExpression = ParseASTLocalVariable(parseContext);
			}
			else {
				initExpression = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_SEMICOLON);
			}
		}
		else {
			parseContext.FetchNext();
		}

		//条件式
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			ifExpression = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_SEMICOLON);
		}
		else {
			parseContext.FetchNext();
		}

		//インクリメント式
		if (parseContext.GetCurrent().type != ScriptTokenType::BracketEnd) {
			incrementExpression = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_BLACKET);
		}
		else {
			parseContext.FetchNext();
		}


		//ブロック開始のカッコがあるかを調べる
		if (parseContext.GetCurrent().type == ScriptTokenType::BlockBegin) {
			//ブロック開始であればtrue処理はコードブロックになる
			parseContext.FetchNext();
			loopStatement = ParseASTCodeBlock(parseContext, true);
		}
		else {
			//ブロック開始でなければ単体ステートメントになる
			loopStatement = ParseASTStatement(parseContext, true);
		}

		ASTNodeRef result(new ASTNodeFor(initExpression, ifExpression, incrementExpression, loopStatement, parseContext.GetSourceMetadata()));
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//while文
	ASTNodeRef ASTParser::ParseASTWhile(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::While);
		if (parseContext.GetCurrent().type != ScriptTokenType::While) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}


		parseContext.FetchNext();

		//まず開きカッコが必要
		if (parseContext.GetCurrent().type != ScriptTokenType::BracketBegin) {
			return parseContext.Error(ERROR_AST_032, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//次に条件式
		ASTNodeRef ifExpression = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_BLACKET);
		ASTNodeRef trueStatement = nullptr;

		//ブロック開始のカッコがあるかを調べる
		if (parseContext.GetCurrent().type == ScriptTokenType::BlockBegin) {
			//ブロック開始であればtrue処理はコードブロックになる
			parseContext.FetchNext();
			trueStatement = ParseASTCodeBlock(parseContext, true);
		}
		else {
			//ブロック開始でなければ単体ステートメントになる
			trueStatement = ParseASTStatement(parseContext, true);
		}

		ASTNodeRef result(new ASTNodeWhile(ifExpression, trueStatement, parseContext.GetSourceMetadata()));
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//if文のパース
	ASTNodeRef ASTParser::ParseASTIf(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//条件文、trueブロック、falseブロックがあるはず
		assert(parseContext.GetCurrent().type == ScriptTokenType::If);
		if (parseContext.GetCurrent().type != ScriptTokenType::If) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		//まず開きカッコが必要
		if (parseContext.GetCurrent().type != ScriptTokenType::BracketBegin) {
			return parseContext.Error(ERROR_AST_033, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//次に条件式
		ASTNodeRef ifExpression = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_BLACKET);
		ASTNodeRef trueStatement = nullptr;
		ASTNodeRef falseStatement = nullptr;

		//ブロック開始のカッコがあるかを調べる
		if (parseContext.GetCurrent().type == ScriptTokenType::BlockBegin) {
			//ブロック開始であればtrue処理はコードブロックになる
			parseContext.FetchNext();
			trueStatement = ParseASTCodeBlock(parseContext, true);
		}
		else {
			//ブロック開始でなければ単体ステートメントになる
			trueStatement = ParseASTStatement(parseContext, true);
		}

		ASTNodeRef r;

		//elseの存在をチェック
		if (parseContext.GetCurrent().type == ScriptTokenType::Else) {
			parseContext.FetchNext();

			if (parseContext.GetCurrent().type == ScriptTokenType::BlockBegin) {
				parseContext.FetchNext();
				falseStatement = ParseASTCodeBlock(parseContext, true);
			}
			else {
				falseStatement = ParseASTStatement(parseContext, true);
			}
			r = ASTNodeRef(new ASTNodeIf(ifExpression, trueStatement, falseStatement, parseContext.GetSourceMetadata()));
		}
		else {
			//elseなし
			r = ASTNodeRef(new ASTNodeIf(ifExpression, trueStatement, parseContext.GetSourceMetadata()));
		}

		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//break文
	ASTNodeRef ASTParser::ParseASTBreak(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		
		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Break);
		if (parseContext.GetCurrent().type != ScriptTokenType::Break) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		//セミコロンが必要
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			return parseContext.Error(ERROR_AST_034, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		ASTNodeRef result(new ASTNodeBreak(parseContext.GetSourceMetadata()));
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//continue文
	ASTNodeRef ASTParser::ParseASTContinue(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//対象外の情報にはエラーを返す
		assert(parseContext.GetCurrent().type == ScriptTokenType::Continue);
		if (parseContext.GetCurrent().type != ScriptTokenType::Continue) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		//セミコロンが必要
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			return parseContext.Error(ERROR_AST_035, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		ASTNodeRef result(new ASTNodeContinue(parseContext.GetSourceMetadata()));
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//return文のパース
	ASTNodeRef ASTParser::ParseASTReturn(ASTParseContext& parseContext, bool isLambdaSyntaxSugar) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		if (isLambdaSyntaxSugar) {
			assert(parseContext.GetPrev().type == ScriptTokenType::BlockBegin);
			if (parseContext.GetPrev().type != ScriptTokenType::BlockBegin) {
				return parseContext.Error(ERROR_AST_999, parseContext.GetPrev());
			}
		}
		else {
			assert(parseContext.GetCurrent().type == ScriptTokenType::Return);
			if (parseContext.GetCurrent().type != ScriptTokenType::Return) {
				return parseContext.Error(ERROR_AST_999, parseContext.GetPrev());
			}
			parseContext.FetchNext();
		}

		ASTNodeRef r;
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			//式があってセミコロンで終了な単純系のはず
			ASTNodeRef returnValueNode = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_SEMICOLON);
			r = ASTNodeRef(new ASTNodeReturn(returnValueNode, parseContext.GetSourceMetadata()));
		}
		else {
			//戻り値なし
			parseContext.FetchNext();
			r = ASTNodeRef(new ASTNodeReturn(parseContext.GetSourceMetadata()));
		}

		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//try
	ASTNodeRef ASTParser::ParseASTTry(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		assert(parseContext.GetCurrent().type == ScriptTokenType::Try);
		if (parseContext.GetCurrent().type != ScriptTokenType::Try) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			return parseContext.Error(ERROR_AST_036, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//tryブロック
		ConstASTNodeRef tryBlock = ParseASTCodeBlock(parseContext, true);
		std::shared_ptr<ASTNodeTry> resultNode = std::shared_ptr<ASTNodeTry>(new ASTNodeTry(tryBlock, parseContext.GetSourceMetadata()));

		//catch
		while (parseContext.GetCurrent().type == ScriptTokenType::Catch) {
			parseContext.FetchNext();

			//開カッコがあれば例外変数
			std::string variableName;
			if (parseContext.GetCurrent().type == ScriptTokenType::BracketBegin) {
				parseContext.FetchNext();

				//変数名
				if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
					return parseContext.Error(ERROR_AST_045, parseContext.GetCurrent());
				}
				variableName = parseContext.GetCurrent().body;
				parseContext.FetchNext();

				//閉じカッコ
				if (parseContext.GetCurrent().type != ScriptTokenType::BracketEnd) {
					return parseContext.Error(ERROR_AST_046, parseContext.GetCurrent());
				}
				parseContext.FetchNext();
			}

			//開カッコ
			if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
				return parseContext.Error(ERROR_AST_037, parseContext.GetCurrent());
			}
			parseContext.FetchNext();

			//catchブロック
			ConstASTNodeRef catchBlock = ParseASTCodeBlock(parseContext, true);
			resultNode->AddCatchBlock(catchBlock, variableName);
		}

		//finally
		if (parseContext.GetCurrent().type == ScriptTokenType::Finally) {
			parseContext.FetchNext();

			//開カッコ
			if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
				return parseContext.Error(ERROR_AST_038, parseContext.GetCurrent());
			}
			parseContext.FetchNext();

			//finallyブロック
			ConstASTNodeRef finallyBlock = ParseASTCodeBlock(parseContext, true);
			resultNode->SetFinallyBlock(finallyBlock);
		}

		resultNode->SetSourceRange(beginToken, parseContext.GetPrev());
		return resultNode;
	}

	//throw文のパース
	ASTNodeRef ASTParser::ParseASTThrow(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		assert(parseContext.GetCurrent().type == ScriptTokenType::Throw);
		if (parseContext.GetCurrent().type != ScriptTokenType::Throw) {
			return parseContext.Error(ERROR_AST_999, parseContext.GetCurrent());
		}

		parseContext.FetchNext();

		ASTNodeRef r;
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			//式があってセミコロンで終了な単純系のはず
			ASTNodeRef throwValueNode = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_SEMICOLON);
			r = ASTNodeRef(new ASTNodeThrow(throwValueNode, parseContext.GetSourceMetadata()));
		}
		else {
			//戻り値なし
			parseContext.FetchNext();
			r = ASTNodeRef(new ASTNodeThrow(parseContext.GetSourceMetadata()));
		}

		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//ResolveSymbol -> AssignSymbol
	ASTNodeRef ASTNodeResolveSymbol::ConvertToSetter(const ASTNodeRef& valueNode) const {
		auto node = ASTNodeRef(new ASTNodeAssignSymbol(name, valueNode, valueNode->GetSourceMetadata()));
		node->SetSourceRange(GetSourceRange());
		return node;
	}

	//ResolveMember -> AssignMember
	ASTNodeRef ASTNodeResolveMember::ConvertToSetter(const ASTNodeRef& valueNode) const {
		auto node = ASTNodeRef(new ASTNodeAssignMember(target, key, valueNode, valueNode->GetSourceMetadata()));
		node->SetSourceRange(GetSourceRange());
		return node;
	}

	

}
