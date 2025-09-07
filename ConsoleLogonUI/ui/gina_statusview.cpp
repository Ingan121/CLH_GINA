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
BOOL g_appliedUserChangeOnce = FALSE;

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
		//ginaManager::Get()->PostThemeChange();

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

void external::MessageOrStatusView_Destroy()
{
	//ginaStatusView::Get()->Destroy();
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
	ginaStatusView::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GetRes(GINA_DLG_STATUS_VIEW)), 0, (DLGPROC)DlgProc, 0);
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
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaStatusView::Get()->hDlg);
	}
	if (ginaStatusView::Get()->hDlg != NULL) {
		SetDlgItemTextW(ginaStatusView::Get()->hDlg, GetRes(IDC_STATUS_TEXT), g_statusText.c_str());
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
	SetDlgItemTextW(dlg->hDlg, GetRes(IDC_STATUS_TEXT), g_statusText.c_str());
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
		ginaManager::Get()->MoveChildrenForBranding(hWnd, FALSE);
		SetTimer(hWnd, 20, 20, NULL);
		[[fallthrough]];
	}
	case WM_TIMER:
	{
		// Dirty hack around the timing issue with security view open and status view close
		if (ginaSecurityControl::Get()->isActive || ginaSelectedCredentialView::Get()->isActive)
		{
			ginaStatusView::Get()->Destroy();
			return 0;
		}

		if (ginaManager::Get()->initedPreLogon != IsSystemUser() && !g_appliedUserChangeOnce && !ginaSelectedCredentialViewLocked::Get()->isActive)
		{
			// When the classic theme is enabled (with SetWindowTheme, ThemeSection closing, etc.), the color scheme is not applied properly
			// Windows doesn't properly notify windows about the color scheme change when logging on and off
			// So we have to manually apply the color scheme here
			ApplyUserColors();
			// And make WallHost update the background image
			PostMessage(wallHost::Get()->hWnd, WM_THEMECHANGED, 0, 0);
			
			g_appliedUserChangeOnce = TRUE;
		}

		if (ginaManager::Get()->ginaVersion == GINA_VER_NT4)
		{
			break;
		}

		RECT rect;
		GetClientRect(hWnd, &rect);
		int dlgWidth = rect.right - rect.left;
		int barOffset = ginaStatusView::Get()->barOffset;
		HDC hdc = GetDC(hWnd);
		ginaManager::Get()->PaintBranding(hdc, &rect, FALSE, barOffset);
		ReleaseDC(hWnd, hdc);
		ginaStatusView::Get()->barOffset = (barOffset + 5) % dlgWidth;
		break;
	}
	case WM_COMMAND:
	{
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