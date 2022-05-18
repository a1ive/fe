// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"
#include <process.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <VersionHelpers.h>

VOID FeAddLog(INT lvl, LPCWSTR fmt, ...)
{
	int len;
	va_list args;
	WCHAR* str;
	HWND hEdit = GetDlgItem(gWnd, (lvl == 2) ? IDC_STATIC_JSON : IDC_STATIC_LOG);
	int index = GetWindowTextLengthW(hEdit);
	va_start(args, fmt);
	len = _vscwprintf(fmt, args);
	if (len == -1)
		return;
	str = (WCHAR*)calloc((size_t)len + 1, sizeof(WCHAR));
	if (!str)
		return;
	_vsnwprintf_s(str, (size_t)len + 1, _TRUNCATE, fmt, args);
	if (lvl == 1)
		MessageBoxW(gWnd, str, L"ERROR", MB_OK);
	SendMessageW(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index);
	SendMessageW(hEdit, EM_REPLACESEL, 0, (LPARAM)str);
	free(str);
}

VOID FeClearLog(int lvl)
{
	if (lvl == 0)
		SetDlgItemTextW(gWnd, IDC_STATIC_LOG, L"");
	SetDlgItemTextW(gWnd, IDC_STATIC_JSON, L"");
}

typedef struct _KEYSYM
{
	LPCSTR name; /* the name in unshifted state */
	UINT code;   /* scan code */
} KEYSYM;

/* The table for key symbols. (from GRUB4DOS) */
static KEYSYM keytable[] =
{
	{"Backspace",     VK_BACK},
	{"Tab",           VK_TAB},
	{"Clear",         VK_CLEAR},
	{"Enter",         VK_RETURN},
	{"Pause",         VK_PAUSE},
	{"CapsLock",      VK_CAPITAL},
	{"Escape",        VK_ESCAPE},
	{"Space",         VK_SPACE},
	{"PageUp",        VK_PRIOR},
	{"PageDown",      VK_NEXT},
	{"End",           VK_END},
	{"Home",          VK_HOME},
	{"Leftarrow",     VK_LEFT},
	{"UpArrow",       VK_UP},
	{"RightArrow",    VK_RIGHT},
	{"DownArrow",     VK_DOWN},
	{"Select",        VK_SELECT},
	{"Insert",        VK_INSERT},
	{"Delete",        VK_DELETE},
	// 0x30 - 0x39, '0' - '9'
	{"0",             0x30},
	{"1",             0x31},
	{"2",             0x32},
	{"3",             0x33},
	{"4",             0x34},
	{"5",             0x35},
	{"6",             0x36},
	{"7",             0x37},
	{"8",             0x38},
	{"9",             0x39},
	// 0x41 - 0x5a, 'A' - 'Z'
	{"A",             0x41},
	{"B",             0x42},
	{"C",             0x43},
	{"D",             0x44},
	{"E",             0x45},
	{"F",             0x46},
	{"G",             0x47},
	{"H",             0x48},
	{"I",             0x49},
	{"J",             0x4A},
	{"K",             0x4B},
	{"L",             0x4C},
	{"M",             0x4D},
	{"N",             0x4E},
	{"O",             0x4F},
	{"P",             0x50},
	{"Q",             0x51},
	{"R",             0x52},
	{"S",             0x53},
	{"T",             0x54},
	{"U",             0x55},
	{"V",             0x56},
	{"W",             0x57},
	{"X",             0x58},
	{"Y",             0x59},
	{"Z",             0x5A},
	{"Multiply",      VK_MULTIPLY},
	{"Add",           VK_ADD},
	{"Separator",     VK_SEPARATOR},
	{"Subtract",      VK_SUBTRACT},
	{"Decimal",       VK_DECIMAL},
	{"Divide",        VK_DIVIDE},
	{"F1",            VK_F1},
	{"F2",            VK_F2},
	{"F3",            VK_F3},
	{"F4",            VK_F4},
	{"F5",            VK_F5},
	{"F6",            VK_F6},
	{"F7",            VK_F7},
	{"F8",            VK_F8},
	{"F9",            VK_F9},
	{"F10",           VK_F10},
	{"F11",           VK_F11},
	// 0x7b - 0x87, F12 - F24
	{"NumLock",       VK_NUMLOCK},
	{"ScrLock",       VK_SCROLL},
	{"VolMute",       VK_VOLUME_MUTE},
	{"VolDown",       VK_VOLUME_DOWN},
	{"VolUp",         VK_VOLUME_UP},
};

LPCWSTR FeKeyToStr(UINT fsModifiers, UINT vk)
{
	static WCHAR keyname[64];
	WCHAR tmp[32];
	size_t i;
	ZeroMemory(keyname, sizeof(keyname));
	ZeroMemory(tmp, sizeof(tmp));

	for (i = 0; i < sizeof(keytable) / sizeof(keytable[0]); i++)
	{
		if (vk == keytable[i].code)
		{
			_snwprintf_s(tmp, 32, _TRUNCATE, L"%S", keytable[i].name);
			break;
		}
	}
	if (tmp[0] == 0)
		_snwprintf_s(tmp, 32, _TRUNCATE, L"0x%08x", vk);

	_snwprintf_s(keyname, 64, _TRUNCATE, L"%s%s%s%s%s",
		fsModifiers & MOD_CONTROL ? L"Ctrl-" : L"",
		fsModifiers & MOD_SHIFT ? L"Shift-" : L"",
		fsModifiers & MOD_ALT ? L"Alt-" : L"",
		fsModifiers & MOD_WIN ? L"Win-" : L"", tmp);

	return keyname;
}

UINT
FeStrToKey(LPCSTR pName, UINT* pModifiers)
{
	size_t i;
	UINT vk = 0;
	UINT fsModifiers = IsWindows7OrGreater() ? MOD_NOREPEAT : 0;
	LPCSTR p = pName;

	for (; p && *p;)
	{
		if (_strnicmp(p, "Ctrl-", 5) == 0)
		{
			p += 5;
			fsModifiers |= MOD_CONTROL;
			continue;
		}
		else if (_strnicmp(p, "Shift-", 6) == 0)
		{
			p += 6;
			fsModifiers |= MOD_SHIFT;
			continue;
		}
		else if (_strnicmp(p, "Alt-", 4) == 0)
		{
			p += 4;
			fsModifiers |= MOD_ALT;
			continue;
		}
		else if (_strnicmp(p, "Win-", 4) == 0)
		{
			p += 4;
			// Hotkeys that involve the Windows key are reserved for use by the operating system.
			fsModifiers |= MOD_WIN;
			continue;
		}
		break;
	}
	if (pModifiers)
		*pModifiers = fsModifiers;
	if (!p || !*p)
		return 0;
	for (i = 0; i < sizeof(keytable) / sizeof(keytable[0]); i++)
	{
		if (_stricmp(p, keytable[i].name) == 0)
			return keytable[i].code;
	}
	vk = strtoul(p, NULL, 0);
	return vk;
}

WCHAR* FeUtf8ToWcs(LPCSTR str)
{
	WCHAR* val = NULL;
	int sz = 0;
	if (!str)
		return NULL;
	sz = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	if (sz <= 0)
		return NULL;
	val = (WCHAR*)calloc(1ULL + sz, sizeof(WCHAR));
	if (!val)
		return NULL;
	MultiByteToWideChar(CP_UTF8, 0, str, -1, val, sz);
	return val;
}

static WCHAR cmdline_buf[32767];

BOOL FeExec(LPCWSTR pCmd, WORD wShowWindow, BOOL bWinLogon, BOOL bWait)
{
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi;
	BOOL bRet = FALSE;
	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = wShowWindow;
	si.lpDesktop = (LPWSTR)(bWinLogon ? L"WinSta0\\WinLogon" : L"WinSta0\\Default");
	cmdline_buf[0] = L'\0';
	ExpandEnvironmentStringsW(pCmd, cmdline_buf, 32767);

	bRet = CreateProcessW(NULL, cmdline_buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	if (bRet)
	{
		SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
		if (bWait)
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
		}
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}
	return bRet;
}

VOID
FeShellExec(LPCWSTR lpOperation, LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
	cmdline_buf[0] = L'\0';
	if (lpDirectory)
	{
		ExpandEnvironmentStringsW(lpDirectory, cmdline_buf, 32767);
		ShellExecuteW(NULL, lpOperation, lpFile, lpParameters, cmdline_buf, nShowCmd);
	}
	else
	{
		ExpandEnvironmentStringsW(lpFile, cmdline_buf, 32767);
		ShellExecuteW(NULL, lpOperation, cmdline_buf, lpParameters, NULL, nShowCmd);
	}
}

WORD FeStrToShow(LPCSTR sw)
{
	WORD wCmdShow = SW_NORMAL;
	if (!sw || _stricmp(sw, "normal") == 0)
		wCmdShow = SW_NORMAL;
	else if (_stricmp(sw, "hide") == 0)
		wCmdShow = SW_HIDE;
	else if (_stricmp(sw, "min") == 0)
		wCmdShow = SW_FORCEMINIMIZE;
	else if (_stricmp(sw, "max") == 0)
		wCmdShow = SW_MAXIMIZE;
	else if (_stricmp(sw, "restore") == 0)
		wCmdShow = SW_RESTORE;
	else if (_stricmp(sw, "show") == 0)
		wCmdShow = SW_SHOW;
	return wCmdShow;
}

void
FeKillProcessByName(WCHAR* pName, UINT uExitCode)
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
	PROCESSENTRY32W pEntry = { .dwSize = sizeof(pEntry) };
	BOOL bRet;
	if (hSnapShot == INVALID_HANDLE_VALUE)
		return;
	bRet = Process32FirstW(hSnapShot, &pEntry);
	while (bRet)
	{
		if (_wcsicmp(pEntry.szExeFile, pName) == 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, pEntry.th32ProcessID);
			if (hProcess != NULL)
			{
				TerminateProcess(hProcess, uExitCode);
				CloseHandle(hProcess);
			}
		}
		bRet = Process32NextW(hSnapShot, &pEntry);
	}
	CloseHandle(hSnapShot);
}

void
FeKillProcessById(DWORD dwProcessId, UINT uExitCode)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
	TerminateProcess(hProcess, uExitCode);
	CloseHandle(hProcess);
}

LONG FeSetResolution(LPCWSTR pMonitor, LPCWSTR pResolution, DWORD dwFlags)
{
	LPCWSTR pWidth, pHeight;
	DEVMODEW dMode;
	if (!pResolution)
		return DISP_CHANGE_BADPARAM;
	pWidth = pResolution;
	pHeight = wcschr(pResolution, L'x') + 1;
	if (!pHeight)
		return DISP_CHANGE_BADPARAM;
	ZeroMemory(&dMode, sizeof(dMode));
	dMode.dmSize = sizeof(dMode);
	dMode.dmPelsWidth = wcstoul(pWidth, NULL, 10);
	dMode.dmPelsHeight = wcstoul(pHeight, NULL, 10);
	dMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
	return ChangeDisplaySettingsExW(pMonitor, &dMode, NULL, dwFlags, NULL);
}

struct WINDOW_HOOK_DATA
{
	LPCWSTR FileName;
	int CmdShow;
};

static BOOL CALLBACK FeWindowIter(HWND hWnd, LPARAM lParam)
{
	struct WINDOW_HOOK_DATA *pData = (struct WINDOW_HOOK_DATA*)lParam;
	WCHAR FileName[MAX_PATH];
	GetWindowTextW(hWnd, FileName, MAX_PATH);
	if (hWnd == gWnd || !pData->FileName || FileName[0] == L'\0')
		return TRUE;
	if (StrStrIW(FileName, pData->FileName))
	{
		FeAddLog(0, L"Window %s\r\n", FileName);
		ShowWindow(hWnd, pData->CmdShow);
	}
	return TRUE;
}

VOID FeShowWindowByTitle(LPCWSTR pFileName, INT nCmdHide, INT nCmdShow)
{
	static WORD op = 0;
	struct WINDOW_HOOK_DATA whData;
	whData.FileName = pFileName;
	whData.CmdShow = op ? nCmdShow : nCmdHide;
	op = ~op;
	EnumWindows(FeWindowIter, (LPARAM)&whData);
}

BOOL FeIsChs(VOID)
{
	LANGID lang = GetUserDefaultUILanguage();
	if (lang == 2052)
		return TRUE;
	return FALSE;
}

HTREEITEM FeAddItemToTree(HTREEITEM hParent, LPCWSTR lpszItem, int nLevel, const cJSON* lpConfig)
{
	TVITEMW tvi;
	TVINSERTSTRUCTW tvins;
	static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
	HTREEITEM hti;
	HWND hwndTV = GetDlgItem(gWnd, IDC_STATIC_TREE);

	if (!hwndTV || (nLevel != 1 && !hParent) || !lpszItem)
		return NULL;
	tvi.mask = TVIF_TEXT | TVIF_PARAM;

	tvi.pszText = (LPWSTR)lpszItem;
	tvi.cchTextMax = (int)(wcslen(lpszItem) + 1);

	tvi.lParam = (LPARAM)lpConfig;
	tvins.item = tvi;
	tvins.hInsertAfter = hPrev;

	if (nLevel == 1)
		tvins.hParent = TVI_ROOT;
	else
		tvins.hParent = hParent;

	hPrev = (HTREEITEM)SendMessageW(hwndTV, TVM_INSERTITEM,
		0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

	if (hPrev == NULL)
	{
		return NULL;
	}

	if (nLevel > 1)
	{
		hti = TreeView_GetParent(hwndTV, hPrev);
		tvi.mask = 0;
		tvi.hItem = hti;
		TreeView_SetItem(hwndTV, &tvi);
	}

	return hPrev;
}

VOID FeExpandTree(HTREEITEM hTree)
{
	HWND hwndTV = GetDlgItem(gWnd, IDC_STATIC_TREE);
	if (hwndTV)
		TreeView_Expand(hwndTV, hTree, TVE_EXPAND);
}

VOID FeDeleteTree(VOID)
{
	HWND hwndTV = GetDlgItem(gWnd, IDC_STATIC_TREE);
	if (hwndTV)
		TreeView_DeleteAllItems(hwndTV);
}
