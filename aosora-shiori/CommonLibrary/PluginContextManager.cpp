#include <vector>
#include "CommonLibrary/PluginContextManager.h"
#include "CoreLibrary/CoreClasses.h"
#include "CommonLibrary/CommonClasses.h"
#include "Misc/PluginLoader.h"

namespace sakura {

	std::vector<PluginContext*> PluginContextManager::contextStack;
	std::map<LoadedPluginModule*, PluginHandleManager*> PluginContextManager::plugins;

	//NOTE: ここズレるとおかしくなるので注意
	const aosora::AosoraAccessor PluginContextManager::accessor = {
		ReleaseHandle,

		CreateNumber,
		CreateBool,
		CreateString,
		CreateNull,
		CreateMap,
		CreateArray,
		CreateFunction,

		ToNumber,
		ToBool,
		ToString,
		
		GetValue,
		SetValue,

		GetArgumentCount,
		GetArgument,

		SetReturnValue
	};

	//プラグインロード関数の実行
	ScriptValueRef PluginContextManager::ExecuteModuleLoadFunction(LoadedPluginModule& pluginModule, ScriptExecuteContext& executeContext) {

		//プラグイン用の情報領域を作成
		RegisterPlugin(&pluginModule);

		//コンテキストをプッシュ、ポップを行う
		PluginContext* pluginContext = new PluginContext(executeContext, pluginModule);
		PushContext(pluginContext);

		//ここでプラグイン実行
		pluginModule.fLoad(&accessor);

		//戻り値をフェッチ
		ScriptValueRef returnValue = PeekContext().GetCallContext().returnValue;

		//ポップ
		PopContext();

		if (returnValue != nullptr) {
			return returnValue;
		}
		return ScriptValue::Null;
	}

	//プラグイン関数の一般実行
	void PluginContextManager::ExecutePluginFunction(LoadedPluginModule& pluginModule, aosora::PluginFunctionType pluginFunction, const ScriptValueRef& thisValue, const FunctionRequest& request, FunctionResponse& response) {

		//コンテキストを準備
		PluginContext* pluginContext = new PluginContext(request.GetContext(), pluginModule);
		PushContext(pluginContext);

		//引数をプッシュ
		PeekContext().GetCallContext().thisValue = thisValue;
		for (size_t i = 0; i < request.GetArgumentCount(); i++) {
			PeekContext().GetCallContext().args.push_back(request.GetArgument(i));
		}

		//プラグイン実行
		pluginFunction(&accessor);

		//戻り値をフェッチ
		ScriptValueRef returnValue = PeekContext().GetCallContext().returnValue;
		
		//レスポンス設定
		response.SetReturnValue(returnValue);

		//ポップ
		PopContext();
	}

	//アクセサ関数
	void PluginContextManager::ReleaseHandle(aosora::ValueHandle handle) {
		GetCurrentHandleManager().Release(handle);
	}

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

	aosora::ValueHandle PluginContextManager::CreateFunction(aosora::ValueHandle thisValue, aosora::PluginFunctionType functionBody) {
		auto pluginDelegate = GetCurrentInterpreter().CreateNativeObject<PluginDelegate>(
			GetCurrentPluginModule(),
			functionBody,
			GetCurrentHandleManager().GetValue(thisValue)
		);

		return GetCurrentHandleManager().CreateHandle(ScriptValue::Make(pluginDelegate));
	}

	aosora::ValueHandle PluginContextManager::CreateMap() {
		return GetCurrentHandleManager().CreateHandle(
			ScriptValue::Make(GetCurrentInterpreter().CreateObject())
		);
	}

	aosora::ValueHandle PluginContextManager::CreateArray() {
		return GetCurrentHandleManager().CreateHandle(
			ScriptValue::Make(GetCurrentInterpreter().CreateArray())
		);
	}

	double PluginContextManager::ToNumber(aosora::ValueHandle handle) {
		return GetCurrentHandleManager().GetValue(handle)->ToNumber();
	}

	bool PluginContextManager::ToBool(aosora::ValueHandle handle) {
		return GetCurrentHandleManager().GetValue(handle)->ToBoolean();
	}

	aosora::StringContainer PluginContextManager::ToString(aosora::ValueHandle handle) {
		//プラグイン側に読んでよい文字列を渡すために一時的な文字列キャッシュをつくる
		//プラグインから関数が戻ると使用不可になるのでその場で読み終える必要がある
		const std::string& cachedString = PeekContext().CacheString(
			GetCurrentHandleManager().GetValue(handle)->ToString()
		);
		aosora::StringContainer result;
		result.body = cachedString.c_str();
		result.len = cachedString.size();
		return result;
	}

	void PluginContextManager::SetValue(aosora::ValueHandle target, aosora::ValueHandle key, aosora::ValueHandle value) {
		auto targetValue = GetCurrentHandleManager().GetValue(target);
		auto keyValue = GetCurrentHandleManager().GetValue(key);
		if (targetValue->IsObject()) {
			targetValue->GetObjectRef()->Set(keyValue->ToString(), GetCurrentHandleManager().GetValue(value), GetCurrentExecuteContext());
		}
	}

	aosora::ValueHandle PluginContextManager::GetValue(aosora::ValueHandle target, aosora::ValueHandle key) {
		auto targetValue = GetCurrentHandleManager().GetValue(target);
		auto keyValue = GetCurrentHandleManager().GetValue(key);
		if (targetValue->IsObject()) {
			return GetCurrentHandleManager().CreateHandle(
				targetValue->GetObjectRef()->Get(keyValue->ToString(), GetCurrentExecuteContext())
			);
		}
		else {
			return aosora::INVALID_VALUE_HANDLE;
		}
	}

	size_t PluginContextManager::GetArgumentCount() {
		return PeekContext().GetCallContext().args.size();
	}

	aosora::ValueHandle PluginContextManager::GetArgument(size_t index) {
		if (index < GetArgumentCount()) {
			return GetCurrentHandleManager().CreateHandle(
				PeekContext().GetCallContext().args[index]
			);
		}
		else {
			return aosora::INVALID_VALUE_HANDLE;
		}
	}

	void PluginContextManager::SetReturnValue(aosora::ValueHandle value) {
		PeekContext().GetCallContext().returnValue = GetCurrentHandleManager().GetValue(value);
	}

	void PluginContextManager::FunctionCall(aosora::ValueHandle function, const aosora::ValueHandle* argv, size_t argc) {

		ScriptValueRef functionValue = GetCurrentHandleManager().GetValue(function);

		//呼び出せなければ無視
		if (functionValue->IsObject() && functionValue->GetObjectRef()->CanCall()) {
			FunctionResponse response;

			//引数の展開
			std::vector<ScriptValueRef> args;
			for (size_t i = 0; i < argc; i++) {
				args.push_back(GetCurrentHandleManager().GetValue(argv[i]));
			}

			PeekContext().GetInterpreter().CallFunction(
				*functionValue, response, args, GetCurrentExecuteContext(), nullptr
			);

			// TODO: 戻り値と例外の記録
			// response.GetReturnValue();

		}
	}

}