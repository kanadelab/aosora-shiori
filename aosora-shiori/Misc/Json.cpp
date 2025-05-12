#include <vector>
#include <map>
#include <memory>
#include <string>
#include <regex>
#include <cassert>
#include "Misc/Utility.h"
#include "Misc/Json.h"

//簡単なjsonシリアライズ・デシリアライズ

namespace sakura {

	

	//解析用正規表現
	const std::regex JSON_NUMBER_PATTERN(R"(^\s*(\-?[0-9.]+))");
	const std::regex JSON_STRING_PATTERN("^\\s*\"(([^\\\\\"]+|\\\\.)*)\"");
	const std::regex JSON_TRUE_PATTERN(R"(^\s*true)");
	const std::regex JSON_FALSE_PATTERN(R"(^\s*false)");
	const std::regex JSON_NULL_PATTERN(R"(^\s*null)");
	const std::regex JSON_OBJECT_BEGIN_PATTERN(R"(^\s*\{)");
	const std::regex JSON_OBJECT_END_PATTERN(R"(^\s*\})");
	const std::regex JSON_ARRAY_BEGIN_PATTERN(R"(^\s*\[)");
	const std::regex JSON_ARRAY_END_PATTERN(R"(^\s*\])");
	const std::regex JSON_OBJECT_COMMA_PATTERN(R"(^\s*\,)");
	const std::regex JSON_OBJECT_COLON_PATTERN(R"(^\s*\:)");
	
	class JsonParseContext {
	private:
		const std::string& document;
		std::string_view currentView;
		size_t offset;
		bool hasError;

	public:
		JsonParseContext(const std::string& json) :
			document(json),
			currentView(std::string_view(json)),
			offset(0),
			hasError(false) {
		}

		std::string_view GetCurrent() const {
			return currentView;
		}

		void Offset(size_t offsetSize) {
			offset += offsetSize;
			currentView = std::string_view(document.c_str() + offset, document.size() - offset);
		}

		void Error() {
			hasError = true;
		}

		bool HasError() const {
			return hasError;
		}
	};

	std::shared_ptr<JsonArray> DeserializeArray(JsonParseContext& parseContext);
	std::shared_ptr<JsonObject> DeserializeObject(JsonParseContext& parseContext);
	std::shared_ptr<JsonTokenBase> DeserializeToken(JsonParseContext& parseContext);
	void SerializeArray(const std::shared_ptr<JsonArray>& token, std::string& result);
	void SerializeObject(const std::shared_ptr<JsonObject>& token, std::string& result);

	std::shared_ptr<JsonTokenBase> DeserializeToken(JsonParseContext& parseContext) {

		std::string_view str = parseContext.GetCurrent();
		std::match_results<std::string_view::const_iterator> match;

		if (std::regex_search(str.cbegin(), str.cend(), match, JSON_NUMBER_PATTERN)) {
			//数値
			double v = std::stod(match[1].str());
			parseContext.Offset(match.length());
			return std::shared_ptr<JsonPrimitive>(new JsonPrimitive(v));
		}
		else if (std::regex_search(str.cbegin(), str.cend(), match, JSON_STRING_PATTERN)) {
			//文字列
			parseContext.Offset(match.length());
			std::string body = match[1].str();
			
			//エスケープを解除
			Replace(body, "\\\\", "\\");
			Replace(body, "\\\"", "\"");
			return std::shared_ptr<JsonString>(new JsonString(body));
		}
		else if (std::regex_search(str.cbegin(), str.cend(), match, JSON_TRUE_PATTERN)) {
			//true
			parseContext.Offset(match.length());
			return std::shared_ptr<JsonPrimitive>(new JsonPrimitive(true));
		}
		else if (std::regex_search(str.cbegin(), str.cend(), match, JSON_FALSE_PATTERN)) {
			//false
			parseContext.Offset(match.length());
			return std::shared_ptr<JsonPrimitive>(new JsonPrimitive(false));
		}
		else if (std::regex_search(str.cbegin(), str.cend(), match, JSON_NULL_PATTERN)) {
			//null
			parseContext.Offset(match.length());
			return std::shared_ptr<JsonPrimitive>(new JsonPrimitive());
		}
		else if (std::regex_search(str.cbegin(), str.cend(), match, JSON_ARRAY_BEGIN_PATTERN)) {
			parseContext.Offset(match.length());
			return DeserializeArray(parseContext);
		}
		else if (std::regex_search(str.cbegin(), str.cend(), match, JSON_OBJECT_BEGIN_PATTERN)) {
			parseContext.Offset(match.length());
			return DeserializeObject(parseContext);
		}
		else {
			//不正
			parseContext.Error();
			return nullptr;
		}
	}


	std::shared_ptr<JsonArray> DeserializeArray(JsonParseContext& parseContext) {

		if (parseContext.HasError()) {
			return nullptr;
		}

		std::shared_ptr<JsonArray> result(new JsonArray());

		//からっぽの場合
		std::match_results<std::string_view::const_iterator> match;
		if (std::regex_search(parseContext.GetCurrent().cbegin(), parseContext.GetCurrent().cend(), match, JSON_ARRAY_END_PATTERN)) {
			parseContext.Offset(match.length());
			return result;
		}

		while (!parseContext.HasError()) {

			//アイテム
			std::shared_ptr<JsonTokenBase> token = DeserializeToken(parseContext);
			result->Add(token);

			//カンマもしくは終端
			if (std::regex_search(parseContext.GetCurrent().cbegin(), parseContext.GetCurrent().cend(), match, JSON_ARRAY_END_PATTERN)) {
				parseContext.Offset(match.length());
				return result;
			}
			else if (std::regex_search(parseContext.GetCurrent().cbegin(), parseContext.GetCurrent().cend(), match, JSON_OBJECT_COMMA_PATTERN)) {
				parseContext.Offset(match.length());
				continue;
			}

			//ヒットしないとおかしい
			break;
		}

		parseContext.Error();
		return nullptr;
	}

	std::shared_ptr<JsonObject> DeserializeObject(JsonParseContext& parseContext) {

		std::shared_ptr<JsonObject> result(new JsonObject());
		std::match_results<std::string_view::const_iterator> match;

		//からっぽの場合
		if (std::regex_search(parseContext.GetCurrent().cbegin(), parseContext.GetCurrent().cend(), match, JSON_OBJECT_END_PATTERN)) {
			parseContext.Offset(match.length());
			return result;
		}

		while (!parseContext.HasError()) {

			//key
			auto key = DeserializeToken(parseContext);

			if (parseContext.HasError()) {
				return nullptr;
			}

			//文字列じゃないとエラー
			if (key->GetType() != JsonTokenType::String) {
				parseContext.HasError();
				return nullptr;
			}

			//カンマ
			if (!std::regex_search(parseContext.GetCurrent().cbegin(), parseContext.GetCurrent().cend(), match, JSON_OBJECT_COLON_PATTERN)) {
				//カンマじゃないとエラー
				parseContext.Error();
				return nullptr;
			}
			parseContext.Offset(match.length());

			//value
			auto value = DeserializeToken(parseContext);

			if (parseContext.HasError()) {
				return nullptr;
			}

			//内容を追加
			result->Add(std::static_pointer_cast<JsonString>(key)->GetString(), value);

			if (std::regex_search(parseContext.GetCurrent().cbegin(), parseContext.GetCurrent().cend(), match, JSON_OBJECT_END_PATTERN)) {
				parseContext.Offset(match.length());
				return result;
			}
			else if (std::regex_search(parseContext.GetCurrent().cbegin(), parseContext.GetCurrent().cend(), match, JSON_OBJECT_COMMA_PATTERN)) {
				parseContext.Offset(match.length());
				continue;
			}

			//ヒットしないとエラー
			parseContext.Error();
			return nullptr;

		}
		return result;
	}

	//jsonシリアライズ
	void SerializeJson(const std::shared_ptr<JsonTokenBase>& token, std::string& result) {
		switch (token->GetType()) {
			case JsonTokenType::Number:
				result.append(std::to_string(std::static_pointer_cast<JsonPrimitive>(token)->GetNumber()));
				break;
			case JsonTokenType::Boolean:
				result.append(std::static_pointer_cast<JsonPrimitive>(token)->GetBoolean() ? "true" : "false");
				break;
			case JsonTokenType::Null:
				result.append("null");
				break;
			case JsonTokenType::String:
				result.push_back('"');
				{
					//ダブルクォーテーションのエスケープ処理
					std::string body = std::static_pointer_cast<JsonString>(token)->GetString();
					Replace(body, "\"", "\\\"");
					Replace(body, "\\", "\\\\");
					result.append(body);
				}
				result.push_back('"');
				break;
			case JsonTokenType::Object:
				SerializeObject(std::static_pointer_cast<JsonObject>(token), result);
				break;
			case JsonTokenType::Array:
				SerializeArray(std::static_pointer_cast<JsonArray>(token), result);
				break;
		}
	}

	//Arrayシリアライズ
	void SerializeArray(const std::shared_ptr<JsonArray>& token, std::string& result) {

		if (token->GetCount() == 0) {
			result.append("[]");
		}
		else {
			result.push_back('[');

			bool requireComma = false;
			for (const auto& item : token->GetCollection()) {
				if (requireComma) {
					result.push_back(',');
				}
				SerializeJson(item, result);
				requireComma = true;
			}

			result.push_back(']');
		}
	}

	//Objectシリアライズ
	void SerializeObject(const std::shared_ptr<JsonObject>& token, std::string& result) {
		if (token->GetCount() == 0) {
			result.append("{}");
		}
		else {
			result.push_back('{');

			bool requireComma = false;
			for (const auto& item : token->GetCollection()) {
				if (requireComma) {
					result.push_back(',');
				}
				requireComma = true;
				result.push_back('"');
				result.append(item.first);
				result.push_back('"');
				result.push_back(':');
				SerializeJson(item.second, result);
			}

			result.push_back('}');
		}
	}

	//jsonデシリアライズ
	JsonDeserializeResult JsonSerializer::Deserialize(const std::string& json) {
		JsonParseContext context(json);
		JsonDeserializeResult result;
		auto token = DeserializeToken(context);

		if (!context.HasError()) {
			result.success = true;
			result.token = token;
		}
		else {
			result.success = false;
			result.token = nullptr;
		}
		return result;
	}

	std::string JsonSerializer::Serialize(const std::shared_ptr<JsonTokenBase>& token) {
		std::string result;
		SerializeJson(token, result);
		return result;
	}

	//てすと
	void JsonTest() {
		const std::string json = R"({"test": "test"})";

		auto deserialized = JsonSerializer::Deserialize(json);
		std::string serialized;
		SerializeJson(deserialized.token, serialized);
		printf("%s", serialized.c_str());
	}

}