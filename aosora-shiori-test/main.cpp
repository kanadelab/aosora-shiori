
#include "Shiori.h"
#include "Misc/Utility.h"
#include "Misc/Json.h"
#include "Misc/Message.h"

#include <regex>

//とりあえず試しに動かしてみる用

namespace sakura {

	//ソースコードをパースする
	void Execute(const std::string& document)
	{
		TextSystem::CreateInstance();

		auto tokens = sakura::TokensParser::Parse(document,"test");
		auto ast = sakura::ASTParser::Parse(tokens);

		printf("---Execute---\n");
		sakura::ScriptInterpreter interpreter;
		interpreter.ImportClasses(ast->classMap);
		interpreter.CommitClasses();
		interpreter.Execute(ast->root, false);

		TextSystem::DestroyInstance();
	}
}



int main() {

	std::string sourceCode2 = R"(
	
		print("！".length);

)";

	sakura::Execute(sourceCode2);
}