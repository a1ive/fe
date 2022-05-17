// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "resource.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>

#define MAX_LOADSTRING 100

#define MAX_HOTKEY_ID 0xBFFF

#ifdef __cplusplus
extern "C"
{
#endif

extern HWND gWnd;

#ifdef __cplusplus
}
#endif
