#include "util.h"
#include <wincodec.h>
#include <wincodecsdk.h>
#include <wtsapi32.h>
#include "winsta.h"

#pragma comment(lib, "wtsapi32.lib")

// https://github.com/aubymori/XPLogonUI/blob/f75e9e06f8266ddb92218fabf3cdd7b386233b30/XPLogonUI/util.cpp#L96

DWORD GetLoggedOnUserInfo(LPWSTR lpUsername, UINT cchUsernameMax, LPWSTR lpDomain, UINT cchDomainMax)
{
	if ((!lpUsername && !lpDomain)
		|| (lpUsername && !cchUsernameMax)
		|| (lpDomain && !cchDomainMax))
		return -1;

	DWORD sessionId = 0;
	WTS_SESSION_INFOW* sessions = nullptr;
	DWORD sessionCount = 0;
	if (!WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &sessions, &sessionCount))
		return -1;

	for (DWORD i = 0; i < sessionCount; i++)
	{
		if (sessions[i].State == WTSActive)
		{
			WTS_SESSION_INFOW* session = &sessions[i];
			sessionId = session->SessionId;

			DWORD bytesReturned = 0;
			if (lpUsername)
			{
				LPWSTR pszUsername = nullptr;
				if (!WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSUserName, &pszUsername, &bytesReturned))
				{
					WTSFreeMemory(sessions);
					return -1;
				}

				wcscpy_s(lpUsername, cchUsernameMax, pszUsername);
				WTSFreeMemory(pszUsername);
			}

			if (lpDomain)
			{
				LPWSTR pszDomain = nullptr;
				if (!WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSDomainName, &pszDomain, &bytesReturned))
				{
					WTSFreeMemory(sessions);
					return -1;
				}

				wcscpy_s(lpDomain, cchDomainMax, pszDomain);
				WTSFreeMemory(pszDomain);
			}
		}
	}

	WTSFreeMemory(sessions);
	return sessionId;
}

int GetLastLogonUser(LPWSTR lpUsername, UINT cchUsernameMax)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return -1;

    WCHAR szLastLoggedOnUser[512];
    DWORD dwSize = sizeof(szLastLoggedOnUser);
    if (RegQueryValueExW(hKey, L"LastLoggedOnUser", NULL, NULL, (LPBYTE)szLastLoggedOnUser, &dwSize) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return -1;
    }
    RegCloseKey(hKey);

    LPWSTR pszDomain = wcschr(szLastLoggedOnUser, L'\\');
	if (!pszDomain)
		return -1;

	*pszDomain = L'\0';
	wcscpy_s(lpUsername, cchUsernameMax, pszDomain + 1);

    return 0;
}

bool GetUserLogonTime(LPSYSTEMTIME lpSystemTime)
{
	if (!lpSystemTime)
		return false;

	static HMODULE hWinsta = LoadLibraryW(L"winsta.dll");

	static PWINSTATIONQUERYINFORMATIONW WinStationQueryInformationW
		= (PWINSTATIONQUERYINFORMATIONW)GetProcAddress(hWinsta, "WinStationQueryInformationW");
	if (!WinStationQueryInformationW)
	{
		//dbgprintf(L"Failed to get address of WinStationQueryInformationW");
		return false;
	}

	WINSTATIONINFORMATIONPRIVATEW wsinfo;
	ULONG dummy;
	if (!WinStationQueryInformationW(SERVERNAME_CURRENT, LOGONID_CURRENT, WinStationInformation, &wsinfo, sizeof(wsinfo), &dummy))
	{
		//dbgprintf(L"WinStationQueryInformationW failed");
		return false;
	}

	FILETIME logonTime;
	SYSTEMTIME universalTime;
	logonTime.dwLowDateTime = wsinfo.LogonTime.LowPart;
	logonTime.dwHighDateTime = wsinfo.LogonTime.HighPart;
	FileTimeToSystemTime(&logonTime, &universalTime);
	SystemTimeToTzSpecificLocalTime(nullptr, &universalTime, lpSystemTime);
	return true;
}

bool IsSystemUser(void)
{
	WCHAR szDomainName[256], szUserName[256];
	if (!GetLoggedOnUserInfo(szUserName, 256, szDomainName, 256))
		return true;
	return !szUserName[0] && !szDomainName[0];
}

bool IsFriendlyLogonUI(void)
{
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
		NULL,
		KEY_READ,
		&hKey
	))
		return true;

	DWORD dwResult = 0;
	DWORD cbData = sizeof(DWORD);
	RegQueryValueExW(
		hKey,
		L"dontdisplaylastusername",
		NULL,
		NULL,
		(LPBYTE)&dwResult,
		&cbData
	);
	RegCloseKey(hKey);
	return dwResult == 0;
}