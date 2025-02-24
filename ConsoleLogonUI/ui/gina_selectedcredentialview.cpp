#pragma once
#include "gina_selectedcredentialview.h"
#include <vector>
#include "../util/util.h"
#include "../util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>

std::vector<EditControlWrapper> editControls;

std::wstring g_accountName;

std::atomic<bool> isCredentialViewActive(false);
std::atomic<bool> isChangePwdViewActive(false);
std::mutex credentialViewMutex;

void external::NotifyWasInSelectedCredentialView()
{
}

void external::SelectedCredentialView_SetActive(const wchar_t* accountNameToDisplay, int flag)
{
	std::lock_guard<std::mutex> lock(credentialViewMutex);
	//MessageBox(0, L"SelectedCredentialView_SetActive", L"SelectedCredentialView_SetActive", 0);
	//HideConsoleUI();

	g_accountName = accountNameToDisplay;

	std::thread([=] {
		if (flag == 2) {
			if (isChangePwdViewActive.exchange(true)) {
				return;
			}
		}
		else {
			if (isCredentialViewActive.exchange(true)) {
				return;
			}
		}

		ginaManager::Get()->CloseAllDialogs();

		if (flag == 2) {
			ginaChangePwdView::Get()->Create();
			ginaChangePwdView::Get()->Show();
			ginaChangePwdView::Get()->BeginMessageLoop();
			isChangePwdViewActive = false;
		}
		else {
			ginaSelectedCredentialView::Get()->Create();
			ginaSelectedCredentialView::Get()->Show();
			ginaSelectedCredentialView::Get()->BeginMessageLoop();
			isCredentialViewActive = false;
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
	//MessageBox(0, wrapper.GetFieldName().c_str(), wrapper.GetFieldName().c_str(), 0);

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
	if (ginaSelectedCredentialView::Get()->hDlg != NULL) {
		SetDlgItemTextW(ginaSelectedCredentialView::Get()->hDlg, IDC_CREDVIEW_USERNAME, g_accountName.c_str());
	}
}

void ginaSelectedCredentialView::Destroy()
{
	ginaSelectedCredentialView* dlg = ginaSelectedCredentialView::Get();
	EndDialog(dlg->hDlg, 0);
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
	while (GetMessageW(&msg, dlg->hDlg, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int CALLBACK ginaSelectedCredentialView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

		// Hide the domain chooser
		HWND hDomainChooser = GetDlgItem(hWnd, IDC_CREDVIEW_DOMAIN);
		HWND hDomainChooserLabel = GetDlgItem(hWnd, IDC_CREDVIEW_DOMAIN_LABEL);
		RECT domainRect;
		GetWindowRect(hDomainChooser, &domainRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&domainRect, 2);
		ShowWindow(hDomainChooser, SW_HIDE);
		ShowWindow(hDomainChooserLabel, SW_HIDE);
		dlgHeightToReduce += domainRect.bottom - domainRect.top + 8;
		bottomBtnYToMove = domainRect.bottom - domainRect.top + 8;

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
		//EnableWindow(hCancel, FALSE);

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
		//wchar_t msg[256];
		//wsprintfW(msg, L"WM_COMMAND, wParam = %d, lParam = %d", wParam, lParam);
		//MessageBoxW(hWnd, msg, L"Info", MB_OK | MB_ICONINFORMATION);
		if (LOWORD(wParam) == IDC_CREDVIEW_OK)
		{
			// OK button
			wchar_t username[256];
			wchar_t password[256];
			GetDlgItemTextW(hWnd, 1502, username, 256);
			GetDlgItemTextW(hWnd, 1503, password, 256);
			//wsprintfW(msg, L"Username: %d, Password: %s", editControls.size(), password);
			//MessageBoxW(hWnd, msg, L"Info", MB_OK | MB_ICONINFORMATION);
			editControls[0].SetInputtedText(username);
			editControls[1].SetInputtedText(password);

			HMODULE externalHookModule = GetModuleHandleW(L"ConsoleLogonHook.dll");
			if (!externalHookModule)
			{
				MessageBox(0, L"HOOK DLL NOT FOUND", L"HOOK DLL NOT FOUND", 0);
				return false;
			}

			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_RETURN; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
		}
		else if (LOWORD(wParam) == IDC_CREDVIEW_CANCEL)
		{
			// Cancel button
			KEY_EVENT_RECORD rec;
			rec.wVirtualKeyCode = VK_ESCAPE; //forward it to consoleuiview
			external::ConsoleUIView__HandleKeyInputExternal(external::GetConsoleUIView(), &rec);
		}
		else if (LOWORD(wParam) == IDC_CREDVIEW_SHUTDOWN)
		{
			// Shutdown button
			MessageBoxW(hWnd, L"Shutdown button clicked", L"Info", MB_OK | MB_ICONINFORMATION);
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
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		int origBottom = rect.bottom;
		rect.bottom = GINA_LARGE_BRD_HEIGHT;
		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
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
		PostQuitMessage(0); // Trigger exit thread
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
	if (ginaChangePwdView::Get()->hDlg != NULL) {
		SetDlgItemTextW(ginaChangePwdView::Get()->hDlg, IDC_CHPW_USERNAME, g_accountName.c_str());
	}
}

void ginaChangePwdView::Destroy()
{
	ginaChangePwdView* dlg = ginaChangePwdView::Get();
	EndDialog(dlg->hDlg, 0);
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
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int CALLBACK ginaChangePwdView::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		HWND hDomainChooser = GetDlgItem(hWnd, IDC_CHPW_DOMAIN);
		EnableWindow(hDomainChooser, FALSE);
		std::wstring domain = GetDomainName();
		SetDlgItemTextW(hWnd, IDC_CHPW_DOMAIN, domain.c_str());
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == 1)
		{
			// OK button
			wchar_t username[256];
			wchar_t password[256];
			GetDlgItemTextW(hWnd, IDC_CHPW_USERNAME, username, 256);
			GetDlgItemTextW(hWnd, IDC_CHPW_OLD_PASSWORD, password, 256);
			wchar_t msg[256];
			wsprintfW(msg, L"Username: %s, Password: %s", username, password);
			MessageBoxW(hWnd, msg, L"Info", MB_OK | MB_ICONINFORMATION);
		}
		else if (LOWORD(wParam) == 2)
		{
			// Cancel button
			EndDialog(hWnd, 0);
			PostQuitMessage(0); // Trigger exit thread
		}
		break;
	}
	case WM_CLOSE:
	{
		EndDialog(hWnd, 0);
		PostQuitMessage(0); // Trigger exit thread
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