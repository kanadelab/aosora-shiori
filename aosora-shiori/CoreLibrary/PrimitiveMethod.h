#pragma once
#include "Interpreter/ScriptVariable.h"


namespace sakura {

	//オブジェクト以外の基本型につくメソッド
	class PrimitiveMethod {
	private:
		static void Number_Floor(const FunctionRequest& request, FunctionResponse& response);
		static void Number_Round(const FunctionRequest& request, FunctionResponse& response);
		static void Number_Ceil(const FunctionRequest& request, FunctionResponse& response);
		static void Number_ToAscii(const FunctionRequest& request, FunctionResponse& response);

		static void String_StartWith(const FunctionRequest& request, FunctionResponse& response);
		static void String_EndsWith(const FunctionRequest& request, FunctionResponse& response);
		static void String_Split(const FunctionRequest& request, FunctionResponse& response);
		static void String_Substring(const FunctionRequest& request, FunctionResponse& response);
		static void String_IndexOf(const FunctionRequest& request, FunctionResponse& response);
		static void String_Replace(const FunctionRequest& request, FunctionResponse& response);
		static void String_AddTalk(const FunctionRequest& request, FunctionResponse& response);
		static void String_ToUpper(const FunctionRequest& request, FunctionResponse& response);
		static void String_ToLower(const FunctionRequest& request, FunctionResponse& response);
		static void String_Contains(const FunctionRequest& request, FunctionResponse& response);
		static void String_EscapeSakuraScript(const FunctionRequest& request, FunctionResponse& response);

		static void General_ToString(const FunctionRequest& request, FunctionResponse& response);
		static void General_ToNumber(const FunctionRequest& request, FunctionResponse& response);
		static void General_ToBoolean(const FunctionRequest& request, FunctionResponse& response);
		static void General_IsString(const FunctionRequest& request, FunctionResponse& response);
		static void General_IsNumber(const FunctionRequest& request, FunctionResponse& response);
		static void General_IsBoolean(const FunctionRequest& request, FunctionResponse& response);
		static void General_IsObject(const FunctionRequest& request, FunctionResponse& response);
		static void General_IsNan(const FunctionRequest& request, FunctionResponse& response);
		static void General_IsNull(const FunctionRequest& request, FunctionResponse& response);
		static void General_InstanceOf(const FunctionRequest& request, FunctionResponse& response);
			
	public:
		static ScriptValueRef GetNumberMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context);
		static ScriptValueRef GetBooleanMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext & context);
		static ScriptValueRef GetStringMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context);
		static ScriptValueRef GetGeneralMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context);
	};

}