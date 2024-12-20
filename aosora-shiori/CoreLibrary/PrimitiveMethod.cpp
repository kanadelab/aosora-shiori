#include "CoreLibrary/PrimitiveMethod.h"
#include "CoreLibrary/CoreClasses.h"

namespace sakura {

	void PrimitiveMethod::Number_Floor(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(std::floor(request.GetContext().GetBlockScope()->GetThisValue()->ToNumber())));
	}

	void PrimitiveMethod::Number_Round(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(std::round(request.GetContext().GetBlockScope()->GetThisValue()->ToNumber())));
	}

	void PrimitiveMethod::Number_Ceil(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(std::ceil(request.GetContext().GetBlockScope()->GetThisValue()->ToNumber())));
	}

	ScriptValueRef PrimitiveMethod::GetNumberMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context) {
		if (member == "Round") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::Number_Round, value));
		}
		else if (member == "Floor") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::Number_Floor, value));
		}
		else if (member == "Ceil") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::Number_Ceil, value));
		}

		return nullptr;
	}

	ScriptValueRef PrimitiveMethod::GetBooleanMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context) {
		//今のところなし
		return nullptr;
	}

	ScriptValueRef PrimitiveMethod::GetStringMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context) {
		//いまのところなし
		return nullptr;
	}

}