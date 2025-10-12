use aosora_plugin_rust::aosora;

// テストよびだし
pub extern "C" fn test_function(accessor: aosora::AosoraAccessor) -> () {

	//文字列をかえすだけ
	//accessor.set_return_value(&accessor.create_string("へろー、あおそらすと!"));
	accessor.set_plugin_error("えらーです！");
}

// aosora plugin バージョンチェック
#[no_mangle]
pub extern "C" fn aosora_plugin_get_version(version_info: aosora::PluginVersionInfo) {
	version_info.version_check_ok();
}

// aosora plugin エントリポイント
#[no_mangle]
pub extern "C" fn aosora_plugin_load(accessor: aosora::AosoraAccessor) -> () {
	
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
