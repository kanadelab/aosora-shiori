#include "CommonLibrary/PluginContextManager.h"
#include "CoreLibrary/CoreClasses.h"

namespace sakura {

	//アクセサ関数
	aosora::ValueHandle PluginContextManager::CreateNumber(double value) {
		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(value));
	}

	aosora::ValueHandle PluginContextManager::CreateBool(bool value) {
		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(value));
	}

	aosora::ValueHandle PluginContextManager::CreateString(aosora::StringContainer value) {
		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(std::string(value.body, value.len)));
	}

	aosora::ValueHandle PluginContextManager::CreateNull() {
		return GetCurrentHandleManager().CreateHandle(ScriptValue::Null);
	}

	double PluginContextManager::ToNumber(aosora::ValueHandle handle) {
		return GetCurrentHandleManager().GetValue(handle)->ToNumber();
	}

	bool PluginContextManager::ToBool(aosora::ValueHandle handle) {
		return GetCurrentHandleManager().GetValue(handle)->ToBoolean();
	}

	aosora::StringContainer PluginContextManager::ToString(aosora::ValueHandle handle) {
		//TODO: 文字列対応が要る
		//return GetCurrentHandleManager().GetValue(handle)->ToString();
		return aosora::StringContainer();
	}

	size_t PluginContextManager::GetArgumentCount() {
		return 0;
	}

	aosora::ValueHandle PluginContextManager::GetArgument(size_t index) {
		return 0;
	}

}