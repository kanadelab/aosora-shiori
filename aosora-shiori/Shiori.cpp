﻿#include <fstream>
#include <cstdio>
#include "Shiori.h"


namespace sakura {
	

	Shiori::Shiori():
		isResponsedLoadError(false),
		isBooted(false) {

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

		ghostMasterPath = path;
		interpreter.SetWorkingDirectory(path);

		std::string scriptProjPath = path;
		scriptProjPath.append("\\ghost.asproj");

		//設定ファイルをロードする
		std::ifstream settingsStream(scriptProjPath, std::ios_base::in);
		
		std::vector<std::string> files;
		std::string line;
		while (std::getline(settingsStream, line)) {

			if (line.empty()) {
				//空白行などもとばせるといいかも？
				continue;
			}

			//とりあえずロードすべきファイルの列挙があるということにしてみる
			std::string filename = line;
			files.push_back(filename);
		}

		std::vector<std::shared_ptr<const sakura::ASTParseResult>> parsedFileList;

		//各ファイルを読み込み処理
		for (auto item : files) {
			
			auto loadResult = LoadScriptFile(item);
			if (loadResult->success) {
				parsedFileList.push_back(loadResult);
				interpreter.ImportClasses(loadResult->classMap);
			}
			else {
				scriptLoadErrors.push_back(*loadResult->error);
			}
		}

		//エラーが発生していたら以降の処理を打ち切る
		if (!scriptLoadErrors.empty()) {
			return;
		}

		//クラスのリレーションシップ構築
		interpreter.CommitClasses();

		//ルートスクリプト実行
		for (auto item : parsedFileList) {
			auto rootResult = interpreter.Execute(item->root, false);
			if (!rootResult.success) {

				//起動手順中のエラーとして記録
				bootingExecuteErrorGuide = ToStringRuntimeErrorForSakuraScript(rootResult.error, true);
				bootingExecuteErrorLog = ToStringRuntimeErrorForErrorLog(rootResult.error);
				return;
			}
		}

		//セーブデータロード前にデフォルトセーブデータを用意する機会をつくる
		{
			ShioriResponse response;
			Request(ShioriRequest("OnDefaultSaveData"), response);

			if (response.HasError()) {
				//エラーを報告していたらエラーを引き上げる
				bootingExecuteErrorGuide = response.GetValue();
				bootingExecuteErrorLog = response.GetErrorCollection()[0].GetMessage();
				return;
			}
		}

		//セーブデータロード
		SaveData::Load(interpreter);

		//初期化イベントを発生
		{
			ShioriResponse response;
			Request(ShioriRequest("OnAosoraLoad"), response);

			if (response.HasError()) {
				//エラーを報告していたらエラーを引き上げる
				bootingExecuteErrorGuide = response.GetValue();
				bootingExecuteErrorLog = response.GetErrorCollection()[0].GetMessage();
				return;
			}
		}

		//起動完了としてマーク
		isBooted = true;
	}

	void Shiori::LoadWithoutProject() {

		//クラスのリレーションシップ構築
		interpreter.CommitClasses();
	}

	void Shiori::Unload() {
		//セーブして終了
		SaveData::Save(interpreter);
	}

	std::shared_ptr<const ASTParseResult> Shiori::LoadExternalScriptFile(const std::string& fullPath, const std::string& label) {
		std::ifstream loadStream(fullPath, std::ios_base::in);
		std::string fileBody = std::string(std::istreambuf_iterator<char>(loadStream), std::istreambuf_iterator<char>());
		return LoadScriptString(fileBody, label);
	}

	std::shared_ptr<const ASTParseResult> Shiori::LoadScriptFile(const std::string& path) {
		std::string fullPath = ghostMasterPath + "\\" + path;
		std::ifstream loadStream(fullPath, std::ios_base::in);
		std::string fileBody = std::string(std::istreambuf_iterator<char>(loadStream), std::istreambuf_iterator<char>());
		return LoadScriptString(fileBody, path);
	}

	std::shared_ptr<const ASTParseResult> Shiori::LoadScriptString(const std::string& script, const std::string& name) {
		//解析
		auto tokens = sakura::TokensParser::Parse(script, name.c_str());
		if (!tokens->success) {
			//トークン解析でエラーなら打ち切る
			auto* errorResult = new ASTParseResult();
			errorResult->success = false;
			errorResult->error = tokens->error;
			return std::shared_ptr<const ASTParseResult>(errorResult);
		}

		auto ast = sakura::ASTParser::Parse(tokens);
		return ast;
	}

	void Shiori::Request(const ShioriRequest& request, ShioriResponse& response) {
		RequestInternal(request, response);

		//不要なオブジェクトを開放
		interpreter.CollectObjects();
	}

	void Shiori::RequestInternal(const ShioriRequest& request, ShioriResponse& response) {

		//固定値を返すべきリクエストの場合はそこで終える
		auto infoRecord = shioriInfo.find(request.GetEventId());
		if (infoRecord != shioriInfo.end()) {
			response.SetValue(infoRecord->second);
			return;
		}

		//エラー時むけのリロードイベント処理
		if (request.GetEventId() == "OnAosoraRequestReload") {
			//リロード選択肢。リロードをかけたあと、リロード完了後の状態に対してリロード成否を表示させるためのイベントをraiseする
			response.SetValue("\\![reload,shiori]\\![raise,OnAosoraReloaded]");
			return;
		}


		//イベント名にピリオドがある場合、シンボルとして使用できないため置換する
		std::string eventName = request.GetEventId();
		if (eventName.find(".") != std::string::npos) {
			Replace(eventName, ".", "@");
		}

		//読み込みエラーが生じている場合はフォールバック
		if (!scriptLoadErrors.empty()) {
			RequestScriptLoadErrorFallback(request, response);
			return;
		}
		else if (!bootingExecuteErrorGuide.empty() || !bootingExecuteErrorLog.empty()) {
			RequestScriptBootErrorFallback(request, response);
			return;
		}

		//エラー処理から呼ばれてリロードに成功していた場合。完了の旨を表示して終了
		if (request.GetEventId() == "OnAosoraReloaded") {
			response.SetValue("\\0\\s[0]\\b[2]\\![quicksession,true]■蒼空 リロード完了\\n\\nリロードして、起動エラーはありませんでした。");
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
		auto referenceList = interpreter.CreateNativeObject<ScriptArray>();
		for (size_t i = 0; i < request.GetReferenceCount(); i++) {
			referenceList->Add(ScriptValue::Make(request.GetReference(i)));
		}

		auto shioriMap = shioriObj->GetObjectRef().template Cast<ScriptObject>();
		shioriMap->Clear();
		shioriMap->RawSet("Reference", ScriptValue::Make(referenceList));

		//stautsの該当するレコードにtrueを書き込み(該当なければnullになるため判別に使用できる)
		for (const std::string& st : request.GetStatusCollection()) {
			shioriMap->RawSet(st, ScriptValue::True);
		}

		//グローバル空間からイベントを探す
		auto variable = interpreter.GetGlobalVariable(eventName);

		if (variable != nullptr) {
			//タイプによって挙動を換える
			if (variable->IsString()) {
				//文字列であればそのまま帰す
				std::string result = variable->ToString();
			}
			else if (variable->IsObject() && variable->GetObjectRef()->CanCall()) {
				//呼び出し可能なオブジェクト

				//リクエスト作成
				FunctionResponse funcResponse;

				//呼出
				std::vector<ScriptValueRef> args;
				interpreter.CallFunction(*variable, funcResponse, args);

				//実行時エラー
				if (funcResponse.IsThrew()) {
					HandleRuntimeError(funcResponse.GetThrewError(), response);
					return;
				}

				//結果を文字列化
				if (funcResponse.GetReturnValue() != nullptr)
				{
					auto toStringResult = funcResponse.GetReturnValue()->ToStringWithFunctionCall(interpreter);

					//文字列化時の実行時エラー
					if (!toStringResult.success) {
						HandleRuntimeError(toStringResult.error, response);
						return;
					}

					result = toStringResult.result;
				}
			}
		}

		//その他拡張のイベント処理
		FunctionResponse eventRes;
		if (TalkTimer::HandleEvent(interpreter, eventRes, request, !result.empty())) {
			if (eventRes.IsThrew()) {
				HandleRuntimeError(eventRes.GetThrewError(), response);
				return;
			}

			auto toStringResult = eventRes.GetReturnValue()->ToStringWithFunctionCall(interpreter);
			if (!toStringResult.success) {
				HandleRuntimeError(toStringResult.error, response);
				return;
			}

			response.SetValue(toStringResult.result);
		}
		else {
			response.SetValue(result);
		}
	}

	//読み込みエラーになったときのガイダンスのためのリクエスト処理
	void Shiori::RequestScriptLoadErrorFallback(const ShioriRequest& request, ShioriResponse& response) {

		//初回のGET時、SHIORIプロトコル上でもエラーを発生させる
		if (request.IsGet() && !isResponsedLoadError) {
			for (const auto& err : scriptLoadErrors) {
				response.AddError(ShioriError(ShioriError::ErrorLevel::Error, err.GetPosition().ToString() + " [" + err.GetData().errorCode + "] " + err.GetData().message));
			}
			isResponsedLoadError = true;
		}

		//特定のイベント時に、エラーのガイダンスを表示する
		if (request.GetEventId() == "OnBoot" || request.GetEventId() == "OnMouseDoubleClick" || request.GetEventId() == "OnAosoraErrors" || request.GetEventId() == "OnAosoraReloaded") {
			//起動直後やダブルクリックイベントに対してゴーストをGUIにエラーリストを表示する
			response.SetValue(ShowErrors());
		}
		else if (request.GetEventId() == "OnAosoraErrorView") {
			//エラー個別表示
			size_t index = std::stoul(request.GetReference(0));
			response.SetValue(ShowErrorDetail(index));
		}
		else if (request.GetEventId() == "OnAosoraErrorClose") {
			//「閉じる」ための選択肢イベント。204にならないように適当にスコープ指定だけいれて返す。
			response.SetValue("\\0");
		}
	}

	std::string Shiori::ShowErrors() {
		std::string errorGuide = "\\0\\b[2]\\s[0]\\![quicksession,true]■蒼空 起動エラー / Aosora shiori error\\n(ゴーストをダブルクリックで再度開けます)\\n";

		//エラー情報をリストアップ
		for (size_t i = 0; i < scriptLoadErrors.size(); i++) {
			const auto& err = scriptLoadErrors[i];
			errorGuide += "\\n\\![*]\\q[" + err.GetPosition().ToString() + ",OnAosoraErrorView," + std::to_string(i) + "]\\n" + err.GetData().errorCode + ": " + err.GetData().message + "\\n";
		}

		errorGuide += "\\n\\![*]\\q[閉じる,OnAosoraErrorClose]";
		errorGuide += "\\n\\![*]\\q[ゴーストを再読み込み,OnAosoraRequestReload]";
		return errorGuide;
	}

	std::string Shiori::ShowErrorDetail(size_t index) {
		

		const auto& err = scriptLoadErrors[index];
		std::string errorGuide = "\\0\\b[2]\\s[0]\\![quicksession,true]■蒼空 エラー詳細ビュー　";
		
		//１個戻るボタン
		if (index > 0) {
			errorGuide += "\\q[←,OnAosoraErrorView," + std::to_string(index - 1) + "]";
		}
		else {
			errorGuide += "　";
		}

		//個数表示
		errorGuide += " " + std::to_string(index+1) + " / " + std::to_string(scriptLoadErrors.size()) + " ";

		//１個進むボタン
		if (index + 1 < scriptLoadErrors.size()) {
			errorGuide += "\\q[→,OnAosoraErrorView," + std::to_string(index + 1) + "]";
		}
		else {
			errorGuide += " ";
		}
		
		errorGuide += "\\n\\n\\_?[エラー位置]\\_?\\n\\_?" + err.GetPosition().ToString() + "\\_?\\n\\n\\_?[エラー]\\_?\\n\\_?" + err.GetData().errorCode + ": " + err.GetData().message + "\\_?\\n\\n\\_?[解決のヒント]\\_?\\n\\_?" + err.GetData().hint + "\\_?";
		errorGuide += "\\n\\n\\![*]\\q[エラーリストに戻る,OnAosoraErrors]";
		errorGuide += "\\n\\![*]\\q[閉じる,OnAosoraErrorClose]";
		errorGuide += "\\n\\![*]\\q[ゴーストを再読み込み,OnAosoraRequestReload]";
		return errorGuide;
	}

	void Shiori::RequestScriptBootErrorFallback(const ShioriRequest& request, ShioriResponse& response) {
		//初回のGET時、SHIORIプロトコル上でもエラーを発生させる
		if (request.IsGet() && !isResponsedLoadError) {
			response.AddError(ShioriError(ShioriError::ErrorLevel::Error, bootingExecuteErrorGuide));
			isResponsedLoadError = true;
		}

		if (request.GetEventId() == "OnBoot" || request.GetEventId() == "OnMouseDoubleClick" || request.GetEventId() == "OnAosoraReloaded") {
			response.SetValue(bootingExecuteErrorGuide);
		}
		else if (request.GetEventId() == "OnAosoraErrorClose") {
			//「閉じる」ための選択肢イベント。204にならないように適当にスコープ指定だけいれて返す。
			response.SetValue("\\0");
		}
	}

	//コンソールエラーダンプ用の文字列を取得
	std::string Shiori::GetErrorsString() {
		std::string result;

		//読み込みエラー
		for (const auto& err : scriptLoadErrors) {
			result += err.MakeConsoleErrorString() + "\r\n";
		}

		//起動エラー
		if (!bootingExecuteErrorLog.empty()) {
			result += bootingExecuteErrorLog;
		}
		return result;
	}

	//ランタイムエラーハンドラ
	//エラーが呼び出し元まで戻ってきたときに表示するさくらスクリプト出力
	void Shiori::HandleRuntimeError(const ObjectRef& errObj, ShioriResponse& response) {
	
		response.SetValue(ToStringRuntimeErrorForSakuraScript(errObj, !isBooted));
		response.AddError(ShioriError(ShioriError::ErrorLevel::Error, ToStringRuntimeErrorForErrorLog(errObj)));
	}

	//エラーをさくらスクリプトによるゴースト上での表示むけに整形
	std::string Shiori::ToStringRuntimeErrorForSakuraScript(const ObjectRef& errObj, bool isBooting) {
		auto* err = interpreter.InstanceAs<RuntimeError>(errObj);
		assert(err != nullptr);

		std::string ghostErrorGuide = "\\0\\b[2]\\s[0]\\![quicksession,true]■蒼空 実行エラー / aosora runtime error\\nエラーが発生しため、実行を中断しました。\\n\\n";

		if (isBooting) {
			//起動エラーの場合は区別する
			ghostErrorGuide = "\\0\\b[2]\\s[0]\\![quicksession,true]■蒼空 起動エラー / aosora boot error\\n(ゴーストをダブルクリックで再度開けます)\\n\\n";
		}

		std::string trace;
		std::string firstTrace;
		for (const auto& stackFrame : err->GetCallStackInfo()) {

			//SourceRangeがないものは内部的な処理場で作られているものなのでスクリプトユーザー向けには表示しない
			if (stackFrame.hasSourceRange) {
				if (!stackFrame.funcName.empty()) {
					//関数名に () をつけておく
					trace += stackFrame.funcName + "() ";
				}

				trace += stackFrame.sourceRange.ToString() + "\\n";

				//スタックの最小は別枠で記憶
				if (firstTrace.empty() && !trace.empty()) {
					firstTrace = trace;
				}
			}
		}

		ghostErrorGuide += "\\_?[エラー位置]\\_?\\n" + firstTrace + "\\n\\_?[エラー内容]\\_?\\n" + "\\_?" + err->GetMessage() + "\\_?" + "\\n\\n\\_?[スタックトレース]\\_?\\n" + trace;
		ghostErrorGuide += "\\n\\![*]\\q[閉じる,OnAosoraErrorClose]";
		ghostErrorGuide += "\\n\\![*]\\q[ゴーストを再読み込み,OnAosoraRequestReload]";

		return ghostErrorGuide;
	}

	//エラーをエラーログ等の表示用に平文で整形
	std::string Shiori::ToStringRuntimeErrorForErrorLog(const ObjectRef& errObj) {
		auto* err = interpreter.InstanceAs<RuntimeError>(errObj);
		assert(err != nullptr);

		std::string scriptErrorLog = "蒼空 実行エラー ";

		std::string trace;
		std::string firstTrace;
		for (const auto& stackFrame : err->GetCallStackInfo()) {

			//SourceRangeがないものは内部的な処理場で作られているものなのでスクリプトユーザー向けには表示しない
			if (stackFrame.hasSourceRange) {
				if (!stackFrame.funcName.empty()) {
					//関数名に () をつけておく
					trace += stackFrame.funcName + "() ";
				}

				trace += stackFrame.sourceRange.ToString();

				//スタックの最小は別枠で記憶
				if (firstTrace.empty() && !trace.empty()) {
					firstTrace = trace;
				}
			}
		}

		scriptErrorLog += "[エラー位置] " + firstTrace + " [エラー内容] " + err->GetMessage() + " [スタックトレース] " + trace;
		return scriptErrorLog;
	}
}
