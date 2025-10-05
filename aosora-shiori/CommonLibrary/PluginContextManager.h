#pragma once
#include <map>
#include "AosoraPlugin.h"
#include "Base.h"
#include "Interpreter/Interpreter.h"

/*

	プラグインコンテキストマネージャ
	プラグインとaosoraの相互呼び出しをサポートするためのコンテキスト管理
	TODO: 複数インタプリタサポートが微妙なので注意

*/
namespace sakura {

	struct LoadedPluginModule;
	class ScriptExecuteContext;

	struct PluginCallContext {
		std::vector<ScriptValueRef> args;
		ScriptValueRef thisValue;
		ScriptValueRef returnValue;
		ScriptValueRef threwError;
	};

	class PluginContext {
	private:
		ScriptExecuteContext& executeContext;
		LoadedPluginModule& pluginModule;
		PluginCallContext callContext;

		//最後の処理呼び出しで発生した例外
		//ただ、get/setも含むはず･･･
		ScriptValueRef lastFunctionReturnValue;
		ScriptValueRef lastFunctionError;

		std::vector<std::shared_ptr<std::string>> stringCache;

	public:
		PluginContext(ScriptExecuteContext& executeContext, LoadedPluginModule& pluginModule):
			executeContext(executeContext),
			pluginModule(pluginModule) {

		}

		ScriptExecuteContext& GetExecuteContext() {
			return executeContext;
		}

		ScriptInterpreter& GetInterpreter() {
			return executeContext.GetInterpreter();
		}

		LoadedPluginModule& GetPluginModule() {
			return pluginModule;
		}

		PluginCallContext& GetCallContext() {
			return callContext;
		}

		//文字列をキャッシュする
		//プラグイン側に文字列を転送するために、コンテキストの生存期間だけ生存を保証する文字列インスタンスを作成する
		const std::string& CacheString(const std::string& str) {
			auto ptr = std::shared_ptr<std::string>(new std::string(str));
			stringCache.push_back(ptr);
			return *ptr;
		}
	};

	//ハンドルマネージャ
	//ハンドルとScriptValueを交換する
	class PluginHandleManager {
	public:
		static const aosora::ValueHandle INVALID_HANDLE = 0;
		static const aosora::ValueHandle FIRST_HANDLE = 1;

		struct ValueData {
			ScriptValueRef valueRef;
			uint32_t refCount;
		};

		using ValueMapType = std::map<aosora::ValueHandle, ValueData>;

	private:
		aosora::ValueHandle nextHandle;
		ValueMapType valueMap;

		aosora::ValueHandle GenerateHandle() {
			aosora::ValueHandle handle;

			//重複しないハンドルを出力
			do {
				handle  = nextHandle++;
				if (nextHandle == INVALID_HANDLE) {
					nextHandle = FIRST_HANDLE;
				}
			} while (valueMap.contains(handle));
			return handle;
		}
	public:
		PluginHandleManager() :
			nextHandle(FIRST_HANDLE) {
		}

		//ハンドルと値のセットを登録
		aosora::ValueHandle CreateHandle(const ScriptValueRef& value) {
			ValueData valueData;
			valueData.refCount = 1;
			valueData.valueRef = value;
			
			aosora::ValueHandle handle = GenerateHandle();
			valueMap.insert(ValueMapType::value_type(handle, valueData));
			return handle;
		}

		ScriptValueRef GetValue(aosora::ValueHandle handle) {
			auto it = valueMap.find(handle);
			if (it != valueMap.end()) {
				return it->second.valueRef;
			}
			else {
				return ScriptValue::Null;
			}
		}

		void AddRef(aosora::ValueHandle handle) {
			auto it = valueMap.find(handle);
			if (it != valueMap.end()) {
				it->second.refCount++;
			}
		}

		void Release(aosora::ValueHandle handle) {
			auto it = valueMap.find(handle);
			if (it != valueMap.end()) {
				it->second.refCount--;
				if (it->second.refCount == 0) {
					valueMap.erase(it);
				}
			}
		}

	};

	//プラグインコンテキスト、プラグインと内部処理の相互呼び出しをとりもつ
	class PluginContextManager {
	private:
		static std::vector<PluginContext*> contextStack;
		static std::map<LoadedPluginModule*, PluginHandleManager*> plugins;
		static const aosora::AosoraAccessor accessor;

	public:
		static void PushContext(PluginContext* pluginContext) {
			contextStack.push_back(pluginContext);
		}

		static PluginContext& PeekContext() {
			assert(!contextStack.empty());
			return **contextStack.rbegin();
		}

		static const void PopContext() {
			delete* contextStack.rbegin();
			contextStack.pop_back();
		}

		static void RegisterPlugin(LoadedPluginModule* pluginModule) {
			plugins.insert(decltype(plugins)::value_type(pluginModule, new PluginHandleManager()));
		}

		static void UnregisterPlugin(LoadedPluginModule* pluginModule) {
			auto it = plugins.find(pluginModule);
			if (it != plugins.end()) {
				delete it->second;
				plugins.erase(it);
			}
		}

		static PluginHandleManager& GetCurrentHandleManager() {
			return *plugins[&PeekContext().GetPluginModule()];
		}

		static ScriptInterpreter& GetCurrentInterpreter() {
			return PeekContext().GetInterpreter();
		}

		static LoadedPluginModule* GetCurrentPluginModule() {
			return &PeekContext().GetPluginModule();
		}

		static ScriptExecuteContext& GetCurrentExecuteContext() {
			return PeekContext().GetExecuteContext();
		}

		//プラグイン関数のコール
		static ScriptValueRef ExecuteModuleLoadFunction(LoadedPluginModule& module, ScriptExecuteContext& executeContext);
		static void ExecutePluginFunction(LoadedPluginModule& module, aosora::PluginFunctionType pluginFunction, const ScriptValueRef& thisValue, const FunctionRequest& request, FunctionResponse& response);
		
		//アクセサ関数
		static void ReleaseHandle(aosora::ValueHandle handle);
		static aosora::ValueHandle CreateNumber(double value);
		static aosora::ValueHandle CreateBool(bool value);
		static aosora::ValueHandle CreateString(aosora::StringContainer value);
		static aosora::ValueHandle CreateNull();
		static aosora::ValueHandle CreateFunction(aosora::ValueHandle thisValue, aosora::PluginFunctionType functionBody);
		static aosora::ValueHandle CreateMap();
		static aosora::ValueHandle CreateArray();

		static double ToNumber(aosora::ValueHandle handle);
		static bool ToBool(aosora::ValueHandle handle);
		static aosora::StringContainer ToString(aosora::ValueHandle handle);

		static void SetValue(aosora::ValueHandle target, aosora::ValueHandle key, aosora::ValueHandle value);
		static aosora::ValueHandle GetValue(aosora::ValueHandle target, aosora::ValueHandle key);

		static size_t GetArgumentCount();
		static aosora::ValueHandle GetArgument(size_t index);

		static void SetReturnValue(aosora::ValueHandle value);

		static void FunctionCall(aosora::ValueHandle function, const aosora::ValueHandle* argv, size_t argc);
	};
}