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
			SjisToUtf8(str);
		}
	}

	//書き出し向けに文字コード変換
	void ScriptFileAccess::ConvertWriteCharset(std::string& str, Charset charset) {
		assert(charset != Charset::UNKNOWN);

		if (charset == Charset::SHIFT_JIS) {
			Utf8ToSjis(str);
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
		std::filesystem::copy(srcPath, destPath, errCode);

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

		if (!request.GetArgument(0)->IsString() || !request.GetArgument(1)->ToBoolean()) {
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

}