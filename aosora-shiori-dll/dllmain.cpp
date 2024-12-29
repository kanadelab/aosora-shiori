// dllmain.cpp : DLL アプリケーションのエントリ ポイントを定義します。
#include "pch.h"
#include <cstdlib>
#include <sstream>
#include "Misc/Utility.h"

#if 0
BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  ul_reason_for_call,
					   LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif

const char* BAD_REQUEST = "SHIORI/3.0 400 Bad Request\r\n\r\n";

sakura::Shiori* shioriInstance = nullptr;

HGLOBAL CreateResponseMemory(const std::string& responseStr, long* len) {
	//末尾\0を考慮しない
	HGLOBAL ptr = GlobalAlloc(GMEM_FIXED, responseStr.size());
	*len = responseStr.size();
	memcpy(ptr, responseStr.c_str(), responseStr.size());
	return ptr;
}

extern "C" __declspec(dllexport) BOOL __cdecl load(HGLOBAL h, long len) {
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

	if (requestStr.starts_with("GET Version SHIORI")) {
		//GET Versionが来たら固定の返答をする
		std::string response = "SHIORI/3.0 200 OK\r\nCharset: UTF-8\r\n\r\n";
		return CreateResponseMemory(response, len);
	}

	//解析
	std::istringstream readStream(ptr);
	std::string line;

	//１行目
	if (!std::getline(readStream, line)) {
		return CreateResponseMemory(BAD_REQUEST, len);
	}

	bool isGet = false;
	if (line.starts_with("GET SHIORI")) {
		isGet = true;
	}
	else if (line.starts_with("NOTIFY SHIORI")) {

	}
	else {
		return CreateResponseMemory(BAD_REQUEST, len);
	}

	std::string charset;
	std::string eventId;
	sakura::ShioriRequest shioriRequest;
	shioriRequest.SetIsGet(isGet);

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

			//基本のマッピング二追加
			shioriRequest.AddRawData(key, value);

			//内容を確認
			if (key == "ID") {
				eventId = value;
				shioriRequest.SetEventId(value);
			}
			else if (key == "Charset") {
				charset = value;
			}
			else if (key.starts_with("Reference")) {
				size_t index = std::stol(key.substr(9));
				shioriRequest.SetReference(static_cast<uint32_t>(index), value);
			}
			else if (key == "Status") {
				std::vector<std::string> statusList;
				sakura::SplitString(value, statusList, ',');
				for (std::string& item : statusList) {
					shioriRequest.AddStatus(item);
				}
			}
		}
	}

	//Shiori側に送る
	sakura::ShioriResponse shioriResponse;
	shioriInstance->Request(shioriRequest, shioriResponse);

	//レスポンスの作成
	std::string response = "SHIORI/3.0 ";
	response.append(shioriResponse.GetStatus());
	response.append("\r\n");
	response.append("Charset: UTF-8\r\n");
	response.append("Sender: Aosora\r\n");

	//本文
	if (!shioriResponse.GetValue().empty()) {
		//改行の除去
		std::string value = shioriResponse.GetValue();
		sakura::Replace(value, "\r", "");
		sakura::Replace(value, "\n", "");
		response.append("Value: ");
		response.append(value);
		response.append("\\e");	//念の為えんいー
		response.append("\r\n");
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

	return CreateResponseMemory(response, len);
}