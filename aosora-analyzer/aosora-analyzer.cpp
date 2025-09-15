
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

#include "Misc/Json.h"
#include "Misc/Message.h"
#include "Shiori.h"

sakura::JsonObjectRef MakeSourceRange(const sakura::SourceCodeRange& sourceRange) {
	auto resultObject = sakura::JsonSerializer::MakeObject();
	resultObject->Add("line", sakura::JsonSerializer::From(sourceRange.GetBeginLineIndex()));
	resultObject->Add("column", sakura::JsonSerializer::From(sourceRange.GetBeginColumnIndex()));
	resultObject->Add("endLine", sakura::JsonSerializer::From(sourceRange.GetEndLineIndex()));
	resultObject->Add("endColumn", sakura::JsonSerializer::From(sourceRange.GetEndColumnIndex()));
	return resultObject;
}

int main(int argc, char* argv[])
{
	//コマンドライン引数パース
	const char* lang = "ja-jp"; //デフォルト言語
	for (int i = 1; i < argc; i++) {
		const char* str = argv[i];
		if (strcmp(str, "--language") == 0) {
			if (i < argc - 1) {
				//言語設定
				lang = argv[++i];
			}
		}
	}

	//標準入力からスクリプトをロード
#if true
	std::ostringstream oss;
	oss << std::cin.rdbuf();
	std::string input = oss.str();
#else
	// test.txt の内容を input に読み込む
	std::ifstream ifs(R"(D:\data\aosora-shiori-public\ssp\ghost\test\ghost\master\dict-ai.as)");
	std::string input;
	if (ifs) {
		std::ostringstream oss;
		oss << ifs.rdbuf();
		input = oss.str();
	} else {
		std::cerr << "ファイル test.txt を開けませんでした。" << std::endl;
		return 1;
	}
#endif

	sakura::TextSystem::CreateInstance();
	sakura::TextSystem::GetInstance()->SetPrimaryLanguage(lang);

	//リザルト用オブジェクト
	auto resultObject = sakura::JsonSerializer::MakeObject();

	//蒼空に解析させる
	auto tokens = sakura::TokensParser::Parse(input, sakura::SourceFilePath("test", "test"));
	if (!tokens->success) {
		resultObject->Add("error", sakura::JsonSerializer::From(true));
		resultObject->Add("range", MakeSourceRange(tokens->error->GetPosition()));
		resultObject->Add("message", sakura::JsonSerializer::From(tokens->error->MakeDebuggerErrorString()));
		std::cout << sakura::JsonSerializer::Serialize(resultObject);
		sakura::TextSystem::DestroyInstance();
		return 0;
	}

	auto ast = sakura::ASTParser::Parse(tokens);
	if (!ast->success) {
		resultObject->Add("error", sakura::JsonSerializer::From(true));
		resultObject->Add("range", MakeSourceRange(ast->error->GetPosition()));
		resultObject->Add("message", sakura::JsonSerializer::From(ast->error->MakeDebuggerErrorString()));
		std::cout << sakura::JsonSerializer::Serialize(resultObject);
		sakura::TextSystem::DestroyInstance();
		return 0;
	}

	auto functions = sakura::JsonSerializer::MakeArray();

	//グローバルレベルの関数とトークを収集する
	std::vector<sakura::ConstASTNodeRef> topLevelNodes;
	ast->root->GetChildren(topLevelNodes);

	for (const auto& item : topLevelNodes) {
		if (item->GetType() == sakura::ASTNodeType::FunctionStatement) {
			auto f = sakura::JsonSerializer::MakeObject();
			f->Add("range", MakeSourceRange(item->GetSourceRange()));
			functions->Add(f);
		}
	}

	resultObject->Add("error", sakura::JsonSerializer::From(false));
	resultObject->Add("functions", functions);
	std::cout << sakura::JsonSerializer::Serialize(resultObject);
	sakura::TextSystem::DestroyInstance();
	return 0;
}
