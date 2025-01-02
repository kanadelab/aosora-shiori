#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <sstream>
#include <vector>


namespace sakura{
	inline bool CheckFlags(uint32_t target, uint32_t flag) {
		return (target & flag) != 0u;
	}

	//乱数の初期化
	inline void SRand() {
		srand((unsigned int)time(nullptr));
	}

	//基本のランダム
	inline int Rand(int32_t min, int32_t max) {
		max--;
		return min + (int32_t)(rand() * (max - min + 1.0) / (1.0 + RAND_MAX));
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

	//区切り文字を使用した分割
	inline void SplitString(const std::string& input, std::vector<std::string>& result, char delimiter) {
		std::istringstream ist(input);
		std::string item;
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

	//文字コード変換
	std::string SjisToUtf8(const std::string& input);
	std::string Utf8ToSjis(const std::string& input);

	class File {
	public:
		static bool ReadAllText(const char* filename, std::string& result);
		static bool ReadAllText(const std::string& filename, std::string& result) {
			return ReadAllText(filename.c_str(), result);
		}

		static bool WriteAllText(const char* filename, const std::string& content);
		static bool WriteAllText(const std::string& filename, const std::string& content) {
			return WriteAllText(filename.c_str(), content);
		}
	};
}