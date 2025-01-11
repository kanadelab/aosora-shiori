#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "Misc/Utility.h"

namespace sakura {
	using LoadFunc = BOOL(*)(HGLOBAL, long);
	using UnloadFunc = BOOL(*)();
	using RequestFunc = HGLOBAL(*)(HGLOBAL, long*);

	//リザルトの種類
	enum SaoriResultType {
		SUCCESS,					//成功
		LOAD_DLL_FAILED,			//load: dllのロード失敗
		LOAD_REQUEST_NOT_FOUND,		//load: モジュールにrequest()が見つからない
		LOAD_RESULT_FALSE,			//load: load() が FALSE を返した
		PROTOCOL_ERROR,				//プロトコル書式エラー
		BAD_REQUEST,				//4xxエラー
		INTERNAL_SERVER_ERROR,		//5xxエラー　
		UNKNOWN_CHARSET,			//不明なcharsetが返された
		UNKNOWN_STATUS				//不明なステータスコード
	};

	//さおりモジュール
	struct LoadedSaoriModule {
		HMODULE hModule;
		LoadFunc fLoad;
		UnloadFunc fUnload;
		RequestFunc fRequest;
		Charset charset;
		bool isBasic;
	};

	//さおりの読み込み結果
	struct SaoriModuleLoadResult {
		LoadedSaoriModule* saori;
		SaoriResultType type;
	};

	//さおりの実行結果
	struct SaoriRequestResult {
		SaoriResultType type;
		size_t statusCode;
		std::string result;
		std::string status;
		std::vector<std::string> values;
	};

	//SAORI対応
	SaoriModuleLoadResult LoadSaori(const std::string& saoriPath);
	void RequestSaori(const LoadedSaoriModule* saori, SecurityLevel securityLevel, const std::vector<std::string>& inputArgs, SaoriRequestResult& result);
	void UnloadSaori(LoadedSaoriModule* saori);
	const char* SaoriResultTypeToString(SaoriResultType type);
}