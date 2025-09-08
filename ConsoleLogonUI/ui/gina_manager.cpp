#include <Windows.h>
#include "gina_manager.h"
#include "wallhost.h"
#include <thread>
#include "util/util.h"
#include "util/interop.h"

#pragma comment(lib, "Version.lib")

ginaManager* ginaManager::Get()
{
	static ginaManager manager{};
	return &manager;
}

ginaManager::ginaManager()
{
	hInstance = NULL;
	hGinaDll = NULL;
	hLargeBranding = NULL;
	hSmallBranding = NULL;
	hBar = NULL;
	ginaVersion = 0;
	initedPreLogon = FALSE;
	config = {
		FALSE,
		FALSE,
		FALSE
	};
}

void ginaManager::LoadGina()
{
	// Load DLL
	hGinaDll = LoadLibraryExW(GINA_DLL_NAME, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if (!hGinaDll)
	{
		std::thread([] {
			MessageBoxW(0, L"Failed to load msgina.dll! Please put a copy of msgina.dll from a Windows NT 4.0/2000/XP installation in system32.", L"CLH_GINA", MB_OK | MB_ICONERROR);
		}).detach();
		return;
	}
	else
	{
		DWORD dwHandle;
		HRSRC hRes = FindResourceW(hGinaDll, MAKEINTRESOURCEW(1), RT_VERSION);
		if (hRes)
		{
			HGLOBAL hGlobal = LoadResource(hGinaDll, hRes);
			if (hGlobal)
			{
				LPVOID lpData = LockResource(hGlobal);
				if (lpData)
				{
					UINT uLen = SizeofResource(hGinaDll, hRes);
					if (uLen)
					{
						VS_FIXEDFILEINFO* pFileInfo;
						UINT uFileInfoLen;
						if (VerQueryValueW(lpData, L"\\", (LPVOID*)&pFileInfo, &uFileInfoLen))
						{
							int major = HIWORD(pFileInfo->dwFileVersionMS);
							int minor = LOWORD(pFileInfo->dwFileVersionMS);
							if (major == 5 && minor == 0)
							{
								ginaVersion = GINA_VER_2K;
							}
							else if (major == 5 && (minor == 1 || minor == 2))
							{
								ginaVersion = GINA_VER_XP;
							}
							else if (major == 4)
							{
								ginaVersion = GINA_VER_NT4;
							}
							else
							{
								ginaVersion = GINA_VER_NT3;
							}
						}
					}
				}
			}
		}
		if (!ginaVersion || ginaVersion < GINA_VER_NT4)
		{
			std::thread([] {
				MessageBoxW(0, L"This version of msgina.dll is not supported in this build of CLH_GINA! Please use a msgina.dll from Windows NT 4.0, 2000, or XP.", L"CLH_GINA", MB_OK | MB_ICONERROR);
			}).detach();
			FreeLibrary(hGinaDll);
			hGinaDll = NULL;
			return;
		}
	}

	// Load branding images
	wchar_t customBrdLarge[MAX_PATH], customBrd[MAX_PATH], customBar[MAX_PATH];
	if (GetConfigString(L"CustomBrd", customBrd, MAX_PATH))
	{
		hSmallBranding = (HBITMAP)LoadImageW(NULL, customBrd, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	if (!hSmallBranding)
	{
		hSmallBranding = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(GINA_BMP_BRD_SMALL));
	}
	if (GetConfigString(L"CustomBrdLarge", customBrdLarge, MAX_PATH))
	{
		hLargeBranding = (HBITMAP)LoadImageW(NULL, customBrdLarge, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	}
	if (!hLargeBranding)
	{
		hLargeBranding = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(GINA_BMP_BRD));
	}
	if (!hLargeBranding)
	{
		hLargeBranding = hSmallBranding;
	}
	if (GetConfigString(L"CustomBar", customBar, MAX_PATH))
	{
		hBar = (HBITMAP)LoadImageW(NULL, customBar, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	if (!hBar)
	{
		hBar = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(GINA_BMP_BAR));
	}
	initedPreLogon = IsSystemUser();

	// Options related to branding images
	_crBrandBG = GetConfigInt(L"CustomBrdBG", CLR_DEFAULT);
	if (_crBrandBG == CLR_DEFAULT)
	{
		_crBrandBG = (ginaVersion == GINA_VER_XP) ? RGB(90, 124, 223) : RGB(255, 255, 255);
	}

	_fCenterBrand = GetConfigInt(L"CenterBrand", (ginaVersion == GINA_VER_XP));

	// Get branding image sizes
	BITMAP bm;
	if (hLargeBranding && GetObjectW(hLargeBranding, sizeof(bm), &bm))
	{
		_sizeLargeBrand.cx = bm.bmWidth;
		_sizeLargeBrand.cy = bm.bmHeight;
	}
	if (hSmallBranding && GetObjectW(hSmallBranding, sizeof(bm), &bm))
	{
		_sizeSmallBrand.cx = bm.bmWidth;
		_sizeSmallBrand.cy = bm.bmHeight;
	}
	if (hBar && GetObjectW(hBar, sizeof(bm), &bm))
	{
		_sizeBar.cx = bm.bmWidth;
		_sizeBar.cy = bm.bmHeight;
	}

	// Load "Built on NT Technology" text and font
	if (ginaVersion == GINA_VER_2K)
	{
		_szBuiltOnNT[0] = L'\0';
		LoadStringW(hGinaDll, GINA_STR_BUILT_ON_NT, _szBuiltOnNT, ARRAYSIZE(_szBuiltOnNT));

		int iHeight = 0;
		WCHAR szHeight[10];
		if (LoadStringW(hGinaDll, GINA_STR_BUILT_ON_NT_FONT_SIZE, szHeight, ARRAYSIZE(szHeight)))
		{
			HDC hdc = GetDC(NULL);
			iHeight = MulDiv(_wtol(szHeight), GetDeviceCaps(hdc, LOGPIXELSY), 96);
			ReleaseDC(NULL, hdc);
		}

		LOGFONTW lf = { 0 };
		lf.lfFaceName[0] = L'\0';
		LoadStringW(hGinaDll, GINA_STR_BUILT_ON_NT_FONT, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName));
		lf.lfHeight = -iHeight;
		_hfontBuiltOnNT = CreateFontIndirectW(&lf);

		lf.lfWeight = FW_BOLD;
		_hfontBuiltOnNTBold = CreateFontIndirectW(&lf);
	}

#ifdef SHOWCONSOLE
	config.showConsole = TRUE;
#else
	config.showConsole = GetConfigInt(L"ShowConsole", 0);
#endif

	int classicTheme = GetConfigInt(L"ClassicTheme", -1);
	if (classicTheme == -1)
	{
		config.classicTheme = ginaVersion != GINA_VER_XP;
	}
	else
	{
		config.classicTheme = classicTheme;
	}

	int hideStatusView = GetConfigInt(L"HideStatusView", -1);
	if (hideStatusView == -1)
	{
		config.hideStatusView = ginaVersion == GINA_VER_NT4;
	}
	else
	{
		config.hideStatusView = hideStatusView;
	}

	int hideCapsLockBalloon = GetConfigInt(L"HideCapsLockBalloon", -1);
	if (hideCapsLockBalloon == -1)
	{
		config.hideCapsLockBalloon = ginaVersion != GINA_VER_XP;
	}
	else
	{
		config.hideCapsLockBalloon = hideCapsLockBalloon;
	}

	// Last resort to show console if something goes wrong
	std::thread([] {
		int cnt = 0;
		while (true)
		{
			HWND hDlg = FindWindow(L"#32770", NULL);
			if (hDlg && (IsWindowVisible(hDlg) || ginaStatusView::Get()->isActive))
			{
				cnt = 0;
			}
			else
			{
				cnt++;
			}
			if (cnt > 3)
			{
				external::ShowConsoleUI();
				ginaManager::Get()->config.showConsole = TRUE;
				int res = MessageBoxW(NULL, L"It seems that something went wrong with CLH_GINA. Console UI has been shown to prevent lockout. Interact with the console to make the console UI to show up.\n\nIf you are not able to interact with the console, press OK to restart the logon process, or press Cancel to restart the computer.", L"CLH_GINA", MB_OKCANCEL | MB_ICONERROR);
				if (res == IDOK)
				{
					ExitProcess(0);
				}
				else
				{
					EnableShutdownPrivilege();
					ExitWindowsEx(EWX_REBOOT, 0);
				}
			}
			Sleep(2000);
		}
	}).detach();
}

void ginaManager::UnloadGina()
{
	if (hGinaDll)
	{
		FreeLibrary(hGinaDll);
	}
}

void ginaManager::MoveChildrenForBranding(HWND hwnd, BOOL fLarge)
{
	RECT rc;
	int dy = _sizeBar.cy + (fLarge ? _sizeLargeBrand.cy : _sizeSmallBrand.cy);

	for (HWND hwndSibling = GetWindow(hwnd, GW_CHILD); hwndSibling; hwndSibling = GetWindow(hwndSibling, GW_HWNDNEXT))
	{
		GetWindowRect(hwndSibling, &rc);
		MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
		OffsetRect(&rc, 0, dy);

		SetWindowPos(hwndSibling, NULL,
			rc.left, rc.top, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE);
	}

	GetWindowRect(hwnd, &rc);
	MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rc, 2);

	SetWindowPos(hwnd, NULL,
		0, 0, (rc.right - rc.left), (rc.bottom - rc.top) + dy,
		SWP_NOZORDER | SWP_NOMOVE);
}

void ginaManager::PaintBranding(HDC hdc, RECT *prc, BOOL fLarge /* = FALSE */, int iBarOffset /* = 0 */)
{
	HBITMAP hbm = fLarge ? hLargeBranding : hSmallBranding;
	PSIZE psize = fLarge ? &_sizeLargeBrand : &_sizeSmallBrand;
	HDC hdcMem = CreateCompatibleDC(hdc);

	// Paint BG color
	RECT rcFill = *prc;
	rcFill.bottom = rcFill.top + psize->cy;

	HBRUSH hbrFill = CreateSolidBrush(_crBrandBG);
	FillRect(hdc, &rcFill, hbrFill);
	DeleteObject(hbrFill);

	// Paint brand image
	int xBrand = 0;
	if (_fCenterBrand)
		xBrand = ((prc->right - prc->left) - psize->cx) / 2;

	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);
	BitBlt(
		hdc,
		xBrand, 0,
		psize->cx,
		psize->cy,
		hdcMem,
		0, 0,
		SRCCOPY
	);
	SelectObject(hdcMem, hbmOld);

	// Paint bar image
	hbmOld = (HBITMAP)SelectObject(hdcMem, hBar);
	StretchBlt(
		hdc,
		iBarOffset, psize->cy,
		prc->right - prc->left,
		_sizeBar.cy,
		hdcMem,
		0, 0,
		_sizeBar.cx,
		_sizeBar.cy,
		SRCCOPY
	);
	if (iBarOffset)
	{
		StretchBlt(
			hdc,
			iBarOffset - (prc->right - prc->left), psize->cy,
			prc->right - prc->left,
			_sizeBar.cy,
			hdcMem,
			0, 0,
			_sizeBar.cx,
			_sizeBar.cy,
			SRCCOPY
		);
	}
	SelectObject(hdcMem, hbmOld);

	// Paint "Built on NT Technology" text
	if (fLarge && _szBuiltOnNT[0] && _hfontBuiltOnNT)
	{
		int x = prc->left + xBrand + MulDiv(186, GetDeviceCaps(hdc, LOGPIXELSX), 96);
		int y = prc->top + MulDiv(68, GetDeviceCaps(hdc, LOGPIXELSY), 96);
		MoveToEx(hdc, x, y, nullptr);

		UINT uAlignOld = SetTextAlign(hdc, TA_UPDATECP);
		HFONT hfontOld = (HFONT)SelectObject(hdc, _hfontBuiltOnNT);

		LPWSTR pszText = _szBuiltOnNT;
		LPWSTR pszCurText = pszText;
		int iChunkLength = 0;
		BOOL fBold = FALSE;
		BOOL fStillDrawing = TRUE;
		do
		{
			BOOL fPaint = TRUE;
			if (!_wcsnicmp(pszText, L"<B>", 3))
			{
				fBold = TRUE;
				pszText += 3;
			}
			else if (!_wcsnicmp(pszText, L"</B>", 4))
			{
				fBold = FALSE;
				pszText += 4;
			}
			else if (*pszText)
			{
				iChunkLength++;
				pszText++;
				fPaint = FALSE;
			}
			else
			{
				fStillDrawing = FALSE;
			}

			if (fPaint)
			{
				TextOutW(hdc, 0, 0, pszCurText, iChunkLength);
				iChunkLength = 0;
				pszCurText = pszText;
				SelectObject(hdc, fBold ? _hfontBuiltOnNTBold : _hfontBuiltOnNT);
			}
		}
		while (fStillDrawing);

		SelectObject(hdc, hfontOld);
		SetTextAlign(hdc, uAlignOld);
	}

	DeleteDC(hdcMem);
}

void ginaManager::CloseAllDialogs()
{
	ginaSelectedCredentialView::Get()->Destroy();
	ginaChangePwdView::Get()->Destroy();
	ginaSecurityControl::Get()->Destroy();
	ginaStatusView::Get()->Destroy();
	ginaUserSelect::Get()->Destroy();
}

void ginaManager::PostThemeChange()
{
	HWND hWnds[] = {
		wallHost::Get()->hWnd,
		ginaSelectedCredentialView::Get()->hDlg,
		ginaSelectedCredentialViewLocked::Get()->hDlg,
		ginaChangePwdView::Get()->hDlg,
		ginaSecurityControl::Get()->hDlg,
		ginaStatusView::Get()->hDlg,
		ginaUserSelect::Get()->hDlg
	};
	for (int i = 0; i < sizeof(hWnds) / sizeof(HWND); i++)
	{
		PostMessage(hWnds[i], WM_THEMECHANGED, 0, 0);
	}
}

int GetRes(int nt4, int xp)
{
	if (xp == -1 || ginaManager::Get()->ginaVersion == GINA_VER_NT4)
	{
		return nt4;
	}
	else
	{
		return xp;
	}
}

int CALLBACK HelpDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		CenterWindow(hWnd);
		if (ginaManager::Get()->config.classicTheme)
		{
			MakeWindowClassic(hWnd);
		}
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			EndDialog(hWnd, 0);
		}
		break;
	}
	case WM_CLOSE:
	{
		EndDialog(hWnd, 0);
		break;
	}
	}
	return 0;
}