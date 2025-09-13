#pragma once
#include "Interpreter/Interpreter.h"

//aosoraスタンダードライブラリ
//stdユニットに存在する機能群として作成
namespace sakura {

	//スクリプト向けjsonシリアライザ
	class ScriptJsonSerializer : public Object<ScriptJsonSerializer> {
	public:
		static ScriptValueRef StaticGet(const std::string& key, ScriptExecuteContext& executeContext);

		void ScriptSerialize(const FunctionRequest& request, FunctionResponse& response);
		void ScriptDeserialize(const FunctionRequest& request, FunctionResponse& response);
	};

}
