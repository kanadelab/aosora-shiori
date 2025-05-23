
#include "Shiori.h"
#include "Misc/Utility.h"
#include "Misc/Json.h"
#include "Misc/Message.h"
#include "Debugger/Debugger.h"

#include <regex>
#include <fstream>

//とりあえず試しに動かしてみる用

namespace sakura {

	//ソースコードをパースする
	void Execute(const std::string& document)
	{
		TextSystem::CreateInstance();

		auto tokens = sakura::TokensParser::Parse(document,SourceFilePath("test","test"));
		auto ast = sakura::ASTParser::Parse(tokens);

		printf("---Execute---\n");
		sakura::ScriptInterpreter interpreter;
		interpreter.ImportClasses(ast->classMap);
		interpreter.CommitClasses();
		interpreter.Execute(ast->root, false);

		TextSystem::DestroyInstance();
	}
}

std::string readFile(const char* filename)
{
	std::ifstream ifs(filename);
	return std::string(std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());
}

int main() {

	/*
	std::string sourceCode = readFile(R"(D:\extract\GhostMasquerade7\nise_shako\ghost\master\dict-aiev.as)");

	std::string sourceCode2 = R"(

		print("！".length);

)";

	sakura::Execute(sourceCode);
	*/

	sakura::Debugger::Bootstrap();
}