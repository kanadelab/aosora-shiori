#include "Shiori.h"
#include "SakuraFMOReader.h"

//指定ファイルを単体+プロジェクトを読み込んで送信する想定
int main(int argc, char* argv[]) {

	//起動中のゴーストを取得
	std::vector<sakura::FMORecord> fmoRecords;
	sakura::ReadSakuraFMO(fmoRecords);

	if (fmoRecords.size() > 0) {

		//オプション解析してテストスクリプトとワークスペースを取得
		sakura::Shiori shiori;

		const char* scriptPath = argv[1];

		if (argc >= 3) {
			const char* workspacePath = argv[2];

			//プロジェクトもロード
			shiori.Load(workspacePath);
		}
		else {
			//プロジェクトなしで起動
			shiori.LoadWithoutProject();
		}

		//テスト実行スクリプトをロード
		auto ast = shiori.LoadScriptFile(scriptPath);

		//コードブロックとして解析し、最初に見つかった関数ステートメントを実行
		if (ast->root->GetType() == sakura::ASTNodeType::CodeBlock) {
			auto block = std::static_pointer_cast<const sakura::ASTNodeCodeBlock>(ast->root);

			for (auto item : block->GetStatements()) {
				if (item->GetType() == sakura::ASTNodeType::FunctionStatement) {
					auto func = std::static_pointer_cast<const sakura::ASTNodeFunctionStatement>(item);

					std::string result;
					shiori.ExecuteScript(func->GetFunction()->GetFunctionBody(), result);

					//リザルトのスクリプトを実行
					//TODO: 複数起動時対応
					sakura::SendDirectSSTP(result, fmoRecords[0]);

					break;
				}
			}
		}
	}
}