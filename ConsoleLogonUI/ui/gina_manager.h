#pragma once
#include <windows.h>

//#define SHOWCONSOLE

#ifdef _DEBUG
#define SHOWCONSOLE
#endif

#define WM_NCUAHDRAWCAPTION 0x00AE

// msgina.dll resources
#define GINA_DLL_NAME L"msgina.dll"
#define GINA_BMP_BRD 107
#define GINA_BMP_BRD_SMALL 101
#define GINA_BMP_BAR 103

#define GINA_STR_BUILT_ON_NT           8100
#define GINA_STR_BUILT_ON_NT_FONT      8101
#define GINA_STR_BUILT_ON_NT_FONT_SIZE 8102

// Not implemented in upstream CLH
//#define GINA_DLG_CAD 1400
//#define GINA_DLG_CAD_LOCKED 1900
#define GINA_DLG_USER_SELECT 1450, 1500
#define GINA_DLG_USER_SELECT_LOCKED 1850, 1950
#define GINA_DLG_SECURITY_CONTROL 1650, 1800
#define GINA_DLG_STATUS_VIEW 800, 2450
#define GINA_DLG_CHANGE_PWD 1550, 1700
#define GINA_DLG_SHUTDOWN 1200, 2200
#define GINA_DLG_LOGOFF 1203, 2250

// NT4 only help dialogs
#define GINA_DLG_USER_SELECT_HLP 1950
#define GINA_DLG_CHANGE_PWD_HLP 508

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

enum WINDOWTHEME
{
	WT_COMPOSITED = 0,
	WT_CLASSIC,
	WT_UXTHEME,
	WT_COUNT
};

struct ginaConfig {
	BOOL showConsole;
	WINDOWTHEME classicTheme;
	BOOL hideStatusView;
	BOOL hideCapsLockBalloon;
};

class ginaManager
{
public:
	HINSTANCE hInstance;
	HMODULE hGinaDll;

	HBITMAP  hLargeBranding;
	HBITMAP  hSmallBranding;
	HBITMAP  hBar;
	SIZE     _sizeLargeBrand;
	SIZE     _sizeSmallBrand;
	SIZE     _sizeBar;
	COLORREF _crBrandBG;
	BOOL     _fCenterBrand;

	HFONT _hfontBuiltOnNT;
	HFONT _hfontBuiltOnNTBold;
	WCHAR _szBuiltOnNT[MAX_PATH];

	int ginaVersion;

	BOOL initedPreLogon;

	ginaConfig config;

	ginaManager();

	static ginaManager* Get();

	void LoadGina();
	void UnloadGina();

	void MoveChildrenForBranding(HWND hwnd, BOOL fLarge);
	void PaintBranding(HDC hdc, RECT *prc, BOOL fLarge = FALSE, int iBarOffset = 0);

	void CloseAllDialogs();
	void PostThemeChange();
};

int GetRes(int nt4, int xp = -1);

int CALLBACK HelpDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);