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
		std::string ghostMasterPath;
		ScriptInterpreter interpreter;
		std::map<std::string, std::string> shioriInfo;
		std::vector<std::shared_ptr<const sakura::ASTParseResult>> parsedFileList;
		std::vector<ScriptParseError> scriptLoadErrors;
		bool isResponsedLoadError;

	private:
		//エラーガイダンス用のエラー情報取得
		std::string ShowErrors();
		std::string ShowErrorDetail(size_t index);

		//SHIORI内部用のファイルロード
		std::shared_ptr<const ASTParseResult> LoadScriptFile(const std::string& path);
		std::shared_ptr<const ASTParseResult> LoadScriptString(const std::string& script, const std::string& name);

		//ランタイムエラー処理
		std::string HandleRuntimeError(const FunctionResponse& response);

	public:
		Shiori();
		void Load(const std::string& path);
		void LoadWithoutProject();
		void Unload();
		void Request(const ShioriRequest& request, ShioriResponse& response);
		void RequestErrorFallback(const ShioriRequest& request, ShioriResponse& response);
		void ExecuteScript(const ConstASTNodeRef& node, std::string& response) {
			interpreter.Execute(node, response);
		}

		//外部向けでフルパスのファイルロード
		std::shared_ptr<const ASTParseResult> LoadExternalScriptFile(const std::string& fullPath, const std::string& label);
		
		//エラー表示系
		bool HasError() const { return scriptLoadErrors.size(); }

		//コンソール出力用のエラー情報取得
		std::string GetErrorsString();
		const std::string& GetGhostMasterPath() const { return ghostMasterPath; }

		
	};
}