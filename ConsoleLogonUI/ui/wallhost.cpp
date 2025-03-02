#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <shlobj.h>
#include "wallhost.h"
#include "util/util.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>

HBITMAP g_wpBitmap;
int g_wpStyle = WP_STYLE_CENTER;
HANDLE g_sWallHostProcess = NULL;

std::atomic<bool> isWallHostActive(false);
std::mutex wallHostMutex;

void InitWallHost()
{
	wchar_t customWallHost[MAX_PATH];
	if (GetConfigString(L"CustomWallHost", customWallHost, MAX_PATH)) {
		if (std::filesystem::exists(customWallHost)) {
			wchar_t customWallHostArgs[MAX_PATH];
			GetConfigString(L"CustomWallHostArgs", customWallHostArgs, MAX_PATH);
			PROCESS_INFORMATION pi;
			STARTUPINFO si = { sizeof(si) };
			if (CreateProcessW(customWallHost, customWallHostArgs, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
				g_sWallHostProcess = pi.hProcess;
				CloseHandle(pi.hThread);
				return;
			}
		}
	}

	std::lock_guard<std::mutex> lock(wallHostMutex);

	std::thread([=] {
		if (isWallHostActive.exchange(true)) {
			return;
		}

		wallHost::Create();
		wallHost::Show();
		wallHost::BeginMessageLoop();
		isWallHostActive = false;
	}).detach();
}

wallHost* wallHost::Get()
{
	static wallHost dlg;
	return &dlg;
}

void LoadWallpaper()
{
	// Note: IDesktopWallpaper from shobjidl_core.h doesn't work on pre-logon sessions (CoCreateInstance fails with class not registered)

	// Load the wallpaper image
	wchar_t wallpaperPath[MAX_PATH];
	BOOL tileWallpaper = FALSE;
	int wallpaperStyle = 0;
	if (!IsSystemUser()) {
		wchar_t lpUsername[256];
		if (GetLoggedOnUserInfo(lpUsername, 256, NULL, 0)) {
			if (GetUserHomeDir(lpUsername, wallpaperPath, MAX_PATH)) {
				PathAppendW(wallpaperPath, L"AppData\\Roaming\\Microsoft\\Windows\\Themes\\TranscodedWallpaper");
			}
		}
	}
	else {
		// TranscodedWallpaper of SYSTEM user, unlikely to exist?
		SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wallpaperPath);
		PathAppendW(wallpaperPath, L"Microsoft\\Windows\\Themes\\TranscodedWallpaper");
	}

	HKEY hive;
	GetUserRegHive(KEY_READ, &hive);
	if (hive) {
		HKEY hKey;
		if (RegOpenKeyExW(hive, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			if (wallpaperPath[0] == NULL || !std::filesystem::exists(wallpaperPath)) {
				DWORD cbData = MAX_PATH;
				RegQueryValueExW(hKey, L"Wallpaper", NULL, NULL, (LPBYTE)wallpaperPath, &cbData);
			}

			if (wallpaperPath[0] == NULL || !std::filesystem::exists(wallpaperPath)) {
				return;
			}

			DWORD cbData = sizeof(wchar_t) * 2;
			wchar_t tileWallpaperStr[2];
			if (RegQueryValueExW(hKey, L"TileWallpaper", NULL, NULL, (LPBYTE)&tileWallpaperStr, &cbData) == ERROR_SUCCESS) {
				tileWallpaper = wcscmp(tileWallpaperStr, L"1") == 0;
			}
			cbData = sizeof(wchar_t) * 3;
			wchar_t wallpaperStyleStr[3];
			if (RegQueryValueExW(hKey, L"WallpaperStyle", NULL, NULL, (LPBYTE)&wallpaperStyleStr, &cbData) == ERROR_SUCCESS) {
				wallpaperStyle = _wtoi(wallpaperStyleStr);
			}
			RegCloseKey(hKey);
		}
	}

	if (wallpaperPath[0] == NULL || !std::filesystem::exists(wallpaperPath)) {
		return;
	}

	if (tileWallpaper) {
		g_wpStyle = WP_STYLE_TILE;
	}
	else {
		switch (wallpaperStyle) {
		case 0:
			g_wpStyle = WP_STYLE_CENTER;
			break;
		case 2:
			g_wpStyle = WP_STYLE_STRETCH;
			break;
		case 6:
			g_wpStyle = WP_STYLE_FIT;
			break;
		case 10:
			g_wpStyle = WP_STYLE_FILL;
			break;
		case 22:
			g_wpStyle = WP_STYLE_SPAN;
			break;
		}
	}

	// Try loading as BMP first
	g_wpBitmap = (HBITMAP)LoadImageW(NULL, wallpaperPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	if (g_wpBitmap) {
		return;
	}

	// Initialize GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	// Load the image
	Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(wallpaperPath);
	if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok)
	{
		bitmap->GetHBITMAP(NULL, &g_wpBitmap);
	}

	// Cleanup
	delete bitmap;
	Gdiplus::GdiplusShutdown(gdiplusToken);
}

void wallHost::Create()
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	RECT screenRect;
	screenRect.left = 0;
	screenRect.top = 0;
	screenRect.right = GetSystemMetrics(SM_CXSCREEN);
	screenRect.bottom = GetSystemMetrics(SM_CYSCREEN);

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = wallHost::WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"ClhGinaWallHost";
	RegisterClass(&wc);
	wallHost::Get()->hWnd = CreateWindowExW(0, L"ClhGinaWallHost", L"CLH_GINA Wallpaper Host", WS_POPUP | WS_VISIBLE, 0, 0, screenRect.right, screenRect.bottom, 0, 0, hInstance, 0);
	SetWindowPos(wallHost::Get()->hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	LoadWallpaper();
}

void wallHost::Destroy()
{
	if (g_sWallHostProcess) {
		TerminateProcess(g_sWallHostProcess, 0);
		CloseHandle(g_sWallHostProcess);
		g_sWallHostProcess = NULL;
	}
	wallHost* dlg = wallHost::Get();
	if (dlg->hWnd)
	{
		DestroyWindow(dlg->hWnd);
	}
	if (g_wpBitmap)
	{
		DeleteObject(g_wpBitmap);
	}
}

void wallHost::Show()
{
	wallHost* dlg = wallHost::Get();
	ShowWindow(dlg->hWnd, SW_SHOW);
	UpdateWindow(dlg->hWnd);
}

void wallHost::Hide()
{
	wallHost* dlg = wallHost::Get();
	ShowWindow(dlg->hWnd, SW_HIDE);
}

void wallHost::BeginMessageLoop()
{
	wallHost* dlg = wallHost::Get();
	MSG msg;
	while (GetMessageW(&msg, dlg->hWnd, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

LRESULT CALLBACK wallHost::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	switch (message)
	{
	case WM_CREATE:
	{
		break;
	}
	case WM_PAINT:
	{
		if (g_wpBitmap == NULL)
		{
			break;
		}
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rect;
		GetClientRect(hWnd, &rect);
		HDC hdcMem = CreateCompatibleDC(hdc);
		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, g_wpBitmap);
		BITMAP bm;
		GetObject(g_wpBitmap, sizeof(bm), &bm);
		SetStretchBltMode(hdc, HALFTONE);
		SetBrushOrgEx(hdc, 0, 0, NULL);
		switch (g_wpStyle)
		{
		case WP_STYLE_CENTER:
		{
			int x = (rect.right - bm.bmWidth) / 2;
			int y = (rect.bottom - bm.bmHeight) / 2;
			BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
			break;
		}
		case WP_STYLE_TILE:
		{
			for (int x = 0; x < rect.right; x += bm.bmWidth)
			{
				for (int y = 0; y < rect.bottom; y += bm.bmHeight)
				{
					BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
				}
			}
			break;
		}
		case WP_STYLE_STRETCH:
		{
			StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
			break;
		}
		case WP_STYLE_FIT:
		{
			if (bm.bmWidth > rect.right || bm.bmHeight > rect.bottom)
			{
				double ratio = (double)bm.bmWidth / bm.bmHeight;
				int newWidth = rect.right;
				int newHeight = (int)(rect.right / ratio);
				if (newHeight > rect.bottom)
				{
					newHeight = rect.bottom;
					newWidth = (int)(rect.bottom * ratio);
				}
				int x = (rect.right - newWidth) / 2;
				int y = (rect.bottom - newHeight) / 2;
				StretchBlt(hdc, x, y, newWidth, newHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
			}
			else
			{
				int x = (rect.right - bm.bmWidth) / 2;
				int y = (rect.bottom - bm.bmHeight) / 2;
				BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
			}
			break;
		}
		case WP_STYLE_FILL:
		case WP_STYLE_SPAN: // No multi-monitor support yet!
		{
			if (bm.bmWidth < rect.right || bm.bmHeight < rect.bottom)
			{
				double ratio = (double)bm.bmWidth / bm.bmHeight;
				int newWidth = rect.right;
				int newHeight = (int)(rect.right / ratio);
				if (newHeight < rect.bottom)
				{
					newHeight = rect.bottom;
					newWidth = (int)(rect.bottom * ratio);
				}
				int x = (rect.right - newWidth) / 2;
				int y = rect.bottom - newHeight;
				if (g_wpStyle == WP_STYLE_SPAN)
				{
					y /= 2;
				}
				else
				{
					y /= 3; // idk why but Windows does this
				}
				StretchBlt(hdc, x, y, newWidth, newHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
			}
			else
			{
				double ratio = (double)bm.bmWidth / bm.bmHeight;
				int newHeight = rect.bottom;
				int newWidth = (int)(rect.bottom * ratio);
				if (newWidth < rect.right)
				{
					newWidth = rect.right;
					newHeight = (int)(rect.right / ratio);
				}
				int x = (rect.right - newWidth) / 2;
				int y = rect.bottom - newHeight;
				if (g_wpStyle == WP_STYLE_SPAN)
				{
					y /= 2;
				}
				else
				{
					y /= 3; // idk why but Windows does this
				}
				StretchBlt(hdc, x, y, newWidth, newHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
			}
			break;
		}
		}
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		// Note: this returns black color by default on pre-logon sessions
		// To override this color, set HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\Background to a valid color
		// Or just delete that key to make it use HKCU(of SYSTEM)\Control Panel\Colors\Background instead as usual
		COLORREF bgColor = GetSysColor(COLOR_BACKGROUND);
		HBRUSH hBrush = CreateSolidBrush(bgColor);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		return 1;
		break;
	}
	case WM_SETCURSOR:
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		break;
	}
	case WM_WINDOWPOSCHANGING:
	{
		WINDOWPOS* wp = (WINDOWPOS*)lParam;
		wp->flags |= SWP_NOZORDER;
		break;
	}
	case WM_THEMECHANGED:
	{
		LoadWallpaper();
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0); // Trigger exit thread
		break;
	}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}