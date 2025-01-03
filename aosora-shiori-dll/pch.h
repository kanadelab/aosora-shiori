// pch.h: プリコンパイル済みヘッダー ファイルです。
// 次のファイルは、その後のビルドのビルド パフォーマンスを向上させるため 1 回だけコンパイルされます。
// コード補完や多くのコード参照機能などの IntelliSense パフォーマンスにも影響します。
// ただし、ここに一覧表示されているファイルは、ビルド間でいずれかが更新されると、すべてが再コンパイルされます。
// 頻繁に更新するファイルをここに追加しないでください。追加すると、パフォーマンス上の利点がなくなります。

#ifndef PCH_H
#define PCH_H

#if defined(WIN32) || defined(_WIN32)
//#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーからほとんど使用されていない部分を除外する

// Windows ヘッダー ファイル
#include <windows.h>

#else

#include <cstring>
#include <memory>
#include <unordered_map>

#endif // WIN32 or _WIN32

// プリコンパイルするヘッダーをここに追加します
#include "Shiori.h"
#include "Misc/Utility.h"



#endif //PCH_H
