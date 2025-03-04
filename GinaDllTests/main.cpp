#include <windows.h>
#include "../ConsoleLogonUI/ui/gina_manager.h"

HMODULE g_hGinaDll = NULL;

void LoadImages(HWND hDlg, HINSTANCE hInstance, BOOL isLarge)
{
    RECT rect;
    GetWindowRect(hDlg, &rect);
    int dlgWidth = rect.right - rect.left;
    int dlgHeight = rect.bottom - rect.top;
    HBITMAP hBranding = LoadBitmapW(g_hGinaDll, MAKEINTRESOURCEW(isLarge ? GINA_BMP_BRD : GINA_BMP_BRD_SMALL));
    HBITMAP hBar = LoadBitmapW(g_hGinaDll, MAKEINTRESOURCEW(GINA_BMP_BAR));
    BITMAP bmp;
    GetObjectW(hBranding, sizeof(BITMAP), &bmp);
    int brdWidth = bmp.bmWidth;
#ifdef XP
    int brdX = (dlgWidth - brdWidth) / 2;
#else
    int brdX = 0;
#endif
    int topAreaHeight = isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT + GINA_BAR_HEIGHT;

    // Move all existing controls down
    HWND hChild = GetWindow(hDlg, GW_CHILD);
    while (hChild != NULL)
    {
        RECT childRect;
        GetWindowRect(hChild, &childRect);
        MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&childRect, 2);
        SetWindowPos(hChild, NULL, childRect.left, childRect.top + topAreaHeight, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }

    // Create branding and bar controls
    HWND hBrandingWnd = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, brdX, 0, brdWidth, isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT, hDlg, NULL, hInstance, NULL);
    HWND hBarWnd = CreateWindowExW(0, L"STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, 0, isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT, dlgWidth, GINA_BAR_HEIGHT, hDlg, NULL, hInstance, NULL);
    SendMessageW(hBrandingWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBranding);
    SendMessageW(hBarWnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBar);

    // Resize the bar to stretch to dlgWidth
    SetWindowPos(hBarWnd, NULL, 0, isLarge ? GINA_LARGE_BRD_HEIGHT : GINA_SMALL_BRD_HEIGHT, dlgWidth, GINA_BAR_HEIGHT, SWP_NOZORDER);

    // Resize the dialog
    SetWindowPos(hDlg, NULL, 0, 0, dlgWidth, dlgHeight + topAreaHeight, SWP_NOZORDER | SWP_NOMOVE);
}


/*
1500 DIALOGEX 6, 18, 274, 135
STYLE DS_SHELLFONT | DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION
CAPTION "Windows 로그온"
LANGUAGE LANG_KOREAN, SUBLANG_KOREAN
FONT 9, "굴림", FW_DONTCARE, FALSE, 129
{
   CONTROL "사용자 이름(&U):", 1506, STATIC, SS_LEFT | WS_CHILD | WS_VISIBLE | WS_GROUP, 6, 40, 63, 9
   CONTROL "", 1502, EDIT, ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 72, 38, 162, 12
   CONTROL "암호(&P):", 1507, STATIC, SS_LEFT | WS_CHILD | WS_VISIBLE | WS_GROUP, 6, 57, 43, 9
   CONTROL "", 1503, EDIT, ES_LEFT | ES_PASSWORD | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP, 72, 55, 162, 12
   CONTROL "로그온 대상(&L):", 1508, STATIC, SS_LEFT | WS_CHILD | WS_VISIBLE | WS_GROUP, 6, 74, 49, 8
   CONTROL "", 1504, COMBOBOX, CBS_DROPDOWNLIST | CBS_SORT | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP, 72, 72, 162, 144
   CONTROL "전화 접속 연결을 사용하여 로그온(&D)", 1505, BUTTON, BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP, 72, 90, 142, 10
   CONTROL "", 2406, STATIC, SS_ICON | WS_CHILD | WS_VISIBLE, 6, 109, 21, 20
   CONTROL "확인", 1, BUTTON, BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 72, 107, 40, 16
   CONTROL "취소", 2, BUTTON, BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 114, 107, 40, 16
   CONTROL "시스템 종료(&S)...", 1501, BUTTON, BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 156, 107, 65, 16
   CONTROL "옵션 >>", 1514, BUTTON, BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP, 223, 107, 47, 16
   CONTROL "Optional Legal Announcement", 2400, STATIC, SS_LEFT | WS_CHILD | WS_VISIBLE | WS_GROUP, 6, 6, 261, 22
}
*/

int CALLBACK DlgProcSecurityControl(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		RECT dlgRect;
		GetWindowRect(hWnd, &dlgRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dlgRect, 2);
		int dlgHeightToReduce = 0;
		int bottomBtnYToMove = 0;
		LoadImages(hWnd, GetModuleHandleW(NULL), TRUE);
		break;
	}
	}
	return 0;
}

int CALLBACK DlgProcUserSelect(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		RECT dlgRect;
		GetWindowRect(hWnd, &dlgRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dlgRect, 2);
		int dlgHeightToReduce = 0;
		int bottomBtnYToMove = 0;

		// Hide the legal announcement
		HWND hLegalAnnouncement = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_LEGAL));
		RECT legalRect;
		GetWindowRect(hLegalAnnouncement, &legalRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&legalRect, 2);
		dlgHeightToReduce = legalRect.bottom - legalRect.top;
		ShowWindow(hLegalAnnouncement, SW_HIDE);

		HWND hChild = GetWindow(hWnd, GW_CHILD);
		while (hChild != NULL)
		{
			if (hChild != hLegalAnnouncement)
			{
				RECT childRect;
				GetWindowRect(hChild, &childRect);
				MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&childRect, 2);
				if (childRect.top > 6)
				{
					SetWindowPos(hChild, NULL, childRect.left, childRect.top - legalRect.bottom, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
				}
			}
			hChild = GetWindow(hChild, GW_HWNDNEXT);
		}

		// Hide the domain chooser
		HWND hDomainChooser = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_DOMAIN));
		HWND hDomainChooserLabel = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_DOMAIN_LABEL));
		RECT domainRect;
		GetWindowRect(hDomainChooser, &domainRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&domainRect, 2);
		ShowWindow(hDomainChooser, SW_HIDE);
		ShowWindow(hDomainChooserLabel, SW_HIDE);
		dlgHeightToReduce += domainRect.bottom - domainRect.top + 8;
		bottomBtnYToMove = domainRect.bottom - domainRect.top + 8;

		// Hide the dial-up checkbox
		HWND hDialup = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_DIALUP));
		RECT dialupRect;
		GetWindowRect(hDialup, &dialupRect);
		MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dialupRect, 2);
		ShowWindow(hDialup, SW_HIDE);
		dlgHeightToReduce += dialupRect.bottom - dialupRect.top + 8;
		bottomBtnYToMove += dialupRect.bottom - dialupRect.top;

		// Move the OK, Cancel, Shutdown, Options, and language icon controls up
		HWND hOK = GetDlgItem(hWnd, IDC_OK);
		HWND hCancel = GetDlgItem(hWnd, IDC_CANCEL);
		HWND hShutdown = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_SHUTDOWN));
		HWND hOptions = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_OPTIONS));
		HWND hLanguageIcon = GetDlgItem(hWnd, GetRes(IDC_CREDVIEW_LANGUAGE));
		HWND controlsToMove[] = { hOK, hCancel, hShutdown, hOptions, hLanguageIcon };
		for (int i = 0; i < sizeof(controlsToMove) / sizeof(HWND); i++)
		{
			RECT controlRect;
			GetWindowRect(controlsToMove[i], &controlRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&controlRect, 2);
			SetWindowPos(controlsToMove[i], NULL, controlRect.left, controlRect.top - bottomBtnYToMove, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}

		// DIsable the Cancel button
		//EnableWindow(hCancel, FALSE);

		wchar_t optBtnStr[256];
		LoadStringW(g_hGinaDll, GINA_STR_OPTBTN_COLLAPSE, optBtnStr, 256);
		SetDlgItemTextW(hWnd, GetRes(IDC_CREDVIEW_OPTIONS), optBtnStr);

		SetWindowPos(hWnd, NULL, 0, 0, dlgRect.right - dlgRect.left, dlgRect.bottom - dlgRect.top - dlgHeightToReduce, SWP_NOZORDER | SWP_NOMOVE);

		// Load branding and bar images
		LoadImages(hWnd, GetModuleHandleW(NULL), TRUE);
		break;
	}
	case WM_COMMAND:
	{
		wchar_t msg[256];
		//wsprintfW(msg, L"WM_COMMAND, wParam = %d, lParam = %d", wParam, lParam);
		//MessageBoxW(hWnd, msg, L"Info", MB_OK | MB_ICONINFORMATION);
		if (LOWORD(wParam) == 1)
		{
			// OK button
			wchar_t username[256];
			wchar_t password[256];
			GetDlgItemTextW(hWnd, 1502, username, 256);
			GetDlgItemTextW(hWnd, 1503, password, 256);
			wsprintfW(msg, L"Username: %s, Password: %s", username, password);
			MessageBoxW(hWnd, msg, L"Info", MB_OK | MB_ICONINFORMATION);
		}
		else if (LOWORD(wParam) == 2)
		{
			// Cancel button
			EndDialog(hWnd, 0);
			PostQuitMessage(0); // Trigger exit process
		}
		else if (LOWORD(wParam) == 1501)
		{
			// Shutdown button
			MessageBoxW(hWnd, L"Shutdown button clicked", L"Info", MB_OK | MB_ICONINFORMATION);
		}
		else if (LOWORD(wParam) == 1514)
		{
			// Options button
			HWND hShutdown = GetDlgItem(hWnd, 1501);
			BOOL isShutdownVisible = IsWindowVisible(hShutdown);
			ShowWindow(hShutdown, isShutdownVisible ? SW_HIDE : SW_SHOW);
			RECT shutdownRect;
			GetWindowRect(hShutdown, &shutdownRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&shutdownRect, 2);
			HWND controlsToMove[] = {
				GetDlgItem(hWnd, 1),
				GetDlgItem(hWnd, 2)
			};
			for (int i = 0; i < sizeof(controlsToMove) / sizeof(HWND); i++)
			{
				RECT controlRect;
				GetWindowRect(controlsToMove[i], &controlRect);
				MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&controlRect, 2);
				SetWindowPos(controlsToMove[i], NULL, isShutdownVisible ? controlRect.left + shutdownRect.right - shutdownRect.left : controlRect.left - shutdownRect.right + shutdownRect.left, controlRect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			}
			// Update the button string
			wchar_t optBtnStr[256];
			LoadStringW(g_hGinaDll, isShutdownVisible ? GINA_STR_OPTBTN_EXPAND : GINA_STR_OPTBTN_COLLAPSE, optBtnStr, 256);
			SetDlgItemTextW(hWnd, 1514, optBtnStr);
		}
		break;
	}
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		int origBottom = rect.bottom;
		rect.bottom = GINA_LARGE_BRD_HEIGHT;
		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		rect.bottom = origBottom;
		rect.top = GINA_LARGE_BRD_HEIGHT + GINA_BAR_HEIGHT;
		COLORREF btnFace;
		btnFace = GetSysColor(COLOR_BTNFACE);
		hBrush = CreateSolidBrush(btnFace);
		FillRect(hdc, &rect, hBrush);
		DeleteObject(hBrush);
		return 1;
		break;
	}
	case WM_CLOSE:
	{
		EndDialog(hWnd, 0);
		PostQuitMessage(0); // Trigger exit process
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0); // Trigger exit process
		break;
	}
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	g_hGinaDll = LoadLibraryExW(GINA_DLL_NAME, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if (g_hGinaDll)
	{
		if (strcmp(lpCmdLine, "security") == 0)
		{
			HWND hDlgSecurityControl = CreateDialogW(g_hGinaDll, MAKEINTRESOURCEW(GINA_DLG_SECURITY_CONTROL), NULL, (DLGPROC)DlgProcSecurityControl);
			if (hDlgSecurityControl)
			{
				ShowWindow(hDlgSecurityControl, SW_SHOW);
				UpdateWindow(hDlgSecurityControl);
				MSG msg;
				while (GetMessageW(&msg, NULL, 0, 0))
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
			}
			else {
				wchar_t msg[256];
				DWORD err = GetLastError();
				wsprintfW(msg, L"Failed to create security control dialog, GetLastError() returned %d", err);
				MessageBoxW(NULL, msg, L"Error", MB_OK | MB_ICONERROR);
			}
		}
		else {
			HWND hDlgCredView = CreateDialogW(g_hGinaDll, MAKEINTRESOURCEW(GINA_DLG_USER_SELECT), NULL, (DLGPROC)DlgProcUserSelect);
			if (hDlgCredView)
			{
				ShowWindow(hDlgCredView, SW_SHOW);
				UpdateWindow(hDlgCredView);
				MSG msg;
				while (GetMessageW(&msg, NULL, 0, 0))
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
			}
			else {
				wchar_t msg[256];
				DWORD err = GetLastError();
				wsprintfW(msg, L"Failed to create CredView dialog, GetLastError() returned %d", err);
				MessageBoxW(NULL, msg, L"Error", MB_OK | MB_ICONERROR);
			}
		}
	}
	else {
		MessageBoxW(NULL, L"Failed to load msgina.dll", L"Error", MB_OK | MB_ICONERROR);
	}
	MessageBoxW(NULL, L"Exiting...", L"Info", MB_OK | MB_ICONINFORMATION);


	return 0;
}