#pragma once
#include <windows.h>

class wallHost
{
public:
	static HANDLE s_hWallHostProcess;
	static void Create();
	static void Destroy();
};