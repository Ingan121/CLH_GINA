#pragma once
#include <windows.h>
#include <string>
#include "gina_manager.h"
#include <atomic>

#define IDC_CREDVIEW_USERNAME 1502
#define IDC_CREDVIEW_USERNAME_LABEL 1506
#define IDC_CREDVIEW_PASSWORD 1503
#define IDC_CREDVIEW_PASSWORD_LABEL 1507
#define IDC_CREDVIEW_DOMAIN 1504
#define IDC_CREDVIEW_DOMAIN_LABEL 1508
#define IDC_CREDVIEW_DIALUP 1505
#define IDC_CREDVIEW_SHUTDOWN 1501
#define IDC_CREDVIEW_OPTIONS 1514
#define IDC_CREDVIEW_LEGAL 2400
#define IDC_CREDVIEW_LANGUAGE 2406

#define IDC_CREDVIEW_LOCKED_ICON 2404
#define IDC_CREDVIEW_LOCKED_USERNAME_INFO 1952
#define IDC_CREDVIEW_LOCKED_USERNAME 1953
#define IDC_CREDVIEW_LOCKED_PASSWORD 1954
#define IDC_CREDVIEW_LOCKED_DOMAIN 1956
#define IDC_CREDVIEW_LOCKED_DOMAIN_LABEL 1957
#define IDC_CREDVIEW_LOCKED_OPTIONS 2401
#define IDC_CREDVIEW_LOCKED_LANGUAGE 2406

#define IDI_LOCKED 115

#define IDC_CHPW_USERNAME 1702
#define IDC_CHPW_USERNAME_LABEL 1701
#define IDC_CHPW_DOMAIN 1704
#define IDC_CHPW_DOMAIN_LABEL 1703
#define IDC_CHPW_OLD_PASSWORD 1706
#define IDC_CHPW_OLD_PASSWORD_LABEL 1705
#define IDC_CHPW_NEW_PASSWORD 1708
#define IDC_CHPW_NEW_PASSWORD_LABEL 1707
#define IDC_CHPW_CONFIRM_PASSWORD 1710
#define IDC_CHPW_CONFIRM_PASSWORD_LABEL 1709
#define IDC_CHPW_LANGUAGE 2406

#define GINA_STR_OPTBTN_EXPAND 1800
#define GINA_STR_OPTBTN_COLLAPSE 1801
#define GINA_STR_CREDVIEW_LOCKED_USERNAME_INFO 1500

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