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
		Assert::IsTrue(tokens->success, L"token parse error");

		auto ast = sakura::ASTParser::Parse(tokens);
		Assert::IsTrue(ast->success, L"ast parse error");

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
		TEST_METHOD(StringLengthTest)
		{
			std::string sourceCode = R"(
				return "！".length;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "1");
		}

		TEST_METHOD(DefaultArgTest)
		{
			std::string sourceCode = R"(
				function Num(){ return 10; }

				function TestFunc(a, b = 5, c = "qwert".length, d = Num()){
					return a + b + c + d;
				}
				
				return TestFunc(7);
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "27");
		}

		TEST_METHOD(TypeCheckTest)
		{
			std::string sourceCode = R"(
				return 
					([]).InstanceOf(Array) && ({}).InstanceOf(Map);
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "true");
		}

		TEST_METHOD(YieldReturnBasicTest)
		{
			std::string sourceCode = R"(
				function Numbers() {
					yield return 1;
					yield return 2;
					yield return 3;
				}
				local result = "";
				foreach(local v in Numbers()) {
					result += v;
				}
				return result;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "123");
		}

		TEST_METHOD(YieldReturnInLoopTest)
		{
			std::string sourceCode = R"(
				function Range(n) {
					local i = 0;
					while(i < n) {
						yield return i;
						i++;
					}
				}
				local result = "";
				foreach(local v in Range(5)) {
					result += v;
				}
				return result;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "01234");
		}

		TEST_METHOD(YieldReturnConditionalTest)
		{
			std::string sourceCode = R"(
				function EvenNumbers(n) {
					local i = 0;
					while(i < n) {
						if(i % 2 == 0) {
							yield return i;
						}
						i++;
					}
				}
				local result = "";
				foreach(local v in EvenNumbers(10)) {
					result += v + ",";
				}
				return result;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "0,2,4,6,8,");
		}

		TEST_METHOD(YieldReturnWithEarlyReturnTest)
		{
			std::string sourceCode = R"(
				function FirstN(n) {
					local i = 0;
					while(true) {
						if(i >= n) {
							return;
						}
						yield return i;
						i++;
					}
				}
				local result = "";
				foreach(local v in FirstN(3)) {
					result += v;
				}
				return result;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "012");
		}

		TEST_METHOD(YieldReturnNestedForeachTest)
		{
			std::string sourceCode = R"(
				function Outer() {
					yield return "a";
					yield return "b";
				}
				function Inner() {
					yield return 1;
					yield return 2;
				}
				local result = "";
				foreach(local o in Outer()) {
					foreach(local i in Inner()) {
						result += o + i;
					}
				}
				return result;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "a1a2b1b2");
		}

		TEST_METHOD(YieldReturnResultIsArrayTest)
		{
			std::string sourceCode = R"(
				function Gen() {
					yield return 10;
					yield return 20;
					yield return 30;
				}
				local arr = Gen();
				return arr.length;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "3");
		}

		TEST_METHOD(YieldReturnForLoopTest)
		{
			std::string sourceCode = R"(
				function Squares(n) {
					for(local i = 0; i < n; i++) {
						yield return i * i;
					}
				}
				local result = "";
				foreach(local v in Squares(5)) {
					result += v + ",";
				}
				return result;
			)";
			auto result = sakura::Execute(sourceCode);
			Assert::IsTrue(result == "0,1,4,9,16,");
		}
	};
}
