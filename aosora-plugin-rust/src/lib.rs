
mod aosora {
	use std::ffi::{c_char, c_double, CStr, CString};


	type ValueHandle = u64;
	type PluginFunctionType = extern "C" fn(raw: *const AosoraRawAccessor);

	type ReleaseHandleFunctionType = extern "C" fn(handle: ValueHandle);

	type CreateNumberFunctionType = extern "C" fn(value: c_double) -> ValueHandle;
	type CreateBoolFunctionType = extern "C" fn(value: bool) -> ValueHandle;
	type CreateStringFunctionType = extern "C" fn(value: StringContainer) -> ValueHandle;
	type CreateNullFunctionType = extern "C" fn() -> ValueHandle;
	type CreateMapFunctionType = extern "C" fn() -> ValueHandle;
	type CreateArrayFunctionype = extern "C" fn() -> ValueHandle;
	type CreateFunctionFunctionType = extern "C" fn(thisValue: ValueHandle, functionBody: PluginFunctionType) -> ValueHandle;

	type ToNumberFunctionType = extern "C" fn(handle: ValueHandle) -> c_double;
	type ToBoolFunctionType = extern "C" fn(handle: ValueHandle) -> bool;
	type ToStringFunctionType = extern "C" fn(handle: ValueHandle) -> StringContainer;

	type GetValueFunctionType = extern "C" fn(target: ValueHandle, key: ValueHandle) -> ValueHandle;
	type SetValueFunctionType = extern "C" fn(target: ValueHandle, key: ValueHandle, value: ValueHandle);

	type GetArgumentCountFunctionType = extern "C" fn() -> usize;
	//type GetArgumentFunctionType = extern "C" fn(index: usize) -> ValueHandle;

	type SetReturnValueFunctionType = extern "C" fn(value: ValueHandle);

	const INVALID_VALUE_HANDLE:ValueHandle = 0;

	#[repr(C)]
	pub struct StringContainer {
		ptr: *const c_char,
		len: usize,
	}

	#[repr(C)]
	pub struct AosoraRawAccessor {
		release_handle: ReleaseHandleFunctionType,

		create_number: CreateNumberFunctionType,
		create_bool: CreateBoolFunctionType,
		create_string: CreateStringFunctionType,
		create_null: CreateNullFunctionType,
		create_map: CreateMapFunctionType,
		create_array: CreateArrayFunctionype,
		create_function: CreateFunctionFunctionType,

		to_number: ToNumberFunctionType,
		to_bool: ToBoolFunctionType,
		to_string: ToStringFunctionType,

		get_value: GetValueFunctionType,
		set_value: SetValueFunctionType,

		get_argument_count: GetArgumentCountFunctionType,
		get_argument: GetArgumentCountFunctionType,

		set_return_value: SetReturnValueFunctionType
	}

	pub struct AosoraAccessor {
		raw_accessor: *const AosoraRawAccessor
	}

	impl AosoraAccessor {

		// AosoraRawAccessorを安全に操作するためにAosoraAccessorを作成する
		pub fn from(raw: *const AosoraRawAccessor) -> AosoraAccessor{
			return AosoraAccessor {
				raw_accessor: raw
			};
		}

		// aosoraの文字列を作成
		pub fn create_string(&self, str: &str) -> ValueWrapper{
			let c_str = CString::new(str);
			match c_str {
				Ok(str) => 
					ValueWrapper {
						accessor: self.raw_accessor,
						handle:  unsafe { ((*self.raw_accessor).create_string)(StringContainer { ptr: str.as_ptr(), len: str.count_bytes() }) }
					}
				,
				Err(_) => ValueWrapper {
					accessor: self.raw_accessor,
					handle: INVALID_VALUE_HANDLE
				}
			}
		}

		// aosoraの連想配列を作成
		pub fn create_map(&self) -> ValueWrapper {
			ValueWrapper {
				accessor: self.raw_accessor,
				handle: unsafe {
					((*self.raw_accessor).create_map)()
				}
			}
		}

		// aosoraからプラグイン関数を呼び出すためのオブジェクトを作成
		pub fn create_function(&self, this_value: &ValueWrapper, function_body: PluginFunctionType) -> ValueWrapper {
			ValueWrapper {
				accessor: self.raw_accessor,
				handle: unsafe {
					((*self.raw_accessor).create_function)(this_value.handle, function_body)
				}
			}
		}

		// オブジェクトに値を設定する
		pub fn set_value(&self, target: &ValueWrapper, key: &ValueWrapper, value: &ValueWrapper) {
			unsafe {
				((*self.raw_accessor).set_value)(target.handle, key.handle, value.handle);
			}
		}

		// aosoraから呼び出されているプラグイン関数の戻り値を設定する
		pub fn set_return_value(&self, value: &ValueWrapper){
			unsafe {
				((*self.raw_accessor).set_return_value)(value.handle);
			}
		}

		// 無効値ハンドルを取得
		pub fn invalid_handle(&self) -> ValueWrapper{
			ValueWrapper {
				accessor: self.raw_accessor,
				handle: INVALID_VALUE_HANDLE
			}
		}
	}

	pub struct ValueWrapper {
		handle: ValueHandle,
		accessor: *const AosoraRawAccessor
	}

	//解放時にaosoraの参照を適切に破棄するためのラッパー処理
	impl Drop for ValueWrapper {
		fn drop(&mut self){
			if self.handle != INVALID_VALUE_HANDLE {
				unsafe {
					((*self.accessor).release_handle)(self.handle);	
				}
				self.handle = INVALID_VALUE_HANDLE;
			}
		}
	}

	impl ValueWrapper {
		//ハンドルが有効値かを取得する
		pub fn is_valid(&self) -> bool {
			self.handle != INVALID_VALUE_HANDLE
		}
	}


}




// テストよびだし
pub extern "C" fn test_function(raw: *const aosora::AosoraRawAccessor) -> () {
	let accessor = aosora::AosoraAccessor::from(raw);

	//文字列をかえすだけ
	accessor.set_return_value(&accessor.create_string("へろー、あおそらすと!"));
}

// aosora plugin エントリポイント
#[no_mangle]
pub extern "C" fn load(raw: *const aosora::AosoraRawAccessor) -> () {
	let accessor = aosora::AosoraAccessor::from(raw);

	/*
		連想配列を作成
		map["TestFunction"] = test_function;
		return map;
	*/
	let map = accessor.create_map();
	let key = accessor.create_string("TestFunction");
	let function = accessor.create_function(&accessor.invalid_handle(), test_function);

	accessor.set_value(&map, &key,&function);
	accessor.set_return_value(&map);
}
