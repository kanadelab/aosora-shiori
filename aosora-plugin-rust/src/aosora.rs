/*
	aosora モジュール
*/
use std::ffi::{c_void, CStr, CString};

mod raw;
use raw::*;

/*
	rust側の文字列からaosora側の文字列に変換するためのもの
	CString本体とそこから作ったStringContainerをもち寿命をあわせる
*/
#[allow(dead_code)]
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

impl raw::StringContainer {
	fn to_string(&self) -> String {
		//C文字列ポインタから復元
		if self.len == 0 {
			return String::new();
		}

		match unsafe {
			CStr::from_ptr(self.ptr)
		}.to_str() {
			Ok(str) => str.to_string(),
			Err(_) => String::new()
		}
	}
}

//プラグイン関数
pub type PluginFunctionType = extern "C" fn(accessor: AosoraAccessor);

/*
	Aosoraとの相互呼び出し用オブジェクト	
*/
#[repr(C)]
pub struct AosoraAccessor {
	raw_accessor: *const AosoraRawAccessor
}

impl AosoraAccessor {

	//ポインタから実体を作成
	fn from(raw: *const AosoraRawAccessor) -> AosoraAccessor{
		AosoraAccessor{
			raw_accessor: raw
		}
	}

	// aosoraの数値型を作成
	pub fn create_number(&self, value: f64) -> ValueWrapper {
		ValueWrapper { 
			accessor: self.raw_accessor, 
			handle: unsafe {
				((*self.raw_accessor).create_number)(value)
			}
		}
	}

	// aosoraのbool型を作成
	pub fn create_bool(&self, value: bool) -> ValueWrapper {
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).create_bool)(value)
			}
		}
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

	// aosoraのnullを作成
	pub fn create_null(&self) -> ValueWrapper {
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).create_null)()
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

	// aosoraの配列を作成
	pub fn create_array(&self) -> ValueWrapper {
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).create_array)()
			}
		}
	}

	// aosoraからプラグイン関数を呼び出すためのオブジェクトを作成
	pub fn create_function(&self, this_value: Option<&ValueWrapper>, function_body: PluginFunctionType) -> ValueWrapper {
		// 使う側が生ポインタを使用しなくて良いように関数はラッパー型でやりとりする
		// メモリレイアウトはおなじ

		let handle = match this_value {
			Some(v) => v.handle,
			None => raw::INVALID_VALUE_HANDLE
		};

		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).create_function)(handle, std::mem::transmute(function_body))
			}
		}
	}

	// aosoraのメモリバッファを作成
	pub fn create_memory_buffer(&self, size: usize, ptr: *mut *mut c_void) -> ValueWrapper {
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).create_memory_buffer)(size, ptr)
			}
		}	
	}

	// aosoraから数値情報を取得
	fn to_number(&self, target: &ValueWrapper) -> f64 {
		unsafe {
			((*self.raw_accessor).to_number)(target.handle)
		}
	}

	// aosoraからbool値を取得
	fn to_bool(&self, target: &ValueWrapper) -> bool {
		unsafe {
			((*self.raw_accessor).to_bool)(target.handle)
		}
	}

	// aosoraから文字列を取得
	fn to_string(&self, target: &ValueWrapper) -> String {
		let container = unsafe {
			((*self.raw_accessor).to_string)(target.handle)
		};

		container.to_string()
	}

	// TODO: aosoraからメモリバッファを取得

	// データ型を取得
	fn get_value_type(&self, target: &ValueWrapper) -> u32 {
		unsafe {
			((*self.raw_accessor).get_value_type)(target.handle)
		}
	}

	// オブジェクトのタイプIDを取得
	fn get_object_type_id(&self, target: &ValueWrapper) -> u32 {
		unsafe {
			((*self.raw_accessor).get_object_type_id)(target.handle)
		}
	}

	// クラスオブジェクトのタイプIDを取得
	fn get_class_object_type_id(&self, target: &ValueWrapper) -> u32 {
		unsafe {
			((*self.raw_accessor).get_class_object_type_id)(target.handle)
		}
	}

	// インスタンス型判定
	fn instance_of(&self, target: &ValueWrapper, object_type_id: u32) -> bool {
		unsafe {
			((*self.raw_accessor).instance_of)(target.handle, object_type_id)
		}
	}

	// 呼び出し可能か確認
	fn is_callable(&self, target: &ValueWrapper) -> bool {
		unsafe {
			((*self.raw_accessor).is_callable)(target.handle)
		}
	}

	// オブジェクトから値を取得する
	fn get_value(&self, target: &ValueWrapper, key: &ValueWrapper) -> Result<ValueWrapper, ValueWrapper> {
		unsafe {
			let handle = ((*self.raw_accessor).get_value)(target.handle, key.handle);

			if !((*self.raw_accessor).has_last_error)() {
				Ok(ValueWrapper { accessor: self.raw_accessor, handle: handle })
			}
			else {
				Err(ValueWrapper{ accessor: self.raw_accessor, handle: ((*self.raw_accessor).get_last_error)()} )
			}
		}
	}

	// オブジェクトに値を設定する
	fn set_value(&self, target: &ValueWrapper, key: &ValueWrapper, value: &ValueWrapper) -> Result<(), ValueWrapper> {
		unsafe {
			((*self.raw_accessor).set_value)(target.handle, key.handle, value.handle);

			if !((*self.raw_accessor).has_last_error)() {
				Ok(())
			}
			else {
				Err(ValueWrapper{ accessor: self.raw_accessor, handle: ((*self.raw_accessor).get_last_error)()} )
			}
		}
	}

	// 引数の数を取得
	pub fn get_argument_count(&self) -> usize {
		unsafe {
			((*self.raw_accessor).get_argument_count)()
		}
	}

	// 引数を取得
	pub fn get_argument(&self, index: usize) -> ValueWrapper {
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).get_argument)(index)
			}
		}
	}

	// 引数リストを取得
	pub fn get_argument_list(&self) -> Vec<ValueWrapper> {
		let argc = self.get_argument_count();
		let mut vec = Vec::new();
		for i in 0..argc {
			vec.push(self.get_argument(i));
		}
		vec
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

	// 関数呼び出し
	fn call_function(&self, function: &ValueWrapper, args: Option<&[ValueWrapper]>) -> Result<ValueWrapper, ValueWrapper> {
		unsafe {
			//引数の收集
			let mut items:Vec<ValueHandle> = Vec::new();
			let argv = match args {
				Some(a) => {
					items = a.iter().map(|o| o.handle).collect();
					items.as_ptr()
				},
				None => std::ptr::null()
			};

			//呼び出し
			((*self.raw_accessor).call_function)(function.handle, argv, items.len());

			//エラーと戻り値のチェック
			if !((*self.raw_accessor).has_last_error)() {
				Ok(ValueWrapper { accessor: self.raw_accessor, handle: ((*self.raw_accessor).get_last_return_value)() })
			}
			else {
				Err(ValueWrapper { accessor: self.raw_accessor, handle: ((*self.raw_accessor).get_last_error)() })
			}
		}
	}

	// インスタンス作成
	fn create_instance(&self, class_type: &ValueWrapper, args: Option<&[ValueWrapper]>) -> Result<ValueWrapper, ValueWrapper> {
		unsafe {
			//引数を收集
			let mut items:Vec<ValueHandle> = Vec::new();
			let argv = match args {
				Some(a) => {
					items = a.iter().map(|o| o.handle).collect();
					items.as_ptr()
				},
				None => std::ptr::null()
			};

			//呼び出し
			let instance:ValueHandle = ((*self.raw_accessor).create_instance)(class_type.handle, argv, items.len());

			//エラーと戻り値のチェック
			if !((*self.raw_accessor).has_last_error)() {
				Ok(ValueWrapper { accessor: self.raw_accessor, handle: instance })
			}
			else {
				Err(ValueWrapper { accessor: self.raw_accessor, handle: ((*self.raw_accessor).get_last_error)() })
			}
		}
	}

	// エラーオブジェクトからエラーメッセージを取得
	fn get_error_message(&self, error: &ValueWrapper) -> String {
		unsafe {
			((*self.raw_accessor).get_error_message)(error.handle).to_string()
		}
	}

	// エラーオブジェクトからエラーコードを取得
	#[allow(unused)]
	fn get_error_code(&self, error: &ValueWrapper) -> i32 {
		unsafe {
			((*self.raw_accessor).get_error_code)(error.handle)
		}
	}

	// ユニットの取得
	pub fn find_unit(&self, unit_name:&str) -> ValueWrapper{
		let u = StringConverter::new(unit_name);
		unsafe {
			ValueWrapper {
				accessor: self.raw_accessor,
				handle: ((*self.raw_accessor).find_unit)(u.container)
			}
		}
	}

	// ユニットの作成
	pub fn create_unit(&self, unit_name: &str) -> ValueWrapper {
		let u = StringConverter::new(unit_name);
		unsafe {
			ValueWrapper {
				accessor: self.raw_accessor,
				handle: (((*self.raw_accessor).create_unit))(u.container)
			}
		}
	}

	// Mapの操作
	fn map_get_length(&self, map: &ValueWrapper) -> u32{
		unsafe {
			((*self.raw_accessor).map_get_length)(map.handle)
		}
	}

	fn map_contains(&self, map: &ValueWrapper, key: &str) -> bool{
		let k = StringConverter::new(key);
		unsafe {
			((*self.raw_accessor).map_contains)(map.handle, k.container)
		}
	}

	fn map_clear(&self, map: &ValueWrapper) {
		unsafe {
			((*self.raw_accessor).map_clear)(map.handle)
		}
	}

	fn map_remove(&self, map: &ValueWrapper, key: &str) {
		let k = StringConverter::new(key);
		unsafe {
			((*self.raw_accessor).map_remove)(map.handle, k.container)
		}
	}

	fn map_get_keys(&self, map: &ValueWrapper) -> ValueWrapper {
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).map_get_keys)(map.handle)
			}
		}
	}

	fn map_get_value(&self, map: &ValueWrapper, key: &str) -> ValueWrapper {
		let k = StringConverter::new(key);
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe {
				((*self.raw_accessor).map_get_value)(map.handle, k.container)
			}
		}
	}

	fn map_set_value(&self, map: &ValueWrapper, key: &str, value: &ValueWrapper) {
		let k = StringConverter::new(key);
		unsafe {
			((*self.raw_accessor).map_set_value)(map.handle, k.container, value.handle);
		}
	}

	fn array_clear(&self, array: &ValueWrapper) {
		unsafe {
			((*self.raw_accessor).array_clear)(array.handle)
		}
	}

	fn array_add(&self, array: &ValueWrapper, item: &ValueWrapper){
		unsafe {
			((*self.raw_accessor).array_add)(array.handle, item.handle)
		}
	}

	fn array_add_range(&self, array: &ValueWrapper, items: &ValueWrapper) {
		unsafe {
			((*self.raw_accessor).array_add_range)(array.handle, items.handle)
		}
	}

	fn array_insert(&self, array: &ValueWrapper, item: &ValueWrapper, index: u32){
		unsafe {
			((*self.raw_accessor).array_insert)(array.handle, item.handle, index)
		}
	}

	fn array_remove(&self, array: &ValueWrapper, index: u32){
		unsafe {
			((*self.raw_accessor).array_remove)(array.handle, index)
		}
	}

	fn array_get_length(&self, array: &ValueWrapper) -> u32 {
		unsafe {
			((*self.raw_accessor).array_get_length)(array.handle)
		}
	}

	fn array_get_value(&self, array: &ValueWrapper, index: u32) -> ValueWrapper {
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: unsafe { ((*self.raw_accessor).array_get_value)(array.handle, index) }
		}
	}

	fn array_set_value(&self, array: &ValueWrapper, index: u32, value: &ValueWrapper) {
		unsafe {
			((*self.raw_accessor).array_set_value)(array.handle, index, value.handle);
		}
	}

	//値の形種別を取得
	pub fn get_value_type_null(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).value_type_null
		}
	}

	pub fn get_value_type_number(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).value_type_number
		}
	}

	pub fn get_value_type_bool(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).value_type_bool
		}
	}

	pub fn get_value_type_string(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).value_type_string
		}
	}

	pub fn get_value_type_object(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).value_type_object
		}
	}

	pub fn get_type_id_array(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).type_id_array
		}
	}

	pub fn get_type_id_map(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).type_id_map
		}
	}

	pub fn get_type_id_memory_buffer(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).type_id_memory_buffer
		}
	}

	pub fn get_type_id_class(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).type_id_class
		}
	}

	pub fn get_type_id_error(&self) -> u32 {
		unsafe {
			(*self.raw_accessor).type_id_error
		}
	}

	// 無効値ハンドルを取得
	pub fn invalid_value(&self) -> ValueWrapper{
		ValueWrapper {
			accessor: self.raw_accessor,
			handle: INVALID_VALUE_HANDLE
		}
	}

	// 関数処理ヘルパ、戻り値とエラーをそれぞれ処理してaosora側に返す
	// ? を使ってエラー処理をする場合に向いている
	pub fn proc<F: Fn() -> Result<Option<ValueWrapper>, ValueWrapper>>(&self, func: F) -> () {
		let result = func();
		match result {
			Ok(a) => {
				if let Some(return_value) = a {
					self.set_return_value(&return_value);
				}
			}
			Err(e) => {
				if e.is_error() {
					self.set_plugin_error(format!("plugin error: {}", e.get_error_message() ).as_str());
				}
				else if e.is_string() {
					self.set_plugin_error(e.to_string().as_str());
				}
				else {
					self.set_plugin_error("plugin internal error");
				}
			}
		}
	}

}

/*
	aosora 用のエラー処理ヘルパ
	失敗した場合 null 相当に評価される ValueWrapper を入手する
*/
pub trait AosoraResult {
	fn safe_unwrap(self) -> ValueWrapper;
}

impl AosoraResult for Result<ValueWrapper, ValueWrapper> {
	fn safe_unwrap(self) -> ValueWrapper {
		match self {
			Ok(r) => r,
			Err(e) => ValueWrapper { accessor: e.accessor, handle: raw::INVALID_VALUE_HANDLE }
		}
	}
}

pub struct ValueWrapper {
	accessor: *const AosoraRawAccessor,
	handle: ValueHandle
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
	fn accessor(&self) -> AosoraAccessor {
		AosoraAccessor::from(self.accessor)
	}

	//ハンドルが有効値かを取得する
	pub fn is_valid(&self) -> bool {
		self.handle != INVALID_VALUE_HANDLE
	}

	//数値として値を取得する
	pub fn to_unmber(&self) -> f64 {
		self.accessor().to_number(self)
	}

	//boolとして値を取得する
	pub fn to_bool(&self) -> bool {
		self.accessor().to_bool(self)
	}

	//文字列として値を取得する
	pub fn to_string(&self) -> String {
		self.accessor().to_string(self)
	}

	//値の種類を取得する
	pub fn get_value_type(&self) -> u32 {
		self.accessor().get_value_type(self)
	}

	//値がnull型かを取得する
	pub fn is_null(&self) -> bool {
		//invalidの場合はnullとして評価
		!self.is_valid() || self.get_value_type() == self.accessor().get_value_type_null()
	}

	//値が数値型かを確認する
	pub fn is_number(&self) -> bool {
		self.is_valid() && self.get_value_type() == self.accessor().get_value_type_number()
	}

	//値がbool型かを確認する
	pub fn is_bool(&self) -> bool {
		self.is_valid() && self.get_value_type() == self.accessor().get_value_type_bool()
	}

	//値が文字列型か確認する
	pub fn is_string(&self) -> bool {
		self.is_valid() && self.get_value_type() == self.accessor().get_value_type_string()
	}
	
	//値がオブジェクト型かを確認する
	pub fn is_object(&self) -> bool {
		self.is_valid() && self.get_value_type() == self.accessor().get_value_type_object()
	}

	//オブジェクトの形IDを取得する
	pub fn get_object_type_id(&self) -> u32 {
		self.accessor().get_object_type_id(self)
	}

	//線形配列化どうか
	pub fn is_array(&self) -> bool {
		self.is_valid() && self.get_object_type_id() == self.accessor().get_type_id_array()
	}

	//連想配列かどうか
	pub fn is_map(&self) -> bool {
		self.is_valid() && self.get_object_type_id() == self.accessor().get_type_id_map()
	}

	//メモリバッファかどうか
	pub fn is_memory_buffer(&self) -> bool {
		self.is_valid() && self.get_object_type_id() == self.accessor().get_type_id_memory_buffer()
	}

	//クラスオブジェクトかどうか
	pub fn is_class(&self) -> bool {
		self.is_valid() && self.get_object_type_id() == self.accessor().get_type_id_class()
	}

	//エラーオブジェクトかづおか
	pub fn is_error(&self) -> bool {
		self.is_valid() && self.instance_of(self.accessor().get_type_id_error())
	}

	//クラスオブジェクトの場合、その型IDを返す
	pub fn get_class_object_type_id(&self) -> u32 {
		self.accessor().get_class_object_type_id(self)
	}

	//指定オブジェクト型IDに対応するインスタンスかどうかを判定
	pub fn instance_of(&self, test_type_id: u32) -> bool {
		self.accessor().instance_of(self, test_type_id)
	}

	//呼び出し可能なオブジェクトかを判定
	pub fn is_callable(&self) -> bool {
		self.accessor().is_callable(self)
	}

	//値を取得
	pub fn get_value(&self, key: &ValueWrapper) -> Result<ValueWrapper, ValueWrapper> {
		self.accessor().get_value(self, key)
	}

	//簡易的に文字列キーを使って値を取得
	pub fn get_value_with_string_key(&self, key: &str) -> Result<ValueWrapper, ValueWrapper> {
		self.accessor().get_value(self, &self.accessor().create_string(key))
	}

	//簡易的にインデックスキーを使って値を取得
	pub fn get_value_with_index_key(&self, key: u32) -> Result<ValueWrapper, ValueWrapper> {
		self.accessor().get_value(self, &self.accessor().create_number(key as f64))
	}

	//値を設定
	pub fn set_value(&self, key: &ValueWrapper, value: &ValueWrapper) -> Result<(), ValueWrapper> {
		self.accessor().set_value(self, key, value)
	}

	//簡易的に文字列キーを使って値を設定
	pub fn set_value_with_string_key(&self, key: &str, value: &ValueWrapper) -> Result<(), ValueWrapper> {
		self.accessor().set_value(self, &self.accessor().create_string(key), value)
	}

	//簡易的にインデックスキーを使って値を設定
	pub fn set_value_with_index_key(&self, key: u32, value: &ValueWrapper) -> Result<(), ValueWrapper> {
		self.accessor().set_value(self, &self.accessor().create_number(key as f64), value)
	}

	//関数呼び出し
	pub fn call_function(&self, args: Option<&[ValueWrapper]>) -> Result<ValueWrapper, ValueWrapper> {
		self.accessor().call_function(self, args)
	}

	//クラスオブジェクトからインスタンスを作成
	pub fn create_instance(&self, args: Option<&[ValueWrapper]>) -> Result<ValueWrapper, ValueWrapper> {
		self.accessor().create_instance(self, args)
	}

	//エラーオブジェクトからエラーメッセージを取得
	pub fn get_error_message(&self) -> String{
		self.accessor().get_error_message(self)
	}

	pub fn map_get_value(&self, key:&str) -> ValueWrapper {
		self.accessor().map_get_value(self, key)
	}

	pub fn map_set_value(&self, key:&str, value:&ValueWrapper)  {
		self.accessor().map_set_value(self, key, value);
	}

	pub fn map_get_length(&self) -> u32 {
		self.accessor().map_get_length(self)
	}

	pub fn map_contains(&self, key:&str) -> bool{
		self.accessor().map_contains(self, key)
	}

	pub fn map_clear(&self){
		self.accessor().map_clear(self)
	}

	pub fn map_remove(&self, key: &str) {
		self.accessor().map_remove(self, key)
	}

	pub fn map_get_keys(&self) -> ValueWrapper {
		self.accessor().map_get_keys(self)
	}

	pub fn array_clear(&self) {
		self.accessor().array_clear(self)
	}

	pub fn array_add(&self, item: &ValueWrapper) {
		self.accessor().array_add(self, item)
	}

	pub fn array_add_range(&self, items: &ValueWrapper){
		self.accessor().array_add_range(self, items)
	}

	pub fn array_insert(&self, item: &ValueWrapper, index: u32){
		self.accessor().array_insert(self, item, index)
	}

	pub fn array_remove(&self, index: u32){
		self.accessor().array_remove(self, index)
	}

	pub fn array_get_length(&self) -> u32{
		self.accessor().array_get_length(self)
	}

	pub fn array_get_value(&self, index: u32) -> ValueWrapper {
		self.accessor().array_get_value(self, index)
	}

	pub fn array_set_value(&self, index:u32, value:&ValueWrapper ){
		self.accessor().array_set_value(self, index, value);
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

	// バイナリ互換性チェック
	pub fn check_binary_compatibility(&self) -> bool {
		unsafe {
			//プラグインの互換性情報を設定
			(*self.accessor).plugin_compatibility_version = raw::COMPATILBILITY_VERSION;

			//互換性チェック
			if (*self.accessor).compatibility_version == raw::COMPATILBILITY_VERSION {
				true
			}
			else {
				false
			}
		}
	}
	
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

	// バージョンチェック失敗を通知
	pub fn version_check_ng_with_code(&self, code:i32){
		unsafe {
			(*self.accessor).version_check_result = code;
		}
	}

	// バージョンチェック失敗を通知
	pub fn version_check_ng(&self){
		self.version_check_ng_with_code(-1);
	}

}