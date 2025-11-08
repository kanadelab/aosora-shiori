#include <fstream>
#include <cstdio>
#include <filesystem>
#include "Version.h"
#include "Shiori.h"
#include "Misc/Message.h"
#include "Misc/ProjectParser.h"
#include "Debugger/Debugger.h"

namespace sakura {

	//パースエラーをアサートで止める
	bool DEBUG_ENABLE_ASSERT_PARSE_ERROR = false;

	//SHIORI 起動エラー
	const std::string ERROR_SHIORI_002 = "S002";

	Shiori::Shiori():
		isBooted(false),
		isResponsedLoadError(false),
		isForceDisableDebugSystem(false)
	{

		//テキストシステム作成
		//TODO: これのせいで複数インスタンス対応が微妙になってるので注意
		TextSystem::CreateInstance();

		//固定値情報を設定
		shioriInfo["version"] = AOSORA_SHIORI_VERSION " " AOSORA_SHIORI_BUILD;
		shioriInfo["craftman"] = "kanadelab";
		shioriInfo["craftmanw"] = "ななっち";
		shioriInfo["name"] = "Aosora";
	}

	Shiori::~Shiori() {
		TextSystem::DestroyInstance();

		if (Debugger::IsCreated()) {
			//デバッグシステムを終了
			Debugger::Destroy();
		}
	}

	//基準設定とかファイル読み込みとか
	void Shiori::Load(const std::string& path) {

		ProjectSettings projectSettings;
		ghostMasterPath = std::filesystem::path(path).make_preferred().string();
		interpreter.SetWorkingDirectory(path);

#if 0
		//他の言語を用意するまで無効
		{
			//言語設定ファイルがあれば最初に読み、言語を切り替える
			std::string scirptLangPath = path;
			scirptLangPath.append("ghost.aslang");

			std::ifstream langStream(scirptLangPath, std::ios_base::in);
			if (!langStream.fail()) {
				std::string langLine;
				if (std::getline(langStream, langLine)) {
					//言語設定を試みる
					TextSystem::GetInstance()->SetPrimaryLanguage(langLine);
				}
			}
		}

		//設定ファイルをロードする
		{
			std::string scriptProjPath = path;
			scriptProjPath.append("ghost.asproj");
			std::ifstream settingsStream(scriptProjPath, std::ios_base::in);
			if (settingsStream.fail()) {
				ScriptParseErrorData errorData;
				errorData.errorCode = ERROR_SHIORI_001;
				errorData.message = TextSystem::Find(std::string("ERROR_MESSAGE") + ERROR_SHIORI_001);
				errorData.hint = TextSystem::Find(std::string("ERROR_HINT") + ERROR_SHIORI_001);

				scriptLoadErrors.push_back(ScriptParseError(errorData, SourceCodeRange(std::shared_ptr<SourceFilePath>(new SourceFilePath("ghost.asproj", scriptProjPath)), 0, 0, 0, 0)));
				return;
			}
			LoadProjectFile(settingsStream, projectSettings, "", false);
		}

		//デバッグ用の設定ファイルはあればロードする
		{
			std::string debugProjPath = path;
			debugProjPath.append("debug.asproj");
			std::ifstream settingsStream(debugProjPath, std::ios_base::in);
			if (!settingsStream.fail()) {
				LoadProjectFile(settingsStream, projectSettings, "", false);
			}
		}

		//列挙されたユニットファイルをロードする
		{
			for (size_t i = 0; i < projectSettings.unitFiles.size(); i++) {
				std::string target = projectSettings.unitFiles[i];
				std::ifstream settingsStream(target);

				if (settingsStream.fail()) {
					//読み込みエラー
					ScriptParseErrorData errorData;
					errorData.errorCode = ERROR_SHIORI_003;
					errorData.message = TextSystem::Find(std::string("ERROR_MESSAGE") + ERROR_SHIORI_003);
					errorData.hint = TextSystem::Find(std::string("ERROR_HINT") + ERROR_SHIORI_003);
					scriptLoadErrors.push_back(ScriptParseError(errorData, SourceCodeRange(std::shared_ptr<SourceFilePath>(new SourceFilePath(target, ghostMasterPath + target)), 0, 0, 0, 0)));
					return;
				}

				LoadProjectFile(settingsStream, projectSettings, std::filesystem::path(target).parent_path().string(), true);
			}
		}
#endif

		//プロジェクトデータのロード
		LoadProjectDirectory(ghostMasterPath, projectSettings, isForceDisableDebugSystem);

		//言語情報を設定
		if (!projectSettings.primaryLanguage.empty()) {
			TextSystem::GetInstance()->SetPrimaryLanguage(projectSettings.primaryLanguage);
		}

		//プロジェクトロードのエラー情報を統合
		for (auto err : projectSettings.scriptLoadErrors) {
			scriptLoadErrors.push_back(err);
		}

		//設定の適用
		if (projectSettings.setLimitScriptSteps) {
			interpreter.SetLimitScriptSteps(projectSettings.limitScriptSteps);
		}

		//デバッグが有効ならデバッグシステムを起動する
		if (projectSettings.enableDebug || Debugger::IsDebugBootstrapEnabled()) {
			Debugger::Create(projectSettings.debuggerPort);
		}

		//デバッグモードが有効ならデバッグ出力のストリームを開く
		if (projectSettings.enableDebug && projectSettings.enableDebugLog && !projectSettings.debugOutputFilename.empty()) {
			interpreter.OpenDebugOutputStream(projectSettings.debugOutputFilename);
		}

		std::vector<std::shared_ptr<const sakura::ASTParseResult>> parsedFileList;

		//各ファイルを読み込み処理
		for (auto item : projectSettings.scriptFiles) {
			
			auto loadResult = LoadScriptFile(item);
			if (loadResult->success) {
				parsedFileList.push_back(loadResult);
				interpreter.ImportClasses(loadResult->classMap);
				interpreter.RegisterUnit(loadResult->root->GetSourceMetadata()->GetScriptUnit()->GetUnit());
			}
			else {
				scriptLoadErrors.push_back(*loadResult->error);
			}
		}

		//デバッグブートストラップはここで処理する
		Debugger::Bootstrap();

		if (scriptLoadErrors.empty()) {
			//クラスのリレーションシップ構築
			auto commitError = interpreter.CommitClasses();
			if (commitError != nullptr) {
				scriptLoadErrors.push_back(*commitError.get());
			}
		}

		//エラーが発生していたら以降の処理を打ち切る
		if (!scriptLoadErrors.empty()) {

			//デバッガが接続していたらエラーの内容をデバッガに流す
			if (Debugger::IsConnected()) {
				Debugger::NotifyLog("Aosoraの読み込み時にエラーが発生したためゴーストが正しく起動できませんでした。");
				for (const auto& err : scriptLoadErrors) {
					Debugger::NotifyLog(err.MakeDebuggerErrorString(), err.GetPosition(), true);
				}
			}
			return;
		}

		//ルートスクリプト実行
		for (auto item : parsedFileList) {
			auto rootResult = interpreter.Execute(item->root, false, true);
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
			Request(ShioriRequest("OnAosoraDefaultSaveData"), response);

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

	void Shiori::Unload() {

		//起動時エラーがあればセーブを読めてない可能性もあるので保存しない
		if (!HasBootError()) {

			//セーブ前にアンロードイベント
			{
				ShioriResponse response;
				Request(ShioriRequest("OnAosoraUnload"), response);

				if (response.HasError()) {
					//エラーが出ても無視するほかない
				}
			}

			//セーブして終了
			SaveData::Save(interpreter);
		}
	}

	std::shared_ptr<const ASTParseResult> Shiori::LoadExternalScriptFile(const std::string& fullPath, const std::string& label) {
		std::ifstream loadStream(fullPath, std::ios_base::in);
		std::string fileBody = std::string(std::istreambuf_iterator<char>(loadStream), std::istreambuf_iterator<char>());
		return LoadScriptString(fileBody, SourceFilePath(label, fullPath));
	}

	std::shared_ptr<const ASTParseResult> Shiori::LoadScriptFile(const std::string& path) {
		std::string fullPath = ghostMasterPath + path;
		std::ifstream loadStream(fullPath, std::ios_base::in);

		if (loadStream.fail()) {

			ScriptParseErrorData errorData;
			errorData.errorCode = ERROR_SHIORI_002;
			errorData.message = TextSystem::Find(std::string("ERROR_MESSAGE") + ERROR_SHIORI_002);
			errorData.hint = TextSystem::Find(std::string("ERROR_HINT") + ERROR_SHIORI_002);

			//ファイルがなければ打ち切る
			auto* errorResult = new ASTParseResult();
			errorResult->success = false;
			errorResult->error.reset(new ScriptParseError(errorData, SourceCodeRange(std::shared_ptr<SourceFilePath>(new SourceFilePath(path, fullPath)), 0,0,0,0)));
			return std::shared_ptr<const ASTParseResult>(errorResult);
		}

		std::string fileBody = std::string(std::istreambuf_iterator<char>(loadStream), std::istreambuf_iterator<char>());
		return LoadScriptString(fileBody, SourceFilePath(path, fullPath));
	}

	std::shared_ptr<const ASTParseResult> Shiori::LoadScriptString(const std::string& script, const SourceFilePath& filePath) {
		//解析
		auto tokens = sakura::TokensParser::Parse(script, filePath);
		if (!tokens->success) {
			//トークン解析でエラーなら打ち切る
			auto* errorResult = new ASTParseResult();
			errorResult->success = false;
			errorResult->error = tokens->error;
			return std::shared_ptr<const ASTParseResult>(errorResult);
		}

		auto ast = sakura::ASTParser::Parse(tokens);

		//デバッグシステムに読み込み通知
		if (ast->success) {
			Debugger::NotifyScriptFileLoaded(script, filePath.GetFullPath(), *ast.get());
		}

		return ast;
	}

	void Shiori::Request(const ShioriRequest& request, ShioriResponse& response) {
		RequestInternal(request, response);

		//デバッガに戻ったことを通知
		Debugger::NotifyEventReturned();

		//不要なオブジェクトを開放
		interpreter.CollectObjects();
	}

	void Shiori::RequestInternal(const ShioriRequest& request, ShioriResponse& response) {

		//ステップ数制限をリセット
		interpreter.ResetScriptStep();

		//セキュリティレベルを設定
		interpreter.SetSecurityLevel(request.GetSecurityLevel());

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

		//起動エラーがあるかを取得する(主に as SAORI向け）
		if (request.GetEventId() == "HasLoadError") {
			response.SetValue(HasBootError() ? "1" : "0");
			return;
		}

		//最後の呼出で発生したエラーを取得(主に ss SAORI向け）
		if (request.GetEventId() == "GetLastError") {
			MakeSaoriErrorResponse(response);
			return;
		}

		//イベント名にピリオドがある場合、シンボルとして使用できないため置換する
		std::string eventName = request.GetEventId();
		if (eventName.find(".") != std::string::npos) {
			Replace(eventName, ".", "@");
		}

		if (request.IsSaori()) {
			//起動エラー時SAORIは続行しない
			if (HasBootError()) {
				response.SetValue("");
				response.SetInternalServerError();
				return;
			}
		}
		else {
			//SHIORIで読み込みエラーが生じている場合はフォールバックの表示
			if (!scriptLoadErrors.empty()) {
				RequestScriptLoadErrorFallback(request, response);
				return;
			}
			else if (!bootingExecuteErrorGuide.empty() || !bootingExecuteErrorLog.empty()) {
				RequestScriptBootErrorFallback(request, response);
				return;
			}
		}
		
		//ここまで来ると実行可能なので最後に発生したエラーの情報をリセットする
		lastExecuteErrorLog.clear();

		//呼出ごとの設定をリセット
		TalkBuilder::Prepare(interpreter);

		//エラー処理から呼ばれてリロードに成功していた場合。完了の旨を表示して終了
		if (request.GetEventId() == "OnAosoraReloaded") {
			response.SetValue( std::string() + "\\0\\s[0]\\b[2]\\![quicksession,true]■" + TextSystem::Find("AOSORA_ERROR_RELOADED_0") + "\\n\\n" + TextSystem::Find("AOSORA_ERROR_RELOADED_1"));
			return;
		}

		std::string result;

		//リクエストをグローバル空間に書き込み
		auto shioriObj = interpreter.GetUnitVariable("Shiori", "system");
		if (shioriObj == nullptr || shioriObj->GetObjectInstanceTypeId() != ScriptObject::TypeId()) {
			//ScriptObjectになってなかったら上書き
			shioriObj = ScriptValue::Make(interpreter.CreateObject());
			interpreter.SetUnitVariable("Shiori", shioriObj, "system");
		}

		//ReferenceListの作成
		auto referenceList = interpreter.CreateNativeObject<ScriptArray>();
		for (size_t i = 0; i < request.GetReferenceCount(); i++) {
			referenceList->Add(ScriptValue::Make(request.GetReference(i)));
		}

		auto shioriMap = shioriObj->GetObjectRef().template Cast<ScriptObject>();
		shioriMap->Clear();
		shioriMap->RawSet("Reference", ScriptValue::Make(referenceList));

		//SaoriむけにCValue領域を作成(Shiori時でも同一のコードを動かせるよう配列だけは用意しておく)
		auto saoriValues = interpreter.CreateNativeObject<ScriptArray>();
		shioriMap->RawSet("SaoriValues", ScriptValue::Make(saoriValues));

		//コミュニケート用のResponseのReferenceを提供する
		auto shioriResponse = interpreter.CreateArray();
		shioriMap->RawSet("ResponseReference", ScriptValue::Make(shioriResponse));

		//stautsの該当するレコードにtrueを書き込み(該当なければnullになるため判別に使用できる)
		for (const std::string& st : request.GetStatusCollection()) {
			shioriMap->RawSet(st, ScriptValue::True);
		}

		//ヘッダ情報まとめる
		auto headers = interpreter.CreateObject();
		for (const auto& kv : request.GetRawCollection()) {
			headers->Add(kv.first, ScriptValue::Make(kv.second));
		}
		shioriMap->RawSet("Headers", ScriptValue::Make(headers));

		//グローバル空間からイベントを探す
		auto variable = interpreter.GetUnitVariable(eventName, "main");

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
					HandleRuntimeError(funcResponse.GetThrewError(), response, request.IsSaori());
					return;
				}

				//結果を文字列化
				if (funcResponse.GetReturnValue() != nullptr)
				{
					auto toStringResult = funcResponse.GetReturnValue()->ToStringWithFunctionCall(interpreter);

					//文字列化時の実行時エラー
					if (!toStringResult.success) {
						HandleRuntimeError(toStringResult.error, response, request.IsSaori());
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
				HandleRuntimeError(eventRes.GetThrewError(), response, request.IsSaori());
				return;
			}

			auto toStringResult = eventRes.GetReturnValue()->ToStringWithFunctionCall(interpreter);
			if (!toStringResult.success) {
				HandleRuntimeError(toStringResult.error, response, request.IsSaori());
				return;
			}

			response.SetValue(toStringResult.result);
		}
		else {
			response.SetValue(result);
		}

		if (request.IsSaori()) {
			//SAORIの値を返す
			//オブジェクトをたどり直す(配列イニシャライザでも指定できるように)
			auto responseSaoriValues = shioriMap->RawGet("SaoriValues");
			if (responseSaoriValues != nullptr) {
				ScriptArray* scriptItems = interpreter.InstanceAs<ScriptArray>(responseSaoriValues);
				if (scriptItems != nullptr) {
					std::vector<std::string> items;
					for (size_t i = 0; i < scriptItems->Count(); i++) {
						items.push_back(scriptItems->At(i)->ToString());
					}
					response.SetSaoriValues(items);
				}
			}
		}
		else {
			//SHIORIのResponseに入れるReferenceを格納
			//ゴースト間コミュニケートに使われるもの
			auto responseCommunicateReferences = shioriMap->RawGet("ResponseReference");
			if (responseCommunicateReferences != nullptr) {
				ScriptArray* scriptItems = interpreter.InstanceAs<ScriptArray>(responseCommunicateReferences);
				if (scriptItems != nullptr) {
					std::vector<std::string> items;
					for (size_t i = 0; i < scriptItems->Count(); i++) {
						items.push_back(scriptItems->At(i)->ToString());
					}
					response.SetShioriReferences(items);
				}
			}
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
		std::string errorGuide = std::string() + "\\0\\b[2]\\s[0]\\![quicksession,true]■" + TextSystem::Find("AOSORA_BOOT_ERROR_0") + "\\n" + TextSystem::Find("AOSORA_BOOT_ERROR_1") + "\\n";

		//エラー情報をリストアップ
		for (size_t i = 0; i < scriptLoadErrors.size(); i++) {
			const auto& err = scriptLoadErrors[i];
			errorGuide += "\\n\\![*]\\q[" + err.GetPosition().ToString() + ",OnAosoraErrorView," + std::to_string(i) + "]\\n" + err.GetData().errorCode + ": " + err.GetData().message + "\\n";
		}

		errorGuide += std::string() + "\\n\\![*]\\q[" + TextSystem::Find("AOSORA_BALLOON_CLOSE") + ",OnAosoraErrorClose]";
		if (!Debugger::IsDebugBootstrapped()) {
			errorGuide += std::string() + "\\n\\![*]\\q[" + TextSystem::Find("AOSORA_BALLOON_RELOAD") + ",OnAosoraRequestReload]";
		}
		else {
			errorGuide += std::string() + "\\n" + TextSystem::Find("AOSORA_RELOAD_DEBUGGER_HINT");
		}
		return errorGuide;
	}

	std::string Shiori::ShowErrorDetail(size_t index) {
		

		const auto& err = scriptLoadErrors[index];
		std::string errorGuide = std::string() + "\\0\\b[2]\\s[0]\\![quicksession,true]■" + TextSystem::Find("AOSORA_BOOT_ERROR_2") + "　";
		
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
		
		errorGuide += std::string() + "\\n\\n\\_?[" + TextSystem::Find("AOSORA_BOOT_ERROR_4") + "]\\_?\\n\\_?" + err.GetPosition().ToString() + "\\_?\\n\\n\\_?[" + TextSystem::Find("AOSORA_BOOT_ERROR_5") + "]\\_?\\n\\_?" + err.GetData().errorCode + ": " + err.GetData().message + "\\_?\\n\\n\\_?[" + TextSystem::Find("AOSORA_BOOT_ERROR_6") + "]\\_?\\n\\_?" + err.GetData().hint + "\\_?";
		errorGuide += std::string() + "\\n\\n\\![*]\\q[" + TextSystem::Find("AOSORA_BOOT_ERROR_3") + ",OnAosoraErrors]";
		errorGuide += std::string() + "\\n\\![*]\\q[" + TextSystem::Find("AOSORA_BALLOON_CLOSE") + ",OnAosoraErrorClose]";
		if (!Debugger::IsDebugBootstrapped()) {
			errorGuide += std::string() + "\\n\\![*]\\q[" + TextSystem::Find("AOSORA_BALLOON_RELOAD") + ",OnAosoraRequestReload]";
		}
		else {
			errorGuide += std::string() + "\\n" + TextSystem::Find("AOSORA_RELOAD_DEBUGGER_HINT");
		}
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
	void Shiori::HandleRuntimeError(const ObjectRef& errObj, ShioriResponse& response, bool isSaori) {
	
		lastExecuteErrorLog = ToStringRuntimeErrorForErrorLog(errObj);

		if (!isSaori) {
			response.SetValue(ToStringRuntimeErrorForSakuraScript(errObj, !isBooted));
			response.AddError(ShioriError(ShioriError::ErrorLevel::Error, lastExecuteErrorLog));
		}
		else {
			//SAORIの場合、エラーは表示せずにGetLastErrorでエラーにさせる
			response.SetValue("");
			response.SetInternalServerError();
		}
	}

	//SAORIむけ：エラーレスポンスを作成
	void Shiori::MakeSaoriErrorResponse(ShioriResponse& response) {
		if (HasBootError()) {
			//起動エラー
			std::vector<std::string> errorList;
			for (const auto& err : scriptLoadErrors) {
				errorList.push_back(err.GetPosition().ToString() + " [" + err.GetData().errorCode + "] " + err.GetData().message);
			}
			if (!bootingExecuteErrorLog.empty()) {
				errorList.push_back(bootingExecuteErrorLog);
			}
			response.SetSaoriValues(errorList);
			response.SetValue(std::to_string(errorList.size()));
		}
		else if (!lastExecuteErrorLog.empty()) {
			//実行エラー
			std::vector<std::string> errorList;
			errorList.push_back(lastExecuteErrorLog);
			response.SetSaoriValues(errorList);
			response.SetValue("1");
		}
		else {
			//エラーなし
			response.SetValue("0");
		}
	}

	//エラーをさくらスクリプトによるゴースト上での表示むけに整形
	std::string Shiori::ToStringRuntimeErrorForSakuraScript(const ObjectRef& errObj, bool isBooting) {
		auto* err = interpreter.InstanceAs<ScriptError>(errObj);
		assert(err != nullptr);

		std::string ghostErrorGuide = std::string() + "\\0\\b[2]\\s[0]\\![quicksession,true]■" + TextSystem::Find("AOSORA_RUNTIME_ERROR_0") + "\\n" + TextSystem::Find("AOSORA_RUNTIME_ERROR_1") + "\\n\\n";

		if (isBooting) {
			//起動エラーの場合は区別する
			ghostErrorGuide = std::string() + "\\0\\b[2]\\s[0]\\![quicksession,true]■" + TextSystem::Find("AOSORA_BOOT_ERROR_0") + "\\n" + TextSystem::Find("AOSORA_BOOT_ERROR_1") + "\\n\\n";
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

		ghostErrorGuide += std::string() + "\\_?[" + TextSystem::Find("AOSORA_RUNTIME_ERROR_2") + "]\\_?\\n" + firstTrace + "\\n\\_?[" + TextSystem::Find("AOSORA_RUNTIME_ERROR_3") + "]\\_?\\n" + "\\_?" + err->GetErrorMessage() + "\\_?" + "\\n\\n\\_?[" + TextSystem::Find("AOSORA_RUNTIME_ERROR_4") + "]\\_?\\n" + trace;
		ghostErrorGuide += std::string() + "\\n\\![*]\\q[" + TextSystem::Find("AOSORA_BALLOON_CLOSE") + ",OnAosoraErrorClose]";
		if (!Debugger::IsDebugBootstrapped()) {
			ghostErrorGuide += std::string() + "\\n\\![*]\\q[" + TextSystem::Find("AOSORA_BALLOON_RELOAD") + ",OnAosoraRequestReload]";
		}
		else {
			ghostErrorGuide += std::string() + "\\n" + TextSystem::Find("AOSORA_RELOAD_DEBUGGER_HINT");
		}

		return ghostErrorGuide;
	}

	//エラーをエラーログ等の表示用に平文で整形
	std::string Shiori::ToStringRuntimeErrorForErrorLog(const ObjectRef& errObj) {
		auto* err = interpreter.InstanceAs<ScriptError>(errObj);
		assert(err != nullptr);

		std::string scriptErrorLog = TextSystem::Find("AOSORA_RUNTIME_ERROR_5");

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

		scriptErrorLog += std::string() + "[" + TextSystem::Find("AOSORA_RUNTIME_ERROR_2") + "] " + firstTrace + " [" + TextSystem::Find("AOSORA_RUNTIME_ERROR_3") + "] " + err->GetErrorMessage() + " [" + TextSystem::Find("AOSORA_RUNTIME_ERROR_4") + "] " + trace;
		return scriptErrorLog;
	}
}
