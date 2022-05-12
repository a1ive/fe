// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "fe.h"

#ifdef __cplusplus
extern "C"
{
#endif

VOID DBGF(WCHAR* fmt, ...);

VOID ClearLog(VOID);

LPCWSTR GetConfigPath(VOID);

CHAR* LoadConfig(DWORD* pSize);

LPCWSTR KeyToStr(UINT fsModifiers, UINT vk);

UINT StrToKey(LPCSTR pName, UINT* pModifiers);

WCHAR* Utf8ToWcs(LPCSTR str);

BOOL ExecProgram(LPCWSTR pCmd, WORD wShowWindow, BOOL bWinLogon, BOOL bWait);

WORD StrToShow(LPCSTR sw);

void KillProcessByName(WCHAR* pName, UINT uExitCode);

void KillProcessById(DWORD dwProcessId, UINT uExitCode);

LONG SetResolution(LPCWSTR pMonitor, LPCWSTR pResolution, DWORD dwFlags);

VOID ShowWindowByTitle(LPCWSTR pFileName, INT nCmdHide, INT nCmdShow);

BOOL GetScreenShot(LPCWSTR lpScreen, LPCWSTR lpSave);

#ifdef __cplusplus
}
#endif
