#pragma once
#include "AST/AST.h"
#include "Interpreter/Interpreter.h"
#include <ctime>

namespace sakura {

	struct LoadedSaoriModule;

	//日付と時刻
	class Time : public Object<Time> {

	public:
		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);

		//現在時
		static void GetNowHour(const FunctionRequest& request, FunctionResponse& response) {
			std::time_t now = std::time(nullptr);
			std::tm* lt = std::localtime(&now);
			response.SetReturnValue(ScriptValue::Make(static_cast<number>(lt->tm_hour)));
		}

		//現在分
		static void GetNowMinute(const FunctionRequest& request, FunctionResponse& response) {
			std::time_t now = std::time(nullptr);
			std::tm* lt = std::localtime(&now);
			response.SetReturnValue(ScriptValue::Make(static_cast<number>(lt->tm_min)));
		}

		//現在秒
		static void GetNowSecond(const FunctionRequest& request, FunctionResponse& response) {
			std::time_t now = std::time(nullptr);
			std::tm* lt = std::localtime(&now);
			response.SetReturnValue(ScriptValue::Make(static_cast<number>(lt->tm_sec)));
		}

		//現在年
		static void GetNowYear(const FunctionRequest& request, FunctionResponse& response) {
			std::time_t now = std::time(nullptr);
			std::tm* lt = std::localtime(&now);
			response.SetReturnValue(ScriptValue::Make(static_cast<number>(lt->tm_year + 1900)));
		}

		//現在月
		static void GetNowMonth(const FunctionRequest& request, FunctionResponse& response) {
			std::time_t now = std::time(nullptr);
			std::tm* lt = std::localtime(&now);
			response.SetReturnValue(ScriptValue::Make(static_cast<number>(lt->tm_mon+1)));
		}

		//現在日
		static void GetNowDate(const FunctionRequest& request, FunctionResponse& response) {
			std::time_t now = std::time(nullptr);
			std::tm* lt = std::localtime(&now);
			response.SetReturnValue(ScriptValue::Make(static_cast<number>(lt->tm_mday)));
		}

		//現在曜日
		static void GetNowDayOfWeek(const FunctionRequest& request, FunctionResponse& response) {
			std::time_t now = std::time(nullptr);
			std::tm* lt = std::localtime(&now);
			response.SetReturnValue(ScriptValue::Make(static_cast<number>(lt->tm_wday)));
		}

		//Unixエポックの取得
		static void GetNowUnixEpoch(const FunctionRequest& request, FunctionResponse& response) {
			std::time_t now = std::time(nullptr);
			response.SetReturnValue(ScriptValue::Make(static_cast<number>(now)));
		}

	};

	//ランダム
	class Random : public Object<Random> {

	public:
		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);

		//配列から１つ選ぶ
		static void Select(const FunctionRequest& request, FunctionResponse& response);

		//最小最大から１つ選択
		static void GetIndex(const FunctionRequest& request, FunctionResponse& response);

		//0.0-1.0範囲の実数を取得
		static void GetNumber(const FunctionRequest& request, FunctionResponse& response);
	};

	//セーブデータオブジェクト
	class SaveData : public Object<SaveData> {
	public:
		static void StaticSet(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext);
		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);

		//セーブデータ読み込み書き込み系
		static void Load(ScriptInterpreter& interpreter);
		static void Save(ScriptInterpreter& interpreter);
	};

	//SHIORIリクエストオブジェクト
	class ShioriRequest {
	private:
		std::string eventId;
		std::vector<std::string> references;
		std::set<std::string> statuses;
		std::map<std::string, std::string> rawMap;
		SecurityLevel securityLevel;
		bool isGet;
		bool isSaori;

	public:
		ShioriRequest(const std::string& id) :
			securityLevel(SecurityLevel::LOCAL),
			eventId(id),
			isGet(true)
		{}

		ShioriRequest() :
			eventId(),
			isGet(true)
		{}

		void SetEventId(const std::string& id) {
			eventId = id;
		}

		const std::string GetEventId() const {
			return eventId;
		}

		void SetSecurityLevel(SecurityLevel level) {
			securityLevel = level;
		}

		SecurityLevel GetSecurityLevel() const {
			return securityLevel;
		}

		void SetIsGet(bool get) {
			isGet = get;
		}

		bool IsGet() const {
			return isGet;
		}

		//as SAORIリクエストかどうか
		void SetIsSaori(bool saori) {
			isSaori = saori;
		}

		bool IsSaori() const {
			return isSaori;
		}

		void SetReference(uint32_t index, const std::string& value) {
			if (index >= references.size()) {
				references.resize(index + 1);
			}
			references[index] = value;
		}

		std::string GetReference(uint32_t index) const {
			if (index >= references.size()) {
				return std::string();
			}
			return references[index];
		}

		size_t GetReferenceCount() const {
			return references.size();
		}

		const std::vector<std::string>& GetReferenceCollection() const {
			return references;
		}

		void AddStatus(const std::string& status) {
			statuses.insert(status);
		}

		bool HasStatus(const std::string& status) {
			return statuses.contains(status);
		}

		const std::set<std::string>& GetStatusCollection() const {
			return statuses;
		}

		void AddRawData(const std::string& key, const std::string& value) {
			rawMap[key] = value;
		}

		const std::map<std::string, std::string>& GetRawCollection() const {
			return rawMap;
		}
	};

	//SHIORIエラー情報
	class ShioriError {
	public:
		enum class ErrorLevel {
			Info,
			Notice,
			Warning,
			Error,
			Critical
		};

	private:
		ErrorLevel level;
		std::string message;

	public:
		ShioriError(ErrorLevel errorLevel, const std::string& errorMessage):
			level(errorLevel),
			message(errorMessage)
		{}

		ErrorLevel GetLevel() const { return level; }
		const std::string& GetMessage() const { return message; }

		const std::string GetLevelString() const {
			switch (level) {
			case ErrorLevel::Info:
				return "info";
			case ErrorLevel::Notice:
				return "notice";
			case ErrorLevel::Warning:
				return "warning";
			case ErrorLevel::Error:
				return "error";
			case ErrorLevel::Critical:
				return "critical";
			default:
				assert(false);
				return "";
			}
		}
	};

	//SHIORIレスポンスオブジェクト
	class ShioriResponse {
	private:
		std::string status;
		std::string value;
		std::vector<ShioriError> errors;
		std::vector<std::string> saoriValues;

	public:
		void SetBadRequest() {
			status = "400 Bad Request";
		}

		void SetInternalServerError() {
			status = "500 Internal Server Error";
		}

		void SetValue(const std::string& val) {
			value = val;
		}

		void AddError(const ShioriError& err) {
			errors.push_back(err);
		}

		const std::vector<ShioriError>& GetErrorCollection() const {
			return errors;
		}

		const std::string& GetValue() const {
			return value;
		}

		std::string GetStatus() const {
			if (!status.empty()) {
				return status;
			}

			//エラー等設定されてない場合は204か200を返す
			if (value.empty()) {
				return "204 No Content";
			}
			else {
				return "200 OK";
			}
		}

		bool HasError() const {
			return !errors.empty();
		}

		//SHIORIのErrorDescriptionヘッダを取得
		std::string GetErrorDescriptionList() const {
			assert(HasError());
			std::string result;
			for (size_t i = 0; i < errors.size(); i++) {
				if (i > 0) {
					result += (char)1;
				}
				result += errors[i].GetMessage();
			}
			return result;
		}

		//SHIORIのErrorLevelヘッダを取得
		std::string GetErrorLevelList() {
			assert(HasError());
			std::string result;
			for (size_t i = 0; i < errors.size(); i++) {
				if (i > 0) {
					result += (char)1;
				}
				result += errors[i].GetLevelString();
			}
			return result;
		}

		//SAORIのValue*返却オブジェクトを渡す
		void SetSaoriValues(const std::vector<std::string>& values) {
			saoriValues = values;
		}

		const std::vector<std::string>& GetSaoriValues() const {
			return saoriValues;
		}
	};


	//トークタイマー
	class TalkTimer : public Object<TalkTimer> {
	public:
		static const char* KeyRandomTalk;
		static const char* KeyRandomTalkIntervalSeconds;
		static const char* KeyRandomTalkElapsedSeconds;

		static const char* KeyNadenadeTalk;
		static const char* KeyNadenadeMoveCount;
		static const char* KeyNadenadeMoveThreshold;
		static const char* KeyNadenadeActiveCollision;

		//OnSecondChange にあわせて呼び出すトークタイマー更新系
		//戻り値はトーク相当のスクリプトの実行を試みたかどうか（発話可能時に実際にトークをよびだして結果を得たか）
		static bool HandleEvent(ScriptInterpreter& interpreter, FunctionResponse& response, const ShioriRequest& shioriRequest, bool hasResponseTalk);
		static bool OnSecondChange(ScriptInterpreter& interpreter, FunctionResponse& response, const ShioriRequest& shioriRequest, bool canCallRandomTalk);
		static bool OnMouseMove(ScriptInterpreter& interpreter, FunctionResponse& response, const ShioriRequest& shioriRequest, bool canCallTalk);
		static void ClearMouseMove(ScriptInterpreter& interpreter);

		static void StaticInit(ScriptInterpreter& interpreter);
		static void StaticSet(const std::string& key, const ScriptValueRef& value, ScriptExecuteContext& executeContext);
		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);
	};

	//Saoriマネージャ
	class SaoriManager : public Object<SaoriManager> {

	public:
		static void Load(const FunctionRequest& request, FunctionResponse& response);
		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);
	};

	//Saoriモジュール
	class SaoriModule : public Object<SaoriModule> {
	private:
		LoadedSaoriModule* loadedModule;

	public:
		SaoriModule(LoadedSaoriModule* saoriModule) :
			loadedModule(saoriModule)
		{}

		virtual ~SaoriModule();

		static void Request(const FunctionRequest& request, FunctionResponse& response);

		virtual ScriptValueRef Get(const ObjectRef& self, const std::string& key, ScriptExecuteContext& executeContext) override;
		virtual void FetchReferencedItems(std::list<CollectableBase*>& result) override {}
	};
}