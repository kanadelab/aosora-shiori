#pragma once
#include <vector>
#include <string>
#include <set>
#include "Tokens/TokenParser.h"

namespace sakura {
	class ProjectSettings {
	public:
		std::vector<ScriptParseError> scriptLoadErrors;
		std::string primaryLanguage;

		std::vector<std::string> scriptFiles;
		std::vector<std::string> unitFiles;
		std::set<std::string> scriptFilesSet;
		std::set<std::string> unitFilesSet;
		std::string debugOutputFilename;
		size_t limitScriptSteps;
		uint32_t debuggerPort;
		bool setLimitScriptSteps;
		bool enableDebug;
		bool enableDebugLog;

		ProjectSettings() :
			debugOutputFilename("aosora.log"),
			limitScriptSteps(0),
			debuggerPort(27016),
			setLimitScriptSteps(false),
			enableDebug(false),
			enableDebugLog(false)
		{
		}
	};

	void LoadProjectFile(std::ifstream& loadStream, ProjectSettings& projectSettings, const std::string& basePath, bool isUnitFile, bool isForceDisableDebugSystem);
	void LoadProjectDirectory(const std::string& ghostMasterPath, ProjectSettings& projectSettings, bool isForceDisableDebugSystem);
}
