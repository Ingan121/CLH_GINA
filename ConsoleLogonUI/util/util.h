#pragma once
#include <windows.h>
#include <string>
#include <iostream>
#include <Gdiplus.h>
#include <gdiplusheaders.h>
#include <gdiplusinit.h>
#include <atlbase.h>
#include <vector>
#include <lm.h>
#include <Uxtheme.h>
#include <dwmapi.h>
#include <thread>

#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "netapi32.lib")

static std::string ws2s(const std::wstring& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0);
    char* buf = new char[len];
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, buf, len, 0, 0);
    std::string r(buf);
    delete[] buf;
    return r;
}

static std::wstring s2ws(const std::string& s)
{
    const int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

static std::vector<std::string> split(std::string s, std::string delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

static std::vector<std::wstring> split(std::wstring s, std::wstring delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::wstring token;
    std::vector<std::wstring> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

static void CenterWindow(HWND hWnd)
{
	RECT rc;
	GetWindowRect(hWnd, &rc);
	int windowWidth = rc.right - rc.left;
	int windowHeight = rc.bottom - rc.top;
	int xPos = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
	int yPos = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 3;
	SetWindowPos(hWnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

static bool EnableShutdownPrivilege()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		return false;
	}
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	if (GetLastError() != ERROR_SUCCESS)
	{
		return false;
	}
	return true;
}

static void MakeWindowClassic(HWND hWnd)
{
	SetWindowTheme(hWnd, L" ", L" ");
    DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;
    DwmSetWindowAttribute(hWnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(DWMNCRENDERINGPOLICY));
	// Iterate over all child windows
	HWND hChild = GetWindow(hWnd, GW_CHILD);
	while (hChild != NULL)
	{
		SetWindowTheme(hChild, L" ", L" ");
		hChild = GetWindow(hChild, GW_HWNDNEXT);
	}
}

static void MakeWindowClassicAsync(const wchar_t* title)
{
	std::thread([=] {
		HWND hDlg = NULL;
		while (!hDlg)
		{
			hDlg = FindWindowExW(0, 0, L"#32770", title);
			Sleep(10);
		}
		MakeWindowClassic(hDlg);
	}).detach();
}

DWORD GetLoggedOnUserInfo(LPWSTR lpUsername, UINT cchUsernameMax, LPWSTR lpDomain, UINT cchDomainMax);
int GetLastLogonUser(LPWSTR lpUsername, UINT cchUsernameMax);
bool GetUserLogonTime(LPSYSTEMTIME lpSystemTime);
int GetConfigInt(LPCWSTR lpValueName, int defaultValue);
bool GetConfigString(LPCWSTR lpValueName, LPWSTR lpBuffer, DWORD dwBufferSize, LPCWSTR lpDefaultValue = NULL);
bool IsSystemUser(void);
bool IsFriendlyLogonUI(void);
bool GetUserSid(LPCWSTR lpUsername, LPWSTR lpSid, DWORD dwSidSize);
bool GetUserHomeDir(LPWSTR lpUsername, LPWSTR lpHomeDir, DWORD dwHomeDirSize);
LSTATUS GetUserRegHive(REGSAM samDesired = KEY_READ, PHKEY phkResult = NULL);
void ApplyUserColors();
void EmergencyRestart(void);