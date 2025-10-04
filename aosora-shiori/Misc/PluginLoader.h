#pragma once
#include <string>
#include "Base.h"

#if defined(AOSORA_REQUIRED_WIN32)
#include <Windows.h>
#endif

#include "AosoraPlugin.h"

namespace sakura {

	enum class PluginResultType {
		SUCCESS,
		LOAD_DLL_FAILED,			//load: dllのロード失敗
		LOAD_FUNCTION_NOT_FOUND		//load: load() が みつからない
	};

	struct LoadedPluginModule {
		HMODULE hModule;
		aosora::LoadFunctionType fLoad;
		aosora::UnloadFunctionType fUnload;
	};

	struct PluginModuleLoadResult {
		LoadedPluginModule* plugin;
		PluginResultType type;
	};

	//aosoraプラグイン
	PluginModuleLoadResult LoadPlugin(const std::string& pluginPath);
	void UnloadPlugin(LoadedPluginModule* plugin);

}