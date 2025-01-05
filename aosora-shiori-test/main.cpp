
#include "Shiori.h"
#include "Misc/Utility.h"
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
	
		print("2".length);
		local a = "あ";
		print(a.Substring(0, 1)); // => ""!
		local b = "abc";
		print(b.Substring(1, 2)); // => "b"!

		print("あい".Substring(0,1));
		print("あいうえ".Substring(1,2));

)";

	sakura::Execute(sourceCode2);
}