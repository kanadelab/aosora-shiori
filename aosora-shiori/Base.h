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

//MD5をデバッガに送る
//デバッグ時のブレークポイントの有効性チェックとかに使用されるかと思っていたらそうでもなくて効果のほどが不明なのと
//あたらしめのwindowsでしか動かない疑惑があるのでオフにするためのスイッチ
//#define AOSORA_DEBUGGER_ENABLE_MD5

//型定義
namespace sakura {
	//必要な場合の制度変更用に
	using number = double;
}

//特殊設定
namespace sakura {
	//パースエラー発生時にassertで止める
	extern bool DEBUG_ENABLE_ASSERT_PARSE_ERROR;
}
