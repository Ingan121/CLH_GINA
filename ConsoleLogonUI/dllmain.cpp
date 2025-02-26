// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include "ui/gina_manager.h"
#include "util/interop.h"
#include "ui/wallhost.h"

extern "C" __declspec(dllexport) void InitUI()
{
    external::InitExternal();
	ginaManager::Get()->LoadGina();
	wallHost::Create();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		ginaManager::Get()->hInstance = hModule;
        break;

    case DLL_PROCESS_DETACH:
		ginaManager::Get()->UnloadGina();
		wallHost::Destroy();
        break;
    }
    return TRUE;
}

