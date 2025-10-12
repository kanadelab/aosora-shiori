use core::error;
/*
	aosora モジュール
*/
use std::ffi::{CStr, CString};

mod raw;
use raw::*;

/*
	rust側の文字列からaosora側の文字列に変換するためのもの
	CString本体とそこから作ったStringContainerをもち寿命をあわせる
*/
struct StringConverter {
	cstr: CString,
	container: StringContainer
}

impl StringConverter {
	fn new (str:&str) -> StringConverter{
		let cstr = match CString::new(str) {
			Ok(s) => s,
			Err(_) => CString::new("").unwrap()
		};

		let ptr = cstr.as_ptr();
		let len = cstr.count_bytes();

		StringConverter {
			cstr: cstr,
			container: StringContainer {
				ptr: ptr,
				len: len
			}
		}
	}
}

//プラグイン関数
pub type PluginFunctionType = extern "C" fn(accessor: AosoraAccessor);

#[repr(C)]
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
		// 使う側が生ポインタを使用しなくて良いように関数はラッパー型でやりとりする
		// メモリレイアウトはおなじ
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).create_function)(this_value.handle, std::mem::transmute(function_body))
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

	//プラグインエラーの送出
	pub fn set_plugin_error_with_code(&self, error_message: &str, error_code: i32){
		unsafe {
			let str = StringConverter::new(error_message);
			((*self.raw_accessor).set_plugin_error)(str.container, error_code);
		}
	}

	pub fn set_plugin_error(&self, error_message: &str){
		self.set_plugin_error_with_code(error_message, 0);
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

/*
	プラグインバージョン情報
*/
#[repr(C)]
pub struct PluginVersionInfo {
	accessor: *mut PluginRawVersionInfo
}

impl PluginVersionInfo {
	
	// aosoraのメジャーバージョン X.0.0 を取得
	pub fn get_aosora_major_version(&self) -> i32{
		unsafe { (*self.accessor).major }
	}

	// aosoraのマイナーバージョン 0.X.0 を取得
	pub fn get_aosora_minor_verion(&self) -> i32 {
		unsafe { (*self.accessor).minor }
	}

	// aosoraのリリースバージョン 0.0.X を取得
	pub fn get_aosora_release_version(&self) -> i32 {
		unsafe{ (*self.accessor).release }
	}

	// バージョンチェック成功を通知
	pub fn version_check_ok(&self) {
		unsafe {
			(*self.accessor).version_check_result = 0;
		}
	}

	pub fn version_check_ng_with_code(&self, code:i32){
		unsafe {
			(*self.accessor).version_check_result = code;
		}
	}

	pub fn version_check_ng(&self){
		self.version_check_ng_with_code(-1);
	}

}