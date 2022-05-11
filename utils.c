// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"
#include <process.h>
#include <tlhelp32.h>
#include <shlwapi.h>

static int dbg_offset = 0;

VOID DBGF(WCHAR* fmt, ...)
{
	int len;
	va_list args;
	va_start(args, fmt);
	if (dbg_offset < 0 || dbg_offset >= MAX_STATIC_BUFSZ - 100)
		dbg_offset = 0;
	len = _vsnwprintf_s(gStaticBuf + dbg_offset, MAX_STATIC_BUFSZ - dbg_offset - 1, _TRUNCATE, fmt, args);
	dbg_offset += len;
	SetWindowTextW(gStaticWnd, gStaticBuf);
}

VOID ClearLog(VOID)
{
	dbg_offset = 0;
	gStaticBuf[0] = L'\0';
	SetWindowTextW(gStaticWnd, gStaticBuf);
}

LPCWSTR GetConfigPath(VOID)
{
	static WCHAR FilePath[MAX_PATH];
	WCHAR* pExt = NULL;
	if (!GetModuleFileNameW(NULL, FilePath, MAX_PATH))
	{
		DBGF(L"GetModuleFileName failed.\n");
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

CHAR* LoadConfig(DWORD* pSize)
{
	HANDLE Fp = INVALID_HANDLE_VALUE;
	DWORD dwSize = 0;
	BOOL bRet = TRUE;
	CHAR* pConfig = NULL;
	LPCWSTR FilePath = GetConfigPath();
	
	Fp = CreateFileW(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (Fp == INVALID_HANDLE_VALUE)
	{
		DBGF(L"CreateFile %s failed.\n", FilePath);
		return NULL;
	}
	dwSize = GetFileSize(Fp, NULL);
	if (dwSize == INVALID_FILE_SIZE || dwSize == 0)
	{
		DBGF(L"GetFileSize %s failed.\n", FilePath);
		CloseHandle(Fp);
		return NULL;
	}
	pConfig = (CHAR*)malloc(dwSize);
	if (!pConfig)
	{
		DBGF(L"Out of memory %lu.\n", dwSize);
		CloseHandle(Fp);
		return NULL;
	}
	if (pSize)
		*pSize = dwSize;
	bRet = ReadFile(Fp, pConfig, dwSize, &dwSize, NULL);
	CloseHandle(Fp);
	if (!bRet)
	{
		DBGF(L"File %s read error.\n", FilePath);
		free(pConfig);
		return NULL;
	}
	DBGF(L"Load %s, size %lu.\n", FilePath, dwSize);
	return pConfig;
}

typedef struct _KEYSYM
{
	LPCSTR name; /* the name in unshifted state */
	UINT code;   /* scan code */
} KEYSYM;

/* The table for key symbols. (from GRUB4DOS) */
static KEYSYM keytable[] =
{
	{"backspace",     VK_BACK},
	{"tab",           VK_TAB},
	{"enter",         VK_RETURN},
	{"escape",        VK_ESCAPE},
	{"space",         VK_SPACE},
	{"pageup",        VK_PRIOR},
	{"pagedown",      VK_NEXT},
	{"end",           VK_END},
	{"home",          VK_HOME},
	{"leftarrow",     VK_LEFT},
	{"uparrow",	      VK_UP},
	{"rightarrow",    VK_RIGHT},
	{"downarrow",     VK_DOWN},
	{"select",        VK_SELECT},
	{"insert",        VK_INSERT},
	{"delete",        VK_DELETE},
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
	{"a",             0x41},
	{"b",             0x42},
	{"c",             0x43},
	{"d",             0x44},
	{"e",             0x45},
	{"f",             0x46},
	{"g",             0x47},
	{"h",             0x48},
	{"i",             0x49},
	{"j",             0x4A},
	{"k",             0x4B},
	{"l",             0x4C},
	{"m",             0x4D},
	{"n",             0x4E},
	{"o",             0x4F},
	{"p",             0x50},
	{"q",             0x51},
	{"r",             0x52},
	{"s",             0x53},
	{"t",             0x54},
	{"u",             0x55},
	{"v",             0x56},
	{"w",             0x57},
	{"x",             0x58},
	{"y",             0x59},
	{"z",             0x5A},
	{"f1",            VK_F1},
	{"f2",            VK_F2},
	{"f3",            VK_F3},
	{"f4",            VK_F4},
	{"f5",            VK_F5},
	{"f6",            VK_F6},
	{"f7",            VK_F7},
	{"f8",            VK_F8},
	{"f9",            VK_F9},
	{"f10",           VK_F10},
	{"f11",           VK_F11},
	//{"f12",           VK_F12},
};

LPCWSTR KeyToStr(UINT fsModifiers, UINT vk)
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
		fsModifiers & MOD_CONTROL ? L"ctrl-" : L"",
		fsModifiers & MOD_SHIFT ? L"shift-" : L"",
		fsModifiers & MOD_ALT ? L"alt-" : L"",
		fsModifiers & MOD_WIN ? L"win-" : L"", tmp);

	return keyname;
}

UINT
StrToKey(LPCSTR pName, UINT* pModifiers)
{
	size_t i;
	UINT vk = 0;
	UINT fsModifiers = MOD_NOREPEAT;
	LPCSTR p = pName;

	for (; p && *p;)
	{
		if (_strnicmp(p, "ctrl-", 5) == 0)
		{
			p += 5;
			fsModifiers |= MOD_CONTROL;
			continue;
		}
		else if (_strnicmp(p, "shift-", 6) == 0)
		{
			p += 6;
			fsModifiers |= MOD_SHIFT;
			continue;
		}
		else if (_strnicmp(p, "alt-", 4) == 0)
		{
			p += 4;
			fsModifiers |= MOD_ALT;
			continue;
		}
		else if (_strnicmp(p, "win-", 4) == 0)
		{
			p += 4;
			// Hotkeys that involve the Windows key are reserved for use by the operating system.
			// fsModifiers |= MOD_WIN;
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

WCHAR* Utf8ToWcs(LPCSTR str)
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

BOOL ExecProgram(LPCWSTR pCmd, WORD wShowWindow, BOOL bWinLogon, BOOL bWait)
{
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi;
	BOOL bRet = FALSE;
	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = wShowWindow;
	si.lpDesktop = bWinLogon ? L"WinSta0\\WinLogon" : L"WinSta0\\Default";
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

WORD StrToShow(LPCSTR sw)
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
KillProcessByName(WCHAR* pName, UINT uExitCode)
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
KillProcessById(DWORD dwProcessId, UINT uExitCode)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
	TerminateProcess(hProcess, uExitCode);
	CloseHandle(hProcess);
}

LONG SetResolution(LPCWSTR pMonitor, LPCWSTR pResolution, DWORD dwFlags)
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
		DBGF(L"Window %s\n", FileName);
		ShowWindow(hWnd, pData->CmdShow);
	}
	return TRUE;
}

VOID ShowWindowByTitle(LPCWSTR pFileName, INT nCmdHide, INT nCmdShow)
{
	static WORD op = 0;
	struct WINDOW_HOOK_DATA whData;
	whData.FileName = pFileName;
	whData.CmdShow = op ? nCmdShow : nCmdHide;
	op = ~op;
	EnumWindows(FeWindowIter, (LPARAM)&whData);
}
