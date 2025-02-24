#pragma once
#include "gina_manager.h"
#include "gina_messageview.h"
#include <string>

struct MessageOptionControlWrapper
{
    void* actualInstance;
    int optionflag;

    void Press();

    std::wstring GetText();
};

class messageViewDlg
{
public:
    HWND hDlg;
	std::wstring message;
	static messageViewDlg* Get();

	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};