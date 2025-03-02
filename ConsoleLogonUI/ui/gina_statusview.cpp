#pragma once
#include <windows.h>
#include "gina_statusview.h"
#include "wallhost.h"
#include "util/util.h"
#include "util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>

std::wstring g_statusText;

std::mutex statusViewMutex;

void external::StatusView_SetActive(const wchar_t* text)
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}

	std::lock_guard<std::mutex> lock(statusViewMutex);
	if (!ginaManager::Get()->config.showConsole) {
		HideConsoleUI();
	}

	g_statusText = text;

	std::thread([=] {
		if (ginaStatusView::Get()->isActive.exchange(true)) {
			ginaSelectedCredentialView::Get()->Destroy();
			ginaStatusView::Get()->UpdateText();
			return;
		}

		ginaManager::Get()->CloseAllDialogs();
		
		ginaStatusView::Get()->Create();
		ginaStatusView::Get()->Show();
		ginaStatusView::Get()->BeginMessageLoop();
		ginaStatusView::Get()->isActive = false;
	}).detach();
}

ginaStatusView* ginaStatusView::Get()
{
	static ginaStatusView dlg;
	return &dlg;
}

void ginaStatusView::Create()
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaStatusView::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_STATUS_VIEW), 0, (DLGPROC)DlgProc, 0);
	if (!ginaStatusView::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create status view dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		external::ShowConsoleUI();
		return;
	}
	if (ginaManager::Get()->config.hideStatusView)
	{
		ShowWindow(ginaStatusView::Get()->hDlg, SW_HIDE);
	}
	ginaManager::Get()->PostThemeChange();
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaStatusView::Get()->hDlg);
	}
	if (ginaStatusView::Get()->hDlg != NULL) {
		SetDlgItemTextW(ginaStatusView::Get()->hDlg, IDC_STATUS_TEXT, g_statusText.c_str());
	}
}

void ginaStatusView::Destroy()
{
	ginaStatusView* dlg = ginaStatusView::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaStatusView::Show()
{
	if (ginaManager::Get()->config.hideStatusView)
	{
		return;
	}
	ginaStatusView* dlg = ginaStatusView::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaStatusView::Hide()
{
	ginaStatusView* dlg = ginaStatusView::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaStatusView::UpdateText()
{
	ginaStatusView* dlg = ginaStatusView::Get();
	SetDlgItemTextW(dlg->hDlg, IDC_STATUS_TEXT, g_statusText.c_str());
}

void ginaStatusView::BeginMessageLoop()
{
	ginaStatusView* dlg = ginaStatusView::Get();
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int CALLBACK ginaStatusView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		ginaManager::Get()->LoadBranding(hWnd, FALSE, TRUE);
		SetTimer(hWnd, 20, 20, NULL);
		break;
	}
	case WM_TIMER:
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		int dlgWidth = rect.right - rect.left;
		int barOffset = ginaStatusView::Get()->barOffset;
		HWND bar1 = FindWindowExW(hWnd, NULL, L"STATIC", L"Bar");
		HWND bar2 = FindWindowExW(hWnd, bar1, L"STATIC", L"Bar2");
		SetWindowPos(bar1, NULL, barOffset, GINA_SMALL_BRD_HEIGHT, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		SetWindowPos(bar2, NULL, barOffset - dlgWidth, GINA_SMALL_BRD_HEIGHT, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		ginaStatusView::Get()->barOffset = (barOffset + 5) % dlgWidth;
		break;
	}
	case WM_COMMAND:
	{
		break;
	}
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		int origBottom = rect.bottom;
		rect.bottom = GINA_SMALL_BRD_HEIGHT;
		COLORREF brdColor = RGB(255, 255, 255);
		if (ginaManager::Get()->ginaVersion == GINA_VER_XP)
		{
			brdColor = RGB(90, 124, 223);
		}
		HBRUSH hBrush = CreateSolidBrush(brdColor);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		rect.bottom = origBottom;
		rect.top = GINA_SMALL_BRD_HEIGHT + GINA_BAR_HEIGHT;
		COLORREF btnFace;
		btnFace = GetSysColor(COLOR_BTNFACE);
		hBrush = CreateSolidBrush(btnFace);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		return 1;
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0); // Trigger exit thread
		break;
	}
	}
	return 0;
}