#pragma once
#include <string>
#include <vector>
#include "Base.h"

#if defined(AOSORA_ENABLE_DEBUGGER)
namespace sakura {

	//読み込み済みのスクリプトを列挙してVSCode側に通知する
	class LoadedSourceManager {
	private:
		struct LoadedSource {
#if defined(AOSORA_DEBUGGER_ENABLE_MD5)
			std::string md5;
#endif
			std::string fullName;
		};

		std::vector<LoadedSource> loadedSources;

	public:
		void AddSource(const std::string& body, const std::string& fullName);
		size_t GetLoadedSourceCount() const { return loadedSources.size(); }
		const LoadedSource& GetLoadedSource(size_t index) const { return loadedSources[index]; }
	};
}
#endif