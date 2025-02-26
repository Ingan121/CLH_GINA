#include <Windows.h>
#include "gina_manager.h"
#include <thread>

ginaManager* ginaManager::Get()
{
	static ginaManager manager{};
	return &manager;
}

ginaManager::ginaManager()
{
	hGinaDll = NULL;
}

void ginaManager::LoadGina()
{
	hGinaDll = LoadLibraryExW(GINA_DLL_NAME, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if (!hGinaDll)
	{
		std::thread([] {
			MessageBoxW(0, L"Failed to load msgina.dll! Please put a copy of msgina.dll from a Windows 2000 installation in system32.", L"CLH_GINA", MB_OK | MB_ICONERROR);
		}).detach();
		return;
	}
	hBar = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(GINA_BMP_BAR));
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
    RECT rect;
    GetWindowRect(hDlg, &rect);
    int dlgWidth = rect.right - rect.left;
    int dlgHeight = rect.bottom - rect.top;
    HBITMAP hBranding = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(isLarge ? GINA_BMP_BRD : GINA_BMP_BRD_SMALL));
    BITMAP bmp;
    GetObjectW(hBranding, sizeof(BITMAP), &bmp);
    int brdWidth = bmp.bmWidth;
	int brdHeight = bmp.bmHeight;
#ifdef XP
    int brdX = (dlgWidth - brdWidth) / 2;
#else
    int brdX = 0;
#endif
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