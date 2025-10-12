#pragma once

using uint64_t = unsigned long long;

namespace aosora {

	struct AosoraAccessor;

	//プラグインバージョンチェック用構造体
	struct PluginVersionInfo {
		int32_t major;					//aosoraのメジャーバージョン
		int32_t minor;					//aosoraのリリースバージョン
		int32_t release;				//aosoraのマイナーバージョン
		int32_t versionCheckResult;		//バージョンチェックの結果通知（0で成功、それ以外で失敗）
		int32_t reserved;				//予約（何かのフラグを格納するかも）

		int32_t minMajor;				//プラグイン要求のバージョン
		int32_t minMinor;
		int32_t minRelease;

		int32_t maxMajor;
		int32_t maxMinor;
		int32_t maxRelease;

		int32_t pluginMajor;	//プラグイン側のバージョン
		int32_t pluginMinor;
		int32_t pluginRelease;
	};

	struct StringContainer {
		const char* body;	//本体
		size_t len;			//文字列長
	};

	using ValueHandle = uint64_t;
	const ValueHandle INVALID_VALUE_HANDLE = 0;

	using GetVersionFunctionType = bool(*)(PluginVersionInfo* info);
	using LoadFunctionType = void(*)(const AosoraAccessor* accessor);
	using UnloadFunctionType = void(*)();
	using PluginFunctionType = void(*)(const AosoraAccessor* accessor);
	using BufferDestructFunctionType = void(*)(void* body, size_t len);

	using ReleaseHandleFunctionType = void(*)(ValueHandle handle);
	using AddRefHandleFunctionType = void(*)(ValueHandle handle);

	using CreateNumberFunctionType = ValueHandle(*)(double value);
	using CreateBoolFunctionType = ValueHandle(*)(bool value);
	using CreateStringFunctionType = ValueHandle(*)(StringContainer value);
	using CreateNullFunctionType = ValueHandle(*)();
	using CreateMapFunctionType = ValueHandle(*)();
	using CreateArrayFnctionType = ValueHandle(*)();
	using CreateFunctionFunctionType = ValueHandle(*)(ValueHandle thisValue, PluginFunctionType functionBody);
	using CreateMemoryBufferFunctionType = ValueHandle(*)(size_t size, void** memory, aosora::BufferDestructFunctionType destructFunc);

	using ToNumberFunctionType = double(*)(ValueHandle handle);
	using ToBoolFunctionType = bool(*)(ValueHandle handle);
	using ToStringFunctionType = StringContainer(*)(ValueHandle handle);
	using ToMemoryBufferFunctionType = void* (*)(ValueHandle handle, size_t* size);

	using GetValueTypeFunctionType = uint32_t(*)(ValueHandle handle);
	using GetObjectTypeIdFunctionType = uint32_t(*)(ValueHandle handle);
	using GetClassObjectTypeIdFunctionType = uint32_t(*)(ValueHandle handle);
	using ObjectInstanceOfFunctionType = bool(*)(ValueHandle handle, uint32_t objectTypeId);
	using IsCallableFunctionType = bool(*)(ValueHandle);

	using SetValueFunctionType = void(*)(ValueHandle target, ValueHandle key, ValueHandle value);
	using GetValueFunctionType = aosora::ValueHandle(*)(ValueHandle target, ValueHandle key);

	using GetArgumentCountFunctionType = size_t(*)();
	using GetArgumentFunctionType = ValueHandle(*)(size_t index);

	using SetReturnValueFunctionType = void(*)(ValueHandle returnValue);
	using SetErrorFunctionType = bool(*)(ValueHandle errorObject);
	using SetPluginErrorFunctionType = void(*)(StringContainer errorMessage, int32_t errorCode);

	using CallFunctionFunctionType = void(*)(ValueHandle function, const ValueHandle* argv, size_t argc);
	using CreateInstanceFunctionType = aosora::ValueHandle(*)(ValueHandle classType, const ValueHandle* argv, size_t argc);

	using GetLastReturnValueFunctionType = ValueHandle(*)();
	using HasLastErrorFunctionType = bool(*)();
	using GetLastErrorFunctionType = ValueHandle(*)();
	using GetLastErrorMessageFunctionType = StringContainer(*)();
	using GetLastErrorCodeFunctionType = int32_t(*)();

	using GetErrorMessageFunctionType = StringContainer(*)(ValueHandle handle);
	using GetErrorCodeFunctionType = int32_t(*)(ValueHandle handle);

	using FindUnitObjectFunctionType = ValueHandle(*)(StringContainer unitName);
	using CreateUnitObjectFunctionType = ValueHandle(*)(StringContainer unitName);

	//ここにまとめて関数ポインタを渡す
	struct AosoraAccessor {
		ReleaseHandleFunctionType ReleaseHandle;
		AddRefHandleFunctionType AddRefHandle;

		CreateNumberFunctionType CreateNumber;
		CreateBoolFunctionType CreateBool;
		CreateStringFunctionType CreateString;
		CreateNullFunctionType CreateNull;
		CreateMapFunctionType CreateMap;
		CreateArrayFnctionType CreateArray;
		CreateFunctionFunctionType CreateFunction;
		CreateMemoryBufferFunctionType CreateBuffer;

		ToNumberFunctionType ToNumber;
		ToBoolFunctionType ToBool;
		ToStringFunctionType ToString;
		ToMemoryBufferFunctionType ToBuffer;

		GetValueTypeFunctionType GetValueType;
		GetObjectTypeIdFunctionType GetObjectTypeId;
		GetClassObjectTypeIdFunctionType GetClassObjectTypeId;
		ObjectInstanceOfFunctionType InstanceOf;
		IsCallableFunctionType IsCallable;

		GetValueFunctionType GetValue;
		SetValueFunctionType SetValue;

		GetArgumentCountFunctionType GetArgumentCount;
		GetArgumentFunctionType GetArgument;

		SetReturnValueFunctionType SetReturnValue;
		SetErrorFunctionType SetError;
		SetPluginErrorFunctionType SetPluginError;

		CallFunctionFunctionType CallFunction;
		CreateInstanceFunctionType CreateInstance;

		GetLastReturnValueFunctionType GetLastReturnValue;
		HasLastErrorFunctionType HasLastError;
		GetLastErrorFunctionType GetLastError;
		GetLastErrorMessageFunctionType GetLastErrorMessage;
		GetLastErrorCodeFunctionType GetLastErrorCode;

		GetErrorMessageFunctionType GetErrorMessage;
		GetErrorCodeFunctionType GetErrorCode;

		//TODO: まともな並べ方をするためにスペーシングしたほうがいいかも、あとで変えられるように

		uint32_t VALUE_TYPE_NULL;
		uint32_t VALUE_TYPE_NUMBER;
		uint32_t VALUE_TYPE_BOOL;
		uint32_t VALUE_TYPE_STRING;
		uint32_t VALUE_TYPE_OBJECT;

		uint32_t TYPE_ID_ARRAY;
		uint32_t TYPE_ID_MAP;
		uint32_t TYPE_ID_BUFFER;
		uint32_t TYPE_ID_CLASS;
	};
}
