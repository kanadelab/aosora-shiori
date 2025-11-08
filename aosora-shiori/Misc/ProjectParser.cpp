#include <filesystem>
#include <fstream>
#include "Misc/Utility.h"
#include "Misc/Message.h"
#include "Misc/ProjectParser.h"

namespace sakura {

	const std::string ERROR_SHIORI_001 = "S001";
	const std::string ERROR_SHIORI_003 = "S003";

	void LoadProjectFile(std::ifstream& loadStream, ProjectSettings& projectSettings, const std::string& basePath, bool isUnitFile, bool isForceDisableDebugSystem) {
		std::string line;
		while (std::getline(loadStream, line)) {
#if !(defined(WIN32) || defined(_WIN32))
			// CRLFのCRが残るので削除。
			Replace(line, "\r", "");
#endif // not(WIN32 or _WIN32)

            //コメントの除去
            size_t commentPos = line.find("//");
            if (commentPos != std::string::npos) {
				line = line.substr(0, commentPos);
            }

			//空白行のスキップ
			std::string emptyTest = line;
			Replace(emptyTest, " ", "");
			Replace(emptyTest, "\t", "");
			if (emptyTest.empty()) {
				continue;
			}

			if (line.find(',') != std::string::npos) {
				//スペースを削除して、カンマで分離
				std::string cmd = line;
				Replace(cmd, " ", "");
				std::vector<std::string> commands;
				SplitString(cmd, commands, ",", 2);

				std::string settingsKey = commands[0];
				std::string settingsValue = commands[1];

				//ユニットファイルの列挙
				if (settingsKey == "unit") {
					std::filesystem::path path(basePath);
					path.append(settingsValue);
					std::string pathStr = path.make_preferred().string();

					if (projectSettings.unitFilesSet.insert(pathStr).second) {
						 projectSettings.unitFiles.push_back(pathStr);
					}
				}

				//非ユニットファイルでは各種設定を仕込むことが可能
				if (!isUnitFile) {

					//実行ステップ制限
					if (settingsKey == "limit_script_steps") {
						try {
							projectSettings.limitScriptSteps = std::stol(settingsValue);
							projectSettings.setLimitScriptSteps = true;
						}
						catch (const std::exception&) {}
						continue;
					}

					//デバッグシステムが無効な場合は無視する
					if (!isForceDisableDebugSystem) {
						//デバッグモード
						if (settingsKey == "debug") {
							projectSettings.enableDebug = StringToSettingsBool(settingsValue);
							continue;
						}

						if (settingsKey == "debug.logfile.name") {
							projectSettings.debugOutputFilename = settingsValue;
							continue;
						}

						if (settingsKey == "debug.logfile.enable") {
							projectSettings.enableDebugLog = StringToSettingsBool(settingsValue);
							continue;
						}

						if (settingsKey == "debug.debugger.port") {
							size_t port;
							if (StringToIndex(settingsValue, port)) {
								projectSettings.debuggerPort = static_cast<uint32_t>(port);
							}
						}
					}
				}
			}
			else
			{
				//カンマがない行はロードするファイルの列挙
				std::filesystem::path path(basePath);
				path.append(line);
				std::string pathStr = path.make_preferred().string();
				if (projectSettings.scriptFilesSet.insert(pathStr).second) {
					projectSettings.scriptFiles.push_back(pathStr);
				}
			}
		}
	}

	void LoadProjectDirectory(const std::string& ghostMasterPath, ProjectSettings& projectSettings, bool isForceDisableDebugSystem) {
		std::filesystem::path gmp(ghostMasterPath);

		//言語ファイル
		{
			std::filesystem::path asLangPath = gmp;
			asLangPath.append("ghost.aslang");

			std::ifstream langStream(asLangPath.string(), std::ios_base::in);
			if (!langStream.fail()) {
				std::string langLine;
				while (std::getline(langStream, langLine)) {
					if (!langLine.empty()) {
						projectSettings.primaryLanguage = langLine;
						break;
					}					
				}
			}
		}

		//プロジェクトファイルをロード
		{
			std::filesystem::path asProjPath = gmp;
			asProjPath.append("ghost.asproj");
			std::ifstream settingsStream(asProjPath.string(), std::ios_base::in);
			if (settingsStream.fail()) {
				ScriptParseErrorData errorData;
				errorData.errorCode = ERROR_SHIORI_001;
				errorData.message = TextSystem::Find(std::string("ERROR_MESSAGE") + ERROR_SHIORI_001);
				errorData.hint = TextSystem::Find(std::string("ERROR_HINT") + ERROR_SHIORI_001);

				projectSettings.scriptLoadErrors.push_back(ScriptParseError(errorData, SourceCodeRange(std::shared_ptr<SourceFilePath>(new SourceFilePath("ghost.asproj", asProjPath.string())), 0, 0, 0, 0)));
				return;
			}
			LoadProjectFile(settingsStream, projectSettings, "", false, isForceDisableDebugSystem);
		}
		
		//デバッグ用の設定ファイルをロードする
		{
			std::filesystem::path debugProjPath = gmp;
			debugProjPath.append("debug.asproj");
			std::ifstream settingsStream(debugProjPath.string(), std::ios_base::in);
			if (!settingsStream.fail()) {
				LoadProjectFile(settingsStream, projectSettings, "", false, isForceDisableDebugSystem);
			}
		}

		//列挙されたユニットファイルを
		{
			for (size_t i = 0; i < projectSettings.unitFiles.size(); i++) {
				std::string target = projectSettings.unitFiles[i];
				std::ifstream settingsStream(target);

				if (settingsStream.fail()) {
					//読み込みエラー
					ScriptParseErrorData errorData;
					errorData.errorCode = ERROR_SHIORI_003;
					errorData.message = TextSystem::Find(std::string("ERROR_MESSAGE") + ERROR_SHIORI_003);
					errorData.hint = TextSystem::Find(std::string("ERROR_HINT") + ERROR_SHIORI_003);
					projectSettings.scriptLoadErrors.push_back(ScriptParseError(errorData, SourceCodeRange(std::shared_ptr<SourceFilePath>(new SourceFilePath(target, ghostMasterPath + target)), 0, 0, 0, 0)));
					return;
				}

				LoadProjectFile(settingsStream, projectSettings, std::filesystem::path(target).parent_path().string(), true, isForceDisableDebugSystem);
			}
		}
		
		
	}
}