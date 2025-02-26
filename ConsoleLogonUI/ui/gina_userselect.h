#pragma once
#include "gina_manager.h"
#include <string>

struct SelectableUserOrCredentialControlWrapper
{
public:
    void* actualInstance;
    std::wstring text;
    HBITMAP pfp = 0;
    bool hastext = false;

    std::wstring GetText();

    void Press();
    bool isCredentialControl();
};

class ginaUserSelect
{
public:
	HWND hDlg;
	static ginaUserSelect* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static int CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

