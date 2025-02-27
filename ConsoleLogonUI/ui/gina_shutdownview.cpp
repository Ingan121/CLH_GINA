#pragma once
#include "gina_shutdownview.h"
#include <vector>
#include "../util/util.h"
#include "../util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <powrprof.h>

#pragma comment(lib, "PowrProf.lib")

std::atomic<bool> isShutdownViewActive(false);
std::atomic<bool> isLogoffViewActive(false);

std::mutex shutdownViewMutex;
std::mutex logoffViewMutex;

void ShowShutdownDialog()
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}
	std::lock_guard<std::mutex> lock(shutdownViewMutex);
	
	std::thread([=] {
		if (isShutdownViewActive.exchange(true)) {
			return;
		}
		ginaShutdownView::Get()->Create();
		ginaShutdownView::Get()->Show();
		ginaShutdownView::Get()->BeginMessageLoop();
		isShutdownViewActive = false;
	}).detach();
}

void ShowLogoffDialog()
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}
	std::lock_guard<std::mutex> lock(logoffViewMutex);
	
	std::thread([=] {
		if (isLogoffViewActive.exchange(true)) {
			return;
		}
		ginaLogoffView::Get()->Create();
		ginaLogoffView::Get()->Show();
		ginaLogoffView::Get()->BeginMessageLoop();
		isLogoffViewActive = false;
	}).detach();
}

ginaShutdownView* ginaShutdownView::Get()
{
	static ginaShutdownView dlg;
	return &dlg;
}

void ginaShutdownView::Create(HWND parent)
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaShutdownView::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_SHUTDOWN), parent, (DLGPROC)DlgProc, 0);
}

void ginaShutdownView::Destroy()
{
	ginaShutdownView* dlg = ginaShutdownView::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaShutdownView::Show()
{
	ginaShutdownView* dlg = ginaShutdownView::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaShutdownView::Hide()
{
	ginaShutdownView* dlg = ginaShutdownView::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaShutdownView::BeginMessageLoop()
{
	ginaShutdownView* dlg = ginaShutdownView::Get();
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int CALLBACK ginaShutdownView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;

		HWND hShutdownIcon = GetDlgItem(hWnd, IDC_SHUTDOWN_ICON);
		SendMessageW(hShutdownIcon, STM_SETICON, (WPARAM)LoadIconW(hGinaDll, MAKEINTRESOURCEW(IDI_SHUTDOWN)), 0);

		// Populate shutdown combo
		HWND hShutdownCombo = GetDlgItem(hWnd, IDC_SHUTDOWN_COMBO);
		wchar_t shutdownStr[256];
		if (!IsSystemUser())
		{
			wchar_t logoffStr[256], username[MAX_PATH], domain[MAX_PATH];
			GetLoggedOnUserInfo(username, MAX_PATH, domain, MAX_PATH);
			LoadStringW(hGinaDll, GINA_STR_LOGOFF, logoffStr, 256);
			swprintf_s(shutdownStr, logoffStr, username);
			SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		}
		LoadStringW(hGinaDll, GINA_STR_SHUTDOWN, shutdownStr, 256);
		SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		LoadStringW(hGinaDll, GINA_STR_RESTART, shutdownStr, 256);
		SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		LoadStringW(hGinaDll, GINA_STR_SLEEP, shutdownStr, 256);
		SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		LoadStringW(hGinaDll, GINA_STR_HIBERNATE, shutdownStr, 256);
		SendMessageW(hShutdownCombo, CB_ADDSTRING, 0, (LPARAM)shutdownStr);
		SendMessageW(hShutdownCombo, CB_SETCURSEL, 2, 0);

		wchar_t shutdownDesc[256];
		LoadStringW(hGinaDll, GINA_STR_RESTART_DESC, shutdownDesc, 256);
		SetDlgItemTextW(hWnd, IDC_SHUTDOWN_DESC, shutdownDesc);

		// Hide help button and move the OK and Cancel buttons
		HWND hHelpBtn = GetDlgItem(hWnd, IDC_SHUTDOWN_HELP);
		ShowWindow(hHelpBtn, SW_HIDE);
		HWND hOkBtn = GetDlgItem(hWnd, IDC_SHUTDOWN_OK);
		HWND hCancelBtn = GetDlgItem(hWnd, IDC_SHUTDOWN_CANCEL);
		RECT helpRect, okRect, cancelRect;
		GetWindowRect(hHelpBtn, &helpRect);
		GetWindowRect(hOkBtn, &okRect);
		GetWindowRect(hCancelBtn, &cancelRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&helpRect, 2);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&okRect, 2);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&cancelRect, 2);
		SetWindowPos(hOkBtn, NULL, okRect.left + helpRect.right - helpRect.left, okRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		SetWindowPos(hCancelBtn, NULL, cancelRect.left + helpRect.right - helpRect.left, cancelRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);


		ginaManager::Get()->LoadBranding(hWnd, FALSE);
	}
	case WM_COMMAND:
	{
		if (LOWORD(lParam) == IDC_SHUTDOWN_COMBO)
		{
			HWND hShutdownCombo = GetDlgItem(hWnd, IDC_SHUTDOWN_COMBO);
			int index = SendMessageW(hShutdownCombo, CB_GETCURSEL, 0, 0);
			if (IsSystemUser())
			{
				index += 1;
			}
			int descId = 0;
			switch (index)
			{
			case 0:
				descId = GINA_STR_LOGOFF_DESC;
				break;
			case 1:
				descId = GINA_STR_SHUTDOWN_DESC;
				break;
			case 2:
				descId = GINA_STR_RESTART_DESC;
				break;
			case 3:
				descId = GINA_STR_SLEEP_DESC;
				break;
			case 4:
				descId = GINA_STR_HIBERNATE_DESC;
				break;
			}
			wchar_t shutdownDesc[256];
			LoadStringW(ginaManager::Get()->hGinaDll, descId, shutdownDesc, 256);
			SetDlgItemTextW(hWnd, IDC_SHUTDOWN_DESC, shutdownDesc);
		}
		else if (LOWORD(wParam) == IDC_SHUTDOWN_OK)
		{
			HWND hShutdownCombo = GetDlgItem(hWnd, IDC_SHUTDOWN_COMBO);
			int index = SendMessageW(hShutdownCombo, CB_GETCURSEL, 0, 0);
			if (IsSystemUser())
			{
				index += 1;
			}

			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_ESCAPE; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);

			switch (index)
			{
			case 0:
				ExitWindowsEx(EWX_LOGOFF, 0);
				break;
			case 1:
				ExitWindowsEx(EWX_SHUTDOWN, 0);
				break;
			case 2:
				ExitWindowsEx(EWX_REBOOT, 0);
				break;
			case 3:
				SetSuspendState(FALSE, FALSE, FALSE);
				break;
			case 4:
				SetSuspendState(TRUE, FALSE, FALSE);
				break;
			}
		}
		else if (LOWORD(wParam) == IDC_SHUTDOWN_CANCEL)
		{
			ginaShutdownView::Destroy();
		}
		break;
	}
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		int origBottom = rect.bottom;
		rect.bottom = GINA_SMALL_BRD_HEIGHT;
#ifdef XP
		HBRUSH hBrush = CreateSolidBrush(RGB(90, 124, 223));
#else
		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
#endif
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

ginaLogoffView* ginaLogoffView::Get()
{
	static ginaLogoffView dlg;
	return &dlg;
}

void ginaLogoffView::Create(HWND parent)
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaLogoffView::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_LOGOFF), parent, (DLGPROC)DlgProc, 0);
}

void ginaLogoffView::Destroy()
{
	ginaLogoffView* dlg = ginaLogoffView::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaLogoffView::Show()
{
	ginaLogoffView* dlg = ginaLogoffView::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaLogoffView::Hide()
{
	ginaLogoffView* dlg = ginaLogoffView::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaLogoffView::BeginMessageLoop()
{
	ginaLogoffView* dlg = ginaLogoffView::Get();
	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int CALLBACK ginaLogoffView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
		HWND hLogoffIcon = GetDlgItem(hWnd, IDC_LOGOFF_ICON);
		SendMessageW(hLogoffIcon, STM_SETICON, (WPARAM)LoadIconW(hGinaDll, MAKEINTRESOURCEW(IDI_LOGOFF)), 0);
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_LOGOFF_OK)
		{
			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_ESCAPE; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
			ExitWindowsEx(EWX_LOGOFF, 0);
		}
		else if (LOWORD(wParam) == IDC_LOGOFF_CANCEL)
		{
			ginaLogoffView::Destroy();
		}
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