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
	ginaUserSelect::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GetRes(GINA_DLG_USER_SELECT)), 0, (DLGPROC)DlgProc, 0);
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
	ginaManager::Get()->PostThemeChange();
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
		RECT dlgRect;
		GetWindowRect(hWnd, &dlgRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dlgRect, 2);
		int dlgHeightToReduce = 0;
		int bottomBtnYToMove = 0;

		// Hide the legal announcement
		HWND hLegalAnnouncement = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_LEGAL));
		if (hLegalAnnouncement)
		{
			RECT legalRect;
			GetWindowRect(hLegalAnnouncement, &legalRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&legalRect, 2);
			dlgHeightToReduce = legalRect.bottom - legalRect.top;
			ShowWindow(hLegalAnnouncement, SW_HIDE);

			HWND hChild = GetWindow(hWnd, GW_CHILD);
			while (hChild != NULL)
			{
				if (hChild != hLegalAnnouncement)
				{
					RECT childRect;
					GetWindowRect(hChild, &childRect);
					MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&childRect, 2);
					if (childRect.top > 6)
					{
						SetWindowPos(hChild, NULL, childRect.left, childRect.top - legalRect.bottom, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
					}
				}
				hChild = GetWindow(hChild, GW_HWNDNEXT);
			}
		}

		// Hide the XP-specific locked message for the pre-logon dialog
		// (Used in XP when the Welcome screen is disabled and tsdiscon.exe is used)
		HWND hLockedGroupBox = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_XP_LOCKED_GROUP));
		if (hLockedGroupBox)
		{
			ShowWindow(hLockedGroupBox, SW_HIDE);
			RECT lockedRect;
			GetWindowRect(hLockedGroupBox, &lockedRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&lockedRect, 2);
			HWND hLockedInfo = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_XP_LOCKED_INFO));
			HWND hLockedUsernameInfo = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_LOCKED_USERNAME_INFO));
			HWND hLockedIcon = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_LOCKED_ICON));
			ShowWindow(hLockedInfo, SW_HIDE);
			ShowWindow(hLockedUsernameInfo, SW_HIDE);
			ShowWindow(hLockedIcon, SW_HIDE);
			dlgHeightToReduce += lockedRect.bottom - lockedRect.top;
		}

		// Load the icon (NT4 only)
		HWND hIcon = GetDlgItem(hWnd, IDC_CREDVIEW_ICON);
		if (hIcon)
		{
			SendMessageW(hIcon, STM_SETICON, (WPARAM)LoadImageW(ginaManager::Get()->hGinaDll, MAKEINTRESOURCEW(IDI_LOGON), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR), 0);
		}

		// Replace the username input with a combo box
		HWND hUsername = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_USERNAME));
		RECT usernameRect;
		GetWindowRect(hUsername, &usernameRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&usernameRect, 2);
		ShowWindow(hUsername, SW_HIDE);

		// Add users to the combo box
		g_hUsernameCombo = CreateWindowExW(0, L"COMBOBOX", L"UserSelect", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, usernameRect.left, usernameRect.top, usernameRect.right - usernameRect.left, usernameRect.bottom - usernameRect.top, hWnd, (HMENU)GetRes(IDC_CREDVIEW_USERNAME), NULL, NULL);
		for (int i = buttons.size() - 1; i >= 0; i--)
		{
			SendMessageW(g_hUsernameCombo, CB_ADDSTRING, 0, (LPARAM)buttons[i].GetText().c_str());
		}
		SendMessageW(g_hUsernameCombo, CB_SETCURSEL, 0, 0);
		SendMessageW(g_hUsernameCombo, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE, 0));

		// Hide the password input & label
		HWND hPassword = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_PASSWORD));
		HWND hPasswordLabel = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_PASSWORD_LABEL));
		RECT passwordRect;
		GetWindowRect(hPassword, &passwordRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&passwordRect, 2);
		ShowWindow(hPassword, SW_HIDE);
		ShowWindow(hPasswordLabel, SW_HIDE);
		dlgHeightToReduce += passwordRect.bottom - passwordRect.top + 8;
		bottomBtnYToMove = passwordRect.bottom - passwordRect.top + 8;

		// Hide the domain chooser
		HWND hDomainChooser = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_DOMAIN));
		HWND hDomainChooserLabel = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_DOMAIN_LABEL));
		RECT domainRect;
		GetWindowRect(hDomainChooser, &domainRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&domainRect, 2);
		ShowWindow(hDomainChooser, SW_HIDE);
		ShowWindow(hDomainChooserLabel, SW_HIDE);
		dlgHeightToReduce += domainRect.bottom - domainRect.top + 8;
		bottomBtnYToMove += domainRect.bottom - domainRect.top + 8;

		// Hide the dial-up checkbox
		HWND hDialup = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_DIALUP));
		RECT dialupRect;
		GetWindowRect(hDialup, &dialupRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dialupRect, 2);
		ShowWindow(hDialup, SW_HIDE);
		dlgHeightToReduce += dialupRect.bottom - dialupRect.top + 8;
		bottomBtnYToMove += dialupRect.bottom - dialupRect.top;

		// Move the OK, Cancel, Shutdown, Options, and language icon controls up (2000+)
		HWND hOK = GetDlgItem(hWnd, IDC_OK);
		HWND hCancel = GetDlgItem(hWnd, IDC_CANCEL);
		if (ginaManager::Get()->ginaVersion >= GINA_VER_2K)
		{
			HWND hShutdown = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_SHUTDOWN));
			HWND hOptions = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_OPTIONS));
			HWND hLanguageIcon = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_LANGUAGE));
			HWND controlsToMove[] = { hOK, hCancel, hShutdown, hOptions, hLanguageIcon };
			for (int i = 0; i < sizeof(controlsToMove) / sizeof(HWND); i++)
			{
				RECT controlRect;
				GetWindowRect(controlsToMove[i], &controlRect);
				MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&controlRect, 2);
				SetWindowPos(controlsToMove[i], NULL, controlRect.left, controlRect.top - bottomBtnYToMove, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}
		}
		else
		{
			// NT4 has fixed dialog sizes regardless of the control visibility
			dlgHeightToReduce = 0;
		}

		// DIsable the Cancel button
		EnableWindow(hCancel, FALSE);

		HWND optBtn = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_OPTIONS));
		if (optBtn)
		{
			wchar_t optBtnStr[256];
			LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_OPTBTN_COLLAPSE, optBtnStr, 256);
			SetDlgItemTextW(hWnd, GetRes(IDC_CREDVIEW_OPTIONS), optBtnStr);
		}

		// Resize the dialog (2000+)
		SetWindowPos(hWnd, NULL, 0, 0, dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top - dlgHeightToReduce, SWP_NOZORDER | SWP_NOMOVE);

		// Load branding and bar images
		ginaManager::Get()->LoadBranding(hWnd, TRUE);

		// Set focus to the username combo box
		SetFocus(GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_PASSWORD)));
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
		else if (LOWORD(wParam) == IDC_CREDVIEW_HELP)
		{
			DialogBoxW(ginaManager::Get()->hGinaDll, MAKEINTRESOURCEW(GINA_DLG_USER_SELECT_HLP), hWnd, (DLGPROC)HelpDlgProc);
		}
		else if (LOWORD(wParam) == GetRes(IDC_CREDVIEW_SHUTDOWN))
		{
			// Shutdown button
			ShowShutdownDialog(hWnd);
		}
		else if (LOWORD(wParam) == GetRes(IDC_CREDVIEW_OPTIONS))
		{
			// Options button
			HWND hShutdown = GetDlgItem(hWnd, 1501);
			BOOL isShutdownVisible = IsWindowVisible(hShutdown);
			ShowWindow(hShutdown, isShutdownVisible ? SW_HIDE : SW_SHOW);
			RECT shutdownRect;
			GetWindowRect(hShutdown, &shutdownRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&shutdownRect, 2);
			HWND controlsToMove[] = {
				GetDlgItem(hWnd, 1),
				GetDlgItem(hWnd, 2)
			};
			for (int i = 0; i < sizeof(controlsToMove) / sizeof(HWND); i++)
			{
				RECT controlRect;
				GetWindowRect(controlsToMove[i], &controlRect);
				MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&controlRect, 2);
				SetWindowPos(controlsToMove[i], NULL, isShutdownVisible ? controlRect.left + shutdownRect.right - shutdownRect.left : controlRect.left - shutdownRect.right + shutdownRect.left, controlRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}
			// Update the button string
			HMODULE hGinaDll = ginaManager::Get()->hGinaDll;
			wchar_t optBtnStr[256];
			LoadStringW(hGinaDll, isShutdownVisible ? GINA_STR_OPTBTN_EXPAND : GINA_STR_OPTBTN_COLLAPSE, optBtnStr, 256);
			SetDlgItemTextW(hWnd, 1514, optBtnStr);
		}
		break;
	}
	case WM_ERASEBKGND:
	{
		if (ginaManager::Get()->ginaVersion == GINA_VER_NT4)
		{
			return 0;
		}

		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		int origBottom = rect.bottom;
		rect.bottom = ginaManager::Get()->largeBrandingHeight;
		COLORREF brdColor = RGB(255, 255, 255);
		if (ginaManager::Get()->ginaVersion == GINA_VER_XP)
		{
			brdColor = RGB(90, 124, 223);
		}
		HBRUSH hBrush = CreateSolidBrush(brdColor);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		rect.bottom = origBottom;
		rect.top = ginaManager::Get()->largeBrandingHeight + GINA_BAR_HEIGHT;
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