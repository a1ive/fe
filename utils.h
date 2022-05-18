// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "fe.h"
#include "cJSON/cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif

VOID FeAddLog(INT lvl, LPCWSTR fmt, ...);

VOID FeClearLog(INT lvl);

LPCWSTR FeGetConfigPath(VOID);

cJSON* FeInitializeConfig(VOID);

VOID FeReloadConfig(cJSON** m);

VOID FeEditConfig(HWND hWnd, cJSON** m);

VOID FeParseConfig(const cJSON* hk);

LPCWSTR FeKeyToStr(UINT fsModifiers, UINT vk);

UINT FeStrToKey(LPCSTR pName, UINT* pModifiers);

WCHAR* FeUtf8ToWcs(LPCSTR str);

BOOL FeExec(LPCWSTR pCmd, WORD wShowWindow, BOOL bWinLogon, BOOL bWait);

VOID FeShellExec(LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);

WORD FeStrToShow(LPCSTR sw);

void FeKillProcessByName(WCHAR* pName, UINT uExitCode);

void FeKillProcessById(DWORD dwProcessId, UINT uExitCode);

LONG FeSetResolution(LPCWSTR pMonitor, LPCWSTR pResolution, DWORD dwFlags);

VOID FeShowWindowByTitle(LPCWSTR pFileName, INT nCmdHide, INT nCmdShow);

BOOL FeGetScreenShot(LPCWSTR lpScreen, LPCWSTR lpSave);

VOID FeUnregisterHotkey(VOID);

VOID FeInitializeHotkey(cJSON* jsHotkeys);

VOID FeListHotkey(HWND hWnd);

VOID FeHandleHotkey(const MSG* msg);

BOOL FeIsChs(VOID);

HTREEITEM FeAddItemToTree(HTREEITEM hParent, LPCWSTR lpszItem, int nLevel, const cJSON* lpConfig);

VOID FeExpandTree(HTREEITEM hTree);

VOID FeDeleteTree(VOID);

HRESULT FeCreateShortcut(LPCWSTR pTarget, LPCWSTR pLnkPath, LPCWSTR pParam, LPCWSTR pIcon, INT id, INT sw);

#ifdef __cplusplus
}
#endif
