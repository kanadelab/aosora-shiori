
#include <string_view>
#include <sstream>
#include <assert.h>

#include "Base.h"
#include "Misc/Utility.h"
#include "Misc/SaoriLoader.h"

#if !defined(AOSORA_REQUIRED_WIN32)

#include <cstring>
#include <dlfcn.h>

namespace {
	const long FALSE = 0;
	const int GMEM_FIXED = 0;
};

using BOOL = long;
using HGLOBAL = char *;

inline HMODULE GetProcAddress(void *handle, const char *symbol) {
	return dlsym(handle, symbol);
}

inline int FreeLibrary(HMODULE handle) {
	return dlclose(handle);
}

inline HGLOBAL GlobalAlloc(int _, size_t size) {
	return static_cast<HGLOBAL>(malloc(size));
}

inline void GlobalFree(HGLOBAL ptr) {
	free(ptr);
}

#endif // AOSORA_ENABLE_SAORI_LOADER

namespace sakura {

	SaoriModuleLoadResult LoadSaori(const std::string& saoriPath) {
		SaoriModuleLoadResult loadResult;
		loadResult.saori = nullptr;
		loadResult.type = SaoriResultType::SUCCESS;

		LoadedSaoriModule loadedModule;
#if defined(WIN32) || defined(_WIN32)
		loadedModule.hModule = LoadLibraryEx(saoriPath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
        // SAORI名。
        // dlsymで呼び出す関数名はSAORIの名前に依存する。
        std::string name;
		if (getenv("SAORI_FALLBACK_ALWAYS")) {
			loadedModule.hModule = nullptr;
		}
		else {
			loadedModule.hModule = dlopen(saoriPath.c_str(), RTLD_LAZY);
		}
		if (loadedModule.hModule == nullptr) {
			char *tmp = getenv("SAORI_FALLBACK_PATH");
			if (tmp == nullptr) {
				loadResult.type = SaoriResultType::LOAD_DLL_FAILED;
				return loadResult;
			}

			auto posSlash = saoriPath.rfind('/');
			auto posBackSlash = saoriPath.rfind('\\');
			if (posSlash == std::string::npos) {
				posSlash = 0;
			}
			if (posBackSlash == std::string::npos) {
				posBackSlash = 0;
			}
			std::string saoriFilename = saoriPath.substr(std::max(posSlash, posBackSlash) + 1);
			{
				auto pos = saoriFilename.rfind('.');
				if (pos == std::string::npos) {
					name = saoriFilename;
				}
				else {
					name = saoriFilename.substr(0, pos);
				}
			}
			std::string fallbackPath(tmp);
			std::vector<std::string> fallbackPathList;
			for (std::string::size_type index = 0; index < fallbackPath.length(); index++) {
				auto pos = fallbackPath.find(':', index);
				if (pos == std::string::npos) {
					fallbackPathList.push_back(fallbackPath.substr(index));
					break;
				}
				else if (index < pos) {
					fallbackPathList.push_back(fallbackPath.substr(index, pos - index));
					index = pos;
				}
			}
			for (auto& path : fallbackPathList) {
				if (!path.ends_with('/')) {
					path += '/';
				}
				std::string p = path + saoriFilename;
				loadedModule.hModule = dlopen(p.c_str(), RTLD_LAZY);
				if (loadedModule.hModule != nullptr) {
					break;
				}
			}
		}
#endif // WIN32 or _WIN32
		if (loadedModule.hModule == nullptr)
		{
			//DLLロード失敗
			loadResult.type = SaoriResultType::LOAD_DLL_FAILED;
			return loadResult;
		}

#if defined(WIN32) || defined(_WIN32)
		loadedModule.fRequest = reinterpret_cast<RequestFunc>(GetProcAddress(loadedModule.hModule, "request"));
#else
		std::string requestName = name + "_saori_request";
		loadedModule.fRequest = reinterpret_cast<RequestFunc>(GetProcAddress(loadedModule.hModule, requestName.c_str()));
#endif // WIN32 or _WIN32
		if (loadedModule.fRequest == nullptr)
		{
			//request()がない
			loadResult.type = SaoriResultType::LOAD_REQUEST_NOT_FOUND;
			FreeLibrary(loadedModule.hModule);
			return loadResult;
		}

#if defined(WIN32) || defined(_WIN32)
		loadedModule.fLoad = reinterpret_cast<LoadFunc>(GetProcAddress(loadedModule.hModule, "load"));
		loadedModule.fUnload = reinterpret_cast<UnloadFunc>(GetProcAddress(loadedModule.hModule, "unload"));
#else
		std::string loadName = name + "_saori_load";
		std::string unloadName = name + "_saori_unload";
		loadedModule.fLoad = reinterpret_cast<LoadFunc>(GetProcAddress(loadedModule.hModule, loadName.c_str()));
		loadedModule.fUnload = reinterpret_cast<UnloadFunc>(GetProcAddress(loadedModule.hModule, unloadName.c_str()));
#endif
		loadedModule.charset = Charset::UNKNOWN;

		//loadがあれば実行
		if (loadedModule.fLoad != nullptr) {
			std::string saoriDir = saoriPath.substr(0, saoriPath.rfind("\\")+1);
			HGLOBAL requestData = GlobalAlloc(GMEM_FIXED, saoriDir.size());
			memcpy(requestData, saoriDir.c_str(), saoriDir.size());
			BOOL ret = loadedModule.fLoad(requestData, saoriDir.size());
			if (ret == FALSE) {
				loadResult.type = SaoriResultType::LOAD_RESULT_FALSE;
				FreeLibrary(loadedModule.hModule);
				return loadResult;
			}
#if !(defined(WIN32) || defined(_WIN32))
			else {
				loadedModule.id = ret;
			}
#endif // not(WIN32 or _WIN32)
		}
		
		//get version
		{
			std::string request = "GET Version SAORI/1.0\r\nSecurityLevel: Local\r\nSender: Aosora\r\nCharset: UTF-8\r\n\r\n";
			HGLOBAL requestData = GlobalAlloc(GMEM_FIXED, request.size());
			memcpy(requestData, request.c_str(), request.size());
			long requestSize = request.size();

			//リクエスト呼出
#if defined(WIN32) || defined(_WIN32)
			HGLOBAL resultData = loadedModule.fRequest(requestData, &requestSize);
#else
			HGLOBAL resultData = loadedModule.fRequest(loadedModule.id, requestData, &requestSize);
#endif // WIN32 or _WIN32
			std::string_view response_view(reinterpret_cast<const char*>(resultData), requestSize);

			//ステータスを解析
			auto status = ProtocolParseStatus(response_view);
			if (status.type != ProtocolType::SAORI || status.version != "1.0") {
				loadResult.type = SaoriResultType::PROTOCOL_ERROR;
				GlobalFree(resultData);
				if (loadedModule.fUnload != nullptr) {
#if defined(WIN32) || defined(_WIN32)
					loadedModule.fUnload();
#else
					loadedModule.fUnload(loadedModule.id);
#endif // WIN32 or _WIN32
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
#if defined(WIN32) || defined(_WIN32)
					loadedModule.fUnload();
#else
					loadedModule.fUnload(loadedModule.id);
#endif // WIN32 or _WIN32
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
#if defined(WIN32) || defined(_WIN32)
			saori->fUnload();
#else
			saori->fUnload(saori->id);
#endif // WIN32 or _WIN32
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
#if defined(WIN32) || defined(_WIN32)
		HGLOBAL resultData = saori->fRequest(requestData, &requestSize);
#else
		HGLOBAL resultData = saori->fRequest(saori->id, requestData, &requestSize);
#endif // WIN32 or _WIN32
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
