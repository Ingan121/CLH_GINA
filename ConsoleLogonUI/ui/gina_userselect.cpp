#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include "gina_userselect.h"
#include "gina_shutdownview.h"
#include "../util/util.h"
#include "util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>

std::vector<SelectableUserOrCredentialControlWrapper> buttons;

std::atomic<bool> isUserSelectActive(false);
std::mutex userSelectMutex;

HWND g_hUsernameCombo = NULL;

void external::UserSelect_SetActive()
{
	if (ginaManager::Get()->hGinaDll && !ginaManager::Get()->config.showConsole) {
		HideConsoleUI();
	}
}

void external::SelectableUserOrCredentialControl_Sort()
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}

	std::lock_guard<std::mutex> lock(userSelectMutex);
	
	std::sort(buttons.begin(), buttons.end(), [](SelectableUserOrCredentialControlWrapper& a, SelectableUserOrCredentialControlWrapper& b) { return a.GetText() < b.GetText(); });
	
	std::thread([=] {
		if (isUserSelectActive.exchange(true)) {
			return;
		}

		ginaManager::Get()->CloseAllDialogs();

		ginaUserSelect::Get()->Create();
		ginaUserSelect::Get()->Show();
		ginaUserSelect::Get()->BeginMessageLoop();
		isUserSelectActive = false;
	}).detach();
}

void external::SelectableUserOrCredentialControl_Create(void* actualInstance, const wchar_t* path)
{
	//MessageBox(0, external::SelectableUserOrCredentialControl_GetText(actualInstance).c_str(), L"test", 0);
    SelectableUserOrCredentialControlWrapper wrapper;
    wrapper.actualInstance = actualInstance;
	//wrapper.pfp = GetHBITMAPFromImageFile(const_cast<WCHAR*>(path)); // i don't need this

    buttons.push_back(wrapper);
}

void external::SelectableUserOrCredentialControl_Destroy(void* actualInstance)
{
    for (int i = 0; i < buttons.size(); ++i)
    {
        auto& button = buttons[i];
        if (button.actualInstance == actualInstance)
        {
            // SPDLOG_INFO("Found button instance and removing!");
            buttons.erase(buttons.begin() + i);
            break;
        }
    }
}

std::wstring SelectableUserOrCredentialControlWrapper::GetText()
{
    text = external::SelectableUserOrCredentialControl_GetText(actualInstance);
    return text;
}

void SelectableUserOrCredentialControlWrapper::Press()
{
    return external::SelectableUserOrCredentialControl_Press(actualInstance);
}

bool SelectableUserOrCredentialControlWrapper::isCredentialControl()
{
    return external::SelectableUserOrCredentialControl_isCredentialControl(actualInstance);
}

ginaUserSelect* ginaUserSelect::Get()
{
	static ginaUserSelect dlg;
	return &dlg;
}

void ginaUserSelect::Create()
{
	if (!IsSystemUser())
	{
		// Somehow canceling the change password dialog causes this to open
		// Since this dialog is only for pre-logon sessions, just ignore it on post-logon sessions (where the Ctrl+Alt+Del dialog and the change password dialog are used)
		return;
	}
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaUserSelect::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_USER_SELECT), 0, (DLGPROC)DlgProc, 0);
	if (!ginaUserSelect::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create user select dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		external::ShowConsoleUI();
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaUserSelect::Get()->hDlg);
	}
}

void ginaUserSelect::Destroy()
{
	ginaUserSelect* dlg = ginaUserSelect::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaUserSelect::Show()
{
	ginaUserSelect* dlg = ginaUserSelect::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaUserSelect::Hide()
{
	ginaUserSelect* dlg = ginaUserSelect::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaUserSelect::BeginMessageLoop()
{
	ginaUserSelect* dlg = ginaUserSelect::Get();
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

int CALLBACK ginaUserSelect::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Load the icon
		HWND hIcon = GetDlgItem(hWnd, IDC_CREDVIEW_ICON);
		SendMessageW(hIcon, STM_SETICON, (WPARAM)LoadImageW(ginaManager::Get()->hGinaDll, MAKEINTRESOURCEW(IDI_LOGON), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR), 0);

		// Replace the username input with a combo box
		HWND hUsername = GetDlgItem(hWnd, IDC_CREDVIEW_USERNAME);
		RECT usernameRect;
		GetWindowRect(hUsername, &usernameRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&usernameRect, 2);
		ShowWindow(hUsername, SW_HIDE);

		// Add users to the combo box
		g_hUsernameCombo = CreateWindowExW(0, L"COMBOBOX", L"UserSelect", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, usernameRect.left, usernameRect.top, usernameRect.right - usernameRect.left, usernameRect.bottom - usernameRect.top, hWnd, (HMENU)IDC_CREDVIEW_USERNAME, NULL, NULL);
		for (int i = buttons.size() - 1; i >= 0; i--)
		{
			SendMessageW(g_hUsernameCombo, CB_ADDSTRING, 0, (LPARAM)buttons[i].GetText().c_str());
		}
		SendMessageW(g_hUsernameCombo, CB_SETCURSEL, 0, 0);
		SendMessageW(g_hUsernameCombo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));

		// Hide the password input & label
		HWND hPassword = GetDlgItem(hWnd, IDC_CREDVIEW_PASSWORD);
		HWND hPasswordLabel = GetDlgItem(hWnd, IDC_CREDVIEW_PASSWORD_LABEL);
		ShowWindow(hPassword, SW_HIDE);
		ShowWindow(hPasswordLabel, SW_HIDE);

		// Hide the domain chooser
		HWND hDomainChooser = GetDlgItem(hWnd, IDC_CREDVIEW_DOMAIN);
		HWND hDomainChooserLabel = GetDlgItem(hWnd, IDC_CREDVIEW_DOMAIN_LABEL);
		ShowWindow(hDomainChooser, SW_HIDE);
		ShowWindow(hDomainChooserLabel, SW_HIDE);

		// Hide the dial-up checkbox
		HWND hDialup = GetDlgItem(hWnd, IDC_CREDVIEW_DIALUP);
		ShowWindow(hDialup, SW_HIDE);

		// DIsable the Cancel button
		HWND hCancel = GetDlgItem(hWnd, IDC_CANCEL);
		EnableWindow(hCancel, FALSE);

		SetFocus(GetDlgItem(hWnd, IDC_CREDVIEW_PASSWORD));
		SendMessage(GetDlgItem(hWnd, IDC_OK), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			// OK button
			int total = buttons.size();
			int index = SendMessageW(g_hUsernameCombo, CB_GETCURSEL, 0, 0);
			buttons[total - index - 1].Press();
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			// Cancel button, disabled
		}
		else if (LOWORD(wParam) == IDC_CREDVIEW_SHUTDOWN)
		{
			// Shutdown button
			ShowShutdownDialog(hWnd);
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