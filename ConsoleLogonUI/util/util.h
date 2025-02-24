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

static std::wstring GetDomainName()
{
	wchar_t* domainName = NULL;
	DWORD size = 0;
	GetComputerNameExW(ComputerNameDnsDomain, NULL, &size);
	domainName = new wchar_t[size];
	GetComputerNameExW(ComputerNameDnsDomain, domainName, &size);
	std::wstring domain = domainName;
	delete[] domainName;
	return domain;
}

static std::wstring GetLogonName()
{
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameW(username, &username_len))
    {
		return GetDomainName() + L"\\" + std::wstring(username);
    }
    return L"";
}

static std::wstring GetLogonTime()
{
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameW(username, &username_len))
    {
        LPUSER_INFO_2 pBuf = NULL;
        NET_API_STATUS nStatus = NetUserGetInfo(NULL, username, 2, (LPBYTE*)&pBuf);
        if (nStatus == NERR_Success)
        {
            time_t logonTime = static_cast<time_t>(pBuf->usri2_last_logon);
            std::wstring logonTimeStr = _wctime(&logonTime);
            NetApiBufferFree(pBuf);
            return logonTimeStr;
        }
    }
    return L"";
}

static void CenterWindow(HWND hWnd)
{
	RECT rc;
	GetWindowRect(hWnd, &rc);
	int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
	int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;
	SetWindowPos(hWnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}