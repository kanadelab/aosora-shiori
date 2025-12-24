#include <filesystem>
#include <fstream>
#include "CommonLibrary/StandardLibrary.h"
#include "CommonLibrary/CommonClasses.h"
#include "Misc/Json.h"
#include "Misc/Message.h"


namespace sakura {

	//Jsonシリアライザ
	ScriptValueRef ScriptJsonSerializer::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {

		if (key == "Serialize") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptJsonSerializer::ScriptSerialize));
		}
		else if (key == "Deserialize") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptJsonSerializer::ScriptDeserialize));
		}

		return nullptr;
	}

	void ScriptJsonSerializer::ScriptSerialize(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() > 0) {
			//文字列二シリアライズして返す
			response.SetReturnValue(
				ScriptValue::Make(JsonSerializer::Serialize(ObjectSerializer::Serialize(request.GetArgument(0))))
			);
		}
		else {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
		}
	}

	void ScriptJsonSerializer::ScriptDeserialize(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() > 0) {
			auto jsonStr = request.GetArgument(0);
			if (jsonStr->IsString()) {
				//文字列をデシリアライズして戻す、失敗時はnullが帰る
				response.SetReturnValue(
					ObjectSerializer::Deserialize(jsonStr->ToString(), request.GetContext().GetInterpreter())
					);
			}
			else {
				// 文字列でない
				response.SetThrewError(
					request.GetContext().GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_JSON_SERIALIZER_ERROR_001"))
				);
			}
		}
		else {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
		}
	}

	
	//ファイルアクセス
	ScriptValueRef ScriptFileAccess::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "ReadAllText") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::ReadAllText));
		}
		else if (key == "WriteAllText") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::WriteAllText));
		}
		else if (key == "Move") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::Move));
		}
		else if (key == "Copy") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::Copy));
		}
		else if (key == "Delete") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::Delete));
		}
		else if (key == "Exists") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::Exists));
		}
		else if (key == "IsDirectory") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::IsDirectory));
		}
		else if (key == "IsFile") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::IsFile));
		}
		else if (key == "CreateDirectory") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptFileAccess::CreateDirectory));
		}

		return nullptr;
	}

	bool ScriptFileAccess::ValidateFileAccess(const FunctionRequest& request, FunctionResponse& response) {
		if (!request.GetInterpreter().IsAllowLocalAccess()) {
			//アクセス不許容
			response.SetReturnValue(ScriptValue::MakeArray(request.GetInterpreter()));
			return false;
		}
		else {
			return true;
		}
	}

	//引数からcharset指定部分を取得
	Charset ScriptFileAccess::CharsetFromFunctionArgs(size_t index, const FunctionRequest& request) {
		if (request.GetArgumentCount() <= index) {
			//指定を省略している場合はutf-8
			return Charset::UTF_8;
		}

		if (request.GetArgument(index)->IsNull()) {
			//nullを指定している場合もutf-8でいい
			return Charset::UTF_8;
		}

		if (!request.GetArgument(index)->IsString()) {
			//指定が正しくない
			return Charset::UNKNOWN;
		}

		return CharsetFromString(request.GetArgument(index)->ToString());
	}

	//読み込み向けに文字コード変換
	void ScriptFileAccess::ConvertReadCharset(std::string& str, Charset charset) {
		assert(charset != Charset::UNKNOWN);

		if (charset == Charset::SHIFT_JIS) {
			str = SjisToUtf8(str);
		}
	}

	//書き出し向けに文字コード変換
	void ScriptFileAccess::ConvertWriteCharset(std::string& str, Charset charset) {
		assert(charset != Charset::UNKNOWN);

		if (charset == Charset::SHIFT_JIS) {
			str = Utf8ToSjis(str);
		}
	}

	void ScriptFileAccess::ReadAllText(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() < 1) {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(0)->IsString() || !request.GetArgument(0)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		//charset取得
		Charset charset = CharsetFromFunctionArgs(1, request);
		if (charset == Charset::UNKNOWN) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_002"))
			);
			return;
		}

		if (!ValidateFileAccess(request, response)) {
			response.SetReturnValue(ScriptValue::Null);
			return;
		}

		//ファイルをよむ
		const std::string targetPath = request.GetInterpreter().GetFileName(request.GetArgument(0)->ToString());
		std::string fileBody;
		if (!File::ReadAllText(targetPath, fileBody)) {
			response.SetReturnValue(ScriptValue::Null);
			return;
		}

		//文字コード変換
		ConvertReadCharset(fileBody, charset);

		//読込結果
		response.SetReturnValue(ScriptValue::Make(fileBody));
	}

	void ScriptFileAccess::WriteAllText(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() < 1) {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(0)->IsString() || !request.GetArgument(0)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		//charset取得
		Charset charset = CharsetFromFunctionArgs(2, request);
		if (charset == Charset::UNKNOWN) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_002"))
			);
			return;
		}

		if (!ValidateFileAccess(request, response)) {
			response.SetReturnValue(ScriptValue::Null);
			return;
		}

		//ファイルを書き込む
		const std::string targetPath = request.GetInterpreter().GetFileName(request.GetArgument(0)->ToString());
		std::string fileBody = request.GetArgument(1)->ToString();
		ConvertWriteCharset(fileBody, charset);
		response.SetReturnValue(ScriptValue::Make(File::WriteAllText(targetPath, fileBody)));
	}

	void ScriptFileAccess::Delete(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() < 1) {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(0)->IsString() || !request.GetArgument(0)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		if (!ValidateFileAccess(request, response)) {
			response.SetReturnValue(ScriptValue::False);
			return;
		}

		std::string targetPath = request.GetInterpreter().GetFileName(request.GetArgument(0)->ToString());
		std::error_code errCode;
		std::filesystem::remove_all(targetPath, errCode);
		if (!errCode) {
			//成功
			response.SetReturnValue(ScriptValue::True);
			return;
		}

		//失敗
		response.SetReturnValue(ScriptValue::False);
	}

	void ScriptFileAccess::Move(const FunctionRequest& request, FunctionResponse& response) {

		if (request.GetArgumentCount() < 2) {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(0)->IsString() || !request.GetArgument(0)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(1)->IsString() || !request.GetArgument(1)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		if (!ValidateFileAccess(request, response)) {
			response.SetReturnValue(ScriptValue::False);
			return;
		}

		const std::string srcPath = request.GetInterpreter().GetFileName(request.GetArgument(0)->ToString());
		const std::string destPath = request.GetInterpreter().GetFileName(request.GetArgument(1)->ToString());

		//移動
		std::error_code errCode;
		std::filesystem::rename(srcPath, destPath, errCode);

		if (!errCode) {
			response.SetReturnValue(ScriptValue::True);
		}
		else {
			response.SetReturnValue(ScriptValue::False);
		}
	}

	void ScriptFileAccess::Copy(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() < 2) {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(0)->IsString() || !request.GetArgument(0)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(1)->IsString() || !request.GetArgument(1)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		if (!ValidateFileAccess(request, response)) {
			response.SetReturnValue(ScriptValue::False);
			return;
		}

		const std::string srcPath = request.GetInterpreter().GetFileName(request.GetArgument(0)->ToString());
		const std::string destPath = request.GetInterpreter().GetFileName(request.GetArgument(1)->ToString());

		//移動
		std::error_code errCode;
		std::filesystem::copy(srcPath, destPath, std::filesystem::copy_options::overwrite_existing, errCode);

		if (!errCode) {
			response.SetReturnValue(ScriptValue::True);
		}
		else {
			response.SetReturnValue(ScriptValue::False);
		}
	}

	void ScriptFileAccess::PathCheck(const FunctionRequest& request, FunctionResponse& response, PathCheckMode mode) {
		if (request.GetArgumentCount() < 1) {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(0)->IsString() || !request.GetArgument(0)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		if (!ValidateFileAccess(request, response)) {
			response.SetReturnValue(ScriptValue::False);
			return;
		}

		std::string targetPath = request.GetInterpreter().GetFileName(request.GetArgument(0)->ToString());
		std::error_code errCode;

		bool result = false;
		switch (mode) {
		case PathCheckMode::Exists:
			result = std::filesystem::exists(targetPath, errCode);
			break;
		case PathCheckMode::IsDirectory:
			result = std::filesystem::is_directory(targetPath, errCode);
			break;
		case PathCheckMode::IsFile:
			result = std::filesystem::is_regular_file(targetPath, errCode);
			break;
		}

		response.SetReturnValue(ScriptValue::Make(result));
	}

	void ScriptFileAccess::Exists(const FunctionRequest& request, FunctionResponse& response) {
		PathCheck(request, response, PathCheckMode::Exists);
	}

	void ScriptFileAccess::IsDirectory(const FunctionRequest& request, FunctionResponse& response) {
		PathCheck(request, response, PathCheckMode::IsDirectory);
	}

	void ScriptFileAccess::IsFile(const FunctionRequest& request, FunctionResponse& response) {
		PathCheck(request, response, PathCheckMode::IsFile);
	}

	void ScriptFileAccess::CreateDirectory(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() < 1) {
			// 引数不足
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_001"))
			);
			return;
		}

		if (!request.GetArgument(0)->IsString() || !request.GetArgument(0)->ToBoolean()) {
			// 入力が文字列でない
			response.SetThrewError(
				request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_FILE_ACCESS_ERROR_001"))
			);
			return;
		}

		if (!ValidateFileAccess(request, response)) {
			response.SetReturnValue(ScriptValue::False);
			return;
		}

		std::string targetPath = request.GetInterpreter().GetFileName(request.GetArgument(0)->ToString());
		std::error_code errCode;
		std::filesystem::create_directories(targetPath, errCode);
		if (!errCode) {
			//成功
			response.SetReturnValue(ScriptValue::True);
			return;
		}

		//失敗
		response.SetReturnValue(ScriptValue::False);
	}

	void ScriptSSTP::GetProperty(const FunctionRequest& request, FunctionResponse& response){

		callbackResult = "";

		//自分で書いた里々のコードを移植
#if defined(AOSORA_REQUIRED_WIN32)
		auto staticStore = request.GetInterpreter().StaticStore<ScriptSSTP>();
		if (!staticStore->hwndList.empty() && request.GetArgumentCount() > 0) {
			const std::string propertyName = request.GetArgument(0)->ToString();

			//蒼空からベースウェアにDirectSSTPを飛ばしてプロパティシステムにアクセスする。元のSHIORI呼出を返さずにプロパティを取れる。

			//結果受信用ウインドウ作成: リソースの仕様を局所化してみたけどオーバーヘッドがでかい場合はSHIORIの初期化周辺に絡めるといいのかも
			const char* windowname = "aosora_get_property";

			WNDCLASSEX windowClass;
			ZeroMemory(&windowClass, sizeof(windowClass));

			windowClass.cbSize = sizeof(windowClass);
			windowClass.hInstance = GetModuleHandle(NULL);
			windowClass.lpszClassName = windowname;
			windowClass.lpfnWndProc = &ScriptSSTP::GetPropertyHandler;

			::RegisterClassEx(&windowClass);

			HWND propertyWindow = ::CreateWindow(windowname, windowname, 0, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

			//リクエスト作成
			const HWND targetHWnd = staticStore->hwndList[0];
			std::ostringstream ost;
			ost << "EXECUTE SSTP/1.1\r\nCommand: GetProperty[" << propertyName << "]\r\nSender: Satori\r\nCharset: Shift_JIS\r\n\r\n";
			std::string sendData = ost.str();

			//メッセージ転送
			COPYDATASTRUCT cds;
			cds.dwData = 9801;
			cds.cbData = sendData.size();
			cds.lpData = malloc(cds.cbData);
			memcpy(cds.lpData, sendData.c_str(), cds.cbData);

			/*LRESULT res =*/ ::SendMessage(targetHWnd, WM_COPYDATA, (WPARAM)propertyWindow, (LPARAM)&cds);

			//リソースの開放
			free(cds.lpData);

			::DestroyWindow(propertyWindow);
			::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		}
#endif
		response.SetReturnValue(ScriptValue::Make(callbackResult));
	}

#if defined(AOSORA_REQUIRED_WIN32)
	std::string ScriptSSTP::callbackResult;
	LRESULT CALLBACK ScriptSSTP::GetPropertyHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		if (message == WM_COPYDATA) {
			const COPYDATASTRUCT* cds = (const COPYDATASTRUCT*)lparam;
			const std::string recvStr((const char*)cds->lpData, cds->cbData);

			std::vector<std::string> lines;
			std::vector<std::string> header;
			SplitString(recvStr, lines, "\r\n", 0);
			if (lines.size() > 2) {
				SplitString(lines[0], header, " ", 2);

				if (header.size() > 1) {
					if (header[1] == "200 OK") {
						//1行取得
						callbackResult = lines[2];
					}
				}
			}
		}
		return CallWindowProc(DefWindowProc, hwnd, message, wparam, lparam);
	}
#endif

	void ScriptSSTP::HandleEvent(ScriptInterpreter& interpreter, const ShioriRequest& shioriRequest) {
#if defined(AOSORA_REQUIRED_WIN32)
		if (!shioriRequest.IsGet() && shioriRequest.GetEventId() == "hwnd") {
			auto staticStore = interpreter.StaticStore<ScriptSSTP>();
			std::vector<std::string> items;
			SplitString(shioriRequest.GetReference(0), items, static_cast<char>(1));

			staticStore->hwndList.clear();
			for (const auto& item : items) {
				staticStore->hwndList.push_back(reinterpret_cast<HWND>(std::stoull(item)));
			}
		}
#endif
	}

	ScriptValueRef ScriptSSTP::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "GetProperty") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&ScriptSSTP::GetProperty));
		}
		return nullptr;
	}

	ScriptValueRef Regex::MakeMatchObject(ScriptInterpreter& scriptInterpreter, const std::string& inputString, const std::string& matchString, size_t index, size_t length) {
		Reference<ScriptObject> result = scriptInterpreter.CreateObject();
		result->RawSet("index", ScriptValue::Make(static_cast<double>(ByteIndexToUncodeCharIndex(inputString, index))));
		result->RawSet("length", ScriptValue::Make(static_cast<double>(CountUnicodeCharacters(matchString))));
		result->RawSet("string", ScriptValue::Make(matchString));
		return ScriptValue::Make(result);
	}

	void Regex::ScriptMatch(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 2) {
			std::string target = request.GetArgument(0)->ToString();
			std::string pattern = request.GetArgument(1)->ToString();
			std::smatch matchResult;
			std::regex pat;
			
			try {
				pat = std::regex(pattern);
			}
			catch(std::exception&){
				response.SetThrewError(request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_REGEX_INVALID_PATTERN")));
				return;
			}

			if (std::regex_search(target, matchResult, pat)) {
				response.SetReturnValue(MakeMatchObject(request.GetInterpreter(), target, matchResult.str(), matchResult.position(), matchResult.length()));
			}
			else {
				response.SetReturnValue(ScriptValue::Null);
			}
		}
		else {
			response.SetThrewError(request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_002")));
		}
	}

	void Regex::ScriptIsMatch(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 2) {
			std::string target = request.GetArgument(0)->ToString();
			std::string pattern = request.GetArgument(1)->ToString();
			std::smatch matchResult;
			std::regex pat;

			try {
				pat = std::regex(pattern);
			}
			catch (std::exception&) {
				response.SetThrewError(request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_REGEX_INVALID_PATTERN")));
				return;
			}

			// 一致するかどうかの判定だけ
			if (std::regex_search(target, matchResult, pat)) {
				response.SetReturnValue(ScriptValue::True);
			}
			else {
				response.SetReturnValue(ScriptValue::False);
			}
		}
		else {
			response.SetThrewError(request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_002")));
		}
	}

	void Regex::ScriptReplace(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 3) {
			std::string target = request.GetArgument(0)->ToString();
			std::string pattern = request.GetArgument(1)->ToString();
			std::string replaced = request.GetArgument(2)->ToString();
			std::smatch matchResult;
			std::regex pat;

			try {
				pat = std::regex(pattern);
			}
			catch (std::exception&) {
				response.SetThrewError(request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_REGEX_INVALID_PATTERN")));
				return;
			}

			size_t index = 0;
			std::string resultString;

			//マッチするだけ繰り返す
			while (true) {
				if (!std::regex_search(target.cbegin() + index, target.cend(), matchResult, pat)) {
					resultString.append(target.cbegin() + index, target.cend());
					break;
				}
				//一致箇所をリプレース
				resultString.append(target.substr(0, matchResult.position()));
				resultString.append(replaced);
				index += matchResult.position() + matchResult.length();
			}
			response.SetReturnValue(ScriptValue::Make(resultString));
		}
		else {
			response.SetThrewError(request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_002")));
		}
	}

	void Regex::ScriptMatchAll(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 2) {
			std::string target = request.GetArgument(0)->ToString();
			std::string pattern = request.GetArgument(1)->ToString();
			std::smatch matchResult;
			std::regex pat;

			try {
				pat = std::regex(pattern);
			}
			catch (std::exception&) {
				response.SetThrewError(request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_REGEX_INVALID_PATTERN")));
				return;
			}

			size_t index = 0;
			Reference<ScriptArray> resultArray = request.GetInterpreter().CreateArray();

			//マッチするだけ繰り返す
			while (true) {
				if (!std::regex_search(target.cbegin() + index, target.cend(), matchResult, pat)) {
					break;
				}
				//一致箇所を記録
				resultArray->Add(MakeMatchObject(request.GetInterpreter(), target, matchResult.str(), index + matchResult.position(), matchResult.length()));
				index += matchResult.position() + matchResult.length();
			}
			response.SetReturnValue(ScriptValue::Make(resultArray));
		}
		else {
			response.SetThrewError(request.GetInterpreter().CreateNativeObject<RuntimeError>(TextSystem::Find("AOSORA_COMMON_ERROR_002")));
		}
	}

	ScriptValueRef Regex::StaticGet(const std::string& key, ScriptExecuteContext& executeContext) {
		if (key == "Match") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Regex::ScriptMatch));
		}
		else if (key == "IsMatch") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Regex::ScriptIsMatch));
		}
		else if (key == "Matches") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Regex::ScriptMatchAll));
		}
		else if (key == "Replace") {
			return ScriptValue::Make(executeContext.GetInterpreter().CreateNativeObject<Delegate>(&Regex::ScriptReplace));
		}
		return nullptr;
	}
}