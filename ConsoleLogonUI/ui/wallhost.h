#pragma once
#include <windows.h>
#include "gina_manager.h"

#define WP_STYLE_CENTER 0
#define WP_STYLE_TILE 1
#define WP_STYLE_STRETCH 2
#define WP_STYLE_FIT 3
#define WP_STYLE_FILL 4
#define WP_STYLE_SPAN 5

void InitWallHost();

class wallHost
{
public:
	HWND hWnd;
	static wallHost* Get();
	static void Create();
	static void Destroy();
	static void Show();
	static void Hide();
	static void BeginMessageLoop();
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};