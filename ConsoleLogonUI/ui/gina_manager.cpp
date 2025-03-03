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
	hBar = NULL;
	ginaVersion = 0;
	initedPreLogon = FALSE;
	config = {
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
			MessageBoxW(0, L"Failed to load msgina.dll! Please put a copy of msgina.dll from a Windows 2000/XP installation in system32.", L"CLH_GINA", MB_OK | MB_ICONERROR);
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
		if (!ginaVersion || ginaVersion < GINA_VER_2K)
		{
			std::thread([] {
				MessageBoxW(0, L"This version of msgina.dll is not supported in this build of CLH_GINA! Please use a msgina.dll from Windows 2000 or XP.", L"CLH_GINA", MB_OK | MB_ICONERROR);
			}).detach();
			FreeLibrary(hGinaDll);
			hGinaDll = NULL;
			return;
		}
	}
	hBar = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(GINA_BMP_BAR));
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
			if (hDlg)
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
    RECT rect, clientRect;
    GetWindowRect(hDlg, &rect);
	GetClientRect(hDlg, &clientRect);
    int dlgWidth = rect.right - rect.left;
    int dlgHeight = rect.bottom - rect.top;
	int dlgClientWidth = clientRect.right - clientRect.left;
    HBITMAP hBranding = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(isLarge ? GINA_BMP_BRD : GINA_BMP_BRD_SMALL));
    BITMAP bmp;
    GetObjectW(hBranding, sizeof(BITMAP), &bmp);
    int brdWidth = bmp.bmWidth;
	int brdHeight = bmp.bmHeight;
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
    HWND hBrandingWnd = CreateWindowExW(0, L"STATIC", L"Branding", WS_CHILD | WS_VISIBLE | SS_BITMAP, brdX, 0, brdWidth, isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT, hDlg, NULL, hInstance, NULL);
    SendMessageW(hBrandingWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBranding);
    HWND hBarWnd = CreateWindowExW(0, L"STATIC", L"Bar", WS_CHILD | WS_VISIBLE | SS_BITMAP, 0, isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT, dlgWidth, GINA_BAR_HEIGHT, hDlg, NULL, hInstance, NULL);
    SendMessageW(hBarWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBar);
	    
    // Resize the bar to stretch to dlgWidth
    SetWindowPos(hBarWnd, NULL, 0, brdHeight, dlgWidth, GINA_BAR_HEIGHT, SWP_NOZORDER);

	if (createTwoBars)
	{
		// Create a second bar control, initially hidden in the left side of the first bar
		HWND hBarWnd2 = CreateWindowExW(0, L"STATIC", L"Bar2", WS_CHILD | WS_VISIBLE | SS_BITMAP, 0, isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT, dlgWidth, GINA_BAR_HEIGHT, hDlg, NULL, hInstance, NULL);
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
	PostMessage(wallHost::Get()->hWnd, WM_THEMECHANGED, 0, 0);
	PostMessage(ginaSelectedCredentialView::Get()->hDlg, WM_THEMECHANGED, 0, 0);
	PostMessage(ginaChangePwdView::Get()->hDlg, WM_THEMECHANGED, 0, 0);
	PostMessage(ginaSecurityControl::Get()->hDlg, WM_THEMECHANGED, 0, 0);
	PostMessage(ginaStatusView::Get()->hDlg, WM_THEMECHANGED, 0, 0);
	PostMessage(ginaUserSelect::Get()->hDlg, WM_THEMECHANGED, 0, 0);
}