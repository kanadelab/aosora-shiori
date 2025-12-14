#pragma once

//プラットフォームヘッダのインクルードを処理する
//プリプロセッサ分岐をここで処理するので何も考えずにインクルードすればよい想定
#include "Base.h"

#if defined(AOSORA_REQUIRED_WIN32)
#include <windows.h>
#endif