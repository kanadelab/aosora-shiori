#pragma once

#include <stdint.h>

namespace aosora {
	namespace raw {

		struct AosoraRawAccessor;

		constexpr const int32_t COMPATILBILITY_VERSION = 1;

		//プラグインバージョンチェック用構造体
		struct PluginVersionInfo {
			int32_t compatibilityVersion;		//互換性バージョン これが異なると互換性がない
			int32_t pluginCompatibilityVersion;	//プラグイン側の互換性バージョン

			int32_t major;						//aosoraのメジャーバージョン
			int32_t minor;						//aosoraのリリースバージョン
			int32_t release;					//aosoraのマイナーバージョン
			int32_t versionCheckResult;			//バージョンチェックの結果通知（0で成功、それ以外で失敗）
			uint32_t flags;						//予約（何かのフラグを格納するかも）

			int32_t minMajor;					//プラグイン要求のバージョン
			int32_t minMinor;
			int32_t minRelease;

			int32_t maxMajor;
			int32_t maxMinor;
			int32_t maxRelease;

			int32_t pluginMajor;				//プラグイン側のバージョン
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
		using LoadFunctionType = void(*)(const AosoraRawAccessor* accessor);
		using UnloadFunctionType = void(*)();
		using PluginFunctionType = void(*)(const AosoraRawAccessor* accessor);
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
		using CreateMemoryBufferFunctionType = ValueHandle(*)(size_t size, void** memory, BufferDestructFunctionType destructFunc);

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
		using GetValueFunctionType = ValueHandle(*)(ValueHandle target, ValueHandle key);

		using GetArgumentCountFunctionType = size_t(*)();
		using GetArgumentFunctionType = ValueHandle(*)(size_t index);

		using SetReturnValueFunctionType = void(*)(ValueHandle returnValue);
		using SetErrorFunctionType = bool(*)(ValueHandle errorObject);
		using SetPluginErrorFunctionType = void(*)(StringContainer errorMessage, int32_t errorCode);

		using CallFunctionFunctionType = void(*)(ValueHandle function, const ValueHandle* argv, size_t argc);
		using CreateInstanceFunctionType = ValueHandle(*)(ValueHandle classType, const ValueHandle* argv, size_t argc);

		using GetLastReturnValueFunctionType = ValueHandle(*)();
		using HasLastErrorFunctionType = bool(*)();
		using GetLastErrorFunctionType = ValueHandle(*)();
		using GetLastErrorMessageFunctionType = StringContainer(*)();
		using GetLastErrorCodeFunctionType = int32_t(*)();

		using GetErrorMessageFunctionType = StringContainer(*)(ValueHandle handle);
		using GetErrorCodeFunctionType = int32_t(*)(ValueHandle handle);

		using FindUnitObjectFunctionType = ValueHandle(*)(StringContainer unitName);
		using CreateUnitObjectFunctionType = ValueHandle(*)(StringContainer unitName);

		using MapGetLengthFunctionType = uint32_t(*)(ValueHandle handle);
		using MapContainsFunctionType = bool(*)(ValueHandle handle, StringContainer key);
		using MapClearFunctionType = void(*)(ValueHandle handle);
		using MapRemoveFunctionType = void(*)(ValueHandle handle, StringContainer key);
		using MapGetKeysFunctionType = ValueHandle(*)(ValueHandle handle);
		using MapGetValueFunctionType = ValueHandle(*)(ValueHandle handle, StringContainer key);
		using MapSetValueFunctionType = void(*)(ValueHandle handle, StringContainer key, ValueHandle item);

		using ArrayClearFunctionType = void(*)(ValueHandle handle);
		using ArrayAddFunctionType = void(*)(ValueHandle handle, ValueHandle item);
		using ArrayAddRangeFunctionType = void(*)(ValueHandle handle, ValueHandle items);
		using ArrayInsertFunctionType = void(*)(ValueHandle handle, ValueHandle item, uint32_t index);
		using ArrayRemoveFunctionType = void(*)(ValueHandle handle, uint32_t index);
		using ArrayGetLengthFunctionType = uint32_t(*)(ValueHandle handle);
		using ArrayGetValueFunctionType = ValueHandle(*)(ValueHandle handle, uint32_t index);
		using ArraySetValueFunctionType = void(*)(ValueHandle handle, uint32_t index, ValueHandle item);

		//ここにまとめて関数ポインタを渡す
		struct AosoraRawAccessor {

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

			FindUnitObjectFunctionType FindUnitObject;
			CreateUnitObjectFunctionType CreateUnitObject;

			MapGetLengthFunctionType MapGetLength;
			MapContainsFunctionType MapContains;
			MapClearFunctionType MapClear;
			MapRemoveFunctionType MapRemove;
			MapGetKeysFunctionType MapGetKeys;
			MapGetValueFunctionType MapGetValue;
			MapSetValueFunctionType MapSetValue;

			ArrayClearFunctionType ArrayClear;
			ArrayAddFunctionType ArrayAdd;
			ArrayAddRangeFunctionType ArrayAddRange;
			ArrayInsertFunctionType ArrayInsert;
			ArrayRemoveFunctionType ArrayRemove;
			ArrayGetLengthFunctionType ArrayGetLength;
			ArrayGetValueFunctionType ArrayGetValue;
			ArraySetValueFunctionType ArraySetValue;

			//あとから関数をはさめるように固定領域で関数ポインタをもつ
			void(*__SPACE0[204])();	//関数256

			uint32_t VALUE_TYPE_NULL;
			uint32_t VALUE_TYPE_NUMBER;
			uint32_t VALUE_TYPE_BOOL;
			uint32_t VALUE_TYPE_STRING;
			uint32_t VALUE_TYPE_OBJECT;

			uint32_t TYPE_ID_ARRAY;
			uint32_t TYPE_ID_MAP;
			uint32_t TYPE_ID_BUFFER;
			uint32_t TYPE_ID_CLASS;
			uint32_t TYPE_ID_ERROR;

			//あとから定数をはさめるように
			uint32_t __SPACE1[246];	//定数256

		};
	}
}