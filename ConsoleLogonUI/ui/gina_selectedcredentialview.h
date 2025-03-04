#pragma once
#include <windows.h>
#include <string>
#include "gina_manager.h"
#include <atomic>

#define IDC_CREDVIEW_USERNAME 1453, 1502
#define IDC_CREDVIEW_USERNAME_LABEL 1457, 1506
#define IDC_CREDVIEW_PASSWORD 1454, 1503
#define IDC_CREDVIEW_PASSWORD_LABEL 1458, 1507
#define IDC_CREDVIEW_DOMAIN 1455, 1504
#define IDC_CREDVIEW_DOMAIN_LABEL 1459, 1508
#define IDC_CREDVIEW_DIALUP 1456, 1505
#define IDC_CREDVIEW_SHUTDOWN 1452, 1501
#define IDC_CREDVIEW_HELP 1451 // nt4
#define IDC_CREDVIEW_ICON 1461 // nt4
#define IDC_CREDVIEW_OPTIONS 1514 // 2k, xp
#define IDC_CREDVIEW_LEGAL 2400 // 2k, xp
#define IDC_CREDVIEW_LANGUAGE 2406 // 2k, xp
#define IDC_CREDVIEW_XP_LOCKED_GROUP 2421 // xp
#define IDC_CREDVIEW_XP_LOCKED_INFO 1852 // xp
#define IDC_CREDVIEW_XP_LOCKED_USERNAME_INFO 1952 // xp
#define IDC_CREDVIEW_XP_LOCKED_ICON 2404 // xp

#define IDC_CREDVIEW_LOCKED_ICON 1851, 1951
#define IDC_CREDVIEW_LOCKED_USERNAME_INFO 1853, 1952
#define IDC_CREDVIEW_LOCKED_USERNAME 1854, 1953
#define IDC_CREDVIEW_LOCKED_PASSWORD 1855, 1954
#define IDC_CREDVIEW_LOCKED_DOMAIN 1856, 1956
#define IDC_CREDVIEW_LOCKED_DOMAIN_LABEL 1860, 1957
#define IDC_CREDVIEW_LOCKED_OPTIONS 2401 // 2k, xp
#define IDC_CREDVIEW_LOCKED_LANGUAGE 2406 // 2k, xp

#define IDI_LOGON 15 // nt4
#define IDI_LOCKED 20, 115

#define IDC_CHPW_USERNAME 1552, 1702
#define IDC_CHPW_USERNAME_LABEL 1551, 1701
#define IDC_CHPW_DOMAIN 1554, 1704
#define IDC_CHPW_DOMAIN_LABEL 1553, 1703
#define IDC_CHPW_OLD_PASSWORD 1556, 1706
#define IDC_CHPW_OLD_PASSWORD_LABEL 1555, 1705
#define IDC_CHPW_NEW_PASSWORD 1558, 1708
#define IDC_CHPW_NEW_PASSWORD_LABEL 1557, 1707
#define IDC_CHPW_CONFIRM_PASSWORD 1560, 1710
#define IDC_CHPW_CONFIRM_PASSWORD_LABEL 1559, 1709
#define IDC_CHPW_HELP 1561 // nt4
#define IDC_CHPW_LANGUAGE 2406 // 2k, xp
#define IDC_CHPW_BACKUP 2509 // XP only

#define GINA_STR_OPTBTN_EXPAND 1800
#define GINA_STR_OPTBTN_COLLAPSE 1801
#define GINA_STR_CREDVIEW_LOCKED_USERNAME_INFO 1500
#define GINA_STR_THIS_COMPUTER 1851
#define GINA_STR_CHPW_TITLE 1509
#define GINA_STR_CHPW_CONFIRM_MISMATCH 1512

struct EditControlWrapper
{
    void* actualInstance;
    std::string inputBuffer = ""; // for imgui
    std::string fieldNameCache = "";

    std::wstring GetFieldName();
    std::wstring GetInputtedText();
    void SetInputtedText(std::wstring input);
    bool isVisible();
};

class ginaSelectedCredentialView
{
public:
	HWND hDlg;
	std::atomic <bool> isActive;
	static ginaSelectedCredentialView* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

class ginaSelectedCredentialViewLocked
{
public:
	HWND hDlg;
	std::atomic <bool> isActive;
	static ginaSelectedCredentialViewLocked* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

class ginaChangePwdView
{
public:
	HWND hDlg;
	std::atomic <bool> isActive;
	static ginaChangePwdView* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};