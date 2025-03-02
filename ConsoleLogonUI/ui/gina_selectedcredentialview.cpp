#pragma once
#include "gina_selectedcredentialview.h"
#include "gina_shutdownview.h"
#include "wallhost.h"
#include "../util/util.h"
#include "../util/interop.h"
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

std::vector<EditControlWrapper> editControls;

std::wstring g_accountName;

std::mutex credentialViewMutex;

void external::NotifyWasInSelectedCredentialView()
{
}

void external::SelectedCredentialView_SetActive(const wchar_t* accountNameToDisplay, int flag)
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}

	std::lock_guard<std::mutex> lock(credentialViewMutex);
	if (!ginaManager::Get()->config.showConsole) {
		HideConsoleUI();
	}

	g_accountName = accountNameToDisplay;

	std::thread([=] {
		if (flag == 2) {
			if (ginaChangePwdView::Get()->isActive.exchange(true)) {
				return;
			}
		}
		else {
			if (IsSystemUser()) {
				if (ginaSelectedCredentialView::Get()->isActive.exchange(true)) {
					return;
				}
			}
			else {
				if (ginaSelectedCredentialViewLocked::Get()->isActive.exchange(true)) {
					return;
				}
			}
		}

		if (flag == 2) {
			ginaChangePwdView::Get()->Create();
			ginaChangePwdView::Get()->Show();
			ginaChangePwdView::Get()->BeginMessageLoop();
			ginaChangePwdView::Get()->isActive = false;
		}
		else {
			ginaManager::Get()->CloseAllDialogs();
			
			if (IsSystemUser())
			{
				ginaSelectedCredentialView::Get()->Create();
				ginaSelectedCredentialView::Get()->Show();
				ginaSelectedCredentialView::Get()->BeginMessageLoop();
				ginaSelectedCredentialView::Get()->isActive = false;
			}
			else
			{
				ginaSelectedCredentialViewLocked::Get()->Create();
				ginaSelectedCredentialViewLocked::Get()->Show();
				ginaSelectedCredentialViewLocked::Get()->BeginMessageLoop();
				ginaSelectedCredentialViewLocked::Get()->isActive = false;
			}
		}
	}).detach();
}

int it = 0;
void external::EditControl_Create(void* actualInstance)
{
	if (!actualInstance)
	{
		return;
	}
	EditControlWrapper wrapper;
	wrapper.actualInstance = actualInstance;
	wrapper.fieldNameCache = ws2s(wrapper.GetFieldName());
	//MessageBox(0, wrapper.GetFieldName().c_str(), wrapper.GetFieldName().c_str(),0);
	wrapper.inputBuffer = ws2s(wrapper.GetInputtedText());

	for (int i = editControls.size() - 1; i >= 0; --i)
	{
		auto& control = editControls[i];
		if (control.GetFieldName() == wrapper.GetFieldName())
		{
			editControls.erase(editControls.begin() + i);
			break;
		}
	}
	it++;

	editControls.push_back(wrapper);
}

void external::EditControl_Destroy(void* actualInstance)
{
	for (int i = 0; i < editControls.size(); ++i)
	{
		auto& button = editControls[i];
		if (button.actualInstance == actualInstance)
		{
			editControls.erase(editControls.begin() + i);
			break;
		}
	}
	it--;
}

ginaSelectedCredentialView* ginaSelectedCredentialView::Get()
{
	static ginaSelectedCredentialView dlg;
	return &dlg;
}

void ginaSelectedCredentialView::Create()
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaSelectedCredentialView::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_USER_SELECT), 0, (DLGPROC)DlgProc, 0);
	if (!ginaSelectedCredentialView::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create selected credential view dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		external::ShowConsoleUI();
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaSelectedCredentialView::Get()->hDlg);
	}
	if (IsFriendlyLogonUI())
	{
		SetDlgItemTextW(ginaSelectedCredentialView::Get()->hDlg, IDC_CREDVIEW_USERNAME, g_accountName.c_str());
	}
	else
	{
		wchar_t username[256];
		GetLastLogonUser(username, 256);
		SetDlgItemTextW(ginaSelectedCredentialView::Get()->hDlg, IDC_CREDVIEW_USERNAME, username);
	}
}

void ginaSelectedCredentialView::Destroy()
{
	ginaSelectedCredentialView* dlg = ginaSelectedCredentialView::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaSelectedCredentialView::Show()
{
	ginaSelectedCredentialView* dlg = ginaSelectedCredentialView::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaSelectedCredentialView::Hide()
{
	ginaSelectedCredentialView* dlg = ginaSelectedCredentialView::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaSelectedCredentialView::BeginMessageLoop()
{
	ginaSelectedCredentialView* dlg = ginaSelectedCredentialView::Get();
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

int CALLBACK ginaSelectedCredentialView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Load the icon
		HWND hIcon = GetDlgItem(hWnd, IDC_CREDVIEW_ICON);
		SendMessageW(hIcon, STM_SETICON, (WPARAM)LoadImageW(ginaManager::Get()->hGinaDll, MAKEINTRESOURCEW(IDI_LOGON), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR), 0);

		// Hide the domain chooser
		HWND hDomainChooser = GetDlgItem(hWnd, IDC_CREDVIEW_DOMAIN);
		HWND hDomainChooserLabel = GetDlgItem(hWnd, IDC_CREDVIEW_DOMAIN_LABEL);
		ShowWindow(hDomainChooser, SW_HIDE);
		ShowWindow(hDomainChooserLabel, SW_HIDE);

		// Hide the dial-up checkbox
		HWND hDialup = GetDlgItem(hWnd, IDC_CREDVIEW_DIALUP);
		ShowWindow(hDialup, SW_HIDE);

		// DIsable the Cancel button
		if (!IsFriendlyLogonUI())
		{
			HWND hCancel = GetDlgItem(hWnd, IDC_CANCEL);
			EnableWindow(hCancel, FALSE);
		}

		SetFocus(GetDlgItem(hWnd, IDC_CREDVIEW_PASSWORD));
		SendMessage(GetDlgItem(hWnd, IDC_OK), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			// OK button
			wchar_t username[256];
			wchar_t password[256];
			GetDlgItemTextW(hWnd, IDC_CREDVIEW_USERNAME, username, 256);
			GetDlgItemTextW(hWnd, IDC_CREDVIEW_PASSWORD, password, 256);

			if (IsFriendlyLogonUI() && wcscmp(username, g_accountName.c_str()) != 0)
			{
				// Go to user select view
				KEY_EVENT_RECORD rec;
				rec.wVirtualKeyCode = VK_ESCAPE;
				external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
				return 0;
			}

			editControls[0].SetInputtedText(username);
			editControls[1].SetInputtedText(password);

			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_RETURN; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			// Cancel button
			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_ESCAPE; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
		}
		else if (LOWORD(wParam) == IDC_CREDVIEW_HELP)
		{
			DialogBoxW(ginaManager::Get()->hGinaDll, MAKEINTRESOURCEW(GINA_DLG_USER_SELECT_HLP), hWnd, (DLGPROC)HelpDlgProc);
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

ginaSelectedCredentialViewLocked* ginaSelectedCredentialViewLocked::Get()
{
	static ginaSelectedCredentialViewLocked dlg;
	return &dlg;
}

void ginaSelectedCredentialViewLocked::Create()
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaSelectedCredentialViewLocked::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_USER_SELECT_LOCKED), 0, (DLGPROC)DlgProc, 0);
	if (!ginaSelectedCredentialViewLocked::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create selected credential view dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		external::ShowConsoleUI();
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaSelectedCredentialViewLocked::Get()->hDlg);
	}
	if (ginaManager::Get()->initedPreLogon)
	{
		// "Use sign-in info to automatically finish setting up my device" is enabled
		// So the locked window opens right after boot
		// But this somehow doesn't fire the WM_THEMECHANGED message to the wallhost
		// So we need to manually send it
		ginaManager::Get()->PostThemeChange();
	}
	WCHAR _wszUserName[MAX_PATH], _wszDomainName[MAX_PATH];
	WCHAR szFormat[256], szText[1024];
	GetLoggedOnUserInfo(_wszUserName, MAX_PATH, _wszDomainName, MAX_PATH);
	LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_CREDVIEW_LOCKED_USERNAME_INFO, szFormat, 256);
	swprintf_s(szText, szFormat, _wszDomainName, _wszUserName, _wszUserName);
	SetDlgItemTextW(ginaSelectedCredentialViewLocked::Get()->hDlg, IDC_CREDVIEW_LOCKED_USERNAME_INFO, szText);
	SetDlgItemTextW(ginaSelectedCredentialViewLocked::Get()->hDlg, IDC_CREDVIEW_LOCKED_USERNAME, _wszUserName);
}

void ginaSelectedCredentialViewLocked::Destroy()
{
	ginaSelectedCredentialViewLocked* dlg = ginaSelectedCredentialViewLocked::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaSelectedCredentialViewLocked::Show()
{
	ginaSelectedCredentialViewLocked* dlg = ginaSelectedCredentialViewLocked::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaSelectedCredentialViewLocked::Hide()
{
	ginaSelectedCredentialViewLocked* dlg = ginaSelectedCredentialViewLocked::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaSelectedCredentialViewLocked::BeginMessageLoop()
{
	ginaSelectedCredentialViewLocked* dlg = ginaSelectedCredentialViewLocked::Get();
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

int CALLBACK ginaSelectedCredentialViewLocked::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		// Load the icon
		HWND hIcon = GetDlgItem(hWnd, IDC_CREDVIEW_LOCKED_ICON);
		SendMessageW(hIcon, STM_SETICON, (WPARAM)LoadImageW(ginaManager::Get()->hGinaDll, MAKEINTRESOURCEW(IDI_LOCKED), IMAGE_ICON, 64, 64, LR_DEFAULTCOLOR), 0);

		// Hide the domain chooser
		HWND hDomainChooser = GetDlgItem(hWnd, IDC_CREDVIEW_LOCKED_DOMAIN);
		HWND hDomainChooserLabel = GetDlgItem(hWnd, IDC_CREDVIEW_LOCKED_DOMAIN_LABEL);
		ShowWindow(hDomainChooser, SW_HIDE);
		ShowWindow(hDomainChooserLabel, SW_HIDE);

		SetFocus(GetDlgItem(hWnd, IDC_CREDVIEW_LOCKED_PASSWORD));
		SendMessage(GetDlgItem(hWnd, IDC_OK), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			// OK button
			wchar_t username[256];
			wchar_t password[256];
			GetDlgItemTextW(hWnd, IDC_CREDVIEW_LOCKED_USERNAME, username, 256);
			GetDlgItemTextW(hWnd, IDC_CREDVIEW_LOCKED_PASSWORD, password, 256);

			if (wcscmp(username, g_accountName.c_str()) != 0)
			{
				// Go to user select view
				KEY_EVENT_RECORD rec;
				rec.wVirtualKeyCode = VK_ESCAPE;
				external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
				return 0;
			}

			editControls[0].SetInputtedText(username);
			editControls[1].SetInputtedText(password);

			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_RETURN; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			// Cancel button
			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_ESCAPE; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
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

ginaChangePwdView* ginaChangePwdView::Get()
{
	static ginaChangePwdView dlg;
	return &dlg;
}

void ginaChangePwdView::Create()
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaChangePwdView::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_CHANGE_PWD), 0, (DLGPROC)DlgProc, 0);
	if (!ginaChangePwdView::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create change password dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		external::ShowConsoleUI();
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaChangePwdView::Get()->hDlg);
	}
}

void ginaChangePwdView::Destroy()
{
	ginaChangePwdView* dlg = ginaChangePwdView::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaChangePwdView::Show()
{
	ginaChangePwdView* dlg = ginaChangePwdView::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
}

void ginaChangePwdView::Hide()
{
	ginaChangePwdView* dlg = ginaChangePwdView::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaChangePwdView::BeginMessageLoop()
{
	ginaChangePwdView* dlg = ginaChangePwdView::Get();
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

int CALLBACK ginaChangePwdView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		wchar_t lpUsername[256], lpDomain[256];
		GetLoggedOnUserInfo(lpUsername, 256, lpDomain, 256);
		SetDlgItemTextW(hWnd, IDC_CHPW_USERNAME, lpUsername);
		EnableWindow(GetDlgItem(hWnd, IDC_CHPW_DOMAIN), FALSE);
		SetDlgItemTextW(hWnd, IDC_CHPW_DOMAIN, lpDomain);

		SetFocus(GetDlgItem(hWnd, IDC_CHPW_OLD_PASSWORD));
		SendMessage(GetDlgItem(hWnd, IDC_OK), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_OK)
		{
			// OK button
			wchar_t username[256];
			wchar_t oldPassword[256];
			wchar_t newPassword[256];
			wchar_t confirmPassword[256];
			GetDlgItemTextW(hWnd, IDC_CHPW_USERNAME, username, 256);
			GetDlgItemTextW(hWnd, IDC_CHPW_OLD_PASSWORD, oldPassword, 256);
			GetDlgItemTextW(hWnd, IDC_CHPW_NEW_PASSWORD, newPassword, 256);
			GetDlgItemTextW(hWnd, IDC_CHPW_CONFIRM_PASSWORD, confirmPassword, 256);

			if (wcscmp(newPassword, confirmPassword) != 0)
			{
				wchar_t title[256], msg[256];
				LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_CHPW_TITLE, title, 256);
				wcscat_s(title, 256, L" "); // Just to let MakeWindowClassicAsync differentiate the message box and this dialog
				LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_CHPW_CONFIRM_MISMATCH, msg, 256);

				if (ginaManager::Get()->config.classicTheme)
				{
					MakeWindowClassicAsync(title);
				}

				MessageBoxW(hWnd, msg, title, MB_OK | MB_ICONERROR);
				return 0;
			}
			
			editControls[0].SetInputtedText(username);
			editControls[1].SetInputtedText(oldPassword);
			editControls[2].SetInputtedText(newPassword);
			editControls[3].SetInputtedText(confirmPassword);

			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_RETURN; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			// Cancel button
			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_ESCAPE; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
		}
		else if (LOWORD(wParam) == IDC_CHPW_HELP)
		{
			DialogBoxW(ginaManager::Get()->hGinaDll, MAKEINTRESOURCEW(GINA_DLG_CHANGE_PWD_HLP), hWnd, (DLGPROC)HelpDlgProc);
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

std::wstring EditControlWrapper::GetFieldName()
{
	return external::EditControl_GetFieldName(actualInstance);
}

std::wstring EditControlWrapper::GetInputtedText()
{
	return external::EditControl_GetInputtedText(actualInstance);
}

void EditControlWrapper::SetInputtedText(std::wstring input)
{
	return external::EditControl_SetInputtedText(actualInstance, input);
}

bool EditControlWrapper::isVisible()
{
	return external::EditControl_isVisible(actualInstance);
}