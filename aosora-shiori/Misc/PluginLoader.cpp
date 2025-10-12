#include <cassert>
#include "PluginLoader.h"
#include "Misc/Utility.h"

namespace sakura {

	//プラグインモジュールのロード
	PluginModuleLoadResult LoadPlugin(const std::string& pluginPath) {
		PluginModuleLoadResult loadResult;
		loadResult.plugin = nullptr;

		LoadedPluginModule loadedModule;
		loadedModule.hModule = LoadLibraryEx(pluginPath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

		if (loadedModule.hModule == nullptr) {
			//DLLロード失敗
			loadResult.type = PluginResultType::LOAD_DLL_FAILED;
			return loadResult;
		}

		loadedModule.fGetVersion = reinterpret_cast<aosora::GetVersionFunctionType>(GetProcAddress(loadedModule.hModule, "aosora_plugin_get_version"));
		if (loadedModule.fGetVersion == nullptr) {
			//aosora_plugin_get_version が見つからない
			FreeLibrary(loadedModule.hModule);
			loadResult.type = PluginResultType::GET_VERSION_FUNCTION_NOT_FOUND;
			return loadResult;
		}

		loadedModule.fLoad = reinterpret_cast<aosora::LoadFunctionType>(GetProcAddress(loadedModule.hModule, "aosora_plugin_load"));
		if (loadedModule.fLoad == nullptr) {
			//aosora_plugin_loadがみつからない
			FreeLibrary(loadedModule.hModule);
			loadResult.type = PluginResultType::LOAD_FUNCTION_NOT_FOUND;
			return loadResult;
		}

		loadedModule.fUnload = reinterpret_cast<aosora::UnloadFunctionType>(GetProcAddress(loadedModule.hModule, "aosora_plugin_unload"));

		//get_versionの呼び出しによる使用可不可のチェック
		aosora::PluginVersionInfo loadVersionInfo;
		VersionInfo shioriVersionInfo = GetVersion();
		loadVersionInfo.major = shioriVersionInfo.major;
		loadVersionInfo.minor = shioriVersionInfo.minor;
		loadVersionInfo.release = shioriVersionInfo.release;
		loadVersionInfo.minMajor = -1;
		loadVersionInfo.minMinor = -1;
		loadVersionInfo.minRelease = -1;
		loadVersionInfo.versionCheckResult = -1;
		loadVersionInfo.maxMajor = -1;
		loadVersionInfo.maxMinor = -1;
		loadVersionInfo.maxRelease = -1;
		loadVersionInfo.pluginMajor = -1;
		loadVersionInfo.pluginMinor = -1;
		loadVersionInfo.pluginRelease = -1;

		loadedModule.fGetVersion(&loadVersionInfo);
		if (loadVersionInfo.versionCheckResult != 0) {
			FreeLibrary(loadedModule.hModule);
			loadResult.type = PluginResultType::GET_VERSION_FAILED;
			return loadResult;
		}
		
		//ヒープにコピー
		loadResult.plugin = new LoadedPluginModule(loadedModule);
		loadResult.type = PluginResultType::SUCCESS;
		return loadResult;
	}

	void UnloadPlugin(LoadedPluginModule* plugin) {
		assert(plugin != nullptr);
		if (plugin != nullptr) {
			if (plugin->fUnload != nullptr) {
				plugin->fUnload();
			}
			FreeLibrary(plugin->hModule);
			delete plugin;
		}
	}
}