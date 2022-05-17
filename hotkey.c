// SPDX-License-Identifier: GPL-3.0-or-later

#include "fe.h"

#include "utils.h"

static cJSON* mHotkeyJson;
static INT mHotkeyCount;

static UINT64 mHotkeyData[MAX_HOTKEY_ID + 1];

VOID
FeUnregisterHotkey(VOID)
{
	int i;
	for (i = 0; i <= mHotkeyCount; i++)
		UnregisterHotKey(NULL, i);
	mHotkeyJson = NULL;
}

VOID
FeInitializeHotkey(cJSON* jsHotkeys)
{
	int i;
	const cJSON* hk = NULL;
	const WCHAR* wkey;
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
		wkey = FeKeyToStr(fsModifiers, vk);
		if (!RegisterHotKey(NULL, i, fsModifiers, vk))
		{
			mHotkeyData[i] = 0;
			FeAddLog(0, L"Register hotkey %d %s failed.\r\n", i, wkey);
			continue;
		}
		mHotkeyData[i] = (((UINT64)fsModifiers) << 32U) | vk;
		FeAddLog(0, L"Register hotkey %d %s OK.\r\n", i, wkey);
	}
}

VOID
FeListHotkey(HWND hWnd)
{
	int i;
	FeClearLog();
	ShowWindow(hWnd, SW_RESTORE);
	FeAddLog(0, L"Hotkeys:\r\n");
	for (i = 0; i < mHotkeyCount; i++)
	{
		const cJSON* hk = cJSON_GetArrayItem(mHotkeyJson, i);
		WCHAR* wn;
		if (mHotkeyData[i] == 0)
			continue;
		wn = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "note")));
		FeAddLog(0, L"%s%s%s\r\n",
			FeKeyToStr(mHotkeyData[i] >> 32, mHotkeyData[i] & 0xFFFFFFFF),
			wn ? L", " : L"", wn ? wn : L"");
		if (wn)
			free(wn);
	}
	FeAddLog(0, L"--------------------------------\r\n");
}

VOID
FeHandleHotkey(const MSG* msg)
{
	const cJSON* hk = NULL;
	int id = (int)msg->wParam;
	if (msg->message != WM_HOTKEY || !mHotkeyJson)
		return;
	hk = cJSON_GetArrayItem(mHotkeyJson, id);
	if (!hk)
		return;
	FeParseConfig(hk);
}
