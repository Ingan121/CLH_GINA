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
	largeBrandingHeight = GINA_LARGE_BRD_HEIGHT;
	smallBrandingHeight = GINA_SMALL_BRD_HEIGHT;
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

void ginaManager::LoadBranding(HWND hDlg, BOOL isLarge, BOOL createTwoBars)
{
	if (!hLargeBranding || !hSmallBranding || !hBar)
	{
		return;
	}

    RECT rect, clientRect;
    GetWindowRect(hDlg, &rect);
	GetClientRect(hDlg, &clientRect);
    int dlgWidth = rect.right - rect.left;
    int dlgHeight = rect.bottom - rect.top;
	int dlgClientWidth = clientRect.right - clientRect.left;
	HBITMAP hBranding = isLarge ? hLargeBranding : hSmallBranding;
	BITMAP bmp = { 0 };
    GetObjectW(hBranding, sizeof(BITMAP), &bmp);
    int brdWidth = bmp.bmWidth;
	int brdHeight = bmp.bmHeight;
	if (isLarge)
	{
		largeBrandingHeight = brdHeight;
	}
	else
	{
		smallBrandingHeight = brdHeight;
	}
	int brdX = 0;
	if (dlgClientWidth < brdWidth)
	{
		dlgWidth = dlgWidth - dlgClientWidth + brdWidth;
	}
	else if (ginaManager::Get()->ginaVersion == GINA_VER_XP)
	{
		brdX = (dlgWidth - brdWidth) / 2;
	}
    int topAreaHeight = brdHeight + GINA_BAR_HEIGHT;

    // Move all existing controls down
    HWND hChild = GetWindow(hDlg, GW_CHILD);
    while (hChild != NULL)
    {
        RECT childRect;
        GetWindowRect(hChild, &childRect);
        MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&childRect, 2);
        SetWindowPos(hChild, NULL, childRect.left, childRect.top + topAreaHeight, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }

    // Create branding and bar controls
    HWND hBrandingWnd = CreateWindowExW(0, L"STATIC", L"Branding", WS_CHILD | WS_VISIBLE | SS_BITMAP, brdX, 0, brdWidth, brdHeight, hDlg, NULL, hInstance, NULL);
    SendMessageW(hBrandingWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBranding);
    HWND hBarWnd = CreateWindowExW(0, L"STATIC", L"Bar", WS_CHILD | WS_VISIBLE | SS_BITMAP, 0, brdHeight, dlgWidth, GINA_BAR_HEIGHT, hDlg, NULL, hInstance, NULL);
    SendMessageW(hBarWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBar);
	    
    // Resize the bar to stretch to dlgWidth
    SetWindowPos(hBarWnd, NULL, 0, brdHeight, dlgWidth, GINA_BAR_HEIGHT, SWP_NOZORDER);

	if (createTwoBars)
	{
		// Create a second bar control, initially hidden in the left side of the first bar
		HWND hBarWnd2 = CreateWindowExW(0, L"STATIC", L"Bar2", WS_CHILD | WS_VISIBLE | SS_BITMAP, 0, brdHeight, dlgWidth, GINA_BAR_HEIGHT, hDlg, NULL, hInstance, NULL);
		SendMessageW(hBarWnd2, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBar);

		// Resize the second bar to stretch to dlgWidth
		SetWindowPos(hBarWnd2, NULL, -dlgWidth, brdHeight, dlgWidth, GINA_BAR_HEIGHT, SWP_NOZORDER);
	}

    // Resize the dialog
    SetWindowPos(hDlg, NULL, 0, 0, dlgWidth, dlgHeight + topAreaHeight, SWP_NOZORDER | SWP_NOMOVE);
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