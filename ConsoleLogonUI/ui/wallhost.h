#pragma once
#include <windows.h>
#include "gina_manager.h"

void InitWallHost();

class wallHost
{
public:
	HWND hWin;
	static wallHost* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};