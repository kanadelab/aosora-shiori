#pragma once
#include <string>
#include "AosoraPlugin.h"

namespace aosora {
	class AosoraAccessor;

	//プラグイン関数型
	using PluginFunction = void(*)(AosoraAccessor);
	using MemoryBufferDestructor = raw::BufferDestructFunctionType;

	class ValueWrapper {
		friend class AosoraAccessor;
	private:
		const raw::AosoraRawAccessor* accessor;
		raw::ValueHandle handle;

	private:
		AosoraAccessor Accessor() const;
		void AddRef();
		void Release();

	public:
		ValueWrapper(const raw::AosoraRawAccessor* accessor, raw::ValueHandle handle):
			accessor(accessor),
			handle(handle)
		{ }

		ValueWrapper(const ValueWrapper& inst);
		void operator=(const ValueWrapper& inst);
		~ValueWrapper();

		bool IsValid() const;
		double ToNumber() const;
		bool ToBool() const;
		std::string ToString() const;

		uint32_t GetValueType() const;
		bool IsNull() const;
		bool IsNumber() const;
		bool IsString() const;
		bool IsObject() const;

		uint32_t GetObjectTypeId() const;
		bool IsArray() const;
		bool IsMap() const;
		bool IsMemoryBuffer() const;
		bool IsClass() const;
		bool IsError() const;

		uint32_t GetClassObjectId() const;
		bool InstanceOf(uint32_t testTypeId) const;
		bool IsCallable() const;

		ValueWrapper GetValue(const ValueWrapper& key, ValueWrapper* error) const;
		ValueWrapper GetValue(const std::string& key, ValueWrapper* error) const;
		ValueWrapper GetValue(uint32_t key, ValueWrapper* error) const;

		void SetValue(const ValueWrapper& key, const ValueWrapper& value, ValueWrapper* error) const;
		void SetValue(const std::string& key, const ValueWrapper& value, ValueWrapper* error) const;
		void SetValue(uint32_t key, const ValueWrapper& value, ValueWrapper* error) const;

		ValueWrapper CallFunction(const ValueWrapper* argv, size_t argc, ValueWrapper* error) const;
		ValueWrapper CreateInstance(const ValueWrapper* argv, size_t argc, ValueWrapper* error) const;

		std::string GetErrorMessage() const;
		uint32_t GetErrorCode() const;

		ValueWrapper MapGetValue(const std::string& key) const;
		void MapSetValue(const std::string& key, const ValueWrapper& value) const;
		uint32_t MapGetLength() const;
		bool MapContains(const std::string& key) const;
		void MapClear() const;
		void MapRemove(const std::string& key) const;
		ValueWrapper MapGetKeys() const;

		void ArrayClear() const;
		void ArrayAdd(const ValueWrapper& item) const;
		void ArrayAddRange(const ValueWrapper& items) const;
		void ArrayInsert(const ValueWrapper& item, uint32_t index) const;
		void ArrayRemove(uint32_t index) const;
		uint32_t ArrayGetLength() const;
		ValueWrapper ArrayGetValue(uint32_t index) const;
		void ArraySetValue(uint32_t indes, const ValueWrapper& value) const;

	};

	class AosoraAccessor {
		friend class ValueWrapper;
	private:
		const raw::AosoraRawAccessor* rawAccessor;

	private:
		static raw::StringContainer ToStringContainer(const std::string& str) {
			if (str.empty()) {
				return { nullptr, 0 };
			}
			return { str.c_str(), str.size() };
		}

	public:
		AosoraAccessor(const raw::AosoraRawAccessor* rawAccessor):
			rawAccessor(rawAccessor)
		{ }

		// aosoraの数値型を作成
		ValueWrapper CreateNumber(double value) const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateNumber(value));
		}

		// aosoraのbool型を作成
		ValueWrapper CreateBool(bool value) const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateBool(value));
		}

		// aosoraの文字列型を作成
		ValueWrapper CreateString(const std::string& str) const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateString(ToStringContainer(str)));
		}

		// aosoraのnullを作成
		ValueWrapper CreateNull() const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateNull());
		}

		// aosoraの連想配列を作成
		ValueWrapper CreateMap() const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateMap());
		}

		// aosoraの配列を作成
		ValueWrapper CreateArray() const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateArray());
		}

		// aosoraの関数を作成
		ValueWrapper CreateFunction(PluginFunction functionBody) const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateFunction(raw::INVALID_VALUE_HANDLE, reinterpret_cast<raw::PluginFunctionType>(functionBody)));
		}

		ValueWrapper CreateFunction(const ValueWrapper& thisValue, PluginFunction functionBody) const{
			return ValueWrapper(rawAccessor, rawAccessor->CreateFunction(thisValue.handle, reinterpret_cast<raw::PluginFunctionType>(functionBody)));
		}

		// メモリバッファを作成
		ValueWrapper CreateMemoryBuffer(size_t size, void** ptr, MemoryBufferDestructor destructor = nullptr) const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateBuffer(size, ptr, destructor));
		}

		// 引数の数を取得
		size_t GetArgumentCount() const {
			return rawAccessor->GetArgumentCount();
		}

		ValueWrapper GetArgument(size_t index) const {
			return ValueWrapper(rawAccessor, rawAccessor->GetArgument(index));
		}

		void SetReturnValue(const ValueWrapper& value) const {
			rawAccessor->SetReturnValue(value.handle);
		}

		void SetPluginError(const std::string& errorMessage, int32_t errorCode = 0) const {
			rawAccessor->SetPluginError(ToStringContainer(errorMessage), errorCode);
		}

		ValueWrapper FindUnit(const std::string& unitName) const {
			return ValueWrapper(rawAccessor, rawAccessor->FindUnitObject(ToStringContainer(unitName)));
		}

		ValueWrapper CreateUnit(const std::string& unitName) const {
			return ValueWrapper(rawAccessor, rawAccessor->CreateUnitObject(ToStringContainer(unitName)));
		}

		uint32_t GetValueTypeNull() const {
			return rawAccessor->VALUE_TYPE_NULL;
		}

		uint32_t GetValueNumber() const {
			return rawAccessor->VALUE_TYPE_NUMBER;
		}

		uint32_t GetValueBool() const {
			return rawAccessor->VALUE_TYPE_BOOL;
		}

		uint32_t GetValueString() const {
			return rawAccessor->VALUE_TYPE_STRING;
		}

		uint32_t GetValueObject() const {
			return rawAccessor->VALUE_TYPE_OBJECT;
		}

		uint32_t GetTypeIdArray() const {
			return rawAccessor->TYPE_ID_ARRAY;
		}

		uint32_t GetTypeIdMap() const {
			return rawAccessor->TYPE_ID_MAP;
		}

		uint32_t GetTypeIdMemoryBuffer() const {
			return rawAccessor->TYPE_ID_BUFFER;
		}

		uint32_t GetTypeIdClass() const {
			return rawAccessor->TYPE_ID_CLASS;
		}

		uint32_t GetTypeIdError() const {
			return rawAccessor->TYPE_ID_ERROR;
		}

		ValueWrapper InvalidHandle() const {
			return ValueWrapper(rawAccessor, raw::INVALID_VALUE_HANDLE);
		}

	private:

		double ToNumber(const ValueWrapper& target) const {
			return rawAccessor->ToNumber(target.handle);
		}

		bool ToBool(const ValueWrapper& target) const {
			return rawAccessor->ToBool(target.handle);
		}

		std::string ToString(const ValueWrapper& target) const {
			auto c = rawAccessor->ToString(target.handle);
			return std::string(c.body, c.len);
		}

		void* ToMemoryBuffer(const ValueWrapper& target, size_t* size) const {
			return rawAccessor->ToBuffer(target.handle, size);
		}

		uint32_t GetValueType(const ValueWrapper& target) const {
			return rawAccessor->GetValueType(target.handle);
		}

		uint32_t GetObjectTypeId(const ValueWrapper& target) const {
			return rawAccessor->GetObjectTypeId(target.handle);
		}

		uint32_t GetClassObjectTypeId(const ValueWrapper& target) const {
			return rawAccessor->GetClassObjectTypeId(target.handle);
		}

		bool InstanceOf(const ValueWrapper& target, uint32_t objectTypeId) const {
			return rawAccessor->InstanceOf(target.handle, objectTypeId);
		}

		bool IsCallable(const ValueWrapper& target) const {
			return rawAccessor->IsCallable(target.handle);
		}

		ValueWrapper GetValue(const ValueWrapper& target, const ValueWrapper& key, ValueWrapper* error) const {
			auto handle = rawAccessor->GetValue(target.handle, key.handle);
			if (!rawAccessor->HasLastError()) {
				return ValueWrapper(rawAccessor, handle);
			}
			else {
				if (error != nullptr) {
					*error = ValueWrapper(rawAccessor, rawAccessor->GetLastError());
				}
				return ValueWrapper(rawAccessor, raw::INVALID_VALUE_HANDLE);
			}
		}

		void SetValue(const ValueWrapper& target, const ValueWrapper& key, const ValueWrapper& value, ValueWrapper* error) const {
			rawAccessor->SetValue(target.handle, key.handle, value.handle);
			if (rawAccessor->HasLastError()) {
				if (error != nullptr) {
					*error = ValueWrapper(rawAccessor, rawAccessor->GetLastError());
				}
			}
		}

		ValueWrapper CallFunction(const ValueWrapper& function, const ValueWrapper* argv, size_t argc, ValueWrapper* error) const {
			rawAccessor->CallFunction(function.handle, reinterpret_cast<const raw::ValueHandle*>(argv), argc);

			if (!rawAccessor->HasLastError()) {
				return ValueWrapper(rawAccessor, rawAccessor->GetLastReturnValue());
			}
			else {
				if (error != nullptr) {
					*error = ValueWrapper(rawAccessor, rawAccessor->GetLastError());
				}
				return ValueWrapper(rawAccessor, raw::INVALID_VALUE_HANDLE);
			}
		}

		ValueWrapper CreateInstance(const ValueWrapper& classType, const ValueWrapper* argv, size_t argc, ValueWrapper* error) const {
			auto handle = rawAccessor->CreateInstance(classType.handle, reinterpret_cast<const raw::ValueHandle*>(argv), argc);

			if (!rawAccessor->HasLastError()) {
				return ValueWrapper(rawAccessor, handle);
			}
			else {
				if (error != nullptr) {
					*error = ValueWrapper(rawAccessor, rawAccessor->GetLastError());
				}
				return ValueWrapper(rawAccessor, raw::INVALID_VALUE_HANDLE);
			}
		}

		std::string GetErrorMessage(const ValueWrapper& error) const {
			auto c = rawAccessor->GetErrorMessage(error.handle);
			return std::string(c.body, c.len);
		}

		int32_t GetErrorCode(const ValueWrapper& error) const {
			return rawAccessor->GetErrorCode(error.handle);
		}

		uint32_t MapGetLength(const ValueWrapper& obj) const {
			return rawAccessor->MapGetLength(obj.handle);
		}

		bool MapContains(const ValueWrapper& obj, const std::string& key) const {
			return rawAccessor->MapContains(obj.handle, ToStringContainer(key));
		}

		void MapClear(const ValueWrapper& obj) const {
			rawAccessor->MapClear(obj.handle);
		}

		void MapRemove(const ValueWrapper& obj, const std::string& key) const {
			rawAccessor->MapRemove(obj.handle, ToStringContainer(key));
		}

		ValueWrapper MapGetKeys(const ValueWrapper& obj) const {
			return ValueWrapper(rawAccessor, rawAccessor->MapGetKeys(obj.handle));
		}

		ValueWrapper MapGetValue(const ValueWrapper& obj, const std::string& key) const {
			return ValueWrapper(rawAccessor, rawAccessor->MapGetValue(obj.handle, ToStringContainer(key)));
		}

		void MapSetValue(const ValueWrapper& obj, const std::string& key, const ValueWrapper& value) const {
			rawAccessor->MapSetValue(obj.handle, ToStringContainer(key), value.handle);
		}

		void ArrayClear(const ValueWrapper& obj) const {
			rawAccessor->ArrayClear(obj.handle);
		}

		void ArrayAdd(const ValueWrapper& obj, const ValueWrapper& item) const {
			rawAccessor->ArrayAdd(obj.handle, item.handle);
		}

		void ArrayAddRange(const ValueWrapper& obj, const ValueWrapper& items) const {
			rawAccessor->ArrayAddRange(obj.handle, items.handle);
		}

		void ArrayInsert(const ValueWrapper& obj, const ValueWrapper& item, uint32_t index) const {
			rawAccessor->ArrayInsert(obj.handle, item.handle, index);
		}

		void ArrayRemove(const ValueWrapper& obj, uint32_t index) const {
			rawAccessor->ArrayRemove(obj.handle, index);
		}

		uint32_t ArrayGetLength(const ValueWrapper& obj) const {
			return rawAccessor->ArrayGetLength(obj.handle);
		}

		ValueWrapper ArrayGetValue(const ValueWrapper& obj, uint32_t index) const {
			return ValueWrapper(rawAccessor, rawAccessor->ArrayGetValue(obj.handle, index));
		}

		void ArraySetValue(const ValueWrapper& obj, uint32_t index, const ValueWrapper& value) const {
			rawAccessor->ArraySetValue(obj.handle, index, value.handle);
		}
	};

	/**
	 *	バージョン情報
	 */
	class PluginVersionInfo {
	private:
		raw::PluginVersionInfo* versionInfo;

	public:

		/**
		 *	バイナリ互換性チェック。必ず aosora_plugin_get_version() で呼び出し、falseが帰ったらすぐに戻ること。
		 */
		bool CheckBinaryCompatibility() const {
			versionInfo->pluginCompatibilityVersion = raw::COMPATILBILITY_VERSION;
			if (versionInfo->compatibilityVersion == raw::COMPATILBILITY_VERSION) {
				return true;
			}
			else {
				return false;
			}
		}

		void VersionCheckOk() const {
			versionInfo->versionCheckResult = 0;
		}
	};
}