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

	class PluginModule;

	struct PluginCallContext {
		std::vector<ScriptValueRef> args;
		ScriptValueRef thisValue;
		ScriptValueRef returnValue;
		ScriptValueRef threwError;
	};

	class PluginContext {
	private:
		ScriptInterpreter* interpreter;
		PluginCallContext* callContext;
		PluginModule* pluginModule;

	public:
		ScriptInterpreter& GetInterpreter() {
			return *interpreter;
		}

		PluginModule* GetPluginModule() {
			return pluginModule;
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
		static std::vector<PluginContext> contextStack;
		static std::map<PluginModule*, PluginHandleManager*> plugins;

	public:
		static void PushContext(const PluginContext& pluginContext) {
			contextStack.push_back(pluginContext);
		}

		static PluginContext& PeekContext() {
			assert(!contextStack.empty());
			return *contextStack.rbegin();
		}

		static const void PopContext() {
			contextStack.pop_back();
		}

		static PluginHandleManager& GetCurrentHandleManager() {
			return *plugins[PeekContext().GetPluginModule()];
		}

		static aosora::AosoraAccessor MakeAccessor();

		//アクセサ関数
		static aosora::ValueHandle CreateNumber(double value);
		static aosora::ValueHandle CreateBool(bool value);
		static aosora::ValueHandle CreateString(aosora::StringContainer value);
		static aosora::ValueHandle CreateNull();

		static double ToNumber(aosora::ValueHandle handle);
		static bool ToBool(aosora::ValueHandle handle);
		static aosora::StringContainer ToString(aosora::ValueHandle handle);

		static size_t GetArgumentCount();
		static aosora::ValueHandle GetArgument(size_t index);
	};
}