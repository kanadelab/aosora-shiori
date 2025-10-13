#pragma once
#include <string>
#include "Base.h"

#if defined(AOSORA_REQUIRED_WIN32)
#include <Windows.h>
#endif

#if defined(AOSORA_ENABLE_PLUGIN_LOADER)
#include "AosoraPlugin.h"

namespace sakura {

	enum class PluginResultType {
		SUCCESS,
		LOAD_DLL_FAILED,						//load: dllのロード失敗
		GET_VERSION_FUNCTION_NOT_FOUND,			//load: aosora_plugin_get_version() 関数がみつからない
		GET_VERSION_FAILED,						//load: aosora_plugin_get_version() の失敗
		GET_VERSION_COMPATIBILITY_INVALID,		//load: プラグイン側のバージョンが正しく設定されてない
		GET_VERSION_COMPATIBILITY_OLD_PLUGIN,	//load: プラグインが古い
		GET_VERSION_COMPATIBILITY_NEW_PLUGIN,	//load: プラグインが新しい
		LOAD_FUNCTION_NOT_FOUND					//load: aosora_plugin_load() が みつからない
	};

	struct LoadedPluginModule {
		HMODULE hModule;
		aosora::raw::GetVersionFunctionType fGetVersion;
		aosora::raw::LoadFunctionType fLoad;
		aosora::raw::UnloadFunctionType fUnload;
	};

	struct PluginModuleLoadResult {
		LoadedPluginModule* plugin;
		PluginResultType type;
	};

	//aosoraプラグイン
	PluginModuleLoadResult LoadPlugin(const std::string& pluginPath);
	void UnloadPlugin(LoadedPluginModule* plugin);
	const char* PluginResultTypeToString(PluginResultType type);
}

#endif // defined(AOSORA_ENABLE_PLUGIN_LOADER)