#include <windows.h>
#include <Uxtheme.h>
#include "detours.h"
#include "ui_uxtheme.h"
#include <util/memory_man.h>

enum FRAMESTATES
{
    FS_ACTIVE = 0x1,
    FS_INACTIVE = 0x2
};

HRESULT (WINAPI *GetThemeClass)(HTHEME hTheme, LPWSTR pszClassName, int cchClassName);

HRESULT (WINAPI *DrawThemeTextEx_orig)(
    HTHEME        hTheme,
    HDC           hdc,
    int           iPartId,
    int           iStateId,
    LPCWSTR       pszText,
    int           cchText,
    DWORD         dwTextFlags,
    LPRECT        pRect,
    const DTTOPTS *pOptions
);

HRESULT WINAPI DrawThemeTextEx_hook(
    HTHEME        hTheme,
    HDC           hdc,
    int           iPartId,
    int           iStateId,
    LPCWSTR       pszText,
    int           cchText,
    DWORD         dwTextFlags,
    LPRECT        pRect,
    const DTTOPTS *pOptions
)
{
    if (hTheme && hdc && pOptions
    && (iStateId == FS_ACTIVE || iStateId == FS_INACTIVE))
    {
        WCHAR szThemeClass[64];
        if (SUCCEEDED(GetThemeClass(hTheme, szThemeClass, ARRAYSIZE(szThemeClass)))
        && !_wcsicmp(szThemeClass, L"Window"))
        {
            ((DTTOPTS *)pOptions)->crText = GetSysColor((iStateId == FS_INACTIVE) ? COLOR_INACTIVECAPTIONTEXT : COLOR_CAPTIONTEXT);
        }
    }
    return DrawThemeTextEx_orig(hTheme, hdc, iPartId, iStateId, pszText, cchText, dwTextFlags, pRect, pOptions);
}

void uiUxTheme::InitHooks(uintptr_t baseaddress)
{
    HMODULE hUxTheme = GetModuleHandleW(L"UxTheme.dll");
    if (hUxTheme)
    { 
        *(void **)&GetThemeClass = (void *)GetProcAddress(hUxTheme, (LPCSTR)74);
        if (GetThemeClass)
        {
            *(void **)&DrawThemeTextEx_orig = (void *)GetProcAddress(hUxTheme, "DrawThemeTextEx");
            Hook(DrawThemeTextEx_orig, DrawThemeTextEx_hook);
        }
    }
}