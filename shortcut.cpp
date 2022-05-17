// SPDX-License-Identifier: GPL-3.0-or-later

#include "fe.h"

#include "utils.h"

#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>

HRESULT
FeCreateShortcut(LPCWSTR pTarget, LPCWSTR pLnkPath, LPCWSTR pParam, LPCWSTR pIcon, INT id, INT sw)
{
	WCHAR wExpTarget[MAX_PATH], wExpLnkPath[MAX_PATH], wExpIcon[MAX_PATH];
	IShellLink* pLink;
	HRESULT hResult;
	ExpandEnvironmentStringsW(pTarget, wExpTarget, MAX_PATH);
	if (pIcon)
		ExpandEnvironmentStringsW(pIcon, wExpIcon, MAX_PATH);
	ExpandEnvironmentStringsW(pLnkPath, wExpLnkPath, MAX_PATH);
	wcscat_s(wExpLnkPath, MAX_PATH, L".lnk");
	hResult = CoInitialize(NULL);
	hResult = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (PVOID*)&pLink);
	if (SUCCEEDED(hResult))
	{
		IPersistFile* pFile;
		hResult = pLink->SetShowCmd(sw);
		hResult = pLink->SetPath(wExpTarget);
		if (pParam)
			hResult = pLink->SetArguments(pParam);
		if (pIcon)
			hResult = pLink->SetIconLocation(wExpIcon, id);
		hResult = pLink->QueryInterface(IID_IPersistFile, (PVOID*)&pFile);
		if (SUCCEEDED(hResult))
		{
			hResult = pFile->Save(wExpLnkPath, FALSE);
			pFile->Release();
		}
		pLink->Release();
	}
	CoUninitialize();
	return hResult;
}
