#pragma once

//ベースのヘッダ
//プラットフォーム別で各種の機能のスイッチングなど。

#if defined(WIN32) || defined(_WIN32)
#define AOSORA_REQUIRED_WIN32
#endif

#define AOSORA_ENABLE_SAORI_LOADER
