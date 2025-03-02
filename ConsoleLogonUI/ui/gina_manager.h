#pragma once
#include <windows.h>

//#define SHOWCONSOLE

#ifdef _DEBUG
#define SHOWCONSOLE
#endif

// msgina.dll resources
#define GINA_DLL_NAME L"msgina.dll"

#define GINA_DLG_USER_SELECT 1450
#define GINA_DLG_USER_SELECT_LOCKED 1850
#define GINA_DLG_SECURITY_CONTROL 1650
#define GINA_DLG_STATUS_VIEW 800
#define GINA_DLG_CHANGE_PWD 1550
#define GINA_DLG_SHUTDOWN 1200
#define GINA_DLG_LOGOFF 1203

#define GINA_DLG_USER_SELECT_HLP 1950
#define GINA_DLG_CHANGE_PWD_HLP 508

#define IDC_OK 1
#define IDC_CANCEL 2

#define GINA_VER_NT3 0
#define GINA_VER_NT4 1
#define GINA_VER_2K 2
#define GINA_VER_XP 3

#include "gina_messageview.h"
#include "gina_userselect.h"
#include "gina_selectedcredentialview.h"
#include "gina_statusview.h"
#include "gina_securitycontrol.h"

struct ginaConfig {
	BOOL showConsole;
	BOOL classicTheme;
	BOOL hideStatusView;
};

class ginaManager
{
public:
	HINSTANCE hInstance;
	HMODULE hGinaDll;

	int ginaVersion;

	BOOL initedPreLogon;

	ginaConfig config;

	ginaManager();

	static ginaManager* Get();

	void LoadGina();
	void UnloadGina();

	void CloseAllDialogs();
	void PostThemeChange();
};

int CALLBACK HelpDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);