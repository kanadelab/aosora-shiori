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
		NullCoalescing,

		AssignAdd,
		AssignSub,
		AssignMul,
		AssignDiv,
		AssignMod,
		AssignNullCoalescing,

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
		VerticalBar,
		Talk,
		Word,
		Function,

		Foreach,
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
		Null,
		Try,
		Catch,
		Finally,
		Throw,
		Class,
		Init,
		New,
		Unit,
		Use,
		This,
		Base,
		In,

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

	//スクリプトファイルのパスを示すもの
	class SourceFilePath {
	private:
		std::string sourceName;			//相対パスなど見て理解する名前
		std::string sourceFullPath;		//フルパスなどファイル追跡用

	public:
		SourceFilePath(const std::string& name, const std::string& fullpath):
			sourceName(name),
			sourceFullPath(fullpath) {
		}

		const std::string& GetFileName() const{
			return sourceName;
		}

		const std::string& GetFullPath() const{
			return sourceFullPath;
		}
	};

	//スクリプトデバッグ用のソース上の位置を示すもの
	class SourceCodeRange {
	private:
		std::shared_ptr<SourceFilePath> sourcePath;			//ソースファイル
		uint32_t beginLineIndex;							//開始行
		uint32_t beginColumnIndex;							//開始カラム（文字インデックス）
		uint32_t endLineIndex;								//終了行
		uint32_t endColumnIndex;							//終了カラム

	public:
		SourceCodeRange():
			beginLineIndex(0),
			beginColumnIndex(0),
			endLineIndex(0),
			endColumnIndex(0)
		{}

		SourceCodeRange(const std::shared_ptr<SourceFilePath>& sourceFilePath,uint32_t beginLineIdx, uint32_t beginColumnIdx, uint32_t endLineIdx, uint32_t endColumnIdx):
			sourcePath(sourceFilePath),
			beginLineIndex(beginLineIdx),
			beginColumnIndex(beginColumnIdx),
			endLineIndex(endLineIdx),
			endColumnIndex(endColumnIdx)
		{}

		SourceCodeRange(const SourceCodeRange& begin, const SourceCodeRange& includedEnd) :
			beginLineIndex(0),
			beginColumnIndex(0),
			endLineIndex(0),
			endColumnIndex(0)
		{
			SetRange(begin, includedEnd);
		}

		void SetRange(const SourceCodeRange& begin, const SourceCodeRange& includedEnd) {
			sourcePath = begin.sourcePath;
			beginLineIndex = begin.beginLineIndex;
			beginColumnIndex = begin.beginColumnIndex;
			endLineIndex = includedEnd.endLineIndex;
			endColumnIndex = includedEnd.endColumnIndex;
		}

		std::string ToString() const {
			return GetSourceFileName() + ":" + std::to_string(beginLineIndex + 1);
		}

		const std::string& GetSourceFileName() const {
			return sourcePath->GetFileName();
		}

		const std::string& GetSourceFileFullPath() const {
			return sourcePath->GetFullPath();
		}

		const std::shared_ptr<SourceFilePath>& GetSourceFilePath() const {
			return sourcePath;
		}

		uint32_t GetBeginLineIndex() const {
			return beginLineIndex;
		}

		void SetBeginLineIndex(uint32_t index) {
			beginLineIndex = index;
		}

		uint32_t GetBeginColumnIndex() const {
			return beginColumnIndex;
		}

		void SetBeginColumnIndex(uint32_t index) {
			beginColumnIndex = index;
		}

		uint32_t GetEndLineIndex() const {
			return endLineIndex;
		}

		void SetEndLineIndex(uint32_t index) {
			endLineIndex = index;
		}

		uint32_t GetEndColumnIndex() const {
			return endColumnIndex;
		}

		void SetEndColumnIndex(uint32_t index) {
			endColumnIndex = index;
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

		ScriptToken(const std::string& str, ScriptTokenType tokenType, const std::shared_ptr<SourceFilePath>& sourceFilePath, uint32_t beginLineIdx, uint32_t beginColumnIdx, uint32_t endLineIdx, uint32_t endColumnIdx) :
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

		//デバッガ出力用(位置データを別でもつ)のエラーメッセージ作成
		std::string MakeDebuggerErrorString() const {
			return "[" + GetData().errorCode + "] " + GetData().message;
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
		static void ParseStringLiteral(ScriptTokenParseContext& parseContext, uint32_t blockEndFlags, bool isRawString);

	public:
		static std::shared_ptr<const TokensParseResult> Parse(const std::string& document, const SourceFilePath& filePath);
	};

}
