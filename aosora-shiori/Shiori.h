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
		bool isBooted;

		std::vector<ScriptParseError> scriptLoadErrors;
		std::string bootingExecuteErrorLog;
		std::string bootingExecuteErrorGuide;
		std::string lastExecuteErrorLog;
		bool isResponsedLoadError;
		bool isForceDisableDebugSystem;

	private:
		void RequestInternal(const ShioriRequest& request, ShioriResponse& response);

		//エラーガイダンス用のエラー情報取得
		std::string ShowErrors();
		std::string ShowErrorDetail(size_t index);

		//SHIORI内部用のファイルロード
		std::shared_ptr<const ASTParseResult> LoadScriptFile(const std::string& path);
		std::shared_ptr<const ASTParseResult> LoadScriptString(const std::string& script, const SourceFilePath& filePath);

		//ランタイムエラー処理
		void HandleRuntimeError(const ObjectRef& err, ShioriResponse& response, bool isSaori);

		//SAORI向けエラーレスポンス作成
		void MakeSaoriErrorResponse(ShioriResponse& response);

	public:
		Shiori();
		~Shiori();
		void Load(const std::string& path);
		void Unload();
		void Request(const ShioriRequest& request, ShioriResponse& response);
		void RequestScriptLoadErrorFallback(const ShioriRequest& request, ShioriResponse& response);
		void RequestScriptBootErrorFallback(const ShioriRequest& request, ShioriResponse& response);

		ToStringFunctionCallResult ExecuteScript(const ConstASTNodeRef& node) {
			return interpreter.Execute(node, true);
		}

		//外部向けでフルパスのファイルロード
		std::shared_ptr<const ASTParseResult> LoadExternalScriptFile(const std::string& fullPath, const std::string& label);
		
		//エラー表示系
		bool HasBootError() const { return !scriptLoadErrors.empty() || !bootingExecuteErrorLog.empty(); }
		std::string ToStringRuntimeErrorForSakuraScript(const ObjectRef& err, bool isBooting = false);
		std::string ToStringRuntimeErrorForErrorLog(const ObjectRef& err);

		//コンソール出力用のエラー情報取得
		std::string GetErrorsString();
		const std::string& GetGhostMasterPath() const { return ghostMasterPath; }

		//デバッグ機能の強制的な無効化（aosora-sstpでデバッグ機能を動作させないようにするためのもの）
		void SetForceDisableDebugSystem(bool isDisable) { isForceDisableDebugSystem = isDisable; }
		bool IsForceDisableDebugSystem() const { return isForceDisableDebugSystem; }
	};
}