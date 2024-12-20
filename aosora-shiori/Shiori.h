#pragma once
#include "Tokens/Tokens.h"
#include "AST/AST.h"
#include "Interpreter/Interpreter.h"
#include "CoreLibrary/CoreLibrary.h"
#include "CommonLibrary/CommonClasses.h"

namespace sakura {
	//SHIORIとしての実装相当部分
	class Shiori {
	private:
		ScriptInterpreter interpreter;
		std::map<std::string, std::string> shioriInfo;
		std::vector<std::shared_ptr<const sakura::ASTParseResult>> parsedFileList;

	public:
		Shiori();
		void Load(const std::string& path);
		void LoadWithoutProject();
		void Unload();
		void Request(const ShioriRequest& request, ShioriResponse& response);
		void ExecuteScript(const ConstASTNodeRef& node, std::string& response) {
			interpreter.Execute(node, response);
		}

		static std::shared_ptr<const ASTParseResult> LoadScriptFile(const std::string& path);
		static std::shared_ptr<const ASTParseResult> LoadScriptString(const std::string& script, const std::string& name);
	};
}