#include "pch.h"
#include <regex>
#include <fstream>
#include <string_view>
#include <Windows.h>
#include "CppUnitTest.h"

#include "Shiori.h"
#include "Misc/Utility.h"
#include "Misc/Json.h"
#include "Misc/Message.h"
#include "Debugger/Debugger.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Sandbox の main() 相当の簡易実装をテスト化します。
// 本来の実装は多くのプロジェクト内部依存がありますが、
// テストの目的は main() 実行時に "1" が result に入ることを確認する点です。

namespace sakura {
	//ソースコードをパースする
	std::string Execute(const std::string& document)
	{
		TextSystem::CreateInstance();

		auto tokens = sakura::TokensParser::Parse(document, SourceFilePath("test", "test"));
		auto ast = sakura::ASTParser::Parse(tokens);

		printf("---Execute---\n");
		sakura::ScriptInterpreter interpreter;
		interpreter.ImportClasses(ast->classMap);
		interpreter.CommitClasses();
		auto result = interpreter.Execute(ast->root, true);

		TextSystem::DestroyInstance();

		return result.result;
	}
}



namespace aosorashioritest
{
	TEST_CLASS(aosorashioritest)
	{
	public:
		TEST_METHOD(MainReturnsOne)
		{
			std::string sourceCode = R"(
				return "！".length;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "1", L"Execute() should return \"1\"");
		}
	};
}
