#pragma once
#include <windows.h>
#include "gina_manager.h"

#define IDC_STATUS_TEXT 2451

class ginaStatusView
{
public:
	HWND hDlg;
	int barOffset = 0;
	static ginaStatusView* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void UpdateText();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};