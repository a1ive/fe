// SPDX-License-Identifier: GPL-3.0-or-later

#include "fe.h"

#include "utils.h"

#define MAX_HOTKEY_ID 0xBFFF

static cJSON* mHotkeyJson;
static INT mHotkeyCount;

static UINT64 mHotkeyData[MAX_HOTKEY_ID + 1];

VOID
FeUnregisterHotkey(VOID)
{
	int i;
	for (i = 1; i <= mHotkeyCount; i++)
		UnregisterHotKey(NULL, i);
	mHotkeyJson = NULL;
}

VOID
FeInitializeHotkey(cJSON* jsHotkeys)
{
	int i;
	const cJSON* hk = NULL;
	mHotkeyJson = jsHotkeys;
	if (!mHotkeyJson)
		return;
	mHotkeyCount = cJSON_GetArraySize(mHotkeyJson);
	if (mHotkeyCount <= 0)
		return;
	for (i = 0; i < mHotkeyCount && i <= MAX_HOTKEY_ID; i++)
	{
		UINT vk = 0, fsModifiers = 0;
		const cJSON* key = NULL;
		const char* keyname = NULL;
		hk = cJSON_GetArrayItem(mHotkeyJson, i);
		if (!hk)
		{
			FeAddLog(0, L"Hotkey %d not found.\r\n", i);
			continue;
		}
		key = cJSON_GetObjectItem(hk, "key");
		if (!key)
		{
			FeAddLog(0, L"Hotkey %d no key.\r\n", i);
			continue;
		}
		keyname = cJSON_GetStringValue(key);
		if (!keyname)
		{
			FeAddLog(0, L"Hotkey %d invalid key.\r\n", i);
			continue;
		}
		vk = FeStrToKey(keyname, &fsModifiers);
		if (vk == 0)
		{
			FeAddLog(0, L"Hotkey %d invalid string %S.\r\n", i, keyname);
			continue;
		}
		if (!RegisterHotKey(NULL, i, fsModifiers, vk))
		{
			mHotkeyData[i] = 0;
			FeAddLog(0, L"Register hotkey %d %s failed.\r\n", i, FeKeyToStr(fsModifiers, vk));
			continue;
		}
		mHotkeyData[i] = (((UINT64)fsModifiers) << 32U) | vk;
		FeAddLog(0, L"Register hotkey %d %s OK.\r\n", i, FeKeyToStr(fsModifiers, vk));
	}
}

VOID
FeListHotkey(HWND hWnd)
{
	int i;
	WCHAR* msgText;
	if (!mHotkeyJson)
		return;
	msgText = calloc(65536, sizeof(WCHAR));
	for (i = 0; i < mHotkeyCount; i++)
	{
		const cJSON* hk = cJSON_GetArrayItem(mHotkeyJson, i);
		WCHAR* wn;
		if (mHotkeyData[i] == 0)
			continue;
		wn = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "note")));
		swprintf(msgText, 65535, L"%s%s%s%s\n", msgText,
			FeKeyToStr(mHotkeyData[i] >> 32, mHotkeyData[i] & 0xFFFFFFFF),
			wn ? L", " : L"", wn ? wn : L"");
		if (wn)
			free(wn);
	}
	MessageBoxW(hWnd, msgText, L"Hotkeys", MB_OK);
	free(msgText);
}

VOID
FeHandleHotkey(const MSG* msg)
{
	const cJSON* hk = NULL;
	WCHAR* val = NULL;
	int id = (int)msg->wParam;
	if (msg->message != WM_HOTKEY || !mHotkeyJson)
		return;
	hk = cJSON_GetArrayItem(mHotkeyJson, id);
	if (!hk)
		return;
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
}
