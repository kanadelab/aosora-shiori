#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#else
#include <cstring>
#include <memory>
#include <unordered_map>
#endif // WIN32 or _WIN32

#include <cstdlib>
#include <cstring>
#include <sstream>
#include "Misc/Utility.h"
#include "Shiori.h"
#include "Misc/Utility.h"

namespace {
	const char* BAD_REQUEST = "SHIORI/3.0 400 Bad Request\r\n\r\n";

	void ParseRequest(sakura::ShioriRequest& shioriRequest, std::istringstream& readStream) {
		std::string line;
		bool isEmptyLine = false;

		//必要なものを取り出して処理
		while (std::getline(readStream, line)) {

			//getlineが消しきれない改行コードを処理しておく
			sakura::Replace(line, "\r", "");
			sakura::Replace(line, "\n", "");

			//空行が連続したらプロトコル的に読むのはそこまででよい
			if (line.empty()) {
				if (isEmptyLine) {
					break;
				}
				else {
					isEmptyLine = true;
					continue;
				}
			}
			else {
				isEmptyLine = false;
			}

			//Key: Value 形式のレコードをよむ
			const size_t colonIndex = line.find(": ");
			if (colonIndex != std::string::npos) {
				std::string key = line.substr(0, colonIndex);
				std::string value = line.substr(colonIndex + 2);

				//基本のマッピングに追加
				shioriRequest.AddRawData(key, value);

				//内容を確認
				if (key == "ID") {
					shioriRequest.SetEventId(value);
				}
				else if (key.starts_with("Reference")) {
					size_t index;
					if (sakura::StringToIndex(key.substr(9), index)) {
						shioriRequest.SetReference(static_cast<uint32_t>(index), value);
					}
				}
				else if (key.starts_with("Argument")) {
					//as SAORIとしての場合、１つずらしてReferenceにし、インデックス0はEventにする
					size_t index;
					if (sakura::StringToIndex(key.substr(8), index)) {
						if (index == 0) {
							shioriRequest.SetEventId(value);
						}
						else {
							shioriRequest.SetReference(static_cast<uint32_t>(index - 1), value);
						}
					}
				}
				else if (key == "Status") {
					std::vector<std::string> statusList;
					sakura::SplitString(value, statusList, ',');
					for (std::string& item : statusList) {
						shioriRequest.AddStatus(item);
					}
				}
				else if (key == "SecurityLevel") {
					std::string	lowerValue = value;
					sakura::ToLower(lowerValue);
					if (lowerValue == "local") {
						shioriRequest.SetSecurityLevel(sakura::SecurityLevel::LOCAL);
					}
					else {
						shioriRequest.SetSecurityLevel(sakura::SecurityLevel::OTHER);
					}
				}
			}
		}
	}

	std::string CreateResponse(sakura::ShioriResponse& shioriResponse, bool isRequestClose, bool isSaori, const std::string& charsetHeader) {
		//レスポンスの作成
		std::string response = isSaori ? "SAORI/1.0 " : "SHIORI/3.0 ";
		response.append(shioriResponse.GetStatus());
		response.append("\r\n");
		response.append(charsetHeader);
		response.append("\r\n");
		response.append("Sender: Aosora\r\n");

		//本文
		if (!shioriResponse.GetValue().empty()) {
			//改行の除去
			std::string value = shioriResponse.GetValue();
			sakura::Replace(value, "\r", "");
			sakura::Replace(value, "\n", "");
			response.append(isSaori ? "Result: " : "Value: ");
			response.append(value);
			if (!isSaori && isRequestClose) {
				response.append("\\-");
			}
			response.append("\r\n");
		}

		if (isSaori) {
			//SAORIのValue
			for (size_t i = 0; i < shioriResponse.GetSaoriValues().size(); i++) {
				std::string value = shioriResponse.GetSaoriValues().at(i);
				sakura::Replace(value, "\r", "");
				sakura::Replace(value, "\n", "");
				response.append("Value" + std::to_string(i) + ": " + value + "\r\n");
			}
		}
		else {
			//SHIORIのReference
			for (size_t i = 0; i < shioriResponse.GetShioriReferences().size(); i++) {
				std::string value = shioriResponse.GetShioriReferences().at(i);
				sakura::Replace(value, "\r", "");
				sakura::Replace(value, "\n", "");
				response.append("Reference" + std::to_string(i) + ": " + value + "\r\n");
			}
		}

		//エラー
		if (shioriResponse.HasError()) {
			response.append("ErrorLevel: ");
			response.append(shioriResponse.GetErrorLevelList());
			response.append("\r\n");
			response.append("ErrorDescription: ");
			response.append(shioriResponse.GetErrorDescriptionList());
			response.append("\r\n");
		}

		//終端
		response.append("\r\n");

		return response;
	}
};

#if defined(WIN32) || defined(_WIN32)

sakura::Shiori* shioriInstance = nullptr;

HGLOBAL CreateResponseMemory(const std::string& responseStr, long* len) {
	//末尾\0を考慮しない
	HGLOBAL ptr = GlobalAlloc(GMEM_FIXED, responseStr.size());
	*len = responseStr.size();
	memcpy(ptr, responseStr.c_str(), responseStr.size());
	return ptr;
}

extern "C" __declspec(dllexport) BOOL __cdecl load(HGLOBAL h, long len) {

	//デバッグ時にパースエラーをアサートで止める
	sakura::DEBUG_ENABLE_ASSERT_PARSE_ERROR = true;

	char* ptr = static_cast<char*>(malloc(len + 1));
	std::memcpy(ptr, h, len);
	ptr[len] = '\0';

	shioriInstance = new sakura::Shiori();
	shioriInstance->Load(ptr);
	free(ptr);
	GlobalFree(h);
	return TRUE;
}

extern "C" __declspec(dllexport) BOOL __cdecl unload() {
	shioriInstance->Unload();
	delete shioriInstance;
	shioriInstance = nullptr;

	return TRUE;
}

extern "C" __declspec(dllexport) HGLOBAL __cdecl request(HGLOBAL h, long* len) {

	//リクエスト
	char* ptr = static_cast<char*>(malloc(*len + 1));
	std::memcpy(ptr, h, *len);
	ptr[*len] = '\0';
	GlobalFree(h);

	std::string requestStr(ptr);

	//文字コードコンバート
	auto charset = sakura::ProtocolParseCharset(std::string_view(requestStr));
	std::string charsetHeader = "Charset: UTF-8";

	if (charset == sakura::Charset::UNKNOWN) {
		return CreateResponseMemory(BAD_REQUEST, len);
	}
	else if (charset == sakura::Charset::SHIFT_JIS) {
		//コンバート
		requestStr = sakura::SjisToUtf8(requestStr);
		charsetHeader = "Charset: Shift_JIS";
	}

	//GET Versionへの対応
	if (requestStr.starts_with("GET Version SHIORI/")) {
		//GET Versionが来たら固定の返答をする
		std::string response = "SHIORI/3.0 200 OK\r\n" + charsetHeader + "\r\n\r\n";
		return CreateResponseMemory(response, len);
	}
	else if (requestStr.starts_with("GET Version SAORI/")) {
		//SAORIとして返す
		std::string response = "SAORI/1.0 200 OK\r\n" + charsetHeader + "\r\n\r\n";
		return CreateResponseMemory(response, len);
	}

	//解析
	std::istringstream readStream(requestStr);
	std::string line;

	//１行目
	if (!std::getline(readStream, line)) {
		return CreateResponseMemory(BAD_REQUEST, len);
	}

	bool isGet = false;
	bool isSaori = false;
	if (line.starts_with("GET SHIORI/")) {
		isGet = true;
	}
	else if (line.starts_with("EXECUTE SAORI/")) {
		isGet = true;
		isSaori = true;
	}
	else if (line.starts_with("NOTIFY SHIORI/")) {
		//nop
	}
	else {
		return CreateResponseMemory(BAD_REQUEST, len);
	}

	sakura::ShioriRequest shioriRequest;
	shioriRequest.SetIsGet(isGet);

	ParseRequest(shioriRequest, readStream);
	shioriRequest.SetIsSaori(isSaori);

	//OnCloseであればゴーストを閉じるようにスクリプトを添付する
	bool isClose = false;
	if (shioriRequest.GetEventId() == "OnClose") {
		isClose = true;
	}

	//Shiori側に送る
	sakura::ShioriResponse shioriResponse;
	shioriInstance->Request(shioriRequest, shioriResponse);

	std::string response = CreateResponse(shioriResponse, isClose, isSaori, charsetHeader);

	//文字コードコンバート
	if (charset == sakura::Charset::SHIFT_JIS) {
		response = sakura::Utf8ToSjis(response);
	}

	return CreateResponseMemory(response, len);
}

#else

namespace {
	std::unordered_map<long, std::unique_ptr<sakura::Shiori>> shioriInstance;
	const long maxInstance = 1024;
};

extern "C" long aosora_load(char *path, long len) {
	long id = 0;
	for (long i = 1; i <= maxInstance; i++) {
		if (shioriInstance.count(i) == 0) {
			shioriInstance[i] = std::make_unique<sakura::Shiori>();
			shioriInstance[i]->Load(path);
			id = i;
			break;
		}
	}
	free(path);
	return id;
}

extern "C" int aosora_unload(long id) {
	if (shioriInstance.count(id) != 0) {
		shioriInstance.erase(id);
	}
	return 1;
}

extern "C" char *aosora_request(long id, char *path, long *len) {
	std::string requestStr(path, *len);
	free(path);
	if (shioriInstance.count(id) == 0) {
		*len = 0;
		return NULL;
	}

	if (requestStr.starts_with("GET Version SHIORI")) {
		//GET Versionが来たら固定の返答をする
		std::string response = "SHIORI/3.0 204 No Content\r\nCharset: UTF-8\r\n\r\n";
		*len = response.length();
		return strdup(response.c_str());
	}

	//解析
	std::istringstream readStream(requestStr);
	std::string line;

	//１行目
	if (!std::getline(readStream, line)) {
		std::string response = BAD_REQUEST;
		*len = response.length();
		return strdup(response.c_str());
	}

	bool isGet = false;
	if (line.starts_with("GET SHIORI")) {
		isGet = true;
	}
	else if (line.starts_with("NOTIFY SHIORI")) {
	}
	else {
		std::string response = BAD_REQUEST;
		*len = response.length();
		return strdup(response.c_str());
	}

	sakura::ShioriRequest shioriRequest;
	shioriRequest.SetIsGet(isGet);

	ParseRequest(shioriRequest, readStream);
	// とりあえずSHIORIだけ対応
	shioriRequest.SetIsSaori(false);

	//Shiori側に送る
	sakura::ShioriResponse shioriResponse;
	shioriInstance[id]->Request(shioriRequest, shioriResponse);

	// とりあえずSHIORIだけ対応
	std::string response = CreateResponse(shioriResponse,
			shioriRequest.GetEventId() == "OnClose",
			false, "Charset: UTF-8");

	*len = response.length();
	return strdup(response.c_str());
}

#endif // WIN32 or _WIN32
