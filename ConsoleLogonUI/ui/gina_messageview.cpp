#pragma once
#include <windows.h>
#include <vector>
#include "gina_manager.h"
#include "gina_messageview.h"
#include "util/util.h"
#include "util/interop.h"
#include <thread>
#include <atomic>
#include <mutex>

std::vector<MessageOptionControlWrapper> controls;

std::wstring gMessage;

std::atomic<bool> isMessageViewActive(false);
std::mutex messageViewMutex;

void external::MessageView_SetActive()
{
	std::lock_guard<std::mutex> lock(messageViewMutex);
	if (ginaManager::Get()->hGinaDll && !ginaManager::Get()->config.showConsole) {
		HideConsoleUI();
	}

	std::thread([=] {
		if (isMessageViewActive.exchange(true)) {
			return;
		}

		ginaManager::Get()->CloseAllDialogs();

		wchar_t title[256];
		LoadStringW(ginaManager::Get()->hGinaDll, GINA_STR_LOGON_MESSAGE_TITLE, title, 256);
		
		int btnCount = controls.size();
		int res = 0;

		if (ginaManager::Get()->config.classicTheme)
		{
			std::thread([=] {
				HWND hDlg = NULL;
				while (!hDlg)
				{
					hDlg = FindWindowExW(0, 0, L"#32770", title);
					Sleep(10);
				}
				MakeWindowClassic(hDlg);
				}).detach();
		}

		long mbIcon = ginaManager::Get()->ginaVersion == GINA_VER_NT4 ? MB_ICONERROR : MB_ICONEXCLAMATION;
		if (btnCount <= 1)
		{
			res = MessageBoxW(0, gMessage.c_str(), title, MB_OK | mbIcon);
			controls[0].Press();
		}
		else if (btnCount == 2)
		{
			res = MessageBoxW(0, gMessage.c_str(), title, MB_YESNO | mbIcon);
			if (res == IDYES) {
				controls[0].Press();
			}
			else {
				controls[1].Press();
			}
		}
		else if (btnCount == 3)
		{
			res = MessageBoxW(0, gMessage.c_str(), title, MB_YESNOCANCEL | mbIcon);
			if (res == IDYES) {
				controls[0].Press();
			}
			else if (res == IDNO) {
				controls[1].Press();
			}
			else {
				controls[2].Press();
			}
		}
		else
		{
			ShowConsoleUI();
		}
		isMessageViewActive = false;
	}).detach();
}

void external::MessageOptionControl_Create(void* actualInsance, int optionflag)
{
	MessageOptionControlWrapper wrapper;
	wrapper.actualInstance = actualInsance;
	wrapper.optionflag = optionflag;
	controls.push_back(wrapper);
}

void external::MessageView_SetMessage(const wchar_t* message)
{
	gMessage = message;
}

void external::MessageOptionControl_Destroy(void* actualInstance)
{
	for (int i = 0; i < controls.size(); ++i)
	{
		auto& button = controls[i];
		if (button.actualInstance == actualInstance)
		{
			//SPDLOG_INFO("FOUND AND DELETING MessageOptionControl");
			controls.erase(controls.begin() + i);
			break;
		}
	}
}

void MessageOptionControlWrapper::Press()
{
	_KEY_EVENT_RECORD keyrecord;
	keyrecord.bKeyDown = true;
	keyrecord.wVirtualKeyCode = VK_RETURN;
	int result;
	if (actualInstance)
	{
		//MessageBoxW(NULL, L"Actual instance {} isn't null, so we are calling MessageOptionControl_Press with enter on the control!", L"Info", MB_OK | MB_ICONINFORMATION);

		external::MessageOptionControl_Press(actualInstance, &keyrecord, &result);
	}
	else {
		MessageBoxW(NULL, L"Actual instance is null!", L"Info", MB_OK | MB_ICONINFORMATION);
	}
}

std::wstring MessageOptionControlWrapper::GetText()
{
	return external::MessageOptionControl_GetText(actualInstance);
}