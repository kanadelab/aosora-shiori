use aosora_plugin_rust::aosora;

// テストよびだし
pub extern "C" fn test_function(accessor: aosora::AosoraAccessor) -> () {
	
	accessor.proc(|| {

		let main_unit = accessor.find_unit("main");
		let user_name = main_unit.get_value_with_string_key("ユーザ名")?;
		
		Ok(Some(user_name.call_function(None)?))
	});

}

// aosora plugin バージョンチェック
#[no_mangle]
pub extern "C" fn aosora_plugin_get_version(version_info: aosora::PluginVersionInfo) {
	version_info.version_check_ok();
}

// aosora plugin エントリポイント
#[no_mangle]
pub extern "C" fn aosora_plugin_load(accessor: aosora::AosoraAccessor) -> () {

	accessor.proc(|| {
		let map = accessor.create_map();

		map.set_value_with_string_key("TestFunction", &accessor.create_function(None, test_function))?;

		return Ok(Some(map));
	});
	
}
