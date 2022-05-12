// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "fe.h"

#ifdef __cplusplus
extern "C"
{
#endif

VOID FeAddLog(INT err, WCHAR* fmt, ...);

VOID FeClearLog(VOID);

VOID FeShowLog(VOID);

LPCWSTR FeGetConfigPath(VOID);

CHAR* FeLoadConfig(DWORD* pSize);

LPCWSTR FeKeyToStr(UINT fsModifiers, UINT vk);

UINT FeStrToKey(LPCSTR pName, UINT* pModifiers);

WCHAR* FeUtf8ToWcs(LPCSTR str);

BOOL FeExec(LPCWSTR pCmd, WORD wShowWindow, BOOL bWinLogon, BOOL bWait);

WORD FeStrToShow(LPCSTR sw);

void FeKillProcessByName(WCHAR* pName, UINT uExitCode);

void FeKillProcessById(DWORD dwProcessId, UINT uExitCode);

LONG FeSetResolution(LPCWSTR pMonitor, LPCWSTR pResolution, DWORD dwFlags);

VOID FeShowWindowByTitle(LPCWSTR pFileName, INT nCmdHide, INT nCmdShow);

BOOL FeGetScreenShot(LPCWSTR lpScreen, LPCWSTR lpSave);

#ifdef __cplusplus
}
#endif
