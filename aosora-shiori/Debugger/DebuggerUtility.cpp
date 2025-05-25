#include <vector>
#include <string_view>
#include <string>
#include "Windows.h"
#include "Debugger/DebuggerUtility.h"
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

	//受信データバッファ
	//通信プロトコルに従ってデータをバッファリング＆切り出しを行う
	//ただ、すべてのリクエストを送りっぱなしではなくちゃんとレスポンスを返すようにするなら不要そうな気もする。
	class ReceivedDataBuffer {
	private:
		std::vector<uint8_t> internalBuffer;

	public:

		//受信データの投入
		void AddReceivedData(const uint8_t* data, size_t size)
		{
			//末尾にデータを挿入
			size_t currentSize = internalBuffer.size();
			internalBuffer.resize(currentSize + size);
			memcpy(&internalBuffer[currentSize], data, size);
		}

		//受信データの取得
		bool ReadReceivedString(std::string_view& receivedString)
		{
			//区切りの0バイトを検索
			void* addr = std::memchr(&internalBuffer[0], 0, internalBuffer.size());
			if (addr != nullptr) {
				//範囲を切り出す
				const size_t size = reinterpret_cast<const uint8_t*>(addr) - &internalBuffer[0];
				receivedString = std::string_view(reinterpret_cast<const char*>(&internalBuffer[0]), size);
				return true;
			}
			else {
				return false;
			}
		}

		//受信データの削除
		//ReadReceivedStringで受信した string_view を返却する形でバッファをシフトします
		void RemoveReceivedData(const std::string_view& receivedString)
		{
			//すべての領域を削除する場合は同じベクタをバッファとして使う
			if (receivedString.size() + 1 == internalBuffer.size()) {
				internalBuffer.clear();
				return;
			}

			//あたらしい領域をとりなおしてシフト
			std::vector<uint8_t> newBuffer;
			newBuffer.resize(internalBuffer.size() - (receivedString.size() + 1));
			memcpy(&newBuffer[0], &internalBuffer[receivedString.size() + 1], newBuffer.size());
			internalBuffer = std::move(newBuffer);
		}
	};

	
	void LoadedSourceManager::AddSource(const std::string& body, const std::string& fullName)
	{
		LoadedSource source;
		source.md5 = MD5Hash(body);
		source.fullName = fullName;
		loadedSources.push_back(source);
	}

	
}