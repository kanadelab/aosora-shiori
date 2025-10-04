#pragma once

using uint64_t = unsigned long long;

namespace aosora {

	struct AosoraAccessor;

	struct StringContainer {
		const char* body;	//本体
		size_t len;			//文字列長
	};

	using ValueHandle = uint64_t;
	using PluginFunctionType = void(*)(AosoraAccessor* accessor);
	const ValueHandle INVALID_VALUE_HANDLE = 0;

	using ReleaseHandleFunctionType = void(*)(ValueHandle handle);

	using CreateNumberFunctionType = ValueHandle(*)(double value);
	using CreateBoolFunctionType = ValueHandle(*)(bool value);
	using CreateStringFunctionType = ValueHandle(*)(StringContainer value);
	using CreateNullFunctionType = ValueHandle(*)();
	using CreateMapFunctionType = ValueHandle(*)();
	using CreateArrayFnctionType = ValueHandle(*)();
	using CreateFunctionFunctionType = ValueHandle(*)(ValueHandle thisValue, PluginFunctionType functionBody);
	using CreateBufferFunctionType = ValueHandle(*)(size_t size, const void* memory);

	using ToNumberFunctionType = double(*)(ValueHandle handle);
	using ToBoolFunctionType = bool(*)(ValueHandle handle);
	using ToStringFunctionType = StringContainer(*)(ValueHandle handle);
	using ToBufferFunctionType = void* (*)(ValueHandle handle);

	using SetValueFunctionType = void(*)(ValueHandle target, ValueHandle key, ValueHandle value);
	using GetValueFunctionType = void(*)(ValueHandle target, ValueHandle key);

	using GetArgumentCountFunctionType = size_t(*)();
	using GetArgumentFunctionType = ValueHandle(*)(size_t index);

	using SetReturnValueFunctionType = void(*)(ValueHandle returnValue);
	using SetErrorFunctionType = void(*)(ValueHandle errorObject);

	//ここにまとめて関数ポインタを渡す
	struct AosoraAccessor {
		ReleaseHandleFunctionType ReleaseHandle;

		CreateNumberFunctionType CreateNumber;
		CreateBoolFunctionType CreateBool;
		CreateStringFunctionType CreateString;
		CreateNullFunctionType CreateNull;
		CreateMapFunctionType CreateMap;
		CreateArrayFnctionType CreateArray;
		CreateFunctionFunctionType CreateFunction;

		ToNumberFunctionType ToNumber;
		ToBoolFunctionType ToBool;
		ToStringFunctionType ToString;

		SetValueFunctionType SetValue;
		GetValueFunctionType GetValue;

		GetArgumentCountFunctionType GetArgumentCount;
		GetArgumentFunctionType GetArgument;

		SetReturnValueFunctionType SetReturnValue;
	};

	struct FunctionContext {
		size_t GetArgumentCount();
		ValueHandle GetArgument(size_t index);
		ValueHandle GetThisValue(size_t index);

		void SetReturnValue(ValueHandle handle);
		void SetError(ValueHandle handle);
	};

	struct ResultContext {
		ValueHandle GetReturnValue();
		ValueHandle GetError();
	};

	

	//変数ハンドル
	using LoadFunctionType = void(*)(AosoraAccessor* accessor);
	using UnloadFunctionType = void(*)();

	double ToNumber(ValueHandle v);
	StringContainer ToString(ValueHandle v);

	ValueHandle CreateNumber();
	ValueHandle CreateString();
	ValueHandle CreateFunction(ValueHandle thisValue, PluginFunctionType functionBody);
	//ValueHandle CreatePluginBuffer()

	ValueHandle CallFunction(ValueHandle func, ValueHandle* argv, size_t argc);
	void Set(ValueHandle target, const char* key, ValueHandle value);
	ValueHandle Get(ValueHandle target, const char* key);
}
