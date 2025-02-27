#pragma once
#include <windows.h>
#include <string>
#include "gina_manager.h"

#define IDC_SHUTDOWN_ICON 22
#define IDC_SHUTDOWN_COMBO 2201
#define IDC_SHUTDOWN_DESC 2202
#define IDC_SHUTDOWN_OK 1
#define IDC_SHUTDOWN_CANCEL 2
#define IDC_SHUTDOWN_HELP 9

#define IDC_LOGOFF_ICON 21
#define IDC_LOGOFF_OK 1
#define IDC_LOGOFF_CANCEL 2

#define IDI_SHUTDOWN 22
#define IDI_LOGOFF 21

#define GINA_STR_SHUTDOWN 8000
#define GINA_STR_SHUTDOWN_DESC 8001
#define GINA_STR_RESTART 8002
#define GINA_STR_RESTART_DESC 8003
#define GINA_STR_SLEEP 8004
#define GINA_STR_SLEEP_DESC 8005
#define GINA_STR_HIBERNATE 8008
#define GINA_STR_HIBERNATE_DESC 8009
#define GINA_STR_LOGOFF 8010
#define GINA_STR_LOGOFF_DESC 8011

void ShowShutdownDialog();
void ShowLogoffDialog();

class ginaShutdownView
{
public:
	HWND hDlg;
	static ginaShutdownView* Get();
	static void Create(HWND parent = NULL);
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

class ginaLogoffView
{
public:
	HWND hDlg;
	static ginaLogoffView* Get();
	static void Create(HWND parent = NULL);
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};