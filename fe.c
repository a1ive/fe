// SPDX-License-Identifier: GPL-3.0-or-later

#include "fe.h"

#include "utils.h"

#include <shellapi.h>

HWND gWnd;

static NOTIFYICONDATAW mNotifyIcon;
static cJSON* mJson;

static BOOL
InitializeInstance(HINSTANCE hInstance, int nCmdShow, LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
	gWnd = CreateWindowW(lpClassName, lpWindowName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
		CW_USEDEFAULT, 0, 128, 72, NULL, NULL, hInstance, NULL);

	if (!gWnd)
		return FALSE;

	ShowWindow(gWnd, nCmdShow);
	UpdateWindow(gWnd);

	ZeroMemory(&mNotifyIcon, sizeof(mNotifyIcon));
	mNotifyIcon.cbSize = sizeof(mNotifyIcon);
	mNotifyIcon.hWnd = gWnd;
	mNotifyIcon.uID = 1;
	mNotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	mNotifyIcon.uCallbackMessage = WM_APP;
	mNotifyIcon.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON));
	wcscpy_s(mNotifyIcon.szTip, 64, L"Fe");

	Shell_NotifyIconW(NIM_ADD, &mNotifyIcon);

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
	mJson = cJSON_Parse(JsonConfig);
	if (!mJson)
	{
		const char* error_ptr = cJSON_GetErrorPtr();
		FeAddLog(1, L"Invalid JSON: %S\n", error_ptr ? error_ptr : "UNKNOWN ERROR");
		return;
	}
	free(JsonConfig);
	FeAddLog(0, L"JSON Loaded.\n");
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
			WCHAR wCmd[MAX_PATH + 28];
			LPCWSTR lpConfig = FeGetConfigPath();
			swprintf(wCmd, MAX_PATH + 28, L"notepad.exe \"%s\"", lpConfig);
			bRet = FeExec(wCmd, SW_NORMAL, FALSE, TRUE);
			if (bRet == FALSE)
			{
				swprintf(wCmd, MAX_PATH + 28, L"CANNOT LOAD\n%s", lpConfig);
				MessageBoxW(hWnd, wCmd, L"ERROR", MB_OK);
			}
		}
		/* fall through */
		case IDM_RELOAD:
		{
			FeUnregisterHotkey();
			cJSON_Delete(mJson);
			FeClearLog();
			InitializeJson();
			FeInitializeHotkey(cJSON_GetObjectItem(mJson, "hotkey"));
		}
		break;
		case IDM_LISTKEY:
			FeListHotkey(hWnd);
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
		mNotifyIcon.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &mNotifyIcon);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

static ATOM
MyRegisterClass(HINSTANCE hInstance, LPCWSTR lpClassName)
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
	wcex.lpszClassName = lpClassName;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON));

	return RegisterClassExW(&wcex);
}

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	MSG msg;
	HANDLE hMutex;
	WCHAR wTitle[MAX_LOADSTRING];
	WCHAR wWindowClass[MAX_LOADSTRING];

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, wTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_FE_MENU, wWindowClass, MAX_LOADSTRING);

	mJson = 0;
	FeClearLog();

	hMutex = CreateMutexW(NULL, TRUE, L"Fe{1f0d5242-d60d-4cb3-a1f6-13990bc5dcd2}");
	if (GetLastError() == ERROR_ALREADY_EXISTS || !hMutex)
	{
		return 1;
	}

	MyRegisterClass(hInstance, wWindowClass);

	nCmdShow = SW_HIDE;
	if (!InitializeInstance (hInstance, nCmdShow, wWindowClass, wTitle))
	{
		CloseHandle(hMutex);
		return 1;
	}

	InitializeJson();
	FeInitializeHotkey(cJSON_GetObjectItem(mJson, "hotkey"));

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		FeHandleHotkey(&msg);
	}

	cJSON_Delete(mJson);
	CloseHandle(hMutex);
	return 0;
}
