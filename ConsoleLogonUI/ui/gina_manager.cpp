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
			MessageBoxW(0, L"Failed to load msgina.dll! Please put a copy of msgina.dll from a Windows NT 4.0 installation in system32.", L"CLH_GINA", MB_OK | MB_ICONERROR);
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
							else if (major == 5 && minor == 1)
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
		if (!ginaVersion || ginaVersion != GINA_VER_NT4)
		{
			std::thread([] {
				MessageBoxW(0, L"This version of msgina.dll is not supported in this build of CLH_GINA! Please use a msgina.dll from Windows NT 4.0.", L"CLH_GINA", MB_OK | MB_ICONERROR);
			}).detach();
			FreeLibrary(hGinaDll);
			hGinaDll = NULL;
			return;
		}
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