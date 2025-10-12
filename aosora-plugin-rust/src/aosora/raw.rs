/*
    aosora rawモジュール
    C++側との低レイヤ直接通信
*/
use std::ffi::{c_char, c_double, c_void};

pub type ValueHandle = u64;
pub type RawPluginFunctionType = extern "C" fn(raw: *const AosoraRawAccessor);

type ReleaseHandleFunctionType = extern "C" fn(handle: ValueHandle);
type AddRefHandleFunctionType = extern "C" fn(handle: ValueHandle);

type CreateNumberFunctionType = extern "C" fn(value: c_double) -> ValueHandle;
type CreateBoolFunctionType = extern "C" fn(value: bool) -> ValueHandle;
type CreateStringFunctionType = extern "C" fn(value: StringContainer) -> ValueHandle;
type CreateNullFunctionType = extern "C" fn() -> ValueHandle;
type CreateMapFunctionType = extern "C" fn() -> ValueHandle;
type CreateArrayFunctionype = extern "C" fn() -> ValueHandle;
type CreateFunctionFunctionType = extern "C" fn(thisValue: ValueHandle, functionBody: RawPluginFunctionType) -> ValueHandle;
type CreateMemoryBufferFunctionType = extern "C" fn(size: usize, ptr: *mut *mut c_void) -> ValueHandle;

type ToNumberFunctionType = extern "C" fn(handle: ValueHandle) -> c_double;
type ToBoolFunctionType = extern "C" fn(handle: ValueHandle) -> bool;
type ToStringFunctionType = extern "C" fn(handle: ValueHandle) -> StringContainer;
type ToMemoryBufferFunctionType = extern "C" fn(handle: ValueHandle, size: *mut usize) -> *mut c_void;

type GetValueTypeFunctionType = extern "C" fn(handle: ValueHandle) -> u32;
type GetObjectTypeIdFunctionType = extern "C" fn(handle: ValueHandle) -> u32;
type GetClassObjectTypeIdFunctionType = extern "C" fn(handle: ValueHandle) -> u32;
type ObjectInstanceOfFunctionType = extern "C" fn(handle: ValueHandle, objectTypeId: u32) -> bool;
type IsCallableFunctionType = extern "C" fn(handle: ValueHandle) -> bool;

type GetValueFunctionType = extern "C" fn(target: ValueHandle, key: ValueHandle) -> ValueHandle;
type SetValueFunctionType = extern "C" fn(target: ValueHandle, key: ValueHandle, value: ValueHandle);

type GetArgumentCountFunctionType = extern "C" fn() -> usize;
type GetArgumentFunctionType = extern "C" fn(index: usize) -> ValueHandle;

type SetReturnValueFunctionType = extern "C" fn(value: ValueHandle);
type SetErrorFunctionType = extern "C" fn(errorObject: ValueHandle);
type SetPluginErrorFunctionType = extern "C" fn(errorMessage: StringContainer, errorCode: i32);

type CallFunctionFunctionType = extern "C" fn(function: ValueHandle, argv: *const ValueHandle, argc: usize);
type CreateInstanceFunctionType = extern "C" fn(classType: ValueHandle, argv: *const ValueHandle, argc: usize) -> ValueHandle;

type GetLastReturnValueFunctionType = extern "C" fn() -> ValueHandle;
type HasLastErrorFunctionTyoe = extern "C" fn() -> bool;
type GetLastErrorFunctionType = extern "C" fn() -> ValueHandle;
type GetLastErrorMessageFunctionType = extern "C" fn() -> StringContainer;
type GetLastErrorCodeFunctionType = extern "C" fn() -> i32;

type GetErrorMessageFunctionType = extern "C" fn(handle: ValueHandle) -> StringContainer;
type GetErrorCodeFunctionType = extern "C" fn(handle: ValueHandle) -> i32;

type FindUnitObjectFunctionType = extern "C" fn(unitName:StringContainer) -> ValueHandle;
type CreateUnitObjectFunctionType = extern "C" fn(unitName:StringContainer) -> ValueHandle;

pub const INVALID_VALUE_HANDLE:ValueHandle = 0;

#[repr(C)]
pub struct PluginRawVersionInfo {
	pub(super) major: i32,
	pub(super) minor: i32,
	pub(super) release: i32,
	
	pub(super) version_check_result: i32,
	pub(super) flags: u32,

	pub(super) min_major: i32,
	pub(super) min_minor: i32,
	pub(super) min_release: i32,

	pub(super) max_major: i32,
	pub(super) max_minor: i32,
	pub(super) max_release: i32,

	pub(super) plugin_major: i32,
	pub(super) plugin_minor: i32,
	pub(super) plugin_release: i32
}

#[repr(C)]
pub struct StringContainer {
	pub(super) ptr: *const c_char,
	pub(super) len: usize,
}

#[repr(C)]
pub struct AosoraRawAccessor {
	pub(super) release_handle: ReleaseHandleFunctionType,
	pub(super) addref_handle: AddRefHandleFunctionType,

	pub(super) create_number: CreateNumberFunctionType,
	pub(super) create_bool: CreateBoolFunctionType,
	pub(super) create_string: CreateStringFunctionType,
	pub(super) create_null: CreateNullFunctionType,
	pub(super) create_map: CreateMapFunctionType,
	pub(super) create_array: CreateArrayFunctionype,
	pub(super) create_function: CreateFunctionFunctionType,
	pub(super) create_memory_buffer: CreateMemoryBufferFunctionType,

	pub(super) to_number: ToNumberFunctionType,
	pub(super) to_bool: ToBoolFunctionType,
	pub(super) to_string: ToStringFunctionType,
	pub(super) to_memory_buffer: ToMemoryBufferFunctionType,

	pub(super) get_value_type: GetValueTypeFunctionType,
	pub(super) get_object_type_id: GetObjectTypeIdFunctionType,
	pub(super) get_class_object_type_id: GetClassObjectTypeIdFunctionType,
	pub(super) instance_of: ObjectInstanceOfFunctionType,
	pub(super) is_callable: IsCallableFunctionType,

	pub(super) get_value: GetValueFunctionType,
	pub(super) set_value: SetValueFunctionType,

	pub(super) get_argument_count: GetArgumentCountFunctionType,
	pub(super) get_argument: GetArgumentFunctionType,

	pub(super) set_return_value: SetReturnValueFunctionType,
	pub(super) set_error: SetErrorFunctionType,
	pub(super) set_plugin_error: SetPluginErrorFunctionType,

	pub(super) call_function: CallFunctionFunctionType,
	pub(super) create_instance: CreateInstanceFunctionType,

	pub(super) get_last_return_value: GetLastReturnValueFunctionType,
	pub(super) has_last_error: HasLastErrorFunctionTyoe,
	pub(super) get_last_error: GetLastErrorFunctionType,
	pub(super) get_last_error_message: GetLastErrorMessageFunctionType,
	pub(super) get_last_error_code: GetLastErrorCodeFunctionType,

	pub(super) get_error_message: GetErrorMessageFunctionType,
	pub(super) get_error_code: GetErrorCodeFunctionType,

	pub(super) find_unit: FindUnitObjectFunctionType,
	pub(super) create_unit: CreateUnitObjectFunctionType,

	pub(super) value_type_null: u32,
	pub(super) value_type_number: u32,
	pub(super) value_type_bool: u32,
	pub(super) value_type_string: u32,
	pub(super) value_type_object: u32,

	pub(super) type_id_array: u32,
	pub(super) type_id_map: u32,
	pub(super) type_id_memory_buffer: u32,
	pub(super) type_id_class: u32
}
