#include "AosoraPluginWrapper.h"

namespace aosora {

	ValueWrapper::ValueWrapper(const ValueWrapper& inst):
		accessor(inst.accessor),
		handle(inst.handle) {
		AddRef();
	}

	void ValueWrapper::operator=(const ValueWrapper& inst) {

		//今持ってるハンドルを消す
		Release();

		//ハンドルを複製する
		this->accessor = inst.accessor;
		this->handle = inst.handle;
		AddRef();
	}

	ValueWrapper::~ValueWrapper() {
		Release();
	}

	AosoraAccessor ValueWrapper::Accessor() const {
		return AosoraAccessor(accessor);
	}

	void ValueWrapper::AddRef() {
		if (handle != raw::INVALID_VALUE_HANDLE) {
			accessor->AddRefHandle(handle);
		}
	}

	void ValueWrapper::Release() {
		if (handle != raw::INVALID_VALUE_HANDLE) {
			accessor->ReleaseHandle(handle);
		}
	}

	bool ValueWrapper::IsValid() const {
		return handle != raw::INVALID_VALUE_HANDLE;
	}

	double ValueWrapper::ToNumber() const {
		return Accessor().ToNumber(*this);
	}

	bool ValueWrapper::ToBool() const {
		return Accessor().ToBool(*this);
	}

	std::string ValueWrapper::ToString() const {
		return Accessor().ToString(*this);
	}

	uint32_t ValueWrapper::GetValueType() const {
		return Accessor().GetValueType(*this);
	}

	bool ValueWrapper::IsNull() const {
		return GetValueType() == accessor->VALUE_TYPE_NULL;
	}

	bool ValueWrapper::IsNumber() const {
		return GetValueType() == accessor->VALUE_TYPE_NUMBER;
	}

	bool ValueWrapper::IsString() const {
		return GetValueType() == accessor->VALUE_TYPE_STRING;
	}

	bool ValueWrapper::IsObject() const {
		return GetValueType() == accessor->VALUE_TYPE_OBJECT;
	}

	uint32_t ValueWrapper::GetObjectTypeId() const {
		return Accessor().GetObjectTypeId(*this);
	}

	bool ValueWrapper::IsArray() const {
		return GetObjectTypeId() == accessor->TYPE_ID_ARRAY;
	}

	bool ValueWrapper::IsMap() const {
		return GetObjectTypeId() == accessor->TYPE_ID_MAP;
	}

	bool ValueWrapper::IsMemoryBuffer() const {
		return GetObjectTypeId() == accessor->TYPE_ID_BUFFER;
	}

	bool ValueWrapper::IsClass() const {
		return GetObjectTypeId() == accessor->TYPE_ID_CLASS;
	}

	bool ValueWrapper::IsError() const {
		return InstanceOf(accessor->TYPE_ID_ERROR);
	}

	uint32_t ValueWrapper::GetClassObjectId() const {
		return Accessor().GetClassObjectTypeId(*this);
	}

	bool ValueWrapper::InstanceOf(uint32_t testTypeId) const {
		return Accessor().InstanceOf(*this, testTypeId);
	}

	bool ValueWrapper::IsCallable() const {
		return Accessor().IsCallable(*this);
	}

	ValueWrapper ValueWrapper::GetValue(const ValueWrapper& key, ValueWrapper* error) const {
		return Accessor().GetValue(*this, key, error);
	}

	ValueWrapper ValueWrapper::GetValue(const std::string& key, ValueWrapper* error) const {
		return Accessor().GetValue(*this, Accessor().CreateString(key), error);
	}

	ValueWrapper ValueWrapper::GetValue(uint32_t key, ValueWrapper* error) const {
		return Accessor().GetValue(*this, Accessor().CreateNumber(key), error);
	}

	void ValueWrapper::SetValue(const ValueWrapper& key, const ValueWrapper& value, ValueWrapper* error) const {
		Accessor().SetValue(*this, key, value, error);
	}

	void ValueWrapper::SetValue(const std::string& key, const ValueWrapper& value, ValueWrapper* error) const {
		Accessor().SetValue(*this, Accessor().CreateString(key), value, error);
	}

	void ValueWrapper::SetValue(uint32_t key, const ValueWrapper& value, ValueWrapper* error) const {
		Accessor().SetValue(*this, Accessor().CreateNumber(key), value, error);
	}

	ValueWrapper ValueWrapper::CallFunction(const ValueWrapper* argv, size_t argc, ValueWrapper* error) const {
		return Accessor().CallFunction(*this, argv, argc, error);
	}

	ValueWrapper ValueWrapper::CreateInstance(const ValueWrapper* argv, size_t argc, ValueWrapper* error) const {
		return Accessor().CreateInstance(*this, argv, argc, error);
	}

	std::string ValueWrapper::GetErrorMessage() const {
		return Accessor().GetErrorMessage(*this);
	}

	uint32_t ValueWrapper::GetErrorCode() const {
		return Accessor().GetErrorCode(*this);
	}

	ValueWrapper ValueWrapper::MapGetValue(const std::string& key) const {
		return Accessor().MapGetValue(*this, key);
	}

	void ValueWrapper::MapSetValue(const std::string& key, const ValueWrapper& value) const {
		Accessor().MapSetValue(*this, key, value);
	}

	uint32_t ValueWrapper::MapGetLength() const {
		return Accessor().MapGetLength(*this);
	}

	bool ValueWrapper::MapContains(const std::string& key) const {
		return Accessor().MapContains(*this, key);
	}

	void ValueWrapper::MapClear() const {
		Accessor().MapClear(*this);
	}

	void ValueWrapper::MapRemove(const std::string& key) const {
		Accessor().MapRemove(*this, key);
	}

	ValueWrapper ValueWrapper::MapGetKeys() const {
		return Accessor().MapGetKeys(*this);
	}

	void ValueWrapper::ArrayClear() const {
		Accessor().ArrayClear(*this);
	}

	void ValueWrapper::ArrayAdd(const ValueWrapper& item) const {
		Accessor().ArrayAdd(*this, item);
	}

	void ValueWrapper::ArrayAddRange(const ValueWrapper& items) const {
		Accessor().ArrayAddRange(*this, items);
	}

	void ValueWrapper::ArrayInsert(const ValueWrapper& item, uint32_t index) const {
		Accessor().ArrayInsert(*this, item, index);
	}

	void ValueWrapper::ArrayRemove(uint32_t index) const {
		Accessor().ArrayRemove(*this, index);
	}

	uint32_t ValueWrapper::ArrayGetLength() const {
		return Accessor().ArrayGetLength(*this);
	}

	ValueWrapper ValueWrapper::ArrayGetValue(uint32_t index) const {
		return Accessor().ArrayGetValue(*this, index);
	}

	void ValueWrapper::ArraySetValue(uint32_t index, const ValueWrapper& value) const {
		return Accessor().ArraySetValue(*this, index, value);
	}

}