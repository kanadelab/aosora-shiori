#include <stdio.h>
#include <Windows.h>
#include <fstream>
#include "Misc/Utility.h"

namespace sakura {

	constexpr UINT SHIFT_JIS = 932;

	//ファイル読み込み
	bool File::ReadAllText(const char* filename, std::string& result) {

		FILE* fp = fopen(filename, "r");
		if (fp == nullptr) {
			return false;
		}
		std::ifstream loadStream(fp);
		result = std::string(std::istreambuf_iterator<char>(loadStream), std::istreambuf_iterator<char>());
		fclose(fp);
		return true;
	}

	//ファイル書き込み
	bool File::WriteAllText(const char* filename, const std::string& content) {
		FILE* fp = fopen(filename, "w");
		if (fp == nullptr) {
			return false;
		}

		fprintf(fp, "%s", content.c_str());
		return true;
	}

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

}