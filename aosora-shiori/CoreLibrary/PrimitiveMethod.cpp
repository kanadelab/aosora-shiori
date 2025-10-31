#include <regex>
#include "CoreLibrary/PrimitiveMethod.h"
#include "CoreLibrary/CoreClasses.h"

//Unicode文字数を文字インデックスとして使う
#define AOSORA_USE_UNICODE_CHAR_INDEX

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

	void PrimitiveMethod::General_ToString(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(request.GetThisValue()->ToString()));
	}

	void PrimitiveMethod::Number_ToAscii(const FunctionRequest& request, FunctionResponse& response) {
		size_t n;
		if (request.GetThisValue()->ToIndex(n) && n > 0 && n < 128) {
			std::string v(1, static_cast<char>(n));
			response.SetReturnValue(ScriptValue::Make(v));
		}
	}

	void PrimitiveMethod::General_ToNumber(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(request.GetThisValue()->ToNumber()));
	}

	void PrimitiveMethod::General_ToBoolean(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(request.GetThisValue()->ToBoolean()));
	}

	void PrimitiveMethod::General_IsString(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(request.GetThisValue()->IsString()));
	}

	void PrimitiveMethod::General_IsNumber(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(request.GetThisValue()->IsNumber()));
	}

	void PrimitiveMethod::General_IsBoolean(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(request.GetThisValue()->IsBoolean()));
	}

	void PrimitiveMethod::General_IsObject(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(request.GetThisValue()->IsObject()));
	}

	void PrimitiveMethod::General_IsNull(const FunctionRequest& request, FunctionResponse& response) {
		response.SetReturnValue(ScriptValue::Make(request.GetThisValue()->IsNull()));
	}

	void PrimitiveMethod::General_InstanceOf(const FunctionRequest& request, FunctionResponse& response) {
		//インスタンスの確認
		if (request.GetArgumentCount() >= 1) {
			if (request.GetArgument(0)->GetObjectInstanceTypeId() == ClassData::TypeId()) {
				auto cls = request.GetArgument(0)->GetObjectRef().Cast<ClassData>();
				if (request.GetContext().GetInterpreter().InstanceIs(request.GetThisValue(), cls->GetClassTypeId())) {
					response.SetReturnValue(ScriptValue::True);
					return;
				}
			}
		}
		response.SetReturnValue(ScriptValue::False);
	}

	void PrimitiveMethod::General_IsNan(const FunctionRequest& request, FunctionResponse& response) {
		auto self = request.GetThisValue();
		if (self->IsNumber() && std::isnan(self->ToNumber())) {
			response.SetReturnValue(ScriptValue::True);
		}
		else {
			response.SetReturnValue(ScriptValue::False);
		}
	}

	void PrimitiveMethod::String_StartWith(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 1) {
			std::string target = request.GetThisValue()->ToString();
			std::string search = request.GetArgument(0)->ToString();
			response.SetReturnValue(ScriptValue::Make(target.starts_with(search)));
		}
	}

	void PrimitiveMethod::String_EndsWith(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 1) {
			std::string target = request.GetThisValue()->ToString();
			std::string search = request.GetArgument(0)->ToString();
			response.SetReturnValue(ScriptValue::Make(target.ends_with(search)));
		}
	}

	void PrimitiveMethod::String_Split(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 1) {
			size_t maxCount = 0;
			if (request.GetArgumentCount() >= 2) {
				request.GetArgument(1)->ToIndex(maxCount);
			}
			
			std::string target = request.GetThisValue()->ToString();
			std::string delimiter = request.GetArgument(0)->ToString();

			if (target.empty() || delimiter.empty()) {
				return;
			}
			
			std::vector<std::string> result;
			SplitString(target, result, delimiter, maxCount);

			//リザルトの作成
			auto items = request.GetContext().GetInterpreter().CreateNativeObject<ScriptArray>();
			for (const std::string& item : result) {
				items->Add(ScriptValue::Make(item));
			}

			response.SetReturnValue(ScriptValue::Make(items));
		}
	}

	void PrimitiveMethod::String_Substring(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 1) {
			size_t offset = 0;
			request.GetArgument(0)->ToIndex(offset);

			if (request.GetArgumentCount() >= 2) {
				size_t count = 0;
				if (request.GetArgument(1)->ToIndex(count)) {
					std::string target = request.GetThisValue()->ToString();
#if defined(AOSORA_USE_UNICODE_CHAR_INDEX)
					size_t mbOffset = UnicodeCharIndexToMultiByteIndex(target, offset);
					size_t mbCount = UnicodeCharIndexToMultiByteIndex(target, offset + count) - mbOffset;
					std::string result = target.substr(mbOffset, mbCount);
#else
					std::string result = target.substr(offset, count);
#endif
					response.SetReturnValue(ScriptValue::Make(result));
					return;
				}
			}
			
			std::string target = request.GetThisValue()->ToString();
#if defined(AOSORA_USE_UNICODE_CHAR_INDEX)
			size_t mbOffset = UnicodeCharIndexToMultiByteIndex(target, offset);
			std::string result = target.substr(mbOffset);
#else
			std::string result = target.substr(offset);
#endif
			response.SetReturnValue(ScriptValue::Make(result));
		}
	}

	void PrimitiveMethod::String_IndexOf(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 1) {
			size_t offset = 0;
			if (request.GetArgumentCount() >= 2) {
				request.GetArgument(1)->ToIndex(offset);
			}
			std::string target = request.GetThisValue()->ToString();
			auto pos = target.find(request.GetArgument(0)->ToString(), offset);
			if (pos == std::string::npos) {
				//見つからない場合はnullにしておく
				response.SetReturnValue(ScriptValue::Null);
			}
			else {
#if defined(AOSORA_USE_UNICODE_CHAR_INDEX)
				size_t charPos = ByteIndexToUncodeCharIndex(target, pos);
				response.SetReturnValue(ScriptValue::Make(static_cast<number>(charPos)));
#else
				response.SetReturnValue(ScriptValue::Make(static_cast<number>(pos)));
#endif
			}
		}
	}

	void PrimitiveMethod::String_Replace(const FunctionRequest& request, FunctionResponse& response) {
		if (request.GetArgumentCount() >= 2) {
			std::string search = request.GetArgument(0)->ToString();
			std::string replaced = request.GetArgument(1)->ToString();
			std::string target = request.GetThisValue()->ToString();
			Replace(target, search, replaced);
			response.SetReturnValue(ScriptValue::Make(target));
		}
	}

	void PrimitiveMethod::String_AddTalk(const FunctionRequest& request, FunctionResponse& response) {
		//トークの結合ルールを使用してくっつける
		if (request.GetArgumentCount() >= 1) {
			std::string left = request.GetThisValue()->ToString();
			std::string right = request.GetArgument(0)->ToString();
			response.SetReturnValue(ScriptValue::Make(TalkStringCombiner::CombineTalk(left, right, request.GetContext().GetInterpreter(), nullptr)));
		}
	}

	void PrimitiveMethod::String_ToUpper(const FunctionRequest& request, FunctionResponse& response)
	{
		std::string target = request.GetThisValue()->ToString();
		for (size_t i = 0; i < target.size(); i++) {
			target[i] = static_cast<char>(toupper(target[i]));
		}
		response.SetReturnValue(ScriptValue::Make(target));
	}

	void PrimitiveMethod::String_ToLower(const FunctionRequest& request, FunctionResponse& response)
	{
		std::string target = request.GetThisValue()->ToString();
		for (size_t i = 0; i < target.size(); i++) {
			target[i] = static_cast<char>(tolower(target[i]));
		}
		response.SetReturnValue(ScriptValue::Make(target));
	}

	void PrimitiveMethod::String_Contains(const FunctionRequest& request, FunctionResponse& response)
	{
		//IndexOfがnullを返していればflase、そうでなければtrue
		String_IndexOf(request, response);
		if (response.GetReturnValue() == nullptr || response.GetReturnValue()->IsNull()) {
			response.SetReturnValue(ScriptValue::False);
		}
		else {
			response.SetReturnValue(ScriptValue::True);
		}
	}

	void PrimitiveMethod::String_EscapeSakuraScript(const FunctionRequest& request, FunctionResponse& response)
	{
		const std::string target = request.GetThisValue()->ToString();
		const std::regex re(R"([\\%])");
		response.SetReturnValue(ScriptValue::Make(std::regex_replace(target, re, R"(\$&)")));
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
		else if (member == "ToAscii") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::Number_ToAscii, value));
		}

		return nullptr;
	}

	ScriptValueRef PrimitiveMethod::GetBooleanMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context) {
		//今のところなし
		return nullptr;
	}

	ScriptValueRef PrimitiveMethod::GetStringMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context) {
		
		if (member == "StartsWith") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_StartWith, value));
		}
		else if (member == "EndsWith") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_EndsWith, value));
		}
		else if (member == "Split") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_Split, value));
		}
		else if (member == "IndexOf") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_IndexOf, value));
		}
		else if (member == "Substring") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_Substring, value));
		}
		else if (member == "Replace") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_Replace, value));
		}
		else if (member == "AddTalk") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_AddTalk, value));
		}
		else if (member == "ToLower") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_ToLower, value));
		}
		else if (member == "ToUpper") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_ToUpper, value));
		}
		else if (member == "Contains") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_Contains, value));
		}
		else if (member == "EscapeSakuraScript") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::String_EscapeSakuraScript, value));
		}
		else if (member == "length") {
#if defined(AOSORA_USE_UNICODE_CHAR_INDEX)
			return ScriptValue::Make(static_cast<number>(CountUnicodeCharacters(value->ToString())));
#else
			return ScriptValue::Make(static_cast<number>(value->ToString().size()));
#endif
		}
		return nullptr;
	}

	ScriptValueRef PrimitiveMethod::GetGeneralMember(const ScriptValueRef& value, const std::string& member, ScriptExecuteContext& context) {
		if (member == "ToString") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_ToString, value));
		}
		else if (member == "ToNumber") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_ToNumber, value));
		}
		else if (member == "ToBoolean") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_ToBoolean, value));
		}
		else if (member == "IsString") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_IsString, value));
		}
		else if (member == "IsNumber") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_IsNumber, value));
		}
		else if (member == "IsBoolean") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_IsBoolean, value));
		}
		else if (member == "IsNan") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_IsNan, value));
		}
		else if (member == "IsObject") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_IsObject, value));
		}
		else if (member == "IsNull") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_IsNull, value));
		}
		else if (member == "InstanceOf") {
			return ScriptValue::Make(context.GetInterpreter().CreateNativeObject<Delegate>(&PrimitiveMethod::General_InstanceOf, value));
		}
		return nullptr;
	}
}