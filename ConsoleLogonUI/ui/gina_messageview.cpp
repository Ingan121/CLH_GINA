#pragma once
#include <windows.h>
#include <vector>
#include "gina_manager.h"
#include "gina_messageview.h"
#include "util/util.h"
#include "util/interop.h"

std::vector<MessageOptionControlWrapper> controls;

std::wstring gMessage;

bool bIsInMessageView = false;

void external::MessageView_SetActive()
{
	//MessageBoxW(NULL, gMessage.c_str(), L"Windows", MB_OK);
	//controls[0].Press();
}

void external::MessageOptionControl_Create(void* actualInsance, int optionflag)
{
}

void external::MessageView_SetMessage(const wchar_t* message)
{
}

void external::MessageOptionControl_Destroy(void* actualInstance)
{

}

void external::MessageOrStatusView_Destroy()
{
	if (bIsInMessageView)
	{
		bIsInMessageView = false;
	}
}

messageViewDlg* messageViewDlg::Get()
{
	static messageViewDlg dlg;
	return &dlg;
}

LRESULT CALLBACK messageViewDlg::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HINSTANCE hInstance = ginaManager::Get()->hInstance;

	switch (message)
	{
	case WM_CREATE:
	{
		CreateWindowW(L"STATIC", gMessage.c_str(), WS_CHILD | WS_VISIBLE, 0, 0, 100, 100, hWnd, 0, hInstance, 0);
		for (int i = 0; i < controls.size(); ++i)
		{
			auto& control = controls[i];
			CreateWindowW(L"BUTTON", control.GetText().c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 100, 100, hWnd, (HMENU)i, hInstance, 0);
		}
		break;
	}
	case WM_COMMAND:
	{
		int id = LOWORD(wParam);
		auto& control = controls[id];
		control.Press();
		break;
	}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void MessageOptionControlWrapper::Press()
{
	_KEY_EVENT_RECORD keyrecord;
	keyrecord.bKeyDown = true;
	keyrecord.wVirtualKeyCode = VK_RETURN;
	int result;
	if (actualInstance)
	{
		//SPDLOG_INFO("Actual instance {} isn't null, so we are calling MessageOptionControl_Press with enter on the control!", actualInstance);

		external::MessageOptionControl_Press(actualInstance, &keyrecord, &result);
	}
}

std::wstring MessageOptionControlWrapper::GetText()
{
	return external::MessageOptionControl_GetText(actualInstance);
}