#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <shlobj.h>
#include "wallhost.h"
#include "util/util.h"
#include <thread>
#include <atomic>
#include <mutex>

HBITMAP g_wpBitmap;

std::atomic<bool> isWallHostActive(false);
std::mutex wallHostMutex;

void InitWallHost()
{
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
	wallHost::Get()->hWin = CreateWindowExW(0, L"ClhGinaWallHost", L"CLH_GINA Wallpaper Host", WS_POPUP | WS_VISIBLE, 0, 0, screenRect.right, screenRect.bottom, 0, 0, hInstance, 0);
	SetWindowPos(wallHost::Get()->hWin, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	// Load the TranscodedWallpaper
	//WCHAR wallpaperPath[MAX_PATH];
	const wchar_t* wallpaperPath = L"C:\\Users\\Ingan121\\AppData\\Roaming\\Microsoft\\Windows\\Themes\\TranscodedWallpaper";
	//if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, wallpaperPath)))
	//{
	//	PathAppendW(wallpaperPath, L"\\Microsoft\\Windows\\Themes\\TranscodedWallpaper");

		// Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

        // Load the image
        Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromFile(wallpaperPath);
        if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok)
        {
        bitmap->GetHBITMAP(NULL, &g_wpBitmap);

        // Scale the image to screen size
        HDC hdc = GetDC(wallHost::Get()->hWin);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, g_wpBitmap);
        BITMAP bm;
        GetObject(g_wpBitmap, sizeof(bm), &bm);

        RECT rect;
        GetClientRect(wallHost::Get()->hWin, &rect);
        SetStretchBltMode(hdc, HALFTONE);
        StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
        ReleaseDC(wallHost::Get()->hWin, hdc);
        }

        // Cleanup
        delete bitmap;
        Gdiplus::GdiplusShutdown(gdiplusToken);
	//}

}

void wallHost::Destroy()
{
	wallHost* dlg = wallHost::Get();
	DestroyWindow(dlg->hWin);
}

void wallHost::Show()
{
	wallHost* dlg = wallHost::Get();
	CenterWindow(dlg->hWin);
	ShowWindow(dlg->hWin, SW_SHOW);
	UpdateWindow(dlg->hWin);
}

void wallHost::Hide()
{
	wallHost* dlg = wallHost::Get();
	ShowWindow(dlg->hWin, SW_HIDE);
}

void wallHost::BeginMessageLoop()
{
	wallHost* dlg = wallHost::Get();
	MSG msg;
	while (GetMessageW(&msg, dlg->hWin, 0, 0))
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
        StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
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
		COLORREF bgColor;
		bgColor = GetSysColor(COLOR_BACKGROUND);
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
	case WM_DESTROY:
	{
		PostQuitMessage(0); // Trigger exit thread
		break;
	}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}