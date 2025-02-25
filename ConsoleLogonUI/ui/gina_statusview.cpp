#pragma once
#include <windows.h>
#include "gina_statusview.h"
#include "util/util.h"
#include "util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>

std::wstring g_statusText;

std::atomic<bool> isStatusViewActive(false);
std::mutex statusViewMutex;

void external::StatusView_SetActive(const wchar_t* text)
{	
	std::lock_guard<std::mutex> lock(statusViewMutex);
	//HideConsoleUI();

	ginaManager::Get()->CloseAllDialogs();
	//MessageBox(0, text, L"StatusView_SetActive", 0);

	g_statusText = text;

	std::thread([=] {
		if (isStatusViewActive.exchange(true)) {
			return;
		}
		//MessageBox(0, text, L"StatusView_SetActive inthread", 0);
		
		ginaStatusView::Get()->Create();
		ginaStatusView::Get()->Show();
		ginaStatusView::Get()->BeginMessageLoop();
		isStatusViewActive = false;
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

void ginaStatusView::BeginMessageLoop()
{
	ginaStatusView* dlg = ginaStatusView::Get();
	MSG msg;
	while (GetMessageW(&msg, dlg->hDlg, 0, 0))
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
		HDC hdc = GetDC(hWnd);
		HDC hdcMem = CreateCompatibleDC(hdc);
		HBITMAP hBar = ginaManager::Get()->hBar;
		BITMAP barBmp;
		int barOffset = ginaStatusView::Get()->barOffset;
		GetObject(hBar, sizeof(BITMAP), &barBmp);
		int barWidth = barBmp.bmWidth;
		SelectObject(hdcMem, hBar);
		BitBlt(hdc, barOffset, GINA_SMALL_BRD_HEIGHT, dlgWidth - barOffset, GINA_BAR_HEIGHT, hdcMem, barOffset, 0, SRCCOPY);
		if (barOffset > 0)
		{
			BitBlt(hdc, 0, GINA_SMALL_BRD_HEIGHT, barOffset, GINA_BAR_HEIGHT, hdcMem, dlgWidth - barOffset, 0, SRCCOPY);
		}
		DeleteDC(hdcMem);
		ReleaseDC(hWnd, hdc);
		ginaStatusView::Get()->barOffset = (barOffset + 5) % barWidth;
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
		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
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
	case WM_CLOSE:
	{
		EndDialog(hWnd, 0);
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