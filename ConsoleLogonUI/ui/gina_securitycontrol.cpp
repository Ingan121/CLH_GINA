#pragma once
#include <windows.h>
#include "gina_securitycontrol.h"
#include "gina_shutdownview.h"
#include "util/util.h"
#include "util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>

std::vector<SecurityOptionControlWrapper> buttonsList;

std::mutex securityControlMutex;

void external::SecurityControlButtonsList_Clear()
{
    buttonsList.clear();
}

void external::SecurityControl_SetActive()
{
	if (ginaManager::Get()->hGinaDll && !ginaManager::Get()->config.showConsole) {
		HideConsoleUI();
	}
    buttonsList.clear();
}

void external::SecurityControl_ButtonsReady()
{
	if (!ginaManager::Get()->hGinaDll) {
		return;
	}

	std::lock_guard<std::mutex> lock(securityControlMutex);
	ginaManager::Get()->CloseAllDialogs();

	std::thread([=] {
		if (ginaSecurityControl::Get()->isActive.exchange(true)) {
			return;
		}

		ginaManager::Get()->CloseAllDialogs();

		ginaSecurityControl::Get()->Create();
		ginaSecurityControl::Get()->Show();
		ginaSecurityControl::Get()->BeginMessageLoop();
		ginaSecurityControl::Get()->isActive = false;
	}).detach();
}

ginaSecurityControl* ginaSecurityControl::Get()
{
	static ginaSecurityControl dlg;
	return &dlg;
}

void ginaSecurityControl::Create()
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	HINSTANCE hGinaDll = ginaManager::Get()->hGinaDll;
	ginaSecurityControl::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GetRes(GINA_DLG_SECURITY_CONTROL)), 0, (DLGPROC)DlgProc, 0);
	if (!ginaSecurityControl::Get()->hDlg)
	{
		MessageBoxW(0, L"Failed to create security control dialog! Please make sure your copy of msgina.dll in system32 is valid!", L"Error", MB_OK | MB_ICONERROR);
		external::ShowConsoleUI();
		return;
	}
	if (ginaManager::Get()->config.classicTheme)
	{
		MakeWindowClassic(ginaSecurityControl::Get()->hDlg);
	}
}

void ginaSecurityControl::Destroy()
{
	ginaSecurityControl* dlg = ginaSecurityControl::Get();
	EndDialog(dlg->hDlg, 0);
	PostMessage(dlg->hDlg, WM_DESTROY, 0, 0);
}

void ginaSecurityControl::Show()
{
	ginaSecurityControl* dlg = ginaSecurityControl::Get();
	CenterWindow(dlg->hDlg);
	ShowWindow(dlg->hDlg, SW_SHOW);
	UpdateWindow(dlg->hDlg);
}

void ginaSecurityControl::Hide()
{
	ginaSecurityControl* dlg = ginaSecurityControl::Get();
	ShowWindow(dlg->hDlg, SW_HIDE);
}

void ginaSecurityControl::BeginMessageLoop()
{
	ginaSecurityControl* dlg = ginaSecurityControl::Get();
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

int CALLBACK ginaSecurityControl::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		ginaManager::Get()->CloseAllDialogs();
		ginaManager::Get()->LoadBranding(hWnd, FALSE);

		WCHAR _wszUserName[MAX_PATH], _wszDomainName[MAX_PATH];
		WCHAR szFormat[256], szText[1024];
		GetLoggedOnUserInfo(_wszUserName, MAX_PATH, _wszDomainName, MAX_PATH);
		LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_LOGON_NAME, szFormat, 256);
		swprintf_s(szText, szFormat, _wszUserName, _wszDomainName, _wszUserName);
		SetDlgItemTextW(hWnd, GetRes(IDC_SECURITY_LOGONNAME), szText);

		SYSTEMTIME _logonTime;
		WCHAR szDate[128], szTime[128], szDateText[256];
		GetUserLogonTime(&_logonTime);
		GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, NULL, &_logonTime, NULL, szDate, 128, NULL);
		GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, NULL, &_logonTime, NULL, szTime, 128);
		swprintf_s(szDateText, L"%s %s", szDate, szTime);
		SetDlgItemTextW(hWnd, GetRes(IDC_SECURITY_DATE), szDateText);
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == GetRes(IDC_SECURITY_LOCK))
		{
			//system("rundll32.exe user32.dll,LockWorkStation");
			buttonsList[1].Press();
		}
		else if (LOWORD(wParam) == GetRes(IDC_SECURITY_LOGOFF))
		{
			//buttonsList[2].Press(); // makes the system hang for some reason
			ShowLogoffDialog(hWnd);
		}
		else if (LOWORD(wParam) == GetRes(IDC_SECURITY_SHUTDOWN))
		{
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				wchar_t title[256], desc[256];
				LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_EMERGENCY_RESTART_TITLE, title, 256);
				LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_EMERGENCY_RESTART_DESC, desc, 256);
				if (MessageBoxW(hWnd, desc, title, MB_YESNO | MB_ICONERROR) == IDYES)
				{
					EmergencyRestart();
				}
			}
			else
			{
				ShowShutdownDialog(hWnd);
			}
		}
		else if (LOWORD(wParam) == GetRes(IDC_SECURITY_CHANGEPWD))
		{
			// Not available with MS accounts
			// But checking with number of buttons is not accurate
			// Cuz in RDP, the switch user button is gone
			// Also, all buttons can be hidden with group policy
			// Maybe just implement the password change logic ourselves in the future?
			buttonsList[0].Press();
		}
		else if (LOWORD(wParam) == GetRes(IDC_SECURITY_TASKMGR))
		{
			buttonsList[buttonsList.size() - 2].Press();
		}
		else if (LOWORD(wParam) == IDC_CANCEL)
		{
			buttonsList[buttonsList.size() - 1].Press();
		}
		else
		{
			for (auto& button : buttonsList)
			{
				if (LOWORD(wParam) == (int)button.actualInstance)
				{
					button.Press();
					break;
				}
			}
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
		rect.bottom = ginaManager::Get()->smallBrandingHeight;
		COLORREF brdColor = RGB(255, 255, 255);
		if (ginaManager::Get()->ginaVersion == GINA_VER_XP)
		{
			brdColor = RGB(90, 124, 223);
		}
		HBRUSH hBrush = CreateSolidBrush(brdColor);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		rect.bottom = origBottom;
		rect.top = ginaManager::Get()->smallBrandingHeight + GINA_BAR_HEIGHT;
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

void external::SecurityOptionControl_Create(void* actualInstance)
{
    SecurityOptionControlWrapper button(actualInstance);
    //SPDLOG_INFO("SecurityOptionControl_Create {} {}", (void*)actualInstance, ws2s(button.getString()));

    buttonsList.push_back(button);
}

void external::SecurityOptionControl_Destroy(void* actualInstance)
{
    for (int i = 0; i < buttonsList.size(); ++i)
    {
        auto& button = buttonsList[i];
        if (button.actualInstance == actualInstance)
        {
            //SPDLOG_INFO("FOUND AND DELETING BUTTON");
            buttonsList.erase(buttonsList.begin() + i);
            break;
        }
    }
}

void external::SecurityControl_SetInactive()
{
}

void SecurityOptionControlWrapper::Press()
{
    _KEY_EVENT_RECORD keyrecord;
    keyrecord.bKeyDown = true;
    keyrecord.wVirtualKeyCode = VK_RETURN;
    int result;
    if (actualInstance)
    {
        //SPDLOG_INFO("Actual instance {} isn't null, so we are calling SecurityOptionControl_Press with enter on the control!", actualInstance);

        external::SecurityOptionControl_Press(actualInstance, &keyrecord, &result);
    }
}

std::wstring SecurityOptionControlWrapper::getString()
{
    return external::SecurityOptionControl_getString(actualInstance);
}