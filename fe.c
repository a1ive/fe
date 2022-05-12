// SPDX-License-Identifier: GPL-3.0-or-later

#include "fe.h"

#include "cJSON/cJSON.h"
#include "utils.h"

#include <shellapi.h>

HINSTANCE gInst;
WCHAR gTitle[MAX_LOADSTRING];
WCHAR gWindowClass[MAX_LOADSTRING];
HWND gWnd;
NOTIFYICONDATAW gNotifyIcon;
INT gHotkeyCount;
cJSON* gJson;

static BOOL
InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	gInst = hInstance;

	gWnd = CreateWindowW(gWindowClass, gTitle,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
		CW_USEDEFAULT, 0, 128, 72, NULL, NULL, hInstance, NULL);

	if (!gWnd)
		return FALSE;

	ShowWindow(gWnd, nCmdShow);
	UpdateWindow(gWnd);

	ZeroMemory(&gNotifyIcon, sizeof(gNotifyIcon));
	gNotifyIcon.cbSize = sizeof(gNotifyIcon);
	gNotifyIcon.hWnd = gWnd;
	gNotifyIcon.uID = 1;
	gNotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	gNotifyIcon.uCallbackMessage = WM_APP;
	gNotifyIcon.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wcscpy_s(gNotifyIcon.szTip, 64, L"Fe");

	Shell_NotifyIconW(NIM_ADD, &gNotifyIcon);

	return TRUE;
}

static VOID
ShowContextMenu(HWND hWnd)
{
	POINT pt;
	HMENU hMenu;
	GetCursorPos(&pt);
	hMenu = CreatePopupMenu();
	if (hMenu)
	{
		if (IsWindowVisible(hWnd))
			InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_HIDE, L"Hide");
		else
			InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_SHOW, L"Show");
		InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_RELOAD, L"Reload");
		InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_EDIT, L"Edit");
		InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_LISTKEY, L"List");
		InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_EXIT, L"Exit");
		SetForegroundWindow(hWnd);
		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
		DestroyMenu(hMenu);
	}
}

static VOID
InitializeJson(VOID)
{
	CHAR* JsonConfig = FeLoadConfig(NULL);
	if (!JsonConfig)
		return;
	gJson = cJSON_Parse(JsonConfig);
	if (!gJson)
	{
		const char* error_ptr = cJSON_GetErrorPtr();
		FeAddLog(1, L"Invalid JSON: %S\n", error_ptr ? error_ptr : "UNKNOWN ERROR");
		return;
	}
	free(JsonConfig);
	FeAddLog(0, L"JSON Loaded.\n");
}

static VOID
InitializeHotkey(cJSON* jsHotkeys)
{
	int i;
	const cJSON* hk = NULL;
	if (!jsHotkeys)
		return;
	gHotkeyCount = cJSON_GetArraySize(jsHotkeys);
	if (gHotkeyCount <= 0)
		return;
	for (i = 0; i < gHotkeyCount; i++)
	{
		UINT vk = 0, fsModifiers = 0;
		const cJSON* key = NULL;
		const char* keyname = NULL;
		hk = cJSON_GetArrayItem(jsHotkeys, i);
		if (!hk)
		{
			FeAddLog(0, L"Hotkey %d not found.\n", i);
			continue;
		}
		key = cJSON_GetObjectItem(hk, "key");
		if (!key)
		{
			FeAddLog(0, L"Hotkey %d no key.\n", i);
			continue;
		}
		keyname = cJSON_GetStringValue(key);
		if (!keyname)
		{
			FeAddLog(0, L"Hotkey %d invalid key.\n", i);
			continue;
		}
		vk = FeStrToKey(keyname, &fsModifiers);
		if (vk == 0)
		{
			FeAddLog(0, L"Hotkey %d invalid string %S.\n", i, keyname);
			continue;
		}
		if (!RegisterHotKey(NULL, i + 1, fsModifiers, vk))
		{
			FeAddLog(0, L"Register hotkey %d %s failed.\n", i, FeKeyToStr(fsModifiers, vk));
			continue;
		}
		FeAddLog(0, L"Register hotkey %d %s OK.\n", i, FeKeyToStr(fsModifiers, vk));
	}
}

static VOID
ListHotkey(HWND hWnd)
{
	int i;
	WCHAR* msgText = calloc(65536, sizeof(WCHAR));
	const cJSON* hks = cJSON_GetObjectItem(gJson, "hotkey");
	if (!msgText || !hks)
		return;
	for (i = 0; i < gHotkeyCount; i++)
	{
		const cJSON* hk = cJSON_GetArrayItem(hks, i);
		WCHAR* wk;
		WCHAR* wn;
		wk = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "key")));
		if (!wk)
			continue;
		wn = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "note")));
		swprintf(msgText, 65535, L"%s%s%s%s\n", msgText,
			wk, wn ? L", " : L"", wn ? wn : L"");
		if (wn)
			free(wn);
		free(wk);
	}
	MessageBoxW(hWnd, msgText, L"Hotkeys", MB_OK);
	free(msgText);
}

static LRESULT CALLBACK
WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	break;
	case WM_APP:
	{
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_LOG:
			FeShowLog();
			break;
		case IDM_SHOW:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case IDM_HIDE:
			ShowWindow(hWnd, SW_HIDE);
			break;
		case IDM_EDIT:
		{
			BOOL bRet;
			WCHAR wCmd[MAX_PATH + 12];
			swprintf(wCmd, MAX_PATH + 12, L"notepad.exe \"%s\"", FeGetConfigPath());
			bRet = FeExec(wCmd, SW_NORMAL, FALSE, TRUE);
			if (bRet == FALSE)
				MessageBoxW(hWnd, L"CANNOT LOAD NOTEPAD.EXE!", L"ERROR", MB_OK);
		}
		/* fall through */
		case IDM_RELOAD:
		{
			int i;
			for (i = 1; i <= gHotkeyCount; i++)
				UnregisterHotKey(NULL, i);
			cJSON_Delete(gJson);
			FeClearLog();
			InitializeJson();
			InitializeHotkey(cJSON_GetObjectItem(gJson, "hotkey"));
		}
		break;
		case IDM_LISTKEY:
			ListHotkey(hWnd);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		gNotifyIcon.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &gNotifyIcon);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

static ATOM
MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_FE_MENU);
	wcex.lpszClassName = gWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON));

	return RegisterClassExW(&wcex);
}

static VOID
HandleHotkey(const MSG* msg)
{
	const cJSON* hks = NULL;
	const cJSON* hk = NULL;
	WCHAR* val = NULL;
	int id = (int)msg->wParam - 1;
	if (msg->message != WM_HOTKEY)
		return;
	hks = cJSON_GetObjectItem(gJson, "hotkey");
	if (!hks)
		return;
	hk = cJSON_GetArrayItem(hks, id);
	if (!hk)
		return;
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "exec")));
	if (val)
	{
		FeAddLog(0, L"Exec: %s\n", val);
		FeExec(val, FeStrToShow(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "window"))), FALSE, FALSE);
		free(val);
		return;
	}
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "kill")));
	if (val)
	{
		FeAddLog(0, L"Kill: %s\n", val);
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
		FeAddLog(0, L"Resolution: %s\n", val);
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
		FeAddLog(0, L"Find: %s\n", val);
		FeShowWindowByTitle(val, FeStrToShow(sh ? sh : "hide"), FeStrToShow(ss ? ss : "restore"));
		free(val);
		return;
	}
	val = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "screenshot")));
	if (val)
	{
		WCHAR* save = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "save")));
		FeAddLog(0, L"Screenshot: %s\n", val);
		FeGetScreenShot(val, save);
		if (save)
			free(save);
		free(val);
		return;
	}
}

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	MSG msg;
	HANDLE hMutex;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	gJson = 0;
	FeClearLog();

	LoadStringW(hInstance, IDS_APP_TITLE, gTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_FE_MENU, gWindowClass, MAX_LOADSTRING);

	hMutex = CreateMutexW(NULL, TRUE, L"Fe{1f0d5242-d60d-4cb3-a1f6-13990bc5dcd2}");
	if (GetLastError() == ERROR_ALREADY_EXISTS || !hMutex)
	{
		return 1;
	}

	MyRegisterClass(hInstance);

	nCmdShow = SW_HIDE;
	if (!InitInstance (hInstance, nCmdShow))
	{
		CloseHandle(hMutex);
		return 1;
	}

	InitializeJson();
	InitializeHotkey(cJSON_GetObjectItem(gJson, "hotkey"));

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		HandleHotkey(&msg);
	}

	cJSON_Delete(gJson);
	CloseHandle(hMutex);
	return 0;
}
