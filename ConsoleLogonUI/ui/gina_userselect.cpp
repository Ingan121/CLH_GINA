#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include "gina_userselect.h"
#include "../util/util.h"
#include "util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>

std::vector<SelectableUserOrCredentialControlWrapper> buttons;

std::atomic<bool> isUserSelectActive(false);
std::mutex userSelectMutex;

void external::UserSelect_SetActive()
{
	//HideConsoleUI();
}

void external::SelectableUserOrCredentialControl_Sort()
{
	std::lock_guard<std::mutex> lock(userSelectMutex);
	
	std::sort(buttons.begin(), buttons.end(), [](SelectableUserOrCredentialControlWrapper& a, SelectableUserOrCredentialControlWrapper& b) { return a.GetText() < b.GetText(); });
	//(0, buttons[0].GetText().c_str(), buttons[0].GetText().c_str(), 0);
	
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
	//MessageBox(NULL, text.c_str(), L"Info", MB_OK | MB_ICONINFORMATION);
    return text;
}

void SelectableUserOrCredentialControlWrapper::Press()
{
	//MessageBoxW(NULL, L"2", L"Info", MB_OK | MB_ICONINFORMATION);
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

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = ginaUserSelect::WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"ClhGinaUserSelect";
	RegisterClass(&wc);

	ginaUserSelect::Get()->hWin = CreateWindowExW(0, L"ClhGinaUserSelect", L"Log On to Windows", WS_OVERLAPPEDWINDOW, 0, 0, 600, 200, 0, 0, hInstance, 0);
}

void ginaUserSelect::Destroy()
{
	ginaUserSelect* dlg = ginaUserSelect::Get();
	DestroyWindow(dlg->hWin);
}

void ginaUserSelect::Show()
{
	ginaUserSelect* dlg = ginaUserSelect::Get();
	CenterWindow(dlg->hWin);
	ShowWindow(dlg->hWin, SW_SHOW);
	UpdateWindow(dlg->hWin);
}

void ginaUserSelect::Hide()
{
	ginaUserSelect* dlg = ginaUserSelect::Get();
	ShowWindow(dlg->hWin, SW_HIDE);
}

void ginaUserSelect::BeginMessageLoop()
{
	ginaUserSelect* dlg = ginaUserSelect::Get();
	MSG msg;
	while (GetMessageW(&msg, dlg->hWin, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

LRESULT CALLBACK ginaUserSelect::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;
	switch (message)
	{
	case WM_CREATE:
	{
		wchar_t msg[256];
		//wsprintf(msg, L"Button size: %d", buttons.size());
		//MessageBoxW(NULL, msg, L"Info", MB_OK | MB_ICONINFORMATION);
		for (int i = 0; i < buttons.size(); ++i)
		{
			auto& button = buttons[i];
			CreateWindowW(L"BUTTON", button.GetText().c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100 + i * 100, 25, hWnd, (HMENU)i, hInstance, 0);
		}
		break;
	}
	case WM_COMMAND:
	{
		//MessageBoxW(NULL, L"1", L"Info", MB_OK | MB_ICONINFORMATION);
		int id = LOWORD(wParam);
		auto& button = buttons[id];
		button.Press();
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
		FillRect(hdc, &ps.rcPaint, hBrush);
		DeleteObject(hBrush);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_SETCURSOR:
	{
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return TRUE;
	}
	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
		PostQuitMessage(0); // Trigger exit thread
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0); // Trigger exit thread
		break;
	}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}