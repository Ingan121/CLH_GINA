#pragma once
#include <windows.h>
#include <string>
#include "gina_manager.h"
#include <atomic>

#define IDC_CREDVIEW_USERNAME 1453
#define IDC_CREDVIEW_USERNAME_LABEL 1457
#define IDC_CREDVIEW_PASSWORD 1454
#define IDC_CREDVIEW_PASSWORD_LABEL 1458
#define IDC_CREDVIEW_DOMAIN 1455
#define IDC_CREDVIEW_DOMAIN_LABEL 1459
#define IDC_CREDVIEW_DIALUP 1456
#define IDC_CREDVIEW_SHUTDOWN 1452
#define IDC_CREDVIEW_HELP 1451
#define IDC_CREDVIEW_ICON 1461

#define IDC_CREDVIEW_LOCKED_ICON 1851
#define IDC_CREDVIEW_LOCKED_USERNAME_INFO 1853
#define IDC_CREDVIEW_LOCKED_USERNAME 1854
#define IDC_CREDVIEW_LOCKED_PASSWORD 1855
#define IDC_CREDVIEW_LOCKED_DOMAIN 1856
#define IDC_CREDVIEW_LOCKED_DOMAIN_LABEL 1860

#define IDI_LOGON 15
#define IDI_LOCKED 20

#define IDC_CHPW_USERNAME 1552
#define IDC_CHPW_USERNAME_LABEL 1551
#define IDC_CHPW_DOMAIN 1554
#define IDC_CHPW_DOMAIN_LABEL 1553
#define IDC_CHPW_OLD_PASSWORD 1556
#define IDC_CHPW_OLD_PASSWORD_LABEL 1555
#define IDC_CHPW_NEW_PASSWORD 1558
#define IDC_CHPW_NEW_PASSWORD_LABEL 1557
#define IDC_CHPW_CONFIRM_PASSWORD 1560
#define IDC_CHPW_CONFIRM_PASSWORD_LABEL 1559
#define IDC_CHPW_HELP 1561

#define GINA_STR_CREDVIEW_LOCKED_USERNAME_INFO 1500
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