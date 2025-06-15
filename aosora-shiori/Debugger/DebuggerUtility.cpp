#include "Base.h"

#include <vector>
#include <string_view>
#include <string>
#if defined(AOSORA_REQUIRED_WIN32)
#include "Windows.h"
#else
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>
#endif // AOSORA_REQUIRED_WIN32
#include "Debugger/DebuggerUtility.h"

#if defined(AOSORA_ENABLE_DEBUGGER)
#if defined(AOSORA_REQUIRED_WIN32)
#pragma comment(lib, "Bcrypt.lib")
#endif // AOSORA_REQUIRED_WIN32

namespace sakura {

#if defined(AOSORA_REQUIRED_WIN32)
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
#else
	std::string MD5Hash(const std::string& fileBody)
	{
		unsigned char md[EVP_MAX_MD_SIZE + 1] = {};
		size_t mdlen = 0;
		if (EVP_Q_digest(NULL, "MD5", NULL, fileBody.c_str(), fileBody.size(), md, &mdlen)) {
			std::ostringstream oss;
			for (int i = 0; i < mdlen; i++) {
				oss << std::setfill('0') << std::right << std::setw(2) << static_cast<int>(md[i]);
			}
			return oss.str();
		}
		std::string empty;
		return empty;
	}
#endif // AOSORA_REQUIRED_WIN32

	void LoadedSourceManager::AddSource(const std::string& body, const std::string& fullName)
	{
		LoadedSource source;
		source.md5 = MD5Hash(body);
		source.fullName = fullName;
		loadedSources.push_back(source);
	}
}
#endif
