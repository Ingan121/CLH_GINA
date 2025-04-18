#pragma once
#include <windows.h>
#include "gina_manager.h"
#include <string>
#include <atomic>

#define IDC_SECURITY_LOCK 1651, 1800
#define IDC_SECURITY_LOGOFF 1652, 1801
#define IDC_SECURITY_SHUTDOWN 1653, 1802
#define IDC_SECURITY_CHANGEPWD 1654, 1803
#define IDC_SECURITY_TASKMGR 1655, 1804
#define IDC_SECURITY_INFO 1656, 1805
#define IDC_SECURITY_LOGONNAME 1658, 1806
#define IDC_SECURITY_DATE 1660, 1807

#define GINA_STR_LOGON_NAME 1519
#define GINA_STR_EMERGENCY_RESTART_TITLE 1540
#define GINA_STR_EMERGENCY_RESTART_DESC 1541

struct SecurityOptionControlWrapper
{
    void* actualInstance;

    SecurityOptionControlWrapper(void* instance)
    {
        actualInstance = instance;
    }

    std::wstring getString();

    void Press();
};

class ginaSecurityControl
{
public:
	HWND hDlg;
	std::atomic<bool> isActive;
	static ginaSecurityControl* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};