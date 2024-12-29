#include "Shiori.h"
#include "SakuraFMOReader.h"
#include <iostream>

const int ERROR_CODE_INVALID_ARGS = 1;				//起動引数不正
const int ERROR_CODE_GHOST_NOT_FOUND = 2;			//FMOにターゲットのゴーストが見つからない
const int ERROR_CODE_GHOST_SCRIPT_ERROR = 3;		//ゴーストのスクリプト読み込みエラー
const int ERROR_CODE_PREVIEW_SCRIPT_ERROR = 4;		//プレビュー側のスクリプト読み込みエラー

//指定ファイルを単体+プロジェクトを読み込んで送信する想定
int main(int argc, char* argv[]) {
	
	//オプション解析してテストスクリプトとワークスペースを取得
	sakura::Shiori shiori;

	const char* scriptPath = argv[1];

	if (argc >= 3) {
		const char* workspacePath = argv[2];

		//プロジェクトもロード
		shiori.Load(workspacePath);

		//プロジェクトロード時、エラーが発生したらその時点で打ち切り
		if (shiori.HasError()) {
			std::string errors = shiori.GetErrorsString();
			std::cerr << errors << std::endl;
			return ERROR_CODE_GHOST_SCRIPT_ERROR;
		}
	}
	else {
#if 0
		//プロジェクトなしで起動
		shiori.LoadWithoutProject();
#endif
		return ERROR_CODE_INVALID_ARGS;
	}

	//起動中のゴーストを取得
	std::vector<sakura::FMORecord> fmoRecords;
	sakura::ReadSakuraFMO(fmoRecords);

	if (fmoRecords.size() > 0) {

		std::string wsPath = shiori.GetGhostMasterPath();
		sakura::Replace(wsPath, "/", "\\");
		sakura::ToLower(wsPath);
		const sakura::FMORecord* targetGhost = nullptr;

		//FMOから処理対象のゴーストを探す
		for (const auto& record : fmoRecords) {
			//ゴーストの絶対パスがワークスペースの絶対パスに含まれているかどうかをチェック
			std::string fmoPath = record.ghostPath;

			//パス区切りを統一
			sakura::Replace(fmoPath, "/", "\\");

			//小文字に揃える(windows前提で、小文字ドライブレターをサポートしつつ）
			sakura::ToLower(fmoPath);

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

					std::string result;
					shiori.ExecuteScript(func->GetFunction()->GetFunctionBody(), result);

					//リザルトのスクリプトを実行
					sakura::SendDirectSSTP(result, *targetGhost);

					break;
				}
			}
		}
	}

	return 0;
}