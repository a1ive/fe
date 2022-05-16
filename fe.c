// SPDX-License-Identifier: GPL-3.0-or-later

#include "fe.h"

#include "utils.h"

#include <shellapi.h>

HWND gWnd;

static NOTIFYICONDATAW mNotifyIcon;
static cJSON* mJson;

static BOOL
InitializeInstance(HINSTANCE hInstance, int nCmdShow, DLGPROC lpDialogFunc)
{
	gWnd = CreateDialogParamW(hInstance,
		MAKEINTRESOURCEW(IDD_MAIN_DIALOG), NULL, lpDialogFunc, 0);
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
InitializeJson(VOID)
{
	CHAR* JsonConfig = FeLoadConfig(NULL);
	if (!JsonConfig)
		return;
	mJson = cJSON_Parse(JsonConfig);
	if (!mJson)
	{
		const char* error_ptr = cJSON_GetErrorPtr();
		FeAddLog(1, L"Invalid JSON: %S\r\n", error_ptr ? error_ptr : "UNKNOWN ERROR");
		return;
	}
	free(JsonConfig);
	FeAddLog(0, L"JSON Loaded.\r\n");
}

static INT_PTR
NotifyIconProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	switch (lParam)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_CONTEXTMENU:
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
			InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_LISTKEY, L"Hotkeys");
			InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_EXIT, L"Exit");
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
			DestroyMenu(hMenu);
		}
	}
	break;
	default:
		return (INT_PTR)FALSE;
	}
	return (INT_PTR)TRUE;
}

static INT_PTR CALLBACK
AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

static INT_PTR
MainMenuProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	UNREFERENCED_PARAMETER(lParam);
	switch (wmId)
	{
	case IDM_HOMEPAGE:
		ShellExecuteW(NULL, L"open", L"https://github.com/a1ive/fe", NULL, NULL, SW_SHOW);
		break;
	case IDM_ABOUT:
		DialogBoxParamW((HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE),
			MAKEINTRESOURCEW(IDD_ABOUT_DIALOG), hWnd, AboutDlgProc, 0);
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
		return (INT_PTR)FALSE;
	}
	return (INT_PTR)TRUE;
}

static INT_PTR CALLBACK
MainDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_APP:
		return NotifyIconProc(hWnd, wParam, lParam);
	case WM_COMMAND:
		return MainMenuProc(hWnd, wParam, lParam);
	case WM_DESTROY:
		mNotifyIcon.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &mNotifyIcon);
		PostQuitMessage(0);
		break;
	default:
		return (INT_PTR)FALSE;
	}
	return (INT_PTR) TRUE;
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

	nCmdShow = SW_HIDE;
	if (!InitializeInstance (hInstance, nCmdShow, MainDlgProc))
	{
		CloseHandle(hMutex);
		return 1;
	}

	InitializeJson();
	FeInitializeHotkey(cJSON_GetObjectItem(mJson, "hotkey"));

	while (GetMessage(&msg, NULL, 0, 0))
	{
		FeHandleHotkey(&msg);
		if (!IsDialogMessageW(gWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	cJSON_Delete(mJson);
	CloseHandle(hMutex);
	return 0;
}
