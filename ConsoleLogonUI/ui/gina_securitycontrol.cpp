#pragma once
#include <windows.h>
#include "gina_securitycontrol.h"
#include "util/util.h"
#include "util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>

std::vector<SecurityOptionControlWrapper> buttonsList;

std::atomic<bool> isSecurityControlActive(false);
std::mutex securityControlMutex;

void external::SecurityControlButtonsList_Clear()
{
    buttonsList.clear();
}

void external::SecurityControl_SetActive()
{
    //HideConsoleUI();
    buttonsList.clear();
}

void external::SecurityControl_ButtonsReady()
{
	std::lock_guard<std::mutex> lock(securityControlMutex);

	std::thread([=] {
		if (isSecurityControlActive.exchange(true)) {
			return;
		}

		ginaManager::Get()->CloseAllDialogs();

		ginaSecurityControl::Get()->Create();
		ginaSecurityControl::Get()->Show();
		ginaSecurityControl::Get()->BeginMessageLoop();
		isSecurityControlActive = false;
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
	ginaSecurityControl::Get()->hDlg = CreateDialogParamW(hGinaDll, MAKEINTRESOURCEW(GINA_DLG_SECURITY_CONTROL), 0, (DLGPROC)DlgProc, 0);
}

void ginaSecurityControl::Destroy()
{
	ginaSecurityControl* dlg = ginaSecurityControl::Get();
	EndDialog(dlg->hDlg, 0);
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
	while (GetMessageW(&msg, dlg->hDlg, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

int CALLBACK ginaSecurityControl::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		ginaStatusView::Get()->Destroy();
		ginaManager::Get()->LoadBranding(hWnd, FALSE);
		// TODO: Fix this
		//std::wstring logonName = GetLogonName();
		//wchar_t logonNameStr[512];
		//wchar_t logonNameStrFormatted[512];
		//LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_LOGON_NAME, logonNameStr, 512);
		//wsprintf(logonNameStr, logonNameStr, logonName.c_str());
		//wsprintf(logonNameStrFormatted, logonNameStr, logonName.c_str());
		//SetDlgItemTextW(hWnd, IDC_SECURITY_LOGONNAME, logonNameStrFormatted);
		//std::wstring logonTime = GetLogonTime();
		//SetDlgItemTextW(hWnd, IDC_SECURITY_DATE, logonTime.c_str());
		break;
	}
	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_SECURITY_LOCK)
		{
			//system("rundll32.exe user32.dll,LockWorkStation");
			buttonsList[1].Press();
		}
		else if (LOWORD(wParam) == IDC_SECURITY_LOGOFF)
		{
			//ExitWindowsEx(EWX_LOGOFF, 0);
			buttonsList[2].Press();
		}
		else if (LOWORD(wParam) == IDC_SECURITY_SHUTDOWN)
		{
			//system("shutdown /s /t 0");
			// TODO!
			MessageBoxW(hWnd, L"Shutdown button clicked", L"Info", MB_OK | MB_ICONINFORMATION);
		}
		else if (LOWORD(wParam) == IDC_SECURITY_CHANGEPWD)
		{
			// Not available with MS accounts
			// But checking with number of buttons is not accurate
			// Cuz in RDP, the switch user button is gone
			// Also, all buttons can be hidden with group policy
			// Maybe just implement the password change logic ourselves in the future?
			buttonsList[0].Press();
		}
		else if (LOWORD(wParam) == IDC_SECURITY_TASKMGR)
		{
			buttonsList[buttonsList.size() - 2].Press();
		}
		else if (LOWORD(wParam) == IDC_SECURITY_CANCEL)
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