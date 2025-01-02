
#include "Shiori.h"
#include "Misc/Json.h"

//とりあえず試しに動かしてみる用

namespace sakura {

	//ソースコードをパースする
	void Execute(const std::string& document)
	{
		auto tokens = sakura::TokensParser::Parse(document,"test");
		auto ast = sakura::ASTParser::Parse(tokens);

		printf("---Execute---\n");
		sakura::ScriptInterpreter interpreter;
		interpreter.ImportClasses(ast->classMap);
		interpreter.CommitClasses();
		interpreter.Execute(ast->root, false);
	}
}


int main() {

	std::string sourceCode2 = R"(
		
		function Test() {
			return "2.0" == 2;
		}

		print(Test());

)";

	sakura::Execute(sourceCode2);
}