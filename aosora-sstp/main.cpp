#include "Base.h"
#include "Shiori.h"
#include "SakuraFMOReader.h"
#include <iostream>

const int ERROR_CODE_INVALID_ARGS = 1;				//起動引数不正
const int ERROR_CODE_GHOST_NOT_FOUND = 2;			//FMOにターゲットのゴーストが見つからない
const int ERROR_CODE_GHOST_SCRIPT_ERROR = 3;		//ゴーストのスクリプト読み込みエラー
const int ERROR_CODE_PREVIEW_SCRIPT_ERROR = 4;		//プレビュー側のスクリプト読み込みエラー
const int ERROR_CODE_PREVIEW_EXECUTE_ERROR = 5;		//プレビュースクリプトのランタイムエラー

//指定ファイルを単体+プロジェクトを読み込んで送信する想定
int main(int argc, char* argv[]) {
	
	//オプション解析してテストスクリプトとワークスペースを取得
	sakura::Shiori shiori;

	//デバッグ機能の無効化(ゴーストが起動していると競合するため回避させる)
	shiori.SetForceDisableDebugSystem(true);

	const char* scriptPath = argv[1];

	if (argc >= 3) {
		const char* workspacePath = argv[2];

		//プロジェクトもロード
		shiori.Load(workspacePath);

		//プロジェクトロード時、エラーが発生したらその時点で打ち切り
		if (shiori.HasBootError()) {
			std::string errors = shiori.GetErrorsString();
			std::cerr << errors << std::endl;
			return ERROR_CODE_GHOST_SCRIPT_ERROR;
		}
	}
	else {
		return ERROR_CODE_INVALID_ARGS;
	}

	//起動中のゴーストを取得
	std::vector<sakura::FMORecord> fmoRecords;
	sakura::ReadSakuraFMO(fmoRecords);

	if (fmoRecords.size() > 0) {

		std::string wsPath = shiori.GetGhostMasterPath();
#if defined(AOSORA_REQUIRED_WIN32)
		sakura::Replace(wsPath, "/", "\\");
		sakura::ToLower(wsPath);
#endif // AOSORA_REQUIRED_WIN32
		const sakura::FMORecord* targetGhost = nullptr;

		//FMOから処理対象のゴーストを探す
		for (const auto& record : fmoRecords) {
			//ゴーストの絶対パスがワークスペースの絶対パスに含まれているかどうかをチェック
			std::string fmoPath = record.ghostPath;

#if defined(AOSORA_REQUIRED_WIN32)
			//パス区切りを統一
			sakura::Replace(fmoPath, "/", "\\");

			//小文字に揃える(windows前提で、小文字ドライブレターをサポートしつつ）
			sakura::ToLower(fmoPath);
#endif // AOSORA_REQUIRED_WIN32

			if (wsPath.starts_with(fmoPath)) {
				targetGhost = &record;
			}
		}

		//ターゲットがなければ送信キャンセル
		if (targetGhost == nullptr) {
			return ERROR_CODE_GHOST_NOT_FOUND;
		}

		//テスト実行スクリプトをロード
		auto ast = shiori.LoadExternalScriptFile(scriptPath, "PREVIEW_SCRIPT");
		if (!ast->success) {
			std::cerr << ast->error->MakeConsoleErrorString() << std::endl;
			return ERROR_CODE_PREVIEW_SCRIPT_ERROR;
		}

		//コードブロックとして解析し、最初に見つかった関数ステートメントを実行
		if (ast->root->GetType() == sakura::ASTNodeType::CodeBlock) {
			auto block = std::static_pointer_cast<const sakura::ASTNodeCodeBlock>(ast->root);

			for (auto item : block->GetStatements()) {
				if (item->GetType() == sakura::ASTNodeType::FunctionStatement) {
					auto func = std::static_pointer_cast<const sakura::ASTNodeFunctionStatement>(item);

					auto executeResult = shiori.ExecuteScript(func->GetFunction()->GetFunctionBody());
					if (!executeResult.success) {
						//エラー
						std::cerr << shiori.ToStringRuntimeErrorForErrorLog(executeResult.error) << std::endl;
						return ERROR_CODE_PREVIEW_EXECUTE_ERROR;
					}

					//リザルトのスクリプトを実行
					sakura::SendDirectSSTP(executeResult.result, *targetGhost);
					break;
				}
			}
		}
	}

	return 0;
}
