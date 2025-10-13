#pragma once
#include <map>
#include "AosoraPlugin.h"
#include "Base.h"
#include "Interpreter/Interpreter.h"

#if defined(AOSORA_ENABLE_PLUGIN_LOADER)
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
			pluginModule(pluginModule),
			lastFunctionReturnValue(ScriptValue::Null),
			lastFunctionError(ScriptValue::Null) {

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

		void ClearReturnValueAndError() {
			lastFunctionError = ScriptValue::Null;
			lastFunctionReturnValue = ScriptValue::Null;
		}

		void SetLastFunctionReturnValue(const ScriptValueRef& returnValue) {
			lastFunctionReturnValue = returnValue;
		}

		ScriptValueRef GetLastFunctionReturnValue() {
			return lastFunctionReturnValue;
		}

		void SetLastError(const ScriptValueRef& errorObject) {
			lastFunctionError = errorObject;
		}

		ScriptValueRef GetLastError() {
			return lastFunctionError;
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
		static const aosora::raw::ValueHandle INVALID_HANDLE = 0;
		static const aosora::raw::ValueHandle FIRST_HANDLE = 1;

		struct ValueData {
			ScriptValueRef valueRef;
			uint32_t refCount;
		};

		using ValueMapType = std::map<aosora::raw::ValueHandle, ValueData>;

	private:
		aosora::raw::ValueHandle nextHandle;
		ValueMapType valueMap;

		aosora::raw::ValueHandle GenerateHandle() {
			aosora::raw::ValueHandle handle;

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

		~PluginHandleManager() {
			//ハンドルリークチェック
			assert(valueMap.empty());
		}

		//ハンドルと値のセットを登録
		aosora::raw::ValueHandle CreateHandle(const ScriptValueRef& value) {
			ValueData valueData;
			valueData.refCount = 1;
			valueData.valueRef = value;
			
			aosora::raw::ValueHandle handle = GenerateHandle();
			valueMap.insert(ValueMapType::value_type(handle, valueData));
			return handle;
		}

		ScriptValueRef GetValue(aosora::raw::ValueHandle handle) {
			auto it = valueMap.find(handle);
			if (it != valueMap.end()) {
				return it->second.valueRef;
			}
			else {
				return ScriptValue::Null;
			}
		}

		void AddRef(aosora::raw::ValueHandle handle) {
			auto it = valueMap.find(handle);
			if (it != valueMap.end()) {
				it->second.refCount++;
			}
		}

		void Release(aosora::raw::ValueHandle handle) {
			auto it = valueMap.find(handle);
			if (it != valueMap.end()) {
				it->second.refCount--;
				if (it->second.refCount == 0) {
					valueMap.erase(it);
				}
			}
		}

		void FetchReferencedItems(std::list<CollectableBase*>& result);
	};

	//プラグインコンテキスト、プラグインと内部処理の相互呼び出しをとりもつ
	class PluginContextManager {
	private:
		static std::vector<PluginContext*> contextStack;
		static std::map<LoadedPluginModule*, PluginHandleManager*> plugins;
		static const aosora::raw::AosoraRawAccessor accessor;

		inline static std::string FromStringContainer(const aosora::raw::StringContainer& str) {
			if (str.len == 0 || str.body == nullptr) {
				return std::string();
			}
			return std::string(str.body, str.len);
		}

		inline static aosora::raw::StringContainer ToStringContainer(const std::string& str) {
			const std::string& cache = PeekContext().CacheString(str);
			return { cache.c_str(), cache.size() };
		}

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
		
		static void FetchReferencedItems(LoadedPluginModule* pluginModule, std::list<CollectableBase*>& result);

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
		static void ExecutePluginFunction(LoadedPluginModule& module, aosora::raw::PluginFunctionType pluginFunction, const ScriptValueRef& thisValue, const FunctionRequest& request, FunctionResponse& response);
		
		//アクセサ関数
		static void ReleaseHandle(aosora::raw::ValueHandle handle);
		static void AddRefHandle(aosora::raw::ValueHandle handle);
		static aosora::raw::ValueHandle CreateNumber(double value);
		static aosora::raw::ValueHandle CreateBool(bool value);
		static aosora::raw::ValueHandle CreateString(aosora::raw::StringContainer value);
		static aosora::raw::ValueHandle CreateNull();
		static aosora::raw::ValueHandle CreateFunction(aosora::raw::ValueHandle thisValue, aosora::raw::PluginFunctionType functionBody);
		static aosora::raw::ValueHandle CreateMap();
		static aosora::raw::ValueHandle CreateArray();
		static aosora::raw::ValueHandle CreateMemoryBuffer(size_t size, void** buffer, aosora::raw::BufferDestructFunctionType destructFunc);

		static double ToNumber(aosora::raw::ValueHandle handle);
		static bool ToBool(aosora::raw::ValueHandle handle);
		static aosora::raw::StringContainer ToString(aosora::raw::ValueHandle handle);
		static void* ToMemoryBuffer(aosora::raw::ValueHandle handle, size_t* size);

		static uint32_t GetValueType(aosora::raw::ValueHandle handle);
		static uint32_t GetObjectTypeId(aosora::raw::ValueHandle handle);
		static uint32_t GetClassObjectTypeId(aosora::raw::ValueHandle handle);
		static bool ObjectInstanceOf(aosora::raw::ValueHandle handle, uint32_t objectTypeId);
		static bool IsCallable(aosora::raw::ValueHandle handle);

		static void SetValue(aosora::raw::ValueHandle target, aosora::raw::ValueHandle key, aosora::raw::ValueHandle value);
		static aosora::raw::ValueHandle GetValue(aosora::raw::ValueHandle target, aosora::raw::ValueHandle key);

		static size_t GetArgumentCount();
		static aosora::raw::ValueHandle GetArgument(size_t index);

		static void SetReturnValue(aosora::raw::ValueHandle value);
		static bool SetError(aosora::raw::ValueHandle value);
		static void SetPluginError(aosora::raw::StringContainer errorMessage, int32_t errorCode);

		static void FunctionCall(aosora::raw::ValueHandle function, const aosora::raw::ValueHandle* argv, size_t argc);
		static aosora::raw::ValueHandle NewClassInstance(aosora::raw::ValueHandle classObject, const aosora::raw::ValueHandle* argv, size_t argc);

		static aosora::raw::ValueHandle GetLastReturnValue();
		static bool HasLastError();
		static aosora::raw::ValueHandle GetLastError();
		static aosora::raw::StringContainer GetLastErrorMessage();
		static int32_t GetLastErrorCode();

		static aosora::raw::StringContainer GetErrorMessage(aosora::raw::ValueHandle handle);
		static int32_t GetErrorCode(aosora::raw::ValueHandle handle);

		static aosora::raw::ValueHandle FindUnit(aosora::raw::StringContainer unitName);
		static aosora::raw::ValueHandle CreateUnit(aosora::raw::StringContainer unitName);

		static uint32_t MapGetLength(aosora::raw::ValueHandle handle);
		static bool MapContains(aosora::raw::ValueHandle handle, aosora::raw::StringContainer key);
		static void MapClear(aosora::raw::ValueHandle handle);
		static void MapRemove(aosora::raw::ValueHandle handle, aosora::raw::StringContainer key);
		static aosora::raw::ValueHandle MapGetKeys(aosora::raw::ValueHandle handle);
		static aosora::raw::ValueHandle MapGetValue(aosora::raw::ValueHandle handle, aosora::raw::StringContainer key);
		static void MapSetValue(aosora::raw::ValueHandle handle, aosora::raw::StringContainer key, aosora::raw::ValueHandle value);

		static void ArrayClear(aosora::raw::ValueHandle handle);
		static void ArrayAdd(aosora::raw::ValueHandle handle, aosora::raw::ValueHandle item);
		static void ArrayAddRange(aosora::raw::ValueHandle handle, aosora::raw::ValueHandle items);
		static void ArrayInsert(aosora::raw::ValueHandle handle, aosora::raw::ValueHandle item, uint32_t index);
		static void ArrayRemove(aosora::raw::ValueHandle handle, uint32_t index);
		static uint32_t ArrayGetLength(aosora::raw::ValueHandle handle);
		static aosora::raw::ValueHandle ArrayGetValue(aosora::raw::ValueHandle handle, uint32_t index);
		static void ArraySetValue(aosora::raw::ValueHandle handle, uint32_t index, aosora::raw::ValueHandle value);

	};
}
#endif // defined(AOSORA_ENABLE_PLUGIN_LOADER)