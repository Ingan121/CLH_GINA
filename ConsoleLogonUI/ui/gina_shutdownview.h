#pragma once
#include <windows.h>
#include <string>
#include "gina_manager.h"

#define IDC_SHUTDOWN_ICON 102
#define IDC_SHUTDOWN_SHUTDOWN 300
#define IDC_SHUTDOWN_RESTART 1202
#define IDC_SHUTDOWN_POWEROFF 1204
#define IDC_SHUTDOWN_OK 211
#define IDC_SHUTDOWN_CANCEL 212

#define SHELL32_INFO 1001

void ShowShutdownDialog(HWND parent = NULL);
void ShowLogoffDialog(HWND parent = NULL);

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