#include <cassert>
#include "PluginLoader.h"

namespace sakura {

	//プラグインモジュールのロード
	PluginModuleLoadResult LoadPlugin(const std::string& pluginPath) {
		PluginModuleLoadResult loadResult;
		loadResult.plugin = nullptr;

		LoadedPluginModule loadedModule;
		loadedModule.hModule = LoadLibraryEx(pluginPath.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

		if (loadedModule.hModule == nullptr) {
			//DLLロード失敗
			loadResult.type = PluginResultType::SUCCESS;
			return loadResult;
		}

		loadedModule.fLoad = reinterpret_cast<aosora::LoadFunctionType>(GetProcAddress(loadedModule.hModule, "load"));

		if (loadedModule.fLoad == nullptr) {
			//loadがみつからない
			loadResult.type = PluginResultType::LOAD_FUNCTION_NOT_FOUND;
			return loadResult;
		}

		//TODO: get version?
		
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