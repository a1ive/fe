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
AddUserSystrayMenu(HMENU hMenu, UINT uPosition, UINT uFlags)
{
	UINT_PTR id = IDM_USER_MIN;
	const cJSON* st = cJSON_GetObjectItem(mJson, "systray");
	const cJSON* hk = NULL;
	cJSON_ArrayForEach(hk, st)
	{
		WCHAR* name;
		if (id >= IDM_USER_MAX)
			break;
		name = FeUtf8ToWcs(cJSON_GetStringValue(cJSON_GetObjectItem(hk, "name")));
		if (name)
		{
			InsertMenuW(hMenu, uPosition, uFlags, id, name);
			FeAddLog(0, L"Add Menu %p %s\r\n", id, name);
			free(name);
		}
		id++;
	}
}

static INT_PTR
HandleUserSystrayId(int Id)
{
	int item;
	const cJSON* st;
	const cJSON* hk;
	if (Id < IDM_USER_MIN || Id > IDM_USER_MAX)
		return (INT_PTR)FALSE;
	st = cJSON_GetObjectItem(mJson, "systray");
	item = Id - IDM_USER_MIN;
	hk = cJSON_GetArrayItem(st, item);
	if (!hk)
		return (INT_PTR)FALSE;
	FeParseConfig(hk);
	return (INT_PTR)TRUE;
}

static INT_PTR
NotifyIconProc(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	switch (lParam)
	{
	case WM_LBUTTONDBLCLK:
	{
		if (IsWindowVisible(hWnd))
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, SW_RESTORE);
	}
	break;
	case WM_RBUTTONDOWN:
	case WM_CONTEXTMENU:
	{
		POINT pt;
		HMENU hMenu;
		GetCursorPos(&pt);
		hMenu = CreatePopupMenu();
		if (hMenu)
		{
			AddUserSystrayMenu(hMenu, (UINT)-1, MF_BYPOSITION);
			InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_RELOAD, FeIsChs()? L"刷新配置" : L"Reload");
			InsertMenuW(hMenu, (UINT)-1, MF_BYPOSITION, IDM_EXIT, FeIsChs() ? L"退出" : L"Exit");
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
		FeEditConfig(hWnd, &mJson);
		break;
	case IDM_RELOAD:
		FeReloadConfig(&mJson);
		break;
	case IDM_LISTKEY:
		FeListHotkey(hWnd);
		break;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	default:
		return HandleUserSystrayId(wmId);
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
	case WM_SYSCOMMAND:
		if (wParam == SC_CLOSE)
			ShowWindow(hWnd, SW_HIDE);
		/* fall through */
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

	mJson = FeInitializeConfig();
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
