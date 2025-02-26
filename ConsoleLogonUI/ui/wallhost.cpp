#pragma once
#include <windows.h>
#include "wallhost.h"

HANDLE wallHost::s_hWallHostProcess = NULL;

void wallHost::Create()
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { sizeof(si) };
	if (CreateProcess(L"WallpaperHost.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		s_hWallHostProcess = pi.hProcess;
	}
}

void wallHost::Destroy()
{
	if (s_hWallHostProcess)
	{
		TerminateProcess(s_hWallHostProcess, 0);
		CloseHandle(s_hWallHostProcess);
		s_hWallHostProcess = NULL;
	}
}