// SPDX-License-Identifier: GPL-3.0-or-later

#include "fe.h"

#include "utils.h"

LPCWSTR FeGetConfigPath(VOID)
{
	static WCHAR FilePath[MAX_PATH];
	WCHAR* pExt = NULL;
	if (!GetModuleFileNameW(NULL, FilePath, MAX_PATH))
	{
		FeAddLog(0, L"GetModuleFileName failed.\r\n");
		swprintf(FilePath, MAX_PATH, L"fe.exe");
	}
	pExt = wcsrchr(FilePath, L'.');
	if (pExt)
	{
		*pExt = L'\0';
		swprintf(FilePath, MAX_PATH, L"%s.json", FilePath);
	}
	else
	{
		swprintf(FilePath, MAX_PATH, L"fe.json");
	}
	return FilePath;
}

static CHAR* FeLoadConfigFile(DWORD* pSize)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	CHAR* pConfig = NULL;
	LPCWSTR FilePath = FeGetConfigPath();

	Fp = CreateFileW(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Fp == INVALID_HANDLE_VALUE)
	{
		FeAddLog(0, L"CreateFile %s failed.\r\n", FilePath);
		return NULL;
	}
	dwSize = GetFileSize(Fp, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		FeAddLog(0, L"GetFileSize %s failed.\r\n", FilePath);
		CloseHandle(Fp);
		return NULL;
	}
	pConfig = (CHAR*)malloc(dwSize);
	if (!pConfig)
	{
		FeAddLog(0, L"Out of memory %lu.\r\n", dwSize);
		CloseHandle(Fp);
		return NULL;
	}
	if (pSize)
		*pSize = dwSize;
	bRet = ReadFile(Fp, pConfig, dwSize, &dwSize, NULL);
	CloseHandle(Fp);
	if (!bRet)
	{
		FeAddLog(0, L"File %s read error.\r\n", FilePath);
		free(pConfig);
		return NULL;
	}
	FeAddLog(0, L"Load %s, size %lu.\r\n", FilePath, dwSize);
	return pConfig;
}

cJSON*
FeInitializeConfig(VOID)
{
	cJSON* pJSON = NULL;
	CHAR* pConfigData = FeLoadConfigFile(NULL);
	if (!pConfigData)
		return NULL;
	pJSON = cJSON_Parse(pConfigData);
	if (!pJSON)
	{
		WCHAR* wErr = FeUtf8ToWcs(cJSON_GetErrorPtr());
		FeAddLog(1, L"Invalid JSON: %s\r\n", wErr ? wErr : L"UNKNOWN ERROR");
		if (wErr)
			free(wErr);
		return NULL;
	}
	free(pConfigData);
	FeAddLog(0, L"JSON Loaded.\r\n");
	return pJSON;
}

VOID FeReloadConfig(cJSON** m)
{
	cJSON* pJSON = *m;
	FeUnregisterHotkey();
	cJSON_Delete(pJSON);
	FeClearLog();
	pJSON = FeInitializeConfig();
	FeInitializeHotkey(cJSON_GetObjectItem(pJSON, "hotkey"));
	*m = pJSON;
}

VOID FeEditConfig(HWND hWnd, cJSON** m)
{
	BOOL bRet;
	WCHAR wCmd[MAX_PATH + 28];
	LPCWSTR lpConfig = FeGetConfigPath();
	swprintf(wCmd, MAX_PATH + 28, L"notepad.exe \"%s\"", lpConfig);
	bRet = FeExec(wCmd, SW_NORMAL, FALSE, TRUE);
	if (bRet == FALSE)
	{
		swprintf(wCmd, MAX_PATH + 28, L"CANNOT LOAD\n%s", lpConfig);
		MessageBoxW(hWnd, wCmd, L"ERROR", MB_OK);
	}
	FeReloadConfig(m);
}

VOID FeParseConfig(const cJSON* hk)
{
	WCHAR* val = NULL;
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "exec")));
	if (val)
	{
		FeAddLog(0, L"Exec: %s\r\n", val);
		FeExec(val, FeStrToShow(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "window"))), FALSE, FALSE);
		free(val);
		return;
	}
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "kill")));
	if (val)
	{
		FeAddLog(0, L"Kill: %s\r\n", val);
		if (_wcsnicmp(val, L"pid=", 4) == 0)
			FeKillProcessById(wcstoul(&val[4], NULL, 0), 1);
		else
			FeKillProcessByName(val, 1);
		free(val);
		return;
	}
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "resolution")));
	if (val)
	{
		WCHAR* dev = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "monitor")));
		FeAddLog(0, L"Resolution: %s\r\n", val);
		FeSetResolution(dev, val, CDS_UPDATEREGISTRY);
		if (dev)
			free(dev);
		free(val);
		return;
	}
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "find")));
	if (val)
	{
		CHAR* sh = cJSON_GetStringValue(cJSON_GetObjectItem(hk, "hide"));
		CHAR* ss = cJSON_GetStringValue(cJSON_GetObjectItem(hk, "show"));
		FeAddLog(0, L"Find: %s\r\n", val);
		FeShowWindowByTitle(val, FeStrToShow(sh ? sh : "hide"), FeStrToShow(ss ? ss : "restore"));
		free(val);
		return;
	}
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "screenshot")));
	if (val)
	{
		WCHAR* save = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "save")));
		FeAddLog(0, L"Screenshot: %s\r\n", val);
		FeGetScreenShot(val, save);
		if (save)
			free(save);
		free(val);
		return;
	}
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "shell")));
	if (val)
	{
		WCHAR* file = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "file")));
		WCHAR* param = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "parameters")));
		WCHAR* dir = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "directory")));
		FeAddLog(0, L"Shell: %s %s %s %s\r\n", val,
			file ? file : L"", param ? param : L"", dir ? dir : L"");
		FeShellExec(val, file, param, dir,
			FeStrToShow(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "window"))));
		if (file)
			free(file);
		if (param)
			free(param);
		if (dir)
			free(dir);
		free(val);
		return;
	}
}

