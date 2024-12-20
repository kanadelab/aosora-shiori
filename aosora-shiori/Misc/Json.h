#pragma once

namespace sakura {

	enum class JsonTokenType {
		Number,
		String,
		Boolean,
		Null,
		Object,
		Array
	};

	class JsonTokenBase {
	private:
		const JsonTokenType type;

	public:
		JsonTokenBase(JsonTokenType tokenType) :
			type(tokenType)
		{}

		JsonTokenType GetType() const {
			return type;
		}
	};

	//プリミティブまとめて
	class JsonPrimitive : public JsonTokenBase {
	private:
		union {
			double numberValue;
			bool booleanValue;
		};

	public:
		JsonPrimitive(bool value) : JsonTokenBase(JsonTokenType::Boolean),
			booleanValue(value)
		{}

		JsonPrimitive(double value) : JsonTokenBase(JsonTokenType::Number),
			numberValue(value)
		{}

		JsonPrimitive() : JsonTokenBase(JsonTokenType::Null)
		{}

		double GetNumber() const {
			return numberValue;
		}

		bool GetBoolean() const {
			return booleanValue;
		}
	};

	//文字列型
	class JsonString : public JsonTokenBase {
	private:
		std::string value;

	public:
		JsonString(const std::string& str) : JsonTokenBase(JsonTokenType::String),
			value(str)
		{}

		const std::string& GetString() const {
			return value;
		}
	};

	//Object型
	class JsonObject : public JsonTokenBase {
	private:
		std::map<std::string, std::shared_ptr<JsonTokenBase>> items;

	public:
		JsonObject() : JsonTokenBase(JsonTokenType::Object) {

		}

		void Add(const std::string& key, const std::shared_ptr<JsonTokenBase>& value) {
			items[key] = value;
		}

		const std::map<std::string, std::shared_ptr<JsonTokenBase>>& GetCollection() const {
			return items;
		}

		size_t GetCount() const {
			return items.size();
		}

	};

	//Array型
	class JsonArray : public JsonTokenBase {
	private:
		std::vector<std::shared_ptr<JsonTokenBase>> items;

	public:
		JsonArray() : JsonTokenBase(JsonTokenType::Array) {

		}

		size_t GetCount() const { return items.size(); }

		const std::shared_ptr<JsonTokenBase>& GetItem(size_t index) {
			return items[index];
		}

		const std::vector<std::shared_ptr<JsonTokenBase>>& GetCollection() {
			return items;
		}

		void Add(const std::shared_ptr<JsonTokenBase>& item) {
			items.push_back(item);
		}
	};

	void JsonTest();

	//デシリアライズ結果オブジェクト
	struct JsonDeserializeResult {
		bool success;
		std::shared_ptr<JsonTokenBase> token;
	};

	//jsonシリアライザ
	class JsonSerializer {
	public:
		static JsonDeserializeResult Deserialize(const std::string& json);
		static std::string Serialize(const std::shared_ptr<JsonTokenBase>& token);
	};
}