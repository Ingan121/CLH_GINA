#pragma once
#include <windows.h>
#include "gina_messageview.h"
#include "gina_userselect.h"
#include "gina_selectedcredentialview.h"
#include "gina_statusview.h"
#include "gina_securitycontrol.h"

// Win2K KR msgina.dll resources
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

class ginaManager
{
public:
	HINSTANCE hInstance;
	HMODULE hGinaDll;

	HBITMAP hBar;

	ginaManager();

	static ginaManager* Get();

	void LoadGina();
	void UnloadGina();

	void LoadBranding(HWND hDlg, BOOL isLarge, BOOL noBarAsImgCtrl = FALSE);

	void CloseAllDialogs();
};