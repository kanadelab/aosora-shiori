#include <vector>
#include <string_view>
#include <string>
#include "Windows.h"
#include "Debugger/DebuggerUtility.h"

#if defined(AOSORA_ENABLE_DEBUGGER)
#pragma comment(lib, "Bcrypt.lib")

namespace sakura {

	std::string MD5Hash(const std::string& fileBody)
	{
		BCRYPT_ALG_HANDLE hAlg = nullptr;
		BCRYPT_HASH_HANDLE hHash = nullptr;
		DWORD cbData = 0;
		DWORD cbHash = 0;
		DWORD cbHashObject = 0;
		PBYTE pbHashObject = nullptr;
		PBYTE pbHash = nullptr;

		BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, nullptr, 0);
		BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0);
		BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0);

		pbHash = (PBYTE)malloc(cbHash);
		pbHashObject = (PBYTE)malloc(cbHashObject);

		BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, nullptr, 0, 0);
		BCryptHashData(hHash, (const PBYTE)fileBody.c_str(), fileBody.size(), 0);
		BCryptFinishHash(hHash, pbHash, cbHash, 0);

		//TODO: 雑なのでもうちょっとちゃんとする
		char resultStr[64] = {};
		for (size_t i = 0; i < cbHash; i++) {
			snprintf(resultStr + i * 2, 64 - (i * 2), "%02x", pbHash[i]);
		}
		std::string result = resultStr;

		BCryptCloseAlgorithmProvider(hAlg, 0);
		BCryptDestroyHash(hHash);
		free(pbHash);
		free(pbHashObject);

		return result;
	}

	void LoadedSourceManager::AddSource(const std::string& body, const std::string& fullName)
	{
		LoadedSource source;
		source.md5 = MD5Hash(body);
		source.fullName = fullName;
		loadedSources.push_back(source);
	}
}
#endif