

#include <regex>
#include <fstream>
#include <Windows.h>
//#include <bcrypt.h>

#include "Shiori.h"
#include "Misc/Utility.h"
#include "Misc/Json.h"
#include "Misc/Message.h"
#include "Debugger/Debugger.h"
#pragma comment(lib, "Bcrypt.lib")

//とりあえず試しに動かしてみる用

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

std::string readFile(const char* filename)
{
	std::ifstream ifs(filename);
	return std::string(std::istreambuf_iterator<char>(ifs),
		std::istreambuf_iterator<char>());
}

void MD5Hash()
{
	BCRYPT_ALG_HANDLE hAlg = nullptr;
	BCRYPT_HASH_HANDLE hHash = nullptr;
	DWORD cbData = 0;
	DWORD cbHash = 0;
	DWORD cbHashObject = 0;
	PBYTE pbHashObject = nullptr;
	PBYTE pbHash = nullptr;

	BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, nullptr, 0);
	BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0);
	BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0);

	pbHash = (PBYTE)malloc(cbHash);
	pbHashObject = (PBYTE)malloc(cbHashObject);

	BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, nullptr, 0, 0);

	const char rgbMsg[5] = { '1','2','3','4','5' };

	BCryptHashData(hHash, (PBYTE)rgbMsg, sizeof(rgbMsg), 0);
	BCryptFinishHash(hHash, pbHash, cbHash, 0);

	//文字列化テスト
	char resultStr[128] = {};
	for (size_t i = 0; i < cbHash; i++) {
		snprintf(resultStr + i * 2, 128 - (i * 2), "%02x", pbHash[i]);
	}

	BCryptCloseAlgorithmProvider(hAlg, 0);
	BCryptDestroyHash(hHash);
	free(pbHash);
	free(pbHashObject);

}

int main() {

	std::string sourceCode = R"(
		return "！".length;
	)";

	auto result = sakura::Execute(sourceCode);
	printf("%s", result.c_str());
}