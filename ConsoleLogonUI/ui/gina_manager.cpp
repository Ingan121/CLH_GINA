#include <Windows.h>
#include "gina_manager.h"

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
}

void ginaManager::UnloadGina()
{
	if (hGinaDll)
	{
		FreeLibrary(hGinaDll);
	}
}

void ginaManager::LoadBranding(HWND hDlg, BOOL isLarge)
{
    RECT rect;
    GetWindowRect(hDlg, &rect);
    int dlgWidth = rect.right - rect.left;
    int dlgHeight = rect.bottom - rect.top;
    HBITMAP hBranding = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(isLarge ? GINA_BMP_BRD : GINA_BMP_BRD_SMALL));
    HBITMAP hBar = LoadBitmapW(hGinaDll, MAKEINTRESOURCEW(GINA_BMP_BAR));
    BITMAP bmp;
    GetObjectW(hBranding, sizeof(BITMAP), &bmp);
    int brdWidth = bmp.bmWidth;
#ifdef XP
    int brdX = (dlgWidth - brdWidth) / 2;
#else
    int brdX = 0;
#endif
    int topAreaHeight = isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT + GINA_BAR_HEIGHT;

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
    HWND hBarWnd = CreateWindowExW(0, L"STATIC", L"Bar", WS_CHILD | WS_VISIBLE | SS_BITMAP, 0, isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT, dlgWidth, GINA_BAR_HEIGHT, hDlg, NULL, hInstance, NULL);
    SendMessageW(hBrandingWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBranding);
    SendMessageW(hBarWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBar);

    // Resize the bar to stretch to dlgWidth
    SetWindowPos(hBarWnd, NULL, 0, isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT, dlgWidth, GINA_BAR_HEIGHT, SWP_NOZORDER);

    // Resize the dialog
    SetWindowPos(hDlg, NULL, 0, 0, dlgWidth, dlgHeight + topAreaHeight, SWP_NOZORDER | SWP_NOMOVE);
}

void ginaManager::CloseAllDialogs()
{
	ginaSelectedCredentialView::Get()->Destroy();
	ginaChangePwdView::Get()->Destroy();
	ginaSecurityControl::Get()->Destroy();
	ginaStatusView::Get()->Destroy();
}