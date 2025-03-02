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

void ShowShutdownDialog(HWND parent)
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}
	std::lock_guard<std::mutex> lock(shutdownViewMutex);
	
	std::thread([=] {
		if (isShutdownViewActive.exchange(true)) {
			return;
		}
		if (parent) {
			EnableWindow(parent, FALSE);
		}
		ginaShutdownView::Get()->Create(parent);
		ginaShutdownView::Get()->Show();
		ginaShutdownView::Get()->BeginMessageLoop();
		isShutdownViewActive = false;
		if (parent) {
			EnableWindow(parent, TRUE);
		}
	}).detach();
}

void ShowLogoffDialog(HWND parent)
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}
	std::lock_guard<std::mutex> lock(logoffViewMutex);
	
	std::thread([=] {
		if (isLogoffViewActive.exchange(true)) {
			return;
		}
		if (parent) {
			EnableWindow(parent, FALSE);
		}
		ginaLogoffView::Get()->Create(parent);
		ginaLogoffView::Get()->Show();
		ginaLogoffView::Get()->BeginMessageLoop();
		isLogoffViewActive = false;
		if (parent) {
			EnableWindow(parent, TRUE);
		}
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
	if (!ginaShutdownView::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create shutdown dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaShutdownView::Get()->hDlg);
	}
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
		if (!IsDialogMessageW(dlg->hDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}

int CALLBACK ginaShutdownView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
		HINSTANCE hShell32 = LoadLibraryExW(L"shell32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
		if (!hShell32)
		{
			break;
		}
		HWND hShutdownIcon = GetDlgItem(hWnd, IDC_SHUTDOWN_ICON);
		SendMessageW(hShutdownIcon, STM_SETICON, (WPARAM)LoadImageW(hShell32, MAKEINTRESOURCEW(SHELL32_INFO), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_VGACOLOR), 0);
		FreeLibrary(hShell32);

		// Hide power off option (no one uses non-ACPI systems anymore)
		// And hide duplicate OK and Cancel buttons
		HWND hPowerOff = GetDlgItem(hWnd, IDC_SHUTDOWN_POWEROFF);
		HWND hOK = GetDlgItem(hWnd, IDC_SHUTDOWN_OK);
		HWND hCancel = GetDlgItem(hWnd, IDC_SHUTDOWN_CANCEL);
		ShowWindow(hPowerOff, SW_HIDE);
		ShowWindow(hOK, SW_HIDE);
		ShowWindow(hCancel, SW_HIDE);

		// Check the shutdown checkbox
		HWND hShutdown = GetDlgItem(hWnd, IDC_SHUTDOWN_SHUTDOWN);
		SendMessageW(hShutdown, BM_SETCHECK, BST_CHECKED, 0);
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			int action = 0;
			if (IsDlgButtonChecked(hWnd, IDC_SHUTDOWN_SHUTDOWN) == BST_CHECKED)
			{
				action = EWX_SHUTDOWN;
			}
			else if (IsDlgButtonChecked(hWnd, IDC_SHUTDOWN_RESTART) == BST_CHECKED)
			{
				action = EWX_REBOOT;
			}
			EnableShutdownPrivilege();
			ExitWindowsEx(action, 0);

			ginaShutdownView::Destroy();
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			ginaShutdownView::Destroy();
		}
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
	if (!ginaLogoffView::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create logoff dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaLogoffView::Get()->hDlg);
	}
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
		if (!IsDialogMessageW(dlg->hDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
}

int CALLBACK ginaLogoffView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
		HINSTANCE hShell32 = LoadLibraryExW(L"shell32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
		if (!hShell32)
		{
			break;
		}
		HWND hLogoffIcon = GetDlgItem(hWnd, IDC_SHUTDOWN_ICON);
		SendMessageW(hLogoffIcon, STM_SETICON, (WPARAM)LoadImageW(hShell32, MAKEINTRESOURCEW(SHELL32_INFO), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_VGACOLOR), 0);
		FreeLibrary(hShell32);
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_ESCAPE; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
			ExitWindowsEx(EWX_LOGOFF, 0);
			ginaLogoffView::Destroy();
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
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