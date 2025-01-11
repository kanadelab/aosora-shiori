#include "Misc/Utility.h"
#include "Tokens/Tokens.h"
#include "AST/ASTNodeBase.h"
#include "AST/ASTNodes.h"
#include "AST/ASTParser.h"

//パースエラー発生時にassertで止める
//#define AOSORA_ENABLE_PARSE_ERROR_ASSERT

namespace sakura {

	const ScriptParseErrorData ERROR_AST_001 = { "A001", "コードブロック終了の閉じ括弧 } が見つかりませんでした。", "コードブロックは { } で囲まれる一連の処理ですが、始まりに対して終わりが見つからないエラーです。コードブロックや、付近の { の閉じ括弧を忘れていないか確認してください。"};
	const ScriptParseErrorData ERROR_AST_002 = { "A002", "演算子の使い方が正しくありません。", "2つの要素を足したり掛けたりするタイプの計算式で、足す側と足される側といったような２つの要素が揃っていないようです。"};
	const ScriptParseErrorData ERROR_AST_003 = { "A003", "カッコの対応関係が間違っています。", "計算式に使うカッコ ( ) の対応関係が正しくないようです。計算式を確認してください。"};
	const ScriptParseErrorData ERROR_AST_004 = { "A004", "演算子が必要か、ここでは使えない演算子です。", "値を2つ連続することはできません。足し合わせるなら + を使うなど、式にする必要があります。"};
	const ScriptParseErrorData ERROR_AST_005 = { "A005", "メンバ名を指定する必要があります。", "Value.Item のように、ピリオドはオブジェクトのメンバーを参照するために使用します。"};
	const ScriptParseErrorData ERROR_AST_006 = { "A006", "ここでセミコロン ; は使えません。", "この式ではセミコロン ; を使うことができません。機能の使い方が間違ってないか、確認してみてください。"};
	const ScriptParseErrorData ERROR_AST_007 = { "A007", "ここでコロン : は使えません。", "この式ではコロン : を使うことができません。機能の使い方が間違ってないか、確認してみてください。"};
	const ScriptParseErrorData ERROR_AST_008 = { "A008", "ここで閉じ括弧 } は使えません。", "この式では閉じ括弧 } を使うことができません。機能の使い方が間違ってないか、確認してみてください。"};
	const ScriptParseErrorData ERROR_AST_009 = { "A009", "ここで閉じ括弧 ] は使えません。", "この式では閉じ括弧 ] を使うことができません。機能の使い方が間違ってないか、確認してみてください。"};
	const ScriptParseErrorData ERROR_AST_010 = { "A010", "ここでカンマ , は使えません。", "この式ではカンマ , を使うことができません。機能の使い方が間違ってないか、確認してみてください。"};
	const ScriptParseErrorData ERROR_AST_011 = { "A011", "ここで閉じ括弧 ) は使えません。", "ここでは閉じ括弧 ) を使うことができません。機能の使い方が間違ってないか、確認してみてください。"};
	const ScriptParseErrorData ERROR_AST_012 = { "A012", "カッコの対応関係が正しくありません。", "リストの始端終端のカッコが揃っていないようです。"};
	const ScriptParseErrorData ERROR_AST_013 = { "A013", "引数名が必要です。", "ここは引数リストなので、引数名を書かないといけません"};
	const ScriptParseErrorData ERROR_AST_014 = { "A014", "カンマ , で引数を区切るか、閉じ括弧 ) で引数リストを閉じる必要があります。"};
	const ScriptParseErrorData ERROR_AST_015 = { "A015", "カッコの対応関係が正しくありません。", "引数リストが正しく閉じられていません。"};
	const ScriptParseErrorData ERROR_AST_016 = { "A016", "キーが必要です。", "連想配列の式では キー: 値　の形式で内容を記述します。キーが正しい形式ではないようです。"};
	const ScriptParseErrorData ERROR_AST_017 = { "A017", "コロン : が必要です。", "連想配列の式では キー: 値 の形式で内容を記述します。キーと値を分けるコロンが無いようです。"};
	const ScriptParseErrorData ERROR_AST_018 = { "A018", "開き括弧 { が必要です。", "関数式では { から関数本体を始める必要があります。"};
	const ScriptParseErrorData ERROR_AST_019 = { "A019", "開き括弧 { が必要です。", "トーク式では { からトーク本体を始める必要があります。"};
	const ScriptParseErrorData ERROR_AST_020 = { "A020", "関数名が必要です。", "関数定義ではfunction に続けて関数名が必要です。"};
	const ScriptParseErrorData ERROR_AST_021 = { "A021", "開き括弧 ( が必要です。", "関数定義の発生条件を使用する場合は if に続けて括弧 ( ) で条件を記述します。"};
	const ScriptParseErrorData ERROR_AST_022 = { "A022", "開き括弧 { が必要です。", "関数定義では { から関数本体を始める必要があります。"};
	const ScriptParseErrorData ERROR_AST_023 = { "A023", "書き込めない対象に書き込もうとしています。", "読み取り専用の情報に代入等で変更をしようとしています。"};
	const ScriptParseErrorData ERROR_AST_024 = { "A024", "newキーワードには呼出式 ( ) が必要です。", "new Class() のように、newキーワードには呼出式が必要です。"};
	const ScriptParseErrorData ERROR_AST_025 = { "A025", "クラス名が必要です。", "クラス定義では class キーワードに続いてクラス名を記述します。"};
	const ScriptParseErrorData ERROR_AST_026 = { "A026", "継承クラス名が必要です。", "クラスを継承する場合はクラス名の後のコロン : に続けて継承元のクラス名を記述します。"};
	const ScriptParseErrorData ERROR_AST_027 = { "A027", "開き括弧 { が必要です。", "クラス本体の記述前に始端の括弧 { が必要です。"};
	const ScriptParseErrorData ERROR_AST_028 = { "A028", "開き括弧 { が必要です。", "initの本体の前に始端の括弧 { が必要です。"};
	const ScriptParseErrorData ERROR_AST_029 = { "A029", "閉じ括弧 } が必要です。", "クラス終端の括弧 } が必要です。"};
	const ScriptParseErrorData ERROR_AST_030 = { "A030", "変数名が必要です。", "ここでは変数名が必要です。変数名に使用できないキーワードか記号が使われているかもしれません。"};
	const ScriptParseErrorData ERROR_AST_031 = { "A031", "開き括弧 ( が必要です。", "for文の開き括弧が必要です。"};
	const ScriptParseErrorData ERROR_AST_032 = { "A032", "開き括弧 ( が必要です。", "while文の開き括弧が必要です。"};
	const ScriptParseErrorData ERROR_AST_033 = { "A033", "開き括弧 ( が必要です。", "if文の開き括弧が必要です。"};
	const ScriptParseErrorData ERROR_AST_034 = { "A034", "セミコロン ; が必要です。", "break文の終わりにはセミコロンが必要です。"};
	const ScriptParseErrorData ERROR_AST_035 = { "A035", "セミコロン ; が必要です。", "continue文の終わりにはセミコロンが必要です。"};
	const ScriptParseErrorData ERROR_AST_036 = { "A036", "開き括弧 { が必要です。", "tryブロック始端には開き括弧 { が必要です。"};
	const ScriptParseErrorData ERROR_AST_037 = { "A037", "開き括弧 { が必要です。", "catchブロック始端には開き括弧 { が必要です。"};
	const ScriptParseErrorData ERROR_AST_038 = { "A038", "開き括弧 { が必要です。", "finallyブロック始端には開き括弧 { が必要です。"};
	const ScriptParseErrorData ERROR_AST_039 = { "A039", "セミコロン ; が必要です。", "変数宣言の終わりにはセミコロン ; が必要です。" };


	//四則演算
	const OperatorInformation OPERATOR_ADD = { OperatorType::Add, 6, 2, true, "+" };
	const OperatorInformation OPERATOR_SUB = { OperatorType::Sub, 6, 2, true, "-" };
	const OperatorInformation OPERATOR_MUL = { OperatorType::Mul, 5, 2, true, "*" };
	const OperatorInformation OPERATOR_DIV = { OperatorType::Div, 5, 2, true, "/" };
	const OperatorInformation OPERATOR_MOD = { OperatorType::Mod, 5, 2, true, "%" };

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
	const OperatorInformation OPERATOR_ASSIGN = { OperatorType::Assign, 15, 2, false, "="};
	const OperatorInformation OPERAOTR_ASSIGN_ADD = { OperatorType::AssignAdd, 15, 2, false, "+=" };
	const OperatorInformation OPERATOR_ASSIGN_SUB = { OperatorType::AssignSub, 15, 2, false, "-=" };
	const OperatorInformation OPERATOR_ASSIGN_MUL = { OperatorType::AssignMul, 15, 2, false, "*=" };
	const OperatorInformation OPERATOR_ASSIGN_DIV = { OperatorType::AssignDiv, 15, 2, false, "/=" };
	const OperatorInformation OPERATOR_ASSIGN_MOD = { OperatorType::AssignMod, 15, 2, false, "%=" };

	//式やステートメントの終端として認めるもののフラグ
	const uint32_t SEQUENCE_END_FLAG_COMMA = 1u << 0u;
	const uint32_t SEQUENCE_END_FLAG_BLOCK_BLACKET = 1u << 1u;
	const uint32_t SEQUENCE_END_FLAG_ARRAY_BLACKET = 1u << 3u;
	const uint32_t SEQUENCE_END_FLAG_BLACKET = 1u << 4u;
	const uint32_t SEQUENCE_END_FLAG_SEMICOLON = 1u << 5u;
	const uint32_t SEQUENCE_END_FLAG_COLON = 1u << 6u;
	const uint32_t SEQUENCE_END_FALG_TALK_NEWLINE = 1u << 7u;
	const uint32_t SEQUENCE_END_FLAG_VERTICAL_BAR = 1u << 8u;

	//ASTパース
	class ASTParseContext {
	private:
		ASTParseResult& result;
		const std::list<ScriptToken>& tokens;
		std::list<ScriptToken>::const_iterator current;

		//エラーが出ているかどうか、エラーがあればその場で解析を打ち切るので１つだけしか持たない
		bool hasError;
		ScriptParseErrorData errorData;
		const ScriptToken* errorToken;

	public:
		ASTParseContext(const std::list<ScriptToken>& tokenList, ASTParseResult& parseResult) :
			result(parseResult),
			tokens(tokenList),
			hasError(false),
			errorToken(nullptr)
		{
			//最初のアイテムをとる
			current = tokens.cbegin();
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

		//エラーのセット
		ASTNodeRef Error(const ScriptParseErrorData& error, const ScriptToken& token) {

#if defined(AOSORA_ENABLE_PARSE_ERROR_ASSERT)
			//デバッグのためエラーだったら即止め
			assert(false);
#endif
			//最初に発生したエラーだけを記録(エラーでパースが崩れたのにあわせてまたエラーになるのを回避するため)
			if (!hasError) {
				errorData = error;
				errorToken = &token;
				hasError = true;
			}

			//エラー情報を返してそのまま脱出させる
			return ASTNodeRef(new ASTError());
		}

		//次へ
		void FetchNext() {
			++current;
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

		const ScriptToken* GetErrorToken() const {
			return errorToken;
		}
	};

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
			assert(parseContext.GetErrorToken() != nullptr);
			parseResult->error.reset(new ScriptParseError(parseContext.GetErrorData(), parseContext.GetErrorToken()->sourceRange));
		}

		parseResult->root = codeBlock;

		return parseResult;
	}


	//ソースコードルートのパース
	ASTNodeRef ASTParser::ParseASTSourceRoot(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		std::shared_ptr<ASTNodeCodeBlock> result(new ASTNodeCodeBlock());

		while (!parseContext.IsEnd()) {

			//ASTノードとして扱わないクラスはルート空間のみということにしておく
			if (parseContext.GetCurrent().type == ScriptTokenType::Class) {
				ParseASTClass(parseContext);
				continue;
			}

			//所属しているステートメントをブロック終了までパースする
			auto node = ParseASTStatement(parseContext);
			result->AddStatement(node);
		}

		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//コードブロックのパース
	ASTNodeRef ASTParser::ParseASTCodeBlock(ASTParseContext& parseContext, bool isBlacketEnd) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		std::shared_ptr<ASTNodeCodeBlock> result(new ASTNodeCodeBlock());

		while (!parseContext.IsEnd()) {

			//閉じ括弧があれば終了
			if (isBlacketEnd && parseContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
				parseContext.FetchNext();
				result->SetSourceRange(beginToken, parseContext.GetPrev());
				return result;
			}

			//所属しているステートメントをブロック終了までパースする
			auto node = ParseASTStatement(parseContext);
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
		std::shared_ptr<ASTNodeCodeBlock> result(new ASTNodeCodeBlock());

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
				ASTNodeRef si = ASTNodeRef(new ASTNodeTalkSetSpeaker(std::stoi(parseContext.GetCurrent().body)));
				si->SetSourceRange(parseContext.GetCurrent());
				result->AddStatement(si);
				parseContext.FetchNext();

				//次に話者スイッチ命令が入ってるはずなのでこれを読み飛ばす
				assert(parseContext.GetCurrent().type == ScriptTokenType::SpeakerSwitch);
				parseContext.FetchNext();
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::SpeakerSwitch) {
				//直接speakerが来たら話者スイッチ
				ASTNodeRef sw = ASTNodeRef(new ASTNodeTalkSetSpeaker(ASTNodeTalkSetSpeaker::SPEAKER_INDEX_SWITCH));
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

				ASTNodeRef talkJump = ASTNodeRef(new ASTNodeTalkJump(callNode, args, condition));
				talkJump->SetSourceRange(jumpBeginToken, parseContext.GetPrev());
				result->AddStatement(talkJump);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::SpeakBegin) {
				//発話
				const ScriptToken& talkBegin = parseContext.GetCurrent();
				ASTNodeRef talkBody = ParseASTString(parseContext);
				ASTNodeRef talk = ASTNodeRef(new ASTNodeTalkSpeak(talkBody));

				talk->SetSourceRange(talkBegin, parseContext.GetPrev());
				result->AddStatement(talk);
			}
			else
			{
				//それ以外の場合は通常の関数内ステートメントとして処理
				ASTNodeRef r = ParseASTStatement(parseContext);
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
			}
		}
		
		return nullptr;
	}

	//ASTステートメントのパースへの振り分け
	ASTNodeRef ASTParser::ParseASTStatement(ASTParseContext& parseContext) {
		switch (parseContext.GetCurrent().type) {
		case ScriptTokenType::Local:
			return ParseASTLocalVariable(parseContext);
		case ScriptTokenType::Function:
			return ParseASTFunctionStatement(parseContext);
		case ScriptTokenType::Talk:
			return ParseASTTalkStatement(parseContext);
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
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_ADD, operandLeft, operandRight));
					break;
				case OperatorType::AssignSub:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_SUB, operandLeft, operandRight));
					break;
				case OperatorType::AssignMul:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_MUL, operandLeft, operandRight));
					break;
				case OperatorType::AssignDiv:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_DIV, operandLeft, operandRight));
					break;
				case OperatorType::AssignMod:
					operandRight = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_MOD, operandLeft, operandRight));
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

				expressionStack.push_back(StackItem(ASTNodeRef(new ASTNodeEvalOperator2(*operatorInfo, operandLeft, operandRight))));
			}
			else if (operatorInfo->argCount == 1) {

				//単項演算子、２つポップして演算ノードをプッシュ
				auto operand = expressionStack[expressionStack.size() - 1].operandNode;
				const auto* operatorInfo = expressionStack[expressionStack.size() - 2].operatorInfo;

				expressionStack.pop_back();
				expressionStack.pop_back();

				expressionStack.push_back(StackItem(ASTNodeRef(new ASTNodeEvalOperator1(*operatorInfo, operand))));
			}
			else {
				assert(false);
			}
		}

		//現在までに積まれた式を解決する
		void Reduce(const OperatorInformation& nextOperator, ASTParseContext& parseContext) {

			while (expressionStack.size() > 2) {
				const OperatorInformation* operatorInfo = expressionStack[expressionStack.size() - 2].operatorInfo;

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
			}

			return expressionStack[expressionStack.size() - 1].operandNode;
		}

		//スタックへのオペランド投入
		void PushOperand(const ASTNodeRef& operand) {
			expressionStack.push_back(StackItem(operand));
		}

		//オペランドをポップ
		ASTNodeRef PopOperand() {
			//オペランドじゃなかったらとりあえずアサートで
			assert(expressionStack[expressionStack.size() - 1].operandNode != nullptr);
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

			assert(false);	//ここには来ないはず
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
				switch (parseContext.GetCurrent().type) {
				case ScriptTokenType::Number:
				case ScriptTokenType::Symbol:
				case ScriptTokenType::StringBegin:
				case ScriptTokenType::BlockBegin:
				case ScriptTokenType::Function:
				case ScriptTokenType::Talk:
					return parseContext.Error(ERROR_AST_004, parseContext.GetCurrent());
				}
			}

			//それぞれのトークンを処理
			if (parseContext.GetCurrent().type == ScriptTokenType::Number) {
				parseStack.PushOperand(ParseASTNumberLiteral(parseContext));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::True) {
				ASTNodeRef node = ASTNodeRef(new ASTNodeBooleanLiteral(true));
				node->SetSourceRange(parseContext.GetCurrent());
				parseContext.FetchNext();
				parseStack.PushOperand(node);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::False) {
				ASTNodeRef node = ASTNodeRef(new ASTNodeBooleanLiteral(false));
				node->SetSourceRange(parseContext.GetCurrent());
				parseContext.FetchNext();
				parseStack.PushOperand(node);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::StringBegin) {
				parseStack.PushOperand(ParseASTString(parseContext));
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
					auto arrayExpression = parseStack.PopOperand();
					auto indexExpression = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_ARRAY_BLACKET);

					//アクセスノードを作成
					parseStack.PushOperand(ASTNodeRef(new ASTNodeResolveMember(arrayExpression, indexExpression)));
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
				ASTNodeRef keyNode(new ASTNodeStringLiteral(parseContext.GetCurrent().body));
				keyNode->SetSourceRange(parseContext.GetCurrent());
				parseContext.FetchNext();

				ASTNodeRef target = parseStack.PopOperand();
				parseStack.PushOperand(ASTNodeRef(new ASTNodeResolveMember(target, keyNode)));
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
					parseContext.FetchNext();
					continue;
				}
				parseContext.FetchNext();
				break;
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Increment && !isRequireOperand) {
				parseContext.FetchNext();

				//後置インクリメント
				auto operand = parseStack.PopOperand();

				//1を足すように構成
				auto literalNode = ASTNodeRef(new ASTNodeNumberLiteral(1.0));
				literalNode->SetSourceRange(parseContext.GetCurrent());

				auto addNode = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_ADD, operand, literalNode));
				addNode->SetSourceRange(parseContext.GetCurrent());

				parseStack.PushOperand(ParseASTSet(parseContext, operand, addNode));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Decrement) {
				parseContext.FetchNext();

				//後置デクリメント
				auto operand = parseStack.PopOperand();

				auto literalNode = ASTNodeRef(new ASTNodeNumberLiteral(1.0));
				literalNode->SetSourceRange(parseContext.GetCurrent());

				auto addNode = ASTNodeRef(new ASTNodeEvalOperator2(OPERATOR_SUB, operand, literalNode));
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

						ASTNodeRef func = parseStack.PopOperand();
						std::shared_ptr<ASTNodeFunctionCall> call(new ASTNodeFunctionCall(func, args));
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

		//出揃ったので最終的にまとめて終了
		return parseStack.ReduceAll(parseContext);
	}

	//カンマ区切りで任意の終端をもつ式をまとめる
	void ASTParser::ParseASTExpressionList(ASTParseContext& parseContext, std::vector<ConstASTNodeRef>& result, uint32_t sequenceEndFlags) {
		assert(!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COMMA));	//カンマ終了はおかしいはず

		result.clear();

		//からっぽの場合
		if (IsSequenceEnd(parseContext, sequenceEndFlags)) {
			parseContext.FetchNext();
			return;
		}

		//要素数だけループ
		while (!parseContext.IsEnd()) {
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
		assert(!CheckFlags(sequenceEndFlags, SEQUENCE_END_FLAG_COMMA));	//カンマ終了はおかしいはず

		result.clear();

		//からっぽの場合
		if (IsSequenceEnd(parseContext, sequenceEndFlags)) {
			//parseContext.FetchNext();
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
		assert(parseContext.GetCurrent().type == ScriptTokenType::Number);

		//単純に string to double をかける
		const number value = std::stod(parseContext.GetCurrent().body);
		auto r = ASTNodeRef(new ASTNodeNumberLiteral(value));
		r->SetSourceRange(parseContext.GetCurrent());
		parseContext.FetchNext();
		return r;
	}

	//文字列リテラル / フォーマット文字列
	ASTNodeRef ASTParser::ParseASTString(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		assert(parseContext.GetCurrent().type == ScriptTokenType::StringBegin || parseContext.GetCurrent().type == ScriptTokenType::SpeakBegin);
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
			ASTNodeRef r = ASTNodeRef(new ASTNodeFormatString(items));
			r->SetSourceRange(parseContext.GetCurrent());
			return r;
		}
	}

	//文字列リテラル
	ASTNodeRef ASTParser::ParseASTStringLiteral(ASTParseContext& parseContext) {
		assert(parseContext.GetCurrent().type == ScriptTokenType::String);
		std::string value = parseContext.GetCurrent().body;
		
		auto r = ASTNodeRef(new ASTNodeStringLiteral(value));
		r->SetSourceRange(parseContext.GetCurrent());
		parseContext.FetchNext();
		return r;
	}

	//シンボル解決
	ASTNodeRef ASTParser::ParseASTSymbol(ASTParseContext& parseContext) {
		assert(parseContext.GetCurrent().type == ScriptTokenType::Symbol);

		std::string name = parseContext.GetCurrent().body;
		auto r = ASTNodeRef(new ASTNodeResolveSymbol(name));
		r->SetSourceRange(parseContext.GetCurrent());
		parseContext.FetchNext();
		return r;
	}

	//配列イニシャライザ
	ASTNodeRef ASTParser::ParseASTArrayInitializer(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		assert(parseContext.GetCurrent().type == ScriptTokenType::ArrayBegin);
		parseContext.FetchNext();

		//各要素がexpressionとして存在していて、カンマもしくは終了カッコで閉じられるはずだ
		std::vector<ConstASTNodeRef> items;
		ParseASTExpressionList(parseContext, items, SEQUENCE_END_FLAG_ARRAY_BLACKET);
		auto r = ASTNodeRef(new ASTNodeArrayInitializer(items));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//オブジェクトイニシャライザ
	ASTNodeRef ASTParser::ParseASTObjectInitializer(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		assert(parseContext.GetCurrent().type == ScriptTokenType::BlockBegin);
		parseContext.FetchNext();

		std::vector<ASTNodeObjectInitializer::Item> items;

		//アイテム無しで終了してなければ次に進む
		if (parseContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
			//アイテムなし
			parseContext.FetchNext();
		}
		else {

			//javascriptの記法を参考に
			while (!parseContext.IsEnd()) {

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
		}

		auto r = ASTNodeRef(new ASTNodeObjectInitializer(items));
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
		}

		if (parseContext.GetCurrent().type == ScriptTokenType::Function) {
			parseContext.FetchNext();
		}

		std::vector<std::string> argList;

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
			parseContext.FetchNext();
		}

		//中括弧開
		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			return parseContext.Error(ERROR_AST_018, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//コードブロックを取得
		ASTNodeRef funcBody = ParseASTCodeBlock(parseContext, true);
		auto r = ASTNodeRef(new ASTNodeFunctionInitializer(ScriptFunctionRef(new ScriptFunction(funcBody, argList))));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//関数ステートメント function FuncName:conditions(val) { }
	ASTNodeRef ASTParser::ParseASTFunctionStatement(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		ScriptFunctionDef def = ParseFunctionDef(parseContext, BlockType::Function);
		if (def.func == nullptr) {
			return ASTNodeRef(new ASTError());
		}
		auto r = ASTNodeRef(new ASTNodeFunctionStatement(def.names, def.func, def.condition));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//トークイニシャライザ talk(val) { }
	ASTNodeRef ASTParser::ParseASTTalkInitializer(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		assert(parseContext.GetCurrent().type == ScriptTokenType::Talk);
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
		auto r = ASTNodeRef(new ASTNodeFunctionInitializer(ScriptFunctionRef(new ScriptFunction(funcbody, argList))));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//トークステートメント talk TalkName, TalkName2 : condition (args) {}
	ASTNodeRef ASTParser::ParseASTTalkStatement(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		assert(parseContext.GetCurrent().type == ScriptTokenType::Talk);

		ScriptFunctionDef def = ParseFunctionDef(parseContext, BlockType::Talk);
		if (def.func == nullptr) {
			return ASTNodeRef(new ASTError());
		}
		auto r = ASTNodeRef(new ASTNodeFunctionStatement(def.names, def.func, def.condition));
		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	ScriptFunctionDef ASTParser::ParseFunctionDef(ASTParseContext& parseContext, BlockType blockType) {
		assert(parseContext.GetCurrent().type == ScriptTokenType::Function || blockType != BlockType::Function);
		assert(parseContext.GetCurrent().type == ScriptTokenType::Talk || blockType != BlockType::Talk);
		parseContext.FetchNext();

		ScriptFunctionDef result;
		result.func = nullptr;

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
			return parseContext.Error(ERROR_AST_023, parseContext.GetCurrent());
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

		auto r = ASTNodeRef(new ASTNodeNewClassInstance(fc->GetFunctionNode(), fc->GetArgumentNodes()));

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
	void ASTParser::ParseASTClass(ASTParseContext& parseContext) {
		assert(parseContext.GetCurrent().type == ScriptTokenType::Class);
		parseContext.FetchNext();


		ScriptClassRef result(new ScriptClass());

		//クラス名
		if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
			parseContext.Error(ERROR_AST_025, parseContext.GetCurrent());
			return;
		}
		std::string className = parseContext.GetCurrent().body;
		result->SetName(className);
		parseContext.FetchNext();

		//継承
		if (parseContext.GetCurrent().type == ScriptTokenType::Colon) {
			parseContext.FetchNext();
			if (parseContext.GetCurrent().type != ScriptTokenType::Symbol) {
				parseContext.Error(ERROR_AST_026, parseContext.GetCurrent());
				return;
			}
			result->SetParentClassName(parseContext.GetCurrent().body);
			parseContext.FetchNext();
		}

		//開カッコ
		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			parseContext.Error(ERROR_AST_027, parseContext.GetCurrent());
			return;
		}
		parseContext.FetchNext();

		//メンバを確認
		while (!parseContext.IsEnd()) {

			if (parseContext.GetCurrent().type == ScriptTokenType::BlockEnd) {
				//終了
				parseContext.FetchNext();
				parseContext.AddClass(result);
				return;
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
				}

				//開き中括弧
				if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
					parseContext.Error(ERROR_AST_028, parseContext.GetCurrent());
					return;
				}

				//関数本体
				parseContext.FetchNext();
				ASTNodeRef initBody = ParseASTCodeBlock(parseContext, true);

				result->SetInitFunc(ScriptFunctionRef(new ScriptFunction(initBody, argList)));
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Member) {
				//メンバ変数
				parseContext.FetchNext();

				//ローカル変数のようなスタイル
				std::vector<ScriptVariableDef> defs;
				if (!ParseVariableDef(parseContext, defs)) {
					return;
				}

				//メンバ変数をリストに登録
				for (const ScriptVariableDef& d : defs) {
					result->AddMember(d.name, d.initializer);
				}
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Function) {
				//メンバ関数
				ScriptFunctionDef def = ParseFunctionDef(parseContext, BlockType::Function);
				if (def.func == nullptr) {
					return;
				}

				result->AddFunction(def);
			}
			else if (parseContext.GetCurrent().type == ScriptTokenType::Talk) {
				//メンバトーク
				ScriptFunctionDef def = ParseFunctionDef(parseContext, BlockType::Talk);
				if (def.func == nullptr) {
					return;
				}

				result->AddFunction(def);
			}
			else {
				assert(false);
			}
		}

		if (!parseContext.HasError()) {
			parseContext.Error(ERROR_AST_029, parseContext.GetCurrent());
		}
	}

	//ローカル変数宣言のパース
	ASTNodeRef ASTParser::ParseASTLocalVariable(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//まずlocalを読み飛ばす
		assert(parseContext.GetCurrent().type == ScriptTokenType::Local);
		parseContext.FetchNext();

		//結果リスト
		std::shared_ptr<ASTNodeLocalVariableDeclarationList> result(new ASTNodeLocalVariableDeclarationList());
		std::vector<ScriptVariableDef> defs;
		
		if (!ParseVariableDef(parseContext, defs)) {
			return ASTNodeRef(new ASTError());
		}

		for (size_t i = 0; i < defs.size(); i++) {
			auto val = std::shared_ptr<ASTNodeLocalVariableDeclaration>(new ASTNodeLocalVariableDeclaration(defs[i].name, defs[i].initializer));
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
					//セミコロンでもカンマでもないとエラー。ここは来ないはず
					assert(false);
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

	//for文
	ASTNodeRef ASTParser::ParseASTFor(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		assert(parseContext.GetCurrent().type == ScriptTokenType::For);
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
			loopStatement = ParseASTStatement(parseContext);
		}

		ASTNodeRef result(new ASTNodeFor(initExpression, ifExpression, incrementExpression, loopStatement));
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//while文
	ASTNodeRef ASTParser::ParseASTWhile(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		assert(parseContext.GetCurrent().type == ScriptTokenType::While);
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
			trueStatement = ParseASTStatement(parseContext);
		}

		ASTNodeRef result(new ASTNodeWhile(ifExpression, trueStatement));
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//if文のパース
	ASTNodeRef ASTParser::ParseASTIf(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		//条件文、trueブロック、falseブロックがあるはず
		assert(parseContext.GetCurrent().type == ScriptTokenType::If);
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
			trueStatement = ParseASTStatement(parseContext);
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
				falseStatement = ParseASTStatement(parseContext);
			}
			r = ASTNodeRef(new ASTNodeIf(ifExpression, trueStatement, falseStatement));
		}
		else {
			//elseなし
			r = ASTNodeRef(new ASTNodeIf(ifExpression, trueStatement));
		}

		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//break文
	ASTNodeRef ASTParser::ParseASTBreak(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();
		
		assert(parseContext.GetCurrent().type == ScriptTokenType::Break);
		parseContext.FetchNext();

		//セミコロンが必要
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			return parseContext.Error(ERROR_AST_034, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		ASTNodeRef result(new ASTNodeBreak());
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//continue文
	ASTNodeRef ASTParser::ParseASTContinue(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		assert(parseContext.GetCurrent().type == ScriptTokenType::Continue);
		parseContext.FetchNext();

		//セミコロンが必要
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			return parseContext.Error(ERROR_AST_035, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		ASTNodeRef result(new ASTNodeBreak());
		result->SetSourceRange(beginToken, parseContext.GetPrev());
		return result;
	}

	//return文のパース
	ASTNodeRef ASTParser::ParseASTReturn(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		assert(parseContext.GetCurrent().type == ScriptTokenType::Return);
		parseContext.FetchNext();

		ASTNodeRef r;
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			//式があってセミコロンで終了な単純系のはず
			ASTNodeRef returnValueNode = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_SEMICOLON);
			r = ASTNodeRef(new ASTNodeReturn(returnValueNode));
		}
		else {
			//戻り値なし
			parseContext.FetchNext();
			r = ASTNodeRef(new ASTNodeReturn());
		}

		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//try
	ASTNodeRef ASTParser::ParseASTTry(ASTParseContext& parseContext) {
		const ScriptToken& beginToken = parseContext.GetCurrent();

		assert(parseContext.GetCurrent().type == ScriptTokenType::Try);
		parseContext.FetchNext();

		if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
			return parseContext.Error(ERROR_AST_036, parseContext.GetCurrent());
		}
		parseContext.FetchNext();

		//tryブロック
		ConstASTNodeRef tryBlock = ParseASTCodeBlock(parseContext, true);
		std::shared_ptr<ASTNodeTry> resultNode = std::shared_ptr<ASTNodeTry>(new ASTNodeTry(tryBlock));

		//catch
		while (parseContext.GetCurrent().type == ScriptTokenType::Catch) {
			parseContext.FetchNext();
			
			std::vector<std::string> catchClasses;
			while (true) {

				//catch型シンボルをチェック
				if (parseContext.GetCurrent().type == ScriptTokenType::Symbol) {

					catchClasses.push_back(parseContext.GetCurrent().body);
					parseContext.FetchNext();

					//カンマで複数指定想定
					if (parseContext.GetCurrent().type == ScriptTokenType::Comma) {
						parseContext.FetchNext();
						continue;
					}
				}
				break;
			}

			//開カッコ
			if (parseContext.GetCurrent().type != ScriptTokenType::BlockBegin) {
				return parseContext.Error(ERROR_AST_037, parseContext.GetCurrent());
			}
			parseContext.FetchNext();

			//catchブロック
			ConstASTNodeRef catchBlock = ParseASTCodeBlock(parseContext, true);
			resultNode->AddCatchBlock(catchBlock, catchClasses);
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
		parseContext.FetchNext();

		ASTNodeRef r;
		if (parseContext.GetCurrent().type != ScriptTokenType::Semicolon) {
			//式があってセミコロンで終了な単純系のはず
			ASTNodeRef throwValueNode = ParseASTExpression(parseContext, SEQUENCE_END_FLAG_SEMICOLON);
			r = ASTNodeRef(new ASTNodeThrow(throwValueNode));
		}
		else {
			//戻り値なし
			parseContext.FetchNext();
			r = ASTNodeRef(new ASTNodeThrow());
		}

		r->SetSourceRange(beginToken, parseContext.GetPrev());
		return r;
	}

	//ResolveSymbol -> AssignSymbol
	ASTNodeRef ASTNodeResolveSymbol::ConvertToSetter(const ASTNodeRef& valueNode) const {
		return ASTNodeRef(new ASTNodeAssignSymbol(name, valueNode));
	}

	//ResolveMember -> AssignMember
	ASTNodeRef ASTNodeResolveMember::ConvertToSetter(const ASTNodeRef& valueNode) const {
		return ASTNodeRef(new ASTNodeAssignMember(target, key, valueNode));
	}

	

}
