#pragma once
#include "Interpreter/ScriptVariable.h"


namespace sakura {

	//オブジェクト以外の基本型につくメソッド
	class PrimitiveMethod {
	private:
		static void Number_Floor(const FunctionRequest& request, FunctionResponse& response);
		static void Number_Round(const FunctionRequest& request, FunctionResponse& response);
		static void Number_Ceil(const FunctionRequest& request, FunctionResponse& response);

	public:
		static ScriptValueRef GetNumberMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context);
		static ScriptValueRef GetBooleanMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext & context);
		static ScriptValueRef GetStringMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context);
	};

}