#include <fstream>
#include <cstdio>
#include "Shiori.h"


namespace sakura {
	

	Shiori::Shiori() {

		//ランダムを準備
		SRand();

		//固定値情報を設定
		shioriInfo["version"] = "0.0.1";
		shioriInfo["craftman"] = "kanadelab";
		shioriInfo["craftmanw"] = "ななっち";
		shioriInfo["name"] = "Aosora";
	}

	//基準設定とかファイル読み込みとか
	void Shiori::Load(const std::string& path) {

		std::string ghostMaster = path;
		interpreter.SetWorkingDirectory(path);

		std::string scriptProjPath = path;
		scriptProjPath.append("\\ghost.asproj");

		//設定ファイルをロードする
		std::FILE* settingsFile = std::fopen(scriptProjPath.c_str(), "r");
		std::ifstream settingsStream(settingsFile);
		
		std::vector<std::string> files;
		std::string line;
		while (std::getline(settingsStream, line)) {

			if (line.empty()) {
				//空白行などもとばせるといいかも？
				continue;
			}

			//とりあえずロードすべきファイルの列挙があるということにしてみる
			std::string filename = ghostMaster + "\\" + line;
			files.push_back(filename);
		}
		fclose(settingsFile);

		std::vector<std::shared_ptr<const sakura::ASTParseResult>> parsedFileList;

		//各ファイルを読み込み処理
		for (auto item : files) {
			
			auto ast = LoadScriptFile(item);
			parsedFileList.push_back(ast);
			interpreter.ImportClasses(ast->classMap);
		}

		//クラスのリレーションシップ構築
		interpreter.CommitClasses();

		//ルートスクリプト実行
		for (auto item : parsedFileList) {
			interpreter.Execute(item->root);
		}

		//セーブデータロード前にデフォルトセーブデータを用意する機会をつくる
		{
			ShioriResponse response;
			Request(ShioriRequest("OnDefaultSaveData"), response);
		}

		//セーブデータロード
		SaveData::Load(interpreter);

		//初期化イベントを発生
		{
			ShioriResponse response;
			Request(ShioriRequest("OnAosoraLoad"), response);
		}
	}

	void Shiori::LoadWithoutProject() {

		//クラスのリレーションシップ構築
		interpreter.CommitClasses();

		//ルートスクリプト実行
		for (auto item : parsedFileList) {
			interpreter.Execute(item->root);
		}
	}

	void Shiori::Unload() {
		//セーブして終了
		SaveData::Save(interpreter);
	}

	std::shared_ptr<const ASTParseResult> Shiori::LoadScriptFile(const std::string& path) {
		std::FILE* fp = fopen(path.c_str(), "r");
		std::ifstream loadStream(fp);
		std::string fileBody = std::string(std::istreambuf_iterator<char>(loadStream), std::istreambuf_iterator<char>());
		fclose(fp);
		return LoadScriptString(fileBody, path);
	}

	std::shared_ptr<const ASTParseResult> Shiori::LoadScriptString(const std::string& script, const std::string& name) {
		//解析
		auto tokens = sakura::TokensParser::Parse(script, name.c_str());
		auto ast = sakura::ASTParser::Parse(tokens);
		return ast;
	}

	void Shiori::Request(const ShioriRequest& request, ShioriResponse& response) {

		//固定値を返すべきリクエストの場合はそこで終える
		auto infoRecord = shioriInfo.find(request.GetEventId());
		if (infoRecord != shioriInfo.end()) {
			response.SetValue(infoRecord->second);
			return;
		}

		std::string result;

		//リクエストをグローバル空間に書き込み
		auto shioriObj = interpreter.GetGlobalVariable("Shiori");
		if (shioriObj == nullptr || shioriObj->GetObjectInstanceTypeId() != ScriptObject::TypeId()) {
			//ScriptObjectになってなかったら上書き
			shioriObj = ScriptValue::Make(interpreter.CreateObject());
			interpreter.SetGlobalVariable("Shiori", shioriObj);
		}

		//ReferenceListの作成
		auto referenceList = interpreter.CreateObject();
		for (size_t i = 0; i < request.GetReferenceCount(); i++) {
			referenceList->Push(ScriptValue::Make(request.GetReference(i)));
		}

		auto shioriMap = shioriObj->GetObjectRef().Cast<ScriptObject>();
		shioriMap->Clear();
		shioriMap->RawSet("Reference", ScriptValue::Make(referenceList));

		//stautsの該当するレコードにtrueを書き込み(該当なければnullになるため判別に使用できる)
		for (const std::string& st : request.GetStatusCollection()) {
			shioriMap->RawSet(st, ScriptValue::True);
		}

		//グローバル空間からイベントを探す
		auto variable = interpreter.GetGlobalVariable(request.GetEventId());

		if (variable != nullptr) {
			//タイプによって挙動を換える
			if (variable->IsString()) {
				//文字列であればそのまま帰す
				result = variable->ToString();
			}
			else if (variable->IsObject() && variable->GetObjectRef()->CanCall()) {
				//呼び出し可能なオブジェクト

				//リクエスト作成
				FunctionResponse funcResponse;

				//呼出
				std::vector<ScriptValueRef> args;
				interpreter.CallFunction(*variable, funcResponse, args);

				//結果を文字列化
				if (funcResponse.GetReturnValue() != nullptr)
				{
					result = funcResponse.GetReturnValue()->ToStringWithFunctionCall(interpreter);
					response.SetValue(result);
					printf("%s\n", result.c_str());
				}
			}
		}

		//その他拡張のイベント処理
		FunctionResponse eventRes;
		if (TalkTimer::HandleEvent(interpreter, eventRes, request, !result.empty())) {
			result = eventRes.GetReturnValue()->ToStringWithFunctionCall(interpreter);
			response.SetValue(result);
		}

		//一連の対応が終わったらガベージコレクションを起動する
		interpreter.CollectObjects();
	}
}