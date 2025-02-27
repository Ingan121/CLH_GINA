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
#ifndef SHOWCONSOLE
	HideConsoleUI();
#endif
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
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaUserSelect::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_USER_SELECT), 0, (DLGPROC)DlgProc, 0);
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
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
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
		HWND hLegalAnnouncement = GetDlgItem(hWnd, IDC_CREDVIEW_LEGAL);
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
		RECT passwordRect;
		GetWindowRect(hPassword, &passwordRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&passwordRect, 2);
		ShowWindow(hPassword, SW_HIDE);
		ShowWindow(hPasswordLabel, SW_HIDE);
		dlgHeightToReduce += passwordRect.bottom - passwordRect.top + 8;
		bottomBtnYToMove = passwordRect.bottom - passwordRect.top + 8;

		// Hide the domain chooser
		HWND hDomainChooser = GetDlgItem(hWnd, IDC_CREDVIEW_DOMAIN);
		HWND hDomainChooserLabel = GetDlgItem(hWnd, IDC_CREDVIEW_DOMAIN_LABEL);
		RECT domainRect;
		GetWindowRect(hDomainChooser, &domainRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&domainRect, 2);
		ShowWindow(hDomainChooser, SW_HIDE);
		ShowWindow(hDomainChooserLabel, SW_HIDE);
		dlgHeightToReduce += domainRect.bottom - domainRect.top + 8;
		bottomBtnYToMove += domainRect.bottom - domainRect.top + 8;

		// Hide the dial-up checkbox
		HWND hDialup = GetDlgItem(hWnd, IDC_CREDVIEW_DIALUP);
		RECT dialupRect;
		GetWindowRect(hDialup, &dialupRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dialupRect, 2);
		ShowWindow(hDialup, SW_HIDE);
		dlgHeightToReduce += dialupRect.bottom - dialupRect.top + 8;
		bottomBtnYToMove += dialupRect.bottom - dialupRect.top;

		// Move the OK, Cancel, Shutdown, Options, and language icon controls up
		HWND hOK = GetDlgItem(hWnd, IDC_CREDVIEW_OK);
		HWND hCancel = GetDlgItem(hWnd, IDC_CREDVIEW_CANCEL);
		HWND hShutdown = GetDlgItem(hWnd, IDC_CREDVIEW_SHUTDOWN);
		HWND hOptions = GetDlgItem(hWnd, IDC_CREDVIEW_OPTIONS);
		HWND hLanguageIcon = GetDlgItem(hWnd, IDC_CREDVIEW_LANGUAGE);
		HWND controlsToMove[] = { hOK, hCancel, hShutdown, hOptions, hLanguageIcon };
		for (int i = 0; i < sizeof(controlsToMove) / sizeof(HWND); i++)
		{
			RECT controlRect;
			GetWindowRect(controlsToMove[i], &controlRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&controlRect, 2);
			SetWindowPos(controlsToMove[i], NULL, controlRect.left, controlRect.top - bottomBtnYToMove, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}

		// DIsable the Cancel button
		EnableWindow(hCancel, FALSE);

		ginaManager* manager = ginaManager::Get();
		HMODULE hGinaDll = manager->hGinaDll;
		wchar_t optBtnStr[256];
		LoadStringW(hGinaDll, GINA_STR_OPTBTN_COLLAPSE, optBtnStr, 256);
		SetDlgItemTextW(hWnd, IDC_CREDVIEW_OPTIONS, optBtnStr);

		SetWindowPos(hWnd, NULL, 0, 0, dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top - dlgHeightToReduce, SWP_NOZORDER | SWP_NOMOVE);

		// Load branding and bar images
		manager->LoadBranding(hWnd, TRUE);
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_CREDVIEW_OK)
		{
			// OK button
			int total = buttons.size();
			int index = SendMessageW(g_hUsernameCombo, CB_GETCURSEL, 0, 0);
			buttons[total - index - 1].Press();
		}
		else if (LOWORD(wParam) == IDC_CREDVIEW_CANCEL)
		{
			// Cancel button, disabled
		}
		else if (LOWORD(wParam) == IDC_CREDVIEW_SHUTDOWN)
		{
			// Shutdown button
			ginaShutdownView::Get()->Create(hWnd);
			ginaShutdownView::Get()->Show();
			ginaShutdownView::Get()->BeginMessageLoop();
		}
		else if (LOWORD(wParam) == IDC_CREDVIEW_OPTIONS)
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
	case WM_KEYDOWN:
	{
		if (wParam == VK_RETURN)
		{
			// Handle Enter key
			SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_CREDVIEW_OK, BN_CLICKED), 0);
		}
		else if (wParam == VK_ESCAPE)
		{
			// Handle Esc key
			SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_CREDVIEW_CANCEL, BN_CLICKED), 0);
		}
		break;
	}
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		int origBottom = rect.bottom;
		rect.bottom = GINA_LARGE_BRD_HEIGHT;
#ifdef XP
		HBRUSH hBrush = CreateSolidBrush(RGB(90, 124, 223));
#else
		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
#endif
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		rect.bottom = origBottom;
		rect.top = GINA_LARGE_BRD_HEIGHT + GINA_BAR_HEIGHT;
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

//LRESULT CALLBACK ginaUserSelect::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//{
//	HINSTANCE hInstance = ginaManager::Get()->hInstance;
//	switch (message)
//	{
//	case WM_CREATE:
//	{
//		wchar_t msg[256];
//		//wsprintf(msg, L"Button size: %d", buttons.size());
//		//MessageBoxW(NULL, msg, L"Info", MB_OK | MB_ICONINFORMATION);
//		for (int i = 0; i < buttons.size(); ++i)
//		{
//			auto& button = buttons[i];
//			CreateWindowW(L"BUTTON", button.GetText().c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100 + i * 100, 25, hWnd, (HMENU)i, hInstance, 0);
//		}
//		break;
//	}
//	case WM_COMMAND:
//	{
//		//MessageBoxW(NULL, L"1", L"Info", MB_OK | MB_ICONINFORMATION);
//		int id = LOWORD(wParam);
//		auto& button = buttons[id];
//		button.Press();
//		break;
//	}
//	case WM_PAINT:
//	{
//		PAINTSTRUCT ps;
//		HDC hdc = BeginPaint(hWnd, &ps);
//		HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
//		FillRect(hdc, &ps.rcPaint, hBrush);
//		DeleteObject(hBrush);
//		EndPaint(hWnd, &ps);
//		break;
//	}
//	case WM_SETCURSOR:
//	{
//		SetCursor(LoadCursor(NULL, IDC_ARROW));
//		return TRUE;
//	}
//	case WM_CLOSE:
//	{
//		DestroyWindow(hWnd);
//		break;
//	}
//	case WM_DESTROY:
//	{
//		PostQuitMessage(0); // Trigger exit thread
//		break;
//	}
//	}
//	return DefWindowProc(hWnd, message, wParam, lParam);
//}