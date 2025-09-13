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

	void ScriptSerialize(const FunctionRequest& request, FunctionResponse& response) {
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

	void ScriptDeserialize(const FunctionRequest& request, FunctionResponse& response) {
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

}