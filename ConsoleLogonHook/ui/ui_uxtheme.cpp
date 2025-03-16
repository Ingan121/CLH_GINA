#include <windows.h>
#include <Uxtheme.h>
#include "detours.h"
#include "ui_uxtheme.h"
#include <util/memory_man.h>

// Based on https://windhawk.net/mods/fix-basic-caption-text by aubymory

typedef int WINDOWPARTS, CLOSEBUTTONSTATES;

enum FRAMESTATES
{
    FS_ACTIVE = 0x1,
    FS_INACTIVE = 0x2
};

typedef struct _NCWNDMET
{
    BOOL fValid;
    BOOL fFrame;
    BOOL fSmallFrame;
    BOOL fMin;
    BOOL fMaxed;
    BOOL fFullMaxed;
    BOOL fDirtyRects;
    BOOL fCustomFrame;
    BOOL fCustom;
    DWORD dwStyle;
    DWORD dwExStyle;
    DWORD dwWindowStatus;
    DWORD dwStyleClass;
    WINDOWPARTS rgframeparts[4];
    WINDOWPARTS rgsizehitparts[4];
    FRAMESTATES framestate;
    HFONT hfCaption;
    COLORREF rgbCaption;
    SIZE sizeCaptionText;
    MARGINS CaptionMargins;
    int iMinButtonPart;
    int iMaxButtonPart;
    CLOSEBUTTONSTATES rawCloseBtnState;
    CLOSEBUTTONSTATES rawMinBtnState;
    CLOSEBUTTONSTATES rawMaxBtnState;
    CLOSEBUTTONSTATES rawHelpBtnState;
    int cyMenu;
    int cnMenuOffsetLeft;
    int cnMenuOffsetRight;
    int cnMenuOffsetTop;
    int cnBorders;
    RECT rcS0[26];
    RECT rcW0[26];
} NCWNDMET;

DWORD(__fastcall* _NCWNDMET__GetCaptionColor_orig)(NCWNDMET*, bool);
DWORD __fastcall _NCWNDMET__GetCaptionColor_hook(NCWNDMET* pThis, bool bDarkText)
{
    bool fInactive = (pThis->framestate == FS_INACTIVE);
    return GetSysColor(
        fInactive ? COLOR_INACTIVECAPTIONTEXT : COLOR_CAPTIONTEXT
    );
}

void uiUxTheme::InitHooks(uintptr_t baseaddress)
{
	_NCWNDMET__GetCaptionColor_orig = memory::FindPatternCached<decltype(_NCWNDMET__GetCaptionColor_orig)>("_NCWNDMET__GetCaptionColor", { "48 89 5C 24 10 55 56 57 41 56 41 57 48 8B" }, false, L"UxTheme.dll");
	Hook(_NCWNDMET__GetCaptionColor_orig, _NCWNDMET__GetCaptionColor_hook);
}