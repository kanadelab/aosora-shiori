#include "Misc/Utility.h"
#include "Misc/Message.h"
#include "Tokens/TokenParser.h"
#include <cassert>

namespace sakura {

	const std::string TOKEN_CRLF = "\r\n";
	const std::string TOKEN_LF = "\n";
	const std::string TOKEN_SPACE = " ";
	const std::string TOKEN_TAB = "\t";
	const std::string TOKEN_STRING = "\"";
	const std::string TOKEN_STRING2 = "`";		//バッククォートも " と同じ文字列として使用できる
	const std::string TOKEN_RAW_STRING = "'";

	const std::string TOKEN_OPERATOR_PLUS = "+";
	const std::string TOKEN_OPERATOR_MINUS = "-";
	const std::string TOKEN_OPERATOR_ASTERISK = "*";
	const std::string TOKEN_OPERATOR_SLASH = "/";
	const std::string TOKEN_OPERATOR_EQUAL = "=";
	const std::string TOKEN_OPERATOR_PERCENT = "%";
	
	const std::string TOKEN_OPERATOR_ASSIGN_ADD = "+=";
	const std::string TOKEN_OPERATOR_ASSIGN_SUB = "-=";
	const std::string TOKEN_OPERATOR_ASSIGN_MUL = "*=";
	const std::string TOKEN_OPERATOR_ASSIGN_DIV = "/=";
	const std::string TOKEN_OPERATOR_ASSIGN_MOD = "%=";

	const std::string TOKEN_OPERATOR_INCREMENT = "++";
	const std::string TOKEN_OPERATOR_DECREMENT = "--";

	const std::string TOKEN_LOGICAL_OPERATOR_AND = "&&";
	const std::string TOKEN_LOGICAL_OPERATOR_OR = "||";
	const std::string TOKEN_LOGICAL_OPERATOR_NOT = "!";

	const std::string TOKEN_RELATIONAL_OPERATOR_EQ = "==";
	const std::string TOKEN_RELATIONAL_OPERATOR_NE = "!=";
	const std::string TOKEN_RELATIONAL_OPERATOR_GT = ">";
	const std::string TOKEN_RELATIONAL_OPERATOR_LT = "<";
	const std::string TOKEN_RELATIONAL_OPERATOR_GE = ">=";
	const std::string TOKEN_RELATIONAL_OPERATOR_LE = "<=";

	const std::string TOKEN_COMMON_DOT = ".";
	const std::string TOKEN_COMMON_COMMA = ",";
	const std::string TOKEN_COMMON_SEMICOLON = ";";
	const std::string TOKEN_COMMON_COLON = ":";
	const std::string TOKEN_COMMON_BRACKET_BEGIN = "(";
	const std::string TOKEN_COMMON_BRACKET_END = ")";
	const std::string TOKEN_COMMON_VERTICAL_BAR = "|";

	const std::string TOKEN_KEYWORD_TALK = "talk";
	const std::string TOKEN_KEYWORD_WORD = "word";
	const std::string TOKEN_KEYWORD_FUNCTION = "function";

	const std::string TOKEN_KEYWORD_FOR = "for";
	const std::string TOKEN_KEYWORD_WHILE = "while";
	const std::string TOKEN_KEYWORD_IF = "if";
	const std::string TOKEN_KEYWORD_ELSE = "else";
	const std::string TOKEN_KEYWORD_BREAK = "break";
	const std::string TOKEN_KEYWORD_CONTINUE = "continue";
	const std::string TOKEN_KEYWORD_RETURN = "return";
	const std::string TOKEN_KEYWORD_LOCAL = "local";
	const std::string TOKEN_KEYWORD_CONST = "const";
	const std::string TOKEN_KEYWORD_TRUE = "true";
	const std::string TOKEN_KEYWORD_FALSE = "false";
	const std::string TOKEN_KEYWORD_TRY = "try";
	const std::string TOKEN_KEYWORD_CATCH = "catch";
	const std::string TOKEN_KEYWORD_FINALLY = "finally";
	const std::string TOKEN_KEYWORD_THROW = "throw";
	const std::string TOKEN_KEYWORD_CLASS = "class";
	const std::string TOKEN_KEYWORD_MEMBER = "member";
	const std::string TOKEN_KEYWORD_INIT = "init";
	const std::string TOKEN_KEYWORD_NEW = "new";

	const std::string TOKEN_BLOCK_BEGIN = "{";
	const std::string TOKEN_BLOCK_END = "}";

	const std::string JSON_ARRAY_BEGIN_PATTERN = "[";
	const std::string JSON_ARRAY_END_PATTERN = "]";

	const std::string TOKEN_FORMAT_BEGIN = "{";
	const std::string TOKEN_FUNCSCOPE_BEGIN = "%{";

	const std::string TOKEN_TALK_FUNC_LINE = "$";
	const std::string TOKEN_TALK_JUMP_LINE = ">";
	const std::string TOKEN_TALK_SCOPE_SELECTOR = ":";
	const std::string TOKEN_TALK_SCOPE_SELECTOR2 = "::";

	const std::string TOKEN_COMMENT_LINE = "//";
	const std::string TOKEN_COMMENT_BLOCK_BEGIN = "/*";
	const std::string TOKEN_COMMENT_BLOCK_END = "*/";

	struct ScriptTokenSet
	{
		const std::string& str;
		ScriptTokenType type;
	};

	//一般的な固定トークン
	//線形に比較するので文字数の多いものを手前に置く必要あり
	const ScriptTokenSet TOKEN_BASICS[] = {

		{TOKEN_LOGICAL_OPERATOR_AND, ScriptTokenType::LogicalAnd},
		{TOKEN_LOGICAL_OPERATOR_OR, ScriptTokenType::LogicalOr},

		{TOKEN_RELATIONAL_OPERATOR_EQ, ScriptTokenType::RelationalEq},
		{TOKEN_RELATIONAL_OPERATOR_NE, ScriptTokenType::RelationalNe},
		{TOKEN_RELATIONAL_OPERATOR_GE, ScriptTokenType::RelationalGe},
		{TOKEN_RELATIONAL_OPERATOR_LE, ScriptTokenType::RelationalLe},
		{TOKEN_RELATIONAL_OPERATOR_GT, ScriptTokenType::RelationalGt},
		{TOKEN_RELATIONAL_OPERATOR_LT, ScriptTokenType::RelationalLt},

		{TOKEN_OPERATOR_ASSIGN_ADD, ScriptTokenType::AssignAdd},
		{TOKEN_OPERATOR_ASSIGN_SUB, ScriptTokenType::AssignSub},
		{TOKEN_OPERATOR_ASSIGN_MUL, ScriptTokenType::AssignMul},
		{TOKEN_OPERATOR_ASSIGN_DIV, ScriptTokenType::AssignDiv},
		{TOKEN_OPERATOR_ASSIGN_MOD, ScriptTokenType::AssignMod},

		{TOKEN_OPERATOR_INCREMENT, ScriptTokenType::Increment},
		{TOKEN_OPERATOR_DECREMENT, ScriptTokenType::Decrement},

		{TOKEN_LOGICAL_OPERATOR_NOT, ScriptTokenType::LogicalNot},
		{TOKEN_OPERATOR_PLUS, ScriptTokenType::Plus},
		{TOKEN_OPERATOR_MINUS, ScriptTokenType::Minus},
		{TOKEN_OPERATOR_ASTERISK, ScriptTokenType::Asterisk},
		{TOKEN_OPERATOR_SLASH, ScriptTokenType::Slash},
		{TOKEN_OPERATOR_PERCENT, ScriptTokenType::Percent},
		{TOKEN_OPERATOR_EQUAL, ScriptTokenType::Equal},

		{TOKEN_COMMON_DOT, ScriptTokenType::Dot},
		{TOKEN_COMMON_COMMA, ScriptTokenType::Comma},
		{TOKEN_COMMON_SEMICOLON, ScriptTokenType::Semicolon},
		{TOKEN_COMMON_COLON, ScriptTokenType::Colon},
		{TOKEN_COMMON_BRACKET_BEGIN, ScriptTokenType::BracketBegin},
		{TOKEN_COMMON_BRACKET_END, ScriptTokenType::BracketEnd},
		{TOKEN_COMMON_VERTICAL_BAR, ScriptTokenType::VerticalBar},

		{TOKEN_BLOCK_BEGIN, ScriptTokenType::BlockBegin},
		{TOKEN_BLOCK_END, ScriptTokenType::BlockEnd},
		{JSON_ARRAY_BEGIN_PATTERN, ScriptTokenType::ArrayBegin},
		{JSON_ARRAY_END_PATTERN, ScriptTokenType::ArrayEnd}
	};

	//キーワードトークン
	//固定トークンと異なり、シンボル名として取り出された範囲で一致する必要がある
	const ScriptTokenSet TOKEN_KEYWORDS[] = {
		{TOKEN_KEYWORD_FUNCTION, ScriptTokenType::Function},
		{TOKEN_KEYWORD_FOR, ScriptTokenType::For},
		{TOKEN_KEYWORD_WHILE, ScriptTokenType::While},
		{TOKEN_KEYWORD_IF, ScriptTokenType::If},
		{TOKEN_KEYWORD_ELSE, ScriptTokenType::Else},
		{TOKEN_KEYWORD_BREAK, ScriptTokenType::Break},
		{TOKEN_KEYWORD_CONTINUE, ScriptTokenType::Continue},
		{TOKEN_KEYWORD_RETURN, ScriptTokenType::Return},
		{TOKEN_KEYWORD_LOCAL, ScriptTokenType::Local},
		{TOKEN_KEYWORD_CONST, ScriptTokenType::Const},
		{TOKEN_KEYWORD_TRUE, ScriptTokenType::True},
		{TOKEN_KEYWORD_FALSE, ScriptTokenType::False},
		{TOKEN_KEYWORD_TRY, ScriptTokenType::Try},
		{TOKEN_KEYWORD_CATCH, ScriptTokenType::Catch},
		{TOKEN_KEYWORD_FINALLY, ScriptTokenType::Finally},
		{TOKEN_KEYWORD_THROW, ScriptTokenType::Throw},
		{TOKEN_KEYWORD_CLASS, ScriptTokenType::Class},
		{TOKEN_KEYWORD_MEMBER, ScriptTokenType::Member},
		{TOKEN_KEYWORD_INIT, ScriptTokenType::Init},
		{TOKEN_KEYWORD_NEW, ScriptTokenType::New},
	};

	const std::regex JSON_NUMBER_PATTERN(R"((^[0-9.]+))");
	const std::regex TOKEN_SYMBOL_PATTERN(R"((^[^0-9\!\"\#\$\%\&\'\(\)\*\+\,\-\.\/\<\=\>\?\^\`\{\}\[\]\|\~\\:;\s][^\!\"\#\$\%\&\'\(\)\*\+\,\-\.\/\<\=\>\?\^\`\{\}\[\]\|\~\\:;\s]*))");
	const std::regex TOKEN_TALK_SCOPE_PATTERN(R"(^(\d*)(\:{1,2}))");

	//先頭にだけマッチさせ不要な処理を行わないよう明示的に指定するフラグ
	const auto TOKEN_MATCH_FLAGS = std::regex_constants::match_continuous | std::regex_constants::format_first_only | std::regex_constants::format_no_copy;

	//解析ブロックの終了条件
	const uint32_t BLOCK_END_FLAG_BLOCK_BRACKET = 1u << 0;			// }
	const uint32_t BLOCK_END_FLAG_BRACKET = 1u << 1;				// )
	const uint32_t BLOCK_END_FLAG_NEWLINE = 1u << 2;				// 改行
	const uint32_t BLOCK_END_FLAG_DOUBLE_QUATATION = 1u << 3;		// "
	const uint32_t BLOCK_END_FLAG_SINGLE_QUATATION = 1u << 4;		// '
	const uint32_t BLOCK_END_FLAG_BACK_QUATATION = 1u << 5;			// `

	//トークン解析エラー
	const std::string ERROR_TOKEN_001 = "T001";
	const std::string ERROR_TOKEN_002 = "T002";
	const std::string ERROR_TOKEN_003 = "T003";
	const std::string ERROR_TOKEN_004 = "T004";
	const std::string ERROR_TOKEN_005 = "T005";

	//トークン解析用情報
	class ScriptTokenParseContext {

	private:
		TokensParseResult& result;
		const std::string& document;
		std::shared_ptr<const std::string> sourceFilePath;

		bool hasError;
		ScriptParseErrorData errorData;
		SourceCodeRange errorPosition;

		//NOTE: 個別に位置をもたずにcharindexからline&columnを決定できるような事前解析があったほうがいいのかも
		size_t charIndex;
		size_t lineIndex;
		size_t columnIndex;

		//改行をスキップ
		bool SkipNewLine(std::string_view current) {
			if (current.starts_with(TOKEN_CRLF)) {
				IncrementLine(TOKEN_CRLF.size());
				return true;
			}
			else if (current.starts_with(TOKEN_LF)) {
				IncrementLine(TOKEN_LF.size());
				return true;
			}
			return false;
		}

		//行コメントをスキップ
		void SkipLineComment() {
			while (!IsEOF()) {
				std::string_view current = GetCurrent();

				if (SkipNewLine(current)) {
					//改行で終了
					return;
				}
				else {
					//コメントなので読み飛ばす
					SeekChar(1);
				}
			}
		}

		//ブロックコメントをスキップ
		void SkipBlockComment() {
			while (!IsEOF()) {
				std::string_view current = GetCurrent();

				if (SkipNewLine(current)) {
					//行を数えるため改行を処理
				}
				else if(current.starts_with(TOKEN_COMMENT_BLOCK_END)) {
					//コメント終端まで読み飛ばして終了
					SeekChar(TOKEN_COMMENT_BLOCK_END.size());
					return;
				}
				else {
					//コメントなので読み飛ばす
					SeekChar(1);
				}
			}
		}

	public:

		ScriptTokenParseContext(const std::string& doc, const std::string& sourceFilePath, TokensParseResult& parseResult) :
			result(parseResult),
			document(doc),
			sourceFilePath(new std::string(sourceFilePath)),
			hasError(false),
			charIndex(0),
			lineIndex(0),
			columnIndex(0)
		{}

		//改行
		void IncrementLine(size_t charCount) {
			lineIndex++;
			charIndex += charCount;	//CRLFかLFかで異なる為
			columnIndex = 0;
		}

		//スペース
		void SeekChar(size_t charCount) {
			charIndex += charCount;
			columnIndex += charCount;
		}

		//トークンのプッシュ
		void PushToken(size_t size, ScriptTokenType type) {
			//トークンを追加
			result.tokens.push_back(ScriptToken(document.substr(charIndex, size), type, sourceFilePath, lineIndex, columnIndex, lineIndex, columnIndex+size));	//UTF-8文字数を考える必要がありそう

			//文字数を加算
			charIndex += size;
			columnIndex += size;
		}

		//最後にプッシュしたトークンに追記
		void AppendLastToken(size_t size) {
			//トークンを追加
			result.tokens.rbegin()->body.append(document.substr(charIndex, size));
			//文字数を加算
			charIndex += size;
			columnIndex += size;
		}

		void AppendLastToken(const std::string_view& view, size_t size) {
			result.tokens.rbegin()->body.append(view.substr(size));
		}

		//スペースと改行ぶんのシーク
		//trueで改行があればtrueで帰る
		//コメントも飛ばす
		bool SkipSpaces(bool breakNewLine) {
			while (!IsEOF()) {
				std::string_view current = GetCurrent();

				if (current.starts_with(TOKEN_SPACE)) {
					SeekChar(TOKEN_SPACE.size());
				}
				else if (current.starts_with(TOKEN_TAB)) {
					SeekChar(TOKEN_TAB.size());
				}
				else if (SkipNewLine(current)) {
					if (breakNewLine) {
						return true;
					}
				}
				else if (current.starts_with(TOKEN_COMMENT_LINE)) {
					// "//" 行コメント
					SeekChar(TOKEN_COMMENT_LINE.size());
					SkipLineComment();
				}
				else if (current.starts_with(TOKEN_COMMENT_BLOCK_BEGIN)) {
					// "/*" ブロックコメント
					SeekChar(TOKEN_COMMENT_BLOCK_BEGIN.size());
					SkipBlockComment();
				}
				else {
					return false;
				}
			}
			return false;
		}

		//インデントぶんのスキップ
		void SkipIndent() {
			while (!IsEOF()) {
				std::string_view current = GetCurrent();

				if (current.starts_with(TOKEN_SPACE)) {
					SeekChar(TOKEN_SPACE.size());
				}
				else if (current.starts_with(TOKEN_TAB)) {
					SeekChar(TOKEN_TAB.size());
				}
				else {
					return;
				}
			}
		}

		//文字を取得
		const std::string_view GetCurrent() {
			return std::string_view(&document[charIndex], document.size() - charIndex);
		}

		//ソースコード位置を取得
		const SourceCodeRange GetCurrentPosition() {
			return SourceCodeRange(sourceFilePath, lineIndex, columnIndex, lineIndex, columnIndex);
		}

		//終了?
		bool IsEOF() const {
			return charIndex >= document.size();
		}

		//打ち切るべきか
		bool IsEnd() const {
			return IsEOF() || HasError();
		}

		//エラー情報指定
		void Error(const std::string& errorCode, const SourceCodeRange& position) {
#if 0
			//デバッグのためエラーだったら即止め
			assert(false);
#endif

			errorData.errorCode = errorCode;
			errorData.message = TextSystem::Find(std::string("ERROR_MESSAGE") + errorCode);
			errorData.hint = TextSystem::Find(std::string("ERROR_HINT") + errorCode);

			errorPosition = position;
			hasError = true;
		}

		bool HasError() const {
			return hasError;
		}

		ScriptParseError GetError() const {
			return ScriptParseError(errorData, errorPosition);
		}

		//デバッグダンプ
		void DebugDumpTokens() {
			for (const ScriptToken& token : result.tokens) {
				printf("[%d] %s\n", token.type, token.body.c_str());
			}
		}
	};

	//トークンパースエントリポイント
	std::shared_ptr<const TokensParseResult> TokensParser::Parse(const std::string& document, const std::string& filePath) {
		std::shared_ptr<TokensParseResult> result(new TokensParseResult());
		ScriptTokenParseContext parseContext(document, filePath, *result);

		//パース
		TokensParser::ParseFunctionBlock(parseContext, 0);
		result->success = !parseContext.HasError();
		if (!result->success) {
			result->error.reset(new ScriptParseError(parseContext.GetError()));
		}

#if 0
		printf("---tokens---\n");
		parseContext.DebugDumpTokens();
#endif
		
		return result;
	}

	//トークン解析実装
	//関数ブロックの解析
	void TokensParser::ParseFunctionBlock(ScriptTokenParseContext& parseContext, uint32_t blockEndFlags) {

		int32_t blockDepth = 0;
		int32_t bracketDepth = 0;

		while (!parseContext.IsEnd())
		{
			//スペースを飛ばして次のトークンを探す
			if (parseContext.SkipSpaces(CheckFlags(blockEndFlags, BLOCK_END_FLAG_NEWLINE))) {
				return;
			}

			if (parseContext.IsEnd()) {
				break;
			}

			//演算子の処理
			{
				bool isHit = false;
				std::string_view current = parseContext.GetCurrent();
				for (const auto& op : TOKEN_BASICS) {
					if (current.starts_with(op.str)) {
						parseContext.PushToken(op.str.size(), op.type);

						//終了条件の確認
						if (op.type == ScriptTokenType::BlockBegin) {
							blockDepth++;
						}
						else if (op.type == ScriptTokenType::BlockEnd) {
							if (blockDepth <= 0) {
								if (CheckFlags(blockEndFlags, BLOCK_END_FLAG_BLOCK_BRACKET)) {
									//ここで解析終了
									return;
								}
								else {
									//中括弧の対応条件がおかしい、ここで報告して終えてもいいかも？
									parseContext.Error(ERROR_TOKEN_001, parseContext.GetCurrentPosition());
									return;
								}
							}
							blockDepth--;
						}
						else if (op.type == ScriptTokenType::BracketBegin) {
							bracketDepth++;
						}
						else if (op.type == ScriptTokenType::BracketEnd) {
							if (bracketDepth <= 0) {
								if (CheckFlags(blockEndFlags, BLOCK_END_FLAG_BRACKET)) {
									//解析終了
									return;
								}
							}
							bracketDepth--;
						}

						isHit = true;
						break;
					}
				}

				if (isHit) {
					//ヒットしたら最初から
					continue;
				}
			}

			//数値
			{
				std::match_results<std::string_view::const_iterator> match;
				if (std::regex_search(parseContext.GetCurrent().begin(), parseContext.GetCurrent().end(), match, JSON_NUMBER_PATTERN, TOKEN_MATCH_FLAGS)) {
					parseContext.PushToken(match[1].str().size(), ScriptTokenType::Number);
					continue;
				}
			}

			//文字列
			{
				if (parseContext.GetCurrent().starts_with(TOKEN_STRING)) {
					parseContext.SeekChar(TOKEN_STRING.size());

					//文字列解析に飛ばす
					parseContext.PushToken(0, ScriptTokenType::StringBegin);
					ParseStringLiteral(parseContext, BLOCK_END_FLAG_DOUBLE_QUATATION, false);
					if (parseContext.IsEnd()) {
						return;
					}
					parseContext.PushToken(0, ScriptTokenType::StringEnd);
					continue;
				}
			}

			//文字列（バッククォート）
			{
				if (parseContext.GetCurrent().starts_with(TOKEN_STRING2)) {
					parseContext.SeekChar(TOKEN_STRING2.size());

					//文字列解析に飛ばす
					parseContext.PushToken(0, ScriptTokenType::StringBegin);
					ParseStringLiteral(parseContext, BLOCK_END_FLAG_BACK_QUATATION, false);
					if (parseContext.IsEnd()) {
						return;
					}
					parseContext.PushToken(0, ScriptTokenType::StringEnd);
					continue;
				}
			}

			//raw文字列
			{
				if (parseContext.GetCurrent().starts_with(TOKEN_RAW_STRING)) {
					parseContext.SeekChar(TOKEN_RAW_STRING.size());

					//文字列解析に飛ばす
					parseContext.PushToken(0, ScriptTokenType::StringBegin);
					ParseStringLiteral(parseContext, BLOCK_END_FLAG_SINGLE_QUATATION, true);
					if (parseContext.IsEnd()) {
						return;
					}
					parseContext.PushToken(0, ScriptTokenType::StringEnd);
					continue;
				}
			}

			//シンボルを取り出す
			std::string symbol = "";
			{
				std::match_results<std::string_view::const_iterator> match;
				if (std::regex_search(parseContext.GetCurrent().begin(), parseContext.GetCurrent().end(), match, TOKEN_SYMBOL_PATTERN, TOKEN_MATCH_FLAGS)) {
					symbol = match[1].str();
					
					//シンボルの先頭にマッチしてない場合は無効
					//TODO: １行目だけを対象に取る方法を確認
					if (!parseContext.GetCurrent().starts_with(symbol)) {
						symbol = "";
					}
				}
			}

			//キーワードの処理
			{
				bool isHit = false;
				if (!symbol.empty()) {
					for (const auto& op : TOKEN_KEYWORDS) {

						//キーワードは完全一致
						if (op.str == symbol) {
							parseContext.PushToken(op.str.size(), op.type);
							isHit = true;
							break;
						}
					}
				}

				if (isHit) {
					//ヒットしたら最初から
					continue;
				}
			}

			//トーク
			{
				if (symbol == TOKEN_KEYWORD_TALK) {

					//キーワード
					parseContext.PushToken(TOKEN_KEYWORD_TALK.size(), ScriptTokenType::Talk);

					
					while (true){

						//スペース飛ばして
						parseContext.SkipSpaces(false);

						//シンボルを確認
						std::match_results<std::string_view::const_iterator> match;
						if (std::regex_search(parseContext.GetCurrent().begin(), parseContext.GetCurrent().end(), match, TOKEN_SYMBOL_PATTERN, TOKEN_MATCH_FLAGS)) {
							parseContext.PushToken(match[1].str().size(), ScriptTokenType::Symbol);
						}
						else {
							parseContext.Error(ERROR_TOKEN_002, parseContext.GetCurrentPosition());
							return;
						}

						//スペース飛ばして
						parseContext.SkipSpaces(false);

						//カンマなら連結
						if (parseContext.GetCurrent().starts_with(TOKEN_COMMON_COMMA)) {
							parseContext.PushToken(TOKEN_COMMON_COMMA.size(), ScriptTokenType::Comma);
						}
						else {
							break;
						}
					}

					//スペース飛ばして
					parseContext.SkipSpaces(false);

					if (parseContext.GetCurrent().starts_with(TOKEN_COMMON_BRACKET_BEGIN)) {
						//開始カッコがあれば引数リスト
						parseContext.PushToken(TOKEN_COMMON_BRACKET_BEGIN.size(), ScriptTokenType::BracketBegin);
						ParseFunctionBlock(parseContext, BLOCK_END_FLAG_BRACKET);
						if (parseContext.IsEnd()) {
							return;
						}

						//スペース飛ばして
						parseContext.SkipSpaces(false);
					}

					//ifなら条件部をフェッチ
					if (parseContext.GetCurrent().starts_with(TOKEN_KEYWORD_IF)) {
						parseContext.PushToken(TOKEN_KEYWORD_IF.size(), ScriptTokenType::If);
						parseContext.SkipSpaces(false);

						//かっこで開始してないと構文エラー
						if (!parseContext.GetCurrent().starts_with(TOKEN_COMMON_BRACKET_BEGIN)) {
							parseContext.Error(ERROR_TOKEN_003, parseContext.GetCurrentPosition());
							return;
						}
						parseContext.PushToken(TOKEN_COMMON_BRACKET_BEGIN.size(), ScriptTokenType::BracketBegin);
						ParseFunctionBlock(parseContext, BLOCK_END_FLAG_BRACKET);
						if (parseContext.IsEnd()) {
							return;
						}
						parseContext.SkipSpaces(false);
					}

					//中かっこで開始してないと構文エラー
					if (!parseContext.GetCurrent().starts_with(TOKEN_BLOCK_BEGIN)) {
						parseContext.Error(ERROR_TOKEN_004, parseContext.GetCurrentPosition());
						return;
					}

					//中かっこ
					parseContext.PushToken(TOKEN_BLOCK_BEGIN.size(), ScriptTokenType::BlockBegin);

					//ここからトークの解析コンテキスト、終端カッコまで取り込み
					ParseTalkBlock(parseContext, BLOCK_END_FLAG_BLOCK_BRACKET);
					if (parseContext.IsEnd()) {
						return;
					}

					//次の処理に戻れる
					continue;
				}
			}

			//シンボル
			{
				if (!symbol.empty()) {
					parseContext.PushToken(symbol.size(), ScriptTokenType::Symbol);
					continue;
				}
			}

			//最終的にどのトークンにもあてはまらない場合はエラー
			parseContext.Error(ERROR_TOKEN_005, parseContext.GetCurrentPosition());
			return;
		}
	}

	//トークブロックのパース
	void TokensParser::ParseTalkBlock(ScriptTokenParseContext& parseContext, uint32_t blockEndFlags) {

#if 0	//最初の行だけ飛ばすため全部はスキップしないでおく
		//スペースをスキップ
		parseContext.SkipSpaces(false);
#endif

		bool isFirstLine = true;

		//とりあえず全部文字列として扱ってみる
		while (!parseContext.IsEnd()) {

			//スペースを飛ばして次のトークンを探す
			if (isFirstLine) {
				parseContext.SkipSpaces(false);
			}
			else {
				//空行を認識させる
				if (parseContext.SkipSpaces(true)) {
					parseContext.PushToken(0, ScriptTokenType::SpeakBegin);
					parseContext.PushToken(0, ScriptTokenType::String);
					parseContext.PushToken(0, ScriptTokenType::SpeakEnd);
					parseContext.PushToken(0, ScriptTokenType::TalkLineEnd);
				}
			}

			if (parseContext.IsEnd()) {
				break;
			}

			isFirstLine = false;
			parseContext.SkipIndent();

			//> はジャンプ行。こちらも行末まで関数ベースの解析をさせる
			if (parseContext.GetCurrent().starts_with(TOKEN_TALK_JUMP_LINE)) {
				parseContext.PushToken(TOKEN_TALK_JUMP_LINE.size(), ScriptTokenType::TalkJump);
				ParseFunctionBlock(parseContext, BLOCK_END_FLAG_NEWLINE);
				if (parseContext.IsEnd()) {
					return;
				}
				parseContext.PushToken(0, ScriptTokenType::TalkLineEnd);
				continue;
			}

			//話者指定、行末まで通常行扱い
			{
				std::match_results<std::string_view::const_iterator> match;
				if (std::regex_search(parseContext.GetCurrent().begin(), parseContext.GetCurrent().end(), match, TOKEN_TALK_SCOPE_PATTERN, std::regex_constants::match_continuous | std::regex_constants::format_first_only | std::regex_constants::format_no_copy)) {

					if (match[1].length() > 0) {
						//話者指定付き
						parseContext.PushToken(match[1].length(), ScriptTokenType::SpeakerIndex);
					}

					parseContext.PushToken(match[2].length(), ScriptTokenType::SpeakerSwitch);

					//コロン１個ならインデントを無視する
					const bool removeIndent = match[2].length() == 1;
					if (removeIndent) {
						while (parseContext.GetCurrent().at(0) == ' ' || parseContext.GetCurrent().at(0) == '\t') {
							parseContext.SeekChar(1);
						}
					}

					//残りは行末までトーク本文扱い
					parseContext.PushToken(0, ScriptTokenType::SpeakBegin);
					ParseStringLiteral(parseContext, BLOCK_END_FLAG_NEWLINE, false);
					if (parseContext.IsEnd()) {
						return;
					}
					parseContext.PushToken(0, ScriptTokenType::SpeakEnd);
					parseContext.PushToken(0, ScriptTokenType::TalkLineEnd);
					continue;
				}
			}

			//終端
			if (parseContext.GetCurrent().starts_with(TOKEN_BLOCK_END)) {
				parseContext.PushToken(TOKEN_BLOCK_END.size(), ScriptTokenType::BlockEnd);
				return;
			}

			{
				//それ以外は行末までトーク本文扱いする
				parseContext.PushToken(0, ScriptTokenType::SpeakBegin);
				ParseStringLiteral(parseContext, BLOCK_END_FLAG_NEWLINE, false);
				if (parseContext.IsEnd()) {
					return;
				}
				parseContext.PushToken(0, ScriptTokenType::SpeakEnd);
				parseContext.PushToken(0, ScriptTokenType::TalkLineEnd);
			}
		}
	}

	//文字列のパース
	void TokensParser::ParseStringLiteral(ScriptTokenParseContext& parseContext, uint32_t blockEndFlags, bool isRawString) {
		//とりあえず改行とかも含めて次の " までを文字列という扱いにしてみる

		//トークン0サイズで開始
		bool isNewToken = true;

		while (!parseContext.IsEnd()) {

			if (isNewToken) {
				parseContext.PushToken(0, ScriptTokenType::String);
				isNewToken = false;
			}

			if (parseContext.GetCurrent().starts_with(TOKEN_STRING)) {

				if (CheckFlags(blockEndFlags, BLOCK_END_FLAG_DOUBLE_QUATATION)) {
					//終端
					parseContext.SeekChar(TOKEN_STRING.size());
					return;
				}

				//とりこみ
				parseContext.AppendLastToken(TOKEN_STRING.size());
			}
			else if(parseContext.GetCurrent().starts_with(TOKEN_STRING2)) {
				if (CheckFlags(blockEndFlags, BLOCK_END_FLAG_BACK_QUATATION)) {
					//終端
					parseContext.SeekChar(TOKEN_STRING2.size());
					return;
				}

				//とりこみ
				parseContext.AppendLastToken(TOKEN_STRING2.size());
			}
			else if (parseContext.GetCurrent().starts_with(TOKEN_RAW_STRING)) {
				if (CheckFlags(blockEndFlags, BLOCK_END_FLAG_SINGLE_QUATATION)) {
					//終端
					parseContext.SeekChar(TOKEN_RAW_STRING.size());
					return;
				}

				//とりこみ
				parseContext.AppendLastToken(TOKEN_RAW_STRING.size());
			}
			else if (parseContext.GetCurrent().starts_with(TOKEN_CRLF)) {

				if (CheckFlags(blockEndFlags, BLOCK_END_FLAG_NEWLINE)) {
					parseContext.IncrementLine(TOKEN_CRLF.size());
					return;
				}

				//改行のとりこみ
				parseContext.AppendLastToken(parseContext.GetCurrent(), TOKEN_CRLF.size());
				parseContext.IncrementLine(TOKEN_CRLF.size());
			}
			else if (parseContext.GetCurrent().starts_with(TOKEN_LF)) {

				if (CheckFlags(blockEndFlags, BLOCK_END_FLAG_NEWLINE)) {
					parseContext.IncrementLine(TOKEN_LF.size());
					return;
				}

				parseContext.AppendLastToken(parseContext.GetCurrent(), TOKEN_LF.size());
				parseContext.IncrementLine(TOKEN_LF.size());
			}
			else if(!isRawString){
				//raw文字列ではない場合、フォーマットが有効

				if (parseContext.GetCurrent().starts_with(TOKEN_FORMAT_BEGIN)) {
					// ${} のフォーマット処理。次の中括弧までを関数扱いで解析させる
					parseContext.PushToken(TOKEN_FORMAT_BEGIN.size(), ScriptTokenType::ExpressionInString);
					ParseFunctionBlock(parseContext, BLOCK_END_FLAG_BLOCK_BRACKET);
					if (parseContext.IsEnd()) {
						return;
					}

					//あたらしい文字列トークンを作成
					isNewToken = true;
				}
				else if (parseContext.GetCurrent().starts_with(TOKEN_FUNCSCOPE_BEGIN)) {
					// %{} の関数スコープ処理。次の中括弧まで関数扱いで解析
					parseContext.PushToken(TOKEN_FUNCSCOPE_BEGIN.size(), ScriptTokenType::StatementInString);
					ParseFunctionBlock(parseContext, BLOCK_END_FLAG_BLOCK_BRACKET);
					if (parseContext.IsEnd()) {
						return;
					}

					//あたらしい文字列トークンを作成
					isNewToken = true;
				}
				else {
					//それ以外
					parseContext.AppendLastToken(1);
				}
			}
			else {
				//それ以外
				parseContext.AppendLastToken(1);
			}
		}


	}

}
