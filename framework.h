// header.h : 標準のシステム インクルード ファイルのインクルード ファイル、
// またはプロジェクト専用のインクルード ファイル
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーからほとんど使用されていない部分を除外する
// Windows ヘッダー ファイル
#include <windows.h>
// C ランタイム ヘッダー ファイル
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>

//Windows API
#include <shellapi.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <fileapi.h>

//外部ライブラリ
#include <libheif/heif.h>
#include <turbojpeg.h>

//C++標準ライブラリ
#include <vector>


#include "encoder_jpeg.h"