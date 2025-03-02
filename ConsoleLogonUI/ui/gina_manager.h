#pragma once
#include <windows.h>

//#define SHOWCONSOLE

#ifdef _DEBUG
#define SHOWCONSOLE
#endif

// msgina.dll resources
#define GINA_DLL_NAME L"msgina.dll"
#define GINA_BMP_BRD 107
#define GINA_BMP_BRD_SMALL 101
#define GINA_BMP_BAR 103
// Not implemented in upstream CLH
//#define GINA_DLG_CAD 1400
//#define GINA_DLG_CAD_LOCKED 1900
#define GINA_DLG_USER_SELECT 1500
#define GINA_DLG_USER_SELECT_LOCKED 1950
#define GINA_DLG_SECURITY_CONTROL 1800
#define GINA_DLG_STATUS_VIEW 2450
#define GINA_DLG_CHANGE_PWD 1700
#define GINA_DLG_SHUTDOWN 2200
#define GINA_DLG_LOGOFF 2250

#define GINA_LARGE_BRD_HEIGHT 88
#define GINA_SMALL_BRD_HEIGHT 72
#define GINA_BAR_HEIGHT 5

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

	HBITMAP hBar;

	int ginaVersion;

	BOOL initedPreLogon;

	ginaConfig config;

	ginaManager();

	static ginaManager* Get();

	void LoadGina();
	void UnloadGina();

	void LoadBranding(HWND hDlg, BOOL isLarge, BOOL noBarAsImgCtrl = FALSE);

	void CloseAllDialogs();
	void PostThemeChange();
};