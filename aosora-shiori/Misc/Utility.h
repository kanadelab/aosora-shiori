#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <cassert>
#include <filesystem>
#include "Version.h"
#include "Base.h"


namespace sakura{
	//統一で使用するランダムオブジェクト
	extern std::mt19937 randomEngine;

	inline bool CheckFlags(uint32_t target, uint32_t flag) {
		return (target & flag) != 0u;
	}

	inline std::mt19937& GetInternalRandom() {
		return randomEngine;
	}

	//基本のランダム
	inline int Rand(int32_t min, int32_t max) {
		if (min >= max) {
			//minとmaxが逆
			return min;
		}
		std::uniform_int_distribution<> dist(min, max-1);
		return dist(GetInternalRandom());
	}

	//実数ランダム 0.0以上、1.0未満
	inline number RandNum() {
		std::uniform_real_distribution<> dist(0.0, 1.0);
		return dist(GetInternalRandom());
	}

	//文字列全置換
	inline void Replace(std::string& str, const std::string& search, const std::string& replace) {
		size_t pos = 0;
		size_t offset = 0;
		const size_t len = search.length();
		
		while ((pos = str.find(search, offset)) != std::string::npos) {
			str.replace(pos, len, replace);
			offset = pos + replace.length();
		}
	}

	//小文字にそろえる
	inline void ToLower(std::string& str) {
		for (size_t i = 0; i < str.size(); i++) {
			str[i] = std::tolower(str[i]);
		}
	}

	//数値を文字列化(Javascriptのように中身が整数っぽければ整数にする)
	inline std::string ToString(double val) {
		std::ostringstream ost;
		ost << val;
		return ost.str();
	}

	inline std::string ToString(int64_t val) {
		return std::to_string(val);
	}

	inline std::string ToString(uint64_t val) {
		return std::to_string(val);
	}

	inline std::string ToString(int32_t val) {
		return std::to_string(val);
	}

	inline std::string ToString(uint32_t val) {
		return std::to_string(val);
	}

	//文字からsize_tへ
	inline bool StringToIndex(const std::string& str, size_t& result) {
		try {
			result = std::stoul(str);
			return true;
		}
		catch (const std::exception&) {
			return false;
		}
	}

	//設定ファイル解析用:文字列をboolに変換
	inline bool StringToSettingsBool(std::string& str) {

		//falseと0だけがfalse
		if (str == "0") {
			return false;
		}
		std::string lw = str;
		ToLower(lw);
		if (lw == "false") {
			return false;
		}
		return true;
	}

	//区切り文字を使用した分割
	inline void SplitString(const std::string& input, std::vector<std::string>& result, char delimiter) {
		std::istringstream ist(input);
		std::string item;
		result.clear();
		while (std::getline(ist, item, delimiter)) {
			result.push_back(item);
		}
	}

	//区切り文字列を使用した分割
	inline void SplitString(const std::string& input, std::vector<std::string>& result, const std::string& delimiter, size_t maxItems) {
		size_t offset = 0;
		while (true) {
			auto pos = input.find(delimiter, offset);
			if (pos == std::string::npos) {
				//終了
				result.push_back(input.substr(offset));
				return;
			}

			//切り出し格納
			result.push_back(input.substr(offset, pos - offset));
			offset = pos + delimiter.size();

			//最大アイテム数規制
			if (maxItems > 0) {
				if (result.size() + 1 >= maxItems) {
					result.push_back(input.substr(offset));
					return;
				}
			}
		}
	}

	//文字列結合
	inline std::string JoinString(const std::vector<std::string>& items, size_t index, size_t count, const std::string& delimiter) {
		std::string result = "";
		for (size_t i = index; i < index + count; i++) {
			if (i != index) {
				result.append(delimiter);
			}
			result.append(items[i]);
		}
		return result;
	}

	inline std::string JoinString(const std::vector<std::string>& items, const std::string& delimiter) {
		std::string result = "";
		for (size_t i = 0; i < items.size(); i++) {
			if (i != 0) {
				result.append(delimiter);
			}
			result.append(items[i]);
		}
		return result;
	}

	//文字コード変換
	std::string SjisToUtf8(const std::string& input);
	std::string Utf8ToSjis(const std::string& input);

	//先頭バイトからUTF-8で何バイトか取得
	inline size_t GetUnicodeByteCount(uint8_t firstByte) {
		if (firstByte <= 0x7f) {
			return 1;
		}
		else if (firstByte >= 0xc2 && firstByte <= 0xdf) {
			return 2;
		}
		else if (firstByte >= 0xe0 && firstByte <= 0xef) {
			return 3;
		}
		else if (firstByte >= 0xf0 && firstByte <= 0xf4) {
			return 4;
		}

		//無効
		return 1;
	}

	//文字数からバイトインデックスに変換
	inline size_t UnicodeCharIndexToMultiByteIndex(const std::string& str, size_t charIndex) {

		if (str.empty()) {
			return 0;
		}

		size_t index = 0;
		size_t cIndex = 0;
		while (index < str.size() && cIndex < charIndex) {
			const size_t count = GetUnicodeByteCount(static_cast<uint8_t>(str.at(index)));

			//文字数が足りてなくてutf-8バイト列としておかしいため打ち切り
			if (index + count > str.size()) {
				break;
			}

			//TODO: 異体字セレクタを考慮するなら、異体字セレクタの場合cIndexを足さない
			cIndex++;
			index += count;
		}

		return index;
	}

	//バイトインデックスから文字数に変換
	inline size_t ByteIndexToUncodeCharIndex(const std::string& str, size_t byteIndex) {
		if (str.empty()) {
			return 0;
		}

		size_t index = 0;
		size_t cIndex = 0;
		while (index < str.size() && index < byteIndex) {
			const size_t count = GetUnicodeByteCount(static_cast<uint8_t>(str.at(index)));

			//文字数が足りてなくてutf-8バイト列としておかしいため打ち切り
			if (index + count > str.size()) {
				break;
			}

			//TODO: 異体字セレクタを考慮するなら、異体字セレクタの場合cIndexを足さない
			cIndex++;
			index += count;
		}

		return cIndex;
	}

	//Unicode文字数をカウント
	inline size_t CountUnicodeCharacters(const std::string& str) {
		if (str.empty()) {
			return 0;
		}

		size_t index = 0;
		size_t cIndex = 0;
		while (index < str.size()) {
			const size_t count = GetUnicodeByteCount(static_cast<uint8_t>(str.at(index)));

			//文字数が足りてなくてutf-8バイト列としておかしいため打ち切り
			if (index + count > str.size()) {
				break;
			}

			//TODO: 異体字セレクタを考慮するなら、異体字セレクタの場合cIndexを足さない
			cIndex++;
			index += count;
		}
		return cIndex;
	}

	//SecurityLevelヘッダ
	enum class SecurityLevel {
		OTHER,	//External
		LOCAL
	};

	//charsetのパース
	enum class Charset {
		UNKNOWN,
		SHIFT_JIS,
		UTF_8
	};

	//SHIORI,SAORIのrequest/responseからcharsetを取得する
	inline Charset ProtocolParseCharset(const std::string_view& data) {
		//\r\nで囲まれていることを確認する。やりとりの本文に含まれている場合の誤認識を避けるため
		if (data.find("\r\nCharset: Shift_JIS\r\n") != std::string::npos) {
			return Charset::SHIFT_JIS;
		}
		else if (data.find("\r\nCharset: UTF-8\r\n") != std::string::npos) {
			return Charset::UTF_8;
		}
		else {
			return Charset::UNKNOWN;
		}
	}

	//ファジーにcharsetを読む
	inline Charset CharsetFromString(const std::string& cs) {
		std::string str = cs;
		ToLower(str);
		Replace(str, "_", "-");

		if (str == "utf-8") {
			return Charset::UTF_8;
		}
		else if(str == "shift-jis") {
			return Charset::SHIFT_JIS;
		}
		else {
			return Charset::UNKNOWN;
		}
	}

	enum class ProtocolType {
		UNKNOWN,		//不明もしくは失敗
		SHIORI,
		SAORI
	};

	struct ProtocolStatus {
		ProtocolType type;						//SAORI
		std::string_view version;				//1.0
		std::string_view statusDescription;		//OK
		size_t statusCode;						//200
	};

	inline ProtocolStatus ProtocolParseStatus(const std::string_view& data) {
		ProtocolStatus result;
		result.type = ProtocolType::UNKNOWN;
		result.statusCode = 0;

		const size_t crlfPos = data.find("\r\n");
		if (crlfPos == std::string::npos) {
			return result;
		}

		//"SAORI/1.0 200 OK"
		std::string_view line(data.begin(), data.begin() + crlfPos);
		const size_t slashPos = line.find('/');
		if (slashPos == std::string::npos) {
			return result;
		}

		std::string_view protocol = line.substr(0, slashPos);
		ProtocolType protocolType = ProtocolType::UNKNOWN;
		if (protocol == "SHIORI") {
			protocolType = ProtocolType::SHIORI;
		}
		else if (protocol == "SAORI") {
			protocolType = ProtocolType::SAORI;
		}
		else {
			return result;
		}

		const size_t spacePos = line.find(' ');
		if (spacePos == std::string::npos) {
			return result;
		}

		result.version = line.substr(slashPos + 1, spacePos - slashPos - 1);
		std::string_view status = line.substr(spacePos + 1);

		//レスポンスならステータスコードと説明を分離
		const size_t spacePos2 = status.find(' ');
		if (spacePos2 == std::string::npos) {
			//ステータスコードがないのでおかしい
			return result;
		}

		std::string_view statusCodeStr = status.substr(0, spacePos2);
		result.statusDescription = status.substr(spacePos2 + 1);

		if (!StringToIndex(std::string(statusCodeStr), result.statusCode)) {
			//ステータスコード位置が数値ではない
			return result;
		}

		//ここまでくれば正常
		result.type = protocolType;
		return result;
	}


	class File {
	public:
		static bool ReadAllText(const std::string& filename, std::string& result);
		static bool WriteAllText(const std::string& filename, const std::string& content);

		static void Delete(const std::string& filename, std::error_code& err);
		static void Copy(const std::string& sourceFilename, const std::string& destFilename, bool overwrite, std::error_code& err);
		static void Move(const std::string& sourceFilename, const std::string& destFilename, std::error_code& err);
		static bool Exists(const std::string& filename, std::error_code& err);
		static void CreateDirectories(const std::string& dirname, std::error_code& err);
		static bool IsDirectory(const std::string& filename, std::error_code& err);
		static bool IsFile(const std::string& filename, std::error_code& err);
		static std::vector<std::string> GetFiles(const std::string& filename, bool isRecursive, std::error_code& err);

		//プロセス起動
		//static bool ExecuteProcess();
	};

	//ファイルシステムパスの文字コードとスクリプト側の文字コードの差異を吸収するためのパスオブジェクト
	class FileSystemPath
	{
	private:
		std::filesystem::path body;

		FileSystemPath(const std::filesystem::path& path) :
			body(path)
		{
		}

		//将来的にロケールによって文字コードを選ぶなどの対応？
		static std::string ScriptToFileSystem(const std::string& input) {
			return Utf8ToSjis(input);
		}

		static std::string FileSystemToScript(const std::string& input) {
			return SjisToUtf8(input);
		}

	public:
		FileSystemPath():
			body()
		{ }

		//初期化
		static FileSystemPath FromFileSystemStr(const std::string& path, bool makePreferred = false)
		{
			FileSystemPath result = FileSystemPath(path);
			if (makePreferred) {
				result.MakePreferred();
			}
			return result;
		}

		static FileSystemPath FromScriptStr(const std::string& path, bool makePreferred = false)
		{
			FileSystemPath result = FileSystemPath(ScriptToFileSystem(path));
			if (makePreferred) {
				result.MakePreferred();
			}
			return result;
		}

		//ファイルシステム側の文字コードで文字列を取得
		std::string GetFileSystemStr() const
		{
			return body.string();
		}

		//スクリプト側の文字コードで文字列を取得
		std::string GetScriptStr() const
		{
			return FileSystemToScript(body.string());
		}

		void AppendScript(const std::string& n)
		{
			body.append(ScriptToFileSystem(n));
		}

		void AppendFileSystem(const std::string& n)
		{
			body.append(n);
		}

		FileSystemPath GetParent() const
		{
			return FileSystemPath(body.parent_path());
		}

		void MakePreferred()
		{
			body.make_preferred();
		}

		//ディレクトリを示すパスとして、末尾を区切り文字で終わるようにする
		void MakeDirectoryTerminated()
		{
			if (!body.empty() && body.has_filename()) {
				body /= "";
			}
		}
	};

	//aosoraバージョン取得(major, minor, release)
	struct VersionInfo {
		int32_t major;
		int32_t minor;
		int32_t release;
	};

	inline VersionInfo GetVersion() {
		VersionInfo result = { 0,0,0 };
		std::vector<std::string> versions;
		SplitString(std::string(AOSORA_SHIORI_VERSION), versions, '.');

		if (versions.size() > 0) {
			result.major = std::stoi(versions[0]);
		}

		if (versions.size() > 1) {
			result.minor = std::stoi(versions[1]);
		}

		if (versions.size() > 2) {
			result.release = std::stoi(versions[2]);
		}

		return result;
	}
}
