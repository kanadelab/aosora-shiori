#include <stdio.h>
#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#endif // WIN32 or _WIN32
#include <fstream>
#include "Misc/Utility.h"

namespace sakura {

#if defined(WIN32) || defined(_WIN32)
	constexpr UINT SHIFT_JIS = 932;
#endif // WIN32 or _WIN32

	//ファイル読み込み
	bool File::ReadAllText(const char* filename, std::string& result) {
		std::ifstream loadStream(filename, std::ios_base::in);
		if (!loadStream) {
			return false;
		}
		result = std::string(std::istreambuf_iterator<char>(loadStream), std::istreambuf_iterator<char>());
		return true;
	}

	//ファイル書き込み
	bool File::WriteAllText(const char* filename, const std::string& content) {
		std::ofstream saveStream(filename, std::ios_base::out);
		if (!saveStream) {
			return false;
		}
		saveStream << content;

		return true;
	}

#if defined(WIN32) || defined(_WIN32)
	//Sjift_JISからUTF8へ変換
	std::string ConvertEncoding(const std::string& input, UINT inputEncode, UINT outputEncode) {
		
		//UTF16に変換
		int len = MultiByteToWideChar(inputEncode, 0, input.c_str(), -1, NULL, 0);
		if (len <= 0) {
			return std::string();
		}

		wchar_t* wstr = static_cast<wchar_t*>(malloc(sizeof(wchar_t) * (len + 1)));
		MultiByteToWideChar(inputEncode, 0, input.c_str(), -1, wstr, len);

		//UTF8に変換
		int resultLen = WideCharToMultiByte(outputEncode, 0, wstr, -1, NULL, 0, NULL, NULL);
		if (resultLen <= 0) {
			free(wstr);
			return std::string();
		}

		char* str = static_cast<char*>(malloc(sizeof(char) * (resultLen + 1)));
		WideCharToMultiByte(outputEncode, 0, wstr, -1, str, resultLen, NULL, NULL);
		str[resultLen] = '\0';

		std::string result(str);
		free(wstr);
		free(str);
		return result;
	}

	std::string SjisToUtf8(const std::string& input) {
		return ConvertEncoding(input, SHIFT_JIS, CP_UTF8);
	}

	std::string Utf8ToSjis(const std::string& input) {
		return ConvertEncoding(input, CP_UTF8, SHIFT_JIS);
	}
#else
	std::string ConvertEncoding(const std::string& input, const char *inputEncode, const char *outputEncode) {
        // TODO stub
		return input;
	}

	std::string SjisToUtf8(const std::string& input) {
        // TODO stub
        return input;
	}

	std::string Utf8ToSjis(const std::string& input) {
        // TODO stub
        return input;
	}
#endif // WIN32 or _WIN32
}
