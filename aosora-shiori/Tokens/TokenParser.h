#pragma once

#include <string>
#include <list>
#include <regex>

namespace sakura {

	class ScriptTokenParseContext;

	//スクリプトトークンの種類
	//ここでは意味までは解析していないので、マイナスは単項マイナスか二項の引き算かはわからない
	enum class ScriptTokenType {
		Invalid,		//実体表現としては存在してない、管理用途の無効値

		Plus,
		Minus,
		Asterisk,
		Slash,
		Percent,
		Equal,
		
		AssignAdd,
		AssignSub,
		AssignMul,
		AssignDiv,
		AssignMod,

		LogicalAnd,
		LogicalOr,
		LogicalNot,

		RelationalEq,
		RelationalNe,
		RelationalGt,
		RelationalLt,
		RelationalGe,
		RelationalLe,

		Increment,
		Decrement,

		Dot,
		Comma,
		Semicolon,
		Colon,
		BracketBegin,
		BracketEnd,
		Talk,
		Word,
		Function,

		For,
		While,
		If,
		Else,
		Break,
		Continue,
		Return,
		Local,
		Const,
		True,
		False,
		Try,
		Catch,
		Finally,
		Throw,
		Class,
		Member,
		Init,
		New,

		BlockBegin,
		BlockEnd,
		ArrayBegin,
		ArrayEnd,

		Number,
		StringBegin,
		StringEnd,
		String,
		Symbol,
		ExpressionInString,
		StatementInString,


		TalkJump,
		TalkLineEnd,	//トークの次行に遷移するのを示す
		SpeakerIndex,
		SpeakerSwitch,
		SpeakBegin,
		SpeakEnd
	};

	//スクリプトデバッグ用のソース上の位置を示すもの
	class SourceCodeRange {
	private:
		std::shared_ptr<const std::string> sourcePath;
		uint32_t beginLineIndex;
		uint32_t beginColumnIndex;
		uint32_t endLineIndex;
		uint32_t endColumnIndex;

	public:
		SourceCodeRange():
			beginLineIndex(0),
			beginColumnIndex(0),
			endLineIndex(0),
			endColumnIndex(0)
		{}

		SourceCodeRange(const std::shared_ptr<const std::string>& sourceFilePath, uint32_t beginLineIdx, uint32_t beginColumnIdx, uint32_t endLineIdx, uint32_t endColumnIdx):
			sourcePath(sourceFilePath),
			beginLineIndex(beginLineIdx),
			beginColumnIndex(beginColumnIdx),
			endLineIndex(endLineIdx),
			endColumnIndex(endColumnIdx)
		{}

		void SetRange(const SourceCodeRange& begin, const SourceCodeRange& includedEnd) {
			sourcePath = begin.sourcePath;
			beginLineIndex = begin.beginLineIndex;
			beginColumnIndex = begin.beginColumnIndex;
			endLineIndex = begin.endLineIndex;
			endColumnIndex = begin.endColumnIndex;
		}

		std::string ToString() const {
			return *sourcePath + ":" + std::to_string(beginLineIndex+1);
		}
	};

	//スクリプトトークン情報
	class ScriptToken {
	public:
		ScriptTokenType type;
		std::string body;
		SourceCodeRange sourceRange;
		
		ScriptToken() :
			type(ScriptTokenType::Invalid),
			body()
		{}

		ScriptToken(const std::string& str, ScriptTokenType tokenType, const std::shared_ptr<const std::string>& sourceFilePath, uint32_t beginLineIdx, uint32_t beginColumnIdx, uint32_t endLineIdx, uint32_t endColumnIdx) :
			type(tokenType),
			body(str),
			sourceRange(sourceFilePath, beginLineIdx, beginColumnIdx, endLineIdx, endColumnIdx)
		{}
	};

	//パースエラー定義
	struct ScriptParseErrorData {
		std::string errorCode;
		std::string message;
		std::string hint;
	};

	//発生したパースエラー
	class ScriptParseError {
	private:
		ScriptParseErrorData data;
		SourceCodeRange position;
		std::string previewErrorBefore;
		std::string previewErrorAfter;

	public:
		ScriptParseError(const ScriptParseErrorData& errorData, const SourceCodeRange& sourceRange) :
			data(errorData),
			position(sourceRange)
		{}

		const SourceCodeRange& GetPosition() const {
			return position;
		}

		const ScriptParseErrorData& GetData() const {
			return data;
		}

		//コンソール出力用のエラーを報告
		std::string MakeConsoleErrorString() const {
			return "ERROR: " + GetPosition().ToString() + " [" + GetData().errorCode + "] " + GetData().message;
		}
	};

	//EOFを示すトークン
	const ScriptToken TOKEN_EOF("EOF", ScriptTokenType::Invalid, nullptr, 0, 0, 0, 0);

	//解析結果
	struct TokensParseResult {
		std::list<ScriptToken> tokens;
		bool success;
		std::shared_ptr<ScriptParseError> error;
	};

	//トークン単位のパーサ
	class TokensParser {
	private:
		static void ParseFunctionBlock(ScriptTokenParseContext& parseContext, uint32_t blockEndFlags);
		static void ParseTalkBlock(ScriptTokenParseContext& parseContext, uint32_t blockEndFlags);
		static void ParseStringLiteral(ScriptTokenParseContext& parseContext, uint32_t blockEndFlags);

	public:
		static std::shared_ptr<const TokensParseResult> Parse(const std::string& document, const std::string& filePath);
	};

}