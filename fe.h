// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "resource.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include "cJSON/cJSON.h"

#define MAX_LOADSTRING 100
#define MAX_STATIC_BUFSZ 65535ULL

#ifdef __cplusplus
extern "C"
{
#endif

extern HINSTANCE gInst;
extern WCHAR gTitle[MAX_LOADSTRING];
extern WCHAR gWindowClass[MAX_LOADSTRING];
extern WCHAR gStaticBuf[MAX_STATIC_BUFSZ];
extern HWND gWnd;
extern HWND gStaticWnd;
extern INT gHotkeyCount;
extern cJSON* gJson;

#ifdef __cplusplus
}
#endif
