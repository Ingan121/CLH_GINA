#pragma once
#include <windows.h>
#include "gina_manager.h"

#define IDC_STATUS_TEXT 2451

class ginaStatusView
{
public:
	HWND hDlg;
	static ginaStatusView* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};