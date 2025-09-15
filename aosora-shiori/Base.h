#pragma once

//ベースのヘッダ
//プラットフォーム別で各種の機能のスイッチングなど。

#if defined(WIN32) || defined(_WIN32)
#define AOSORA_REQUIRED_WIN32
#endif

//SAORIローダの有効化
#define AOSORA_ENABLE_SAORI_LOADER

//aosoraデバッガ(VSCodeに対する通信)の有効化
#define AOSORA_ENABLE_DEBUGGER

//型定義
namespace sakura {
	using number = double;
}

//特殊設定
namespace sakura {
	//パースエラー発生時にassertで止める
	extern bool DEBUG_ENABLE_ASSERT_PARSE_ERROR;
}
