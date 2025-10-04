#pragma once
#include <string>
#include "Base.h"

#if defined(AOSORA_REQUIRED_WIN32)
#include <Windows.h>
#endif

#include "AosoraPlugin.h"

namespace sakura {

	struct LoadedPluginModule {
		HMODULE hModule;
		aosora::LoadFunctionType fLoad;
		aosora::UnloadFunctionType fUnload;
	};

	struct PluginModuleLoadResult {
		LoadedPluginModule* plugin;
	};

	//aosoraプラグイン
	PluginModuleLoadResult LoadPlugin(const std::string& pluginPath);
	void UnloadPlugin(LoadedPluginModule* plugin);

}