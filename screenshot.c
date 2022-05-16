// SPDX-License-Identifier: GPL-3.0-or-later

#include "fe.h"
#include "utils.h"
#include "lodepng/lodepng.h"

#include <commdlg.h>

static void GetScreenXY(LPCWSTR str, int* x, int* y, int* w, int* h)
{
	RECT rec = { 0 };
	if (!str || _wcsicmp(str, L"all") == 0)
	{
		*x = GetSystemMetrics(SM_XVIRTUALSCREEN);
		*y = GetSystemMetrics(SM_YVIRTUALSCREEN);
		*w = GetSystemMetrics(SM_CXVIRTUALSCREEN) - (*x);
		*h = GetSystemMetrics(SM_CYVIRTUALSCREEN) - (*y);
		return;
	}
	else if (_wcsicmp(str, L"current") == 0)
	{
		HWND hWnd = GetForegroundWindow();
		if (!hWnd)
			goto fallback;
		if (GetWindowRect(hWnd, &rec) == FALSE)
			goto fallback;
	}
	else
		goto fallback;

	*x = rec.left;
	*y = rec.top;
	*w = rec.right - rec.left;
	*h = rec.bottom - rec.top;
	return;
fallback:
	*x = 0;
	*y = 0;
	*w = GetSystemMetrics(SM_CXSCREEN);
	*h = GetSystemMetrics(SM_CYSCREEN);
}

#pragma pack(1)
typedef struct _RGBA_PIXEL
{
	UINT8 b;
	UINT8 g;
	UINT8 r;
	UINT8 a;
} RGBA_PIXEL;
#pragma pack()

static BOOL BmpToPng(HANDLE hf, HDC hDC, HBITMAP hBitmap, UINT w, UINT h)
{
	BOOL bRet = FALSE;
	DWORD dwPng = 0;
	size_t i, szImg, szPng = 0;
	RGBA_PIXEL* raw = NULL;
	UCHAR* png = NULL;
	BITMAPINFOHEADER bmi = { 0 };
	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biWidth = w;
	bmi.biHeight = -((LONG)h);
	bmi.biCompression = BI_RGB;
	bmi.biSizeImage = 0;
	szImg = ((UINT64)w) * h;
	raw = calloc(szImg, sizeof(RGBA_PIXEL));
	if (!raw)
		return FALSE;
	GetDIBits(hDC, hBitmap, 0, h, raw, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
	for (i = 0; i < szImg; i++)
	{
		UINT8 tmp = raw[i].b;
		raw[i].b = raw[i].r;
		raw[i].r = tmp;
		raw[i].a = 0xFF;
	}
	if (lodepng_encode32(&png, &szPng, (const UINT8*)raw, w, h) != 0)
	{
		free(raw);
		return FALSE;
	}
	free(raw);
	dwPng = (DWORD)szPng;
	bRet = WriteFile(hf, png, dwPng, &dwPng, NULL);
	free(png);
	return bRet;
}

BOOL SaveScreenShot(LPCWSTR lpSave, HDC hDC, HBITMAP hBitmap, UINT w, UINT h)
{
	BOOL bRet = FALSE;
	HANDLE hf = INVALID_HANDLE_VALUE;
	WCHAR FilePath[MAX_PATH];
	SYSTEMTIME st;
	GetSystemTime(&st);
	if (!lpSave || _wcsicmp(lpSave, L"clipboard") == 0)
	{
		OpenClipboard(NULL);
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, hBitmap);
		CloseClipboard();
		return TRUE;
	}
	else if (_wcsicmp(lpSave, L"ask") == 0)
	{
		OPENFILENAMEW ofn;
		swprintf(FilePath, MAX_PATH, L"ScreenShot-%u%u%u%u%u%u.png",
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = gWnd;
		ofn.lpstrFilter = L"PNG (*.png)\0*.png\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = FilePath;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = L"png";
		bRet = GetSaveFileNameW(&ofn);
		if (bRet == FALSE)
			return bRet;
	}
	else
	{
		swprintf(FilePath, MAX_PATH, L"%s-%u%u%u%u%u%u.png", lpSave,
			st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	}

	hf = CreateFileW(FilePath, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hf || hf == INVALID_HANDLE_VALUE)
		return FALSE;
	bRet = BmpToPng(hf, hDC, hBitmap, w, h);
	CloseHandle(hf);
	return bRet;
}

BOOL FeGetScreenShot(LPCWSTR lpScreen, LPCWSTR lpSave)
{
	int x = 0, y = 0, w = 0, h = 0;
	HDC hScreen = NULL;
	HDC hDC = NULL;
	HBITMAP hBitmap = NULL;
	BOOL bRet = FALSE;

	GetScreenXY(lpScreen, &x, &y, &w, &h);
	FeAddLog(0, L"x=%d, y=%d, w=%d, h=%d\r\n", x, y, w, h);
	hScreen = GetDC(NULL);
	hDC = CreateCompatibleDC(hScreen);
	if (!hDC)
		goto out;
	hBitmap = CreateCompatibleBitmap(hScreen, w, h);
	if (!hBitmap)
		goto out;
	SelectObject(hDC, hBitmap);
	bRet = BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);
	if (bRet != TRUE)
		goto out;

	bRet = SaveScreenShot(lpSave, hDC, hBitmap, w, h);

out:
	if (hDC)
		DeleteDC(hDC);
	if (hBitmap)
		DeleteObject(hBitmap);
	ReleaseDC(NULL, hScreen);
	return bRet;
}
