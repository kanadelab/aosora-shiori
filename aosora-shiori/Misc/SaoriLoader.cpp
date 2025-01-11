
#include <string_view>
#include <sstream>
#include <assert.h>

#include "Base.h"
#include "Misc/Utility.h"
#include "Misc/SaoriLoader.h"

#if defined(AOSORA_ENABLE_SAORI_LOADER)
namespace sakura {

	SaoriModuleLoadResult LoadSaori(const std::string& saoriPath) {
		SaoriModuleLoadResult loadResult;
		loadResult.saori = nullptr;
		loadResult.type = SaoriResultType::SUCCESS;

		LoadedSaoriModule loadedModule;
		loadedModule.hModule = LoadLibraryEx(saoriPath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (loadedModule.hModule == nullptr)
		{
			//DLLロード失敗
			loadResult.type = SaoriResultType::LOAD_DLL_FAILED;
			return loadResult;
		}

		loadedModule.fRequest = reinterpret_cast<RequestFunc>(GetProcAddress(loadedModule.hModule, "request"));
		if (loadedModule.fRequest == nullptr)
		{
			//request()がない
			loadResult.type = SaoriResultType::LOAD_REQUEST_NOT_FOUND;
			FreeLibrary(loadedModule.hModule);
			return loadResult;
		}

		loadedModule.fLoad = reinterpret_cast<LoadFunc>(GetProcAddress(loadedModule.hModule, "load"));
		loadedModule.fUnload = reinterpret_cast<UnloadFunc>(GetProcAddress(loadedModule.hModule, "unload"));
		loadedModule.charset = Charset::UNKNOWN;

		//loadがあれば実行
		if (loadedModule.fLoad != nullptr) {
			std::string saoriDir = saoriPath.substr(0, saoriPath.rfind("\\")+1);
			HGLOBAL requestData = GlobalAlloc(GMEM_FIXED, saoriDir.size());
			memcpy(requestData, saoriDir.c_str(), saoriDir.size());
			if (loadedModule.fLoad(requestData, saoriDir.size()) == FALSE) {
				loadResult.type = SaoriResultType::LOAD_RESULT_FALSE;
				FreeLibrary(loadedModule.hModule);
				return loadResult;
			}
		}
		
		//get version
		{
			std::string request = "GET Version SAORI/1.0\r\nSecurityLevel: Local\r\nSender: Aosora\r\nCharset: UTF-8\r\n\r\n";
			HGLOBAL requestData = GlobalAlloc(GMEM_FIXED, request.size());
			memcpy(requestData, request.c_str(), request.size());
			long requestSize = request.size();

			//リクエスト呼出
			HGLOBAL resultData = loadedModule.fRequest(requestData, &requestSize);
			std::string_view response_view(reinterpret_cast<const char*>(resultData), requestSize);

			//ステータスを解析
			auto status = ProtocolParseStatus(response_view);
			if (status.type != ProtocolType::SAORI || status.version != "1.0") {
				loadResult.type = SaoriResultType::PROTOCOL_ERROR;
				GlobalFree(resultData);
				if (loadedModule.fUnload != nullptr) {
					loadedModule.fUnload();
				}
				FreeLibrary(loadedModule.hModule);
				return loadResult;
			}

			//charset取得
			auto charset = ProtocolParseCharset(response_view);
			if (charset == Charset::UNKNOWN) {
				loadResult.type = SaoriResultType::UNKNOWN_CHARSET;
				GlobalFree(resultData);
				if (loadedModule.fUnload != nullptr) {
					loadedModule.fUnload();
				}
				FreeLibrary(loadedModule.hModule);
				return loadResult;
			}

			loadedModule.charset = charset;
			GlobalFree(resultData);
		}

		//ヒープにコピー
		loadResult.saori = new LoadedSaoriModule(loadedModule);
		return loadResult;
	}

	void UnloadSaori(LoadedSaoriModule* saori) {
		assert(saori != nullptr);
		if (saori->fUnload != nullptr) {
			saori->fUnload();
		}
		FreeLibrary(saori->hModule);
		delete saori;
	}

	void RequestSaori(const LoadedSaoriModule* saori, SecurityLevel securityLevel, const std::vector<std::string>& inputArgs, SaoriRequestResult& result) {
		
		//リクエストを生成
		std::string request("EXECUTE SAORI/1.0\r\n");
		
		//引数のプッシュ
		for (size_t i = 0; i < inputArgs.size(); i++) {
			std::string a = inputArgs[i];

			//改行があるとまずいので消す
			Replace(a, "\n", "");
			Replace(a, "\r", "");

			request.append("Argument" + std::to_string(i) + ": " + a + "\r\n");
		}

		switch (saori->charset)
		{
		case Charset::SHIFT_JIS:
			request.append("Charset: Shift_JIS");
			break;

		case Charset::UTF_8:
			request.append("Charset: UTF-8");
			break;

		default:
			assert(false);
			break;
		}

		request.append("SecurityLevel: ");
		switch (securityLevel) {

		case SecurityLevel::LOCAL:
			request.append("Local");
			break;
			
		case SecurityLevel::OTHER:
			request.append("External");
			break;
		}
		request.append("\r\n");
		request.append("Sender: Aosora\r\n\r\n");

		if (saori->charset == Charset::SHIFT_JIS) {
			request = Utf8ToSjis(request);
		}

		//呼出
		long requestSize = request.size();
		HGLOBAL requestData = GlobalAlloc(GMEM_FIXED, requestSize);
		memcpy(requestData, request.c_str(), requestSize);
		HGLOBAL resultData = saori->fRequest(requestData, &requestSize);
		std::string_view responseView(reinterpret_cast<const char*>(resultData), requestSize);

		//レスポンスをチェック
		auto status = ProtocolParseStatus(responseView);
		if (status.type != ProtocolType::SAORI || status.version != "1.0") {
			result.type = SaoriResultType::PROTOCOL_ERROR;
			GlobalFree(resultData);
			return;
		}

		//charsetをチェック
		auto responseCharset = ProtocolParseCharset(responseView);
		if (responseCharset == Charset::UNKNOWN) {
			result.type = SaoriResultType::UNKNOWN_CHARSET;
			GlobalFree(resultData);
			return;
		}

		std::string response(responseView);
		GlobalFree(resultData);

		//レスポンスはそれに含まれている文字コードで変換する
		if (responseCharset == Charset::SHIFT_JIS) {
			response = SjisToUtf8(response);
		}

		//レスポンスをパース
		std::string line;
		std::istringstream readStream(response);
		bool isEmptyLine = false;

		while (std::getline(readStream, line)) {

			//getlineが消しきれない改行コードを処理しておく
			Replace(line, "\r", "");
			Replace(line, "\n", "");

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

				if (key == "Result") {
					result.result = value;
				}
				else if (key.starts_with("Value")) {
					size_t index;
					if (StringToIndex(key.substr(5), index)) {
						if (index > result.values.size()) {
							result.values.resize(index+1);
						}
						result.values[index] = value;
					}
				}
			}
		}

		//ステータスコードの100の位をチェックしてグループにふりわけ
		size_t statusGroup = status.statusCode / 100;
		result.statusCode = status.statusCode;
		if (statusGroup == 2) {
			result.type = SaoriResultType::SUCCESS;
		}
		else if (statusGroup == 4) {
			result.type = SaoriResultType::BAD_REQUEST;
		}
		else if (statusGroup == 5) {
			result.type = SaoriResultType::INTERNAL_SERVER_ERROR;
		}
		else {
			result.type = SaoriResultType::UNKNOWN_STATUS;
		}
	}

	//エラー等の文字列化
	const char* SaoriResultTypeToString(SaoriResultType type) {
		switch (type) {
		case SaoriResultType::SUCCESS:
			return "OK";
		case SaoriResultType::LOAD_DLL_FAILED:
			return "ライブラリの読み込みに失敗しました";
		case SaoriResultType::LOAD_REQUEST_NOT_FOUND:
			return "request関数が見つかりませんでした";
		case SaoriResultType::LOAD_RESULT_FALSE:
			return "loadがFALSEを返しました";
		case SaoriResultType::PROTOCOL_ERROR:
			return "SAORIとの通信が異常です";
		case SaoriResultType::BAD_REQUEST:
			return "呼出の不備";
		case SaoriResultType::INTERNAL_SERVER_ERROR:
			return "SAORI内部のエラー";
		case SaoriResultType::UNKNOWN_CHARSET:
			return "使用できない文字コードが返されました";
		case SaoriResultType::UNKNOWN_STATUS:
			return "認識できないステータスコードが返されました";
		default:
			assert(false);
			return "不明なエラー";
		}
	}
}
#endif //#if defined(AOSORA_ENABLE_SAORI_LOADER)