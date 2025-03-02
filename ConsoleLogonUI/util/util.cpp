#include "util.h"
#include <wincodec.h>
#include <wincodecsdk.h>
#include <wtsapi32.h>
#include <sddl.h>
#include "winsta.h"

#pragma comment(lib, "wtsapi32.lib")

// Some functions are from https://github.com/aubymori/XPLogonUI/blob/f75e9e06f8266ddb92218fabf3cdd7b386233b30/XPLogonUI/util.cpp#L96

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

int GetConfigInt(LPCWSTR lpValueName, int defaultValue)
{
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI\\CLH_GINA", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return defaultValue;

	DWORD dwType, dwData, dwSize = sizeof(dwData);
	if (RegQueryValueExW(hKey, lpValueName, NULL, &dwType, (LPBYTE)&dwData, &dwSize) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return defaultValue;
	}

	RegCloseKey(hKey);
	return dwData;
}

bool GetConfigString(LPCWSTR lpValueName, LPWSTR lpBuffer, DWORD dwBufferSize, LPCWSTR lpDefaultValue)
{
	if (!lpBuffer || !dwBufferSize)
		return false;
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI\\CLH_GINA", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return false;
	DWORD dwType, dwSize = dwBufferSize;
	if (RegQueryValueExW(hKey, lpValueName, NULL, &dwType, (LPBYTE)lpBuffer, &dwSize) != ERROR_SUCCESS)
	{
		if (lpDefaultValue)
		{
			wcscpy_s(lpBuffer, dwBufferSize, lpDefaultValue);
			RegCloseKey(hKey);
			return true;
		}
		RegCloseKey(hKey);
		return false;
	}
	RegCloseKey(hKey);
	return true;
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

bool GetUserSid(LPCWSTR lpUsername, LPWSTR lpSid, DWORD dwSidSize)
{
	if (!lpUsername || !lpSid || !dwSidSize)
		return false;

	SID_NAME_USE SidType;
	DWORD dwSidBufferSize = 0;
	DWORD dwDomainBufferSize = 0;

	// First call to LookupAccountName to get the buffer sizes.
	LookupAccountNameW(NULL, lpUsername, NULL, &dwSidBufferSize, NULL, &dwDomainBufferSize, &SidType);

	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return false;

	PSID pSid = (PSID)malloc(dwSidBufferSize);
	LPWSTR lpDomain = (LPWSTR)malloc(dwDomainBufferSize * sizeof(WCHAR));

	if (!pSid || !lpDomain)
	{
		free(pSid);
		free(lpDomain);
		return false;
	}

	// Second call to LookupAccountName to get the actual SID and domain name.
	if (!LookupAccountNameW(NULL, lpUsername, pSid, &dwSidBufferSize, lpDomain, &dwDomainBufferSize, &SidType))
	{
		free(pSid);
		free(lpDomain);
		return false;
	}

	// Convert the SID to a string.
	LPWSTR pszSid = NULL;
	if (!ConvertSidToStringSidW(pSid, &pszSid))
	{
		free(pSid);
		free(lpDomain);
		return false;
	}

	// Copy the SID string to the output buffer.
	wcsncpy_s(lpSid, dwSidSize, pszSid, _TRUNCATE);

	free(pSid);
	free(lpDomain);
	return true;
}

bool GetUserHomeDir(LPWSTR lpUsername, LPWSTR lpHomeDir, DWORD dwHomeDirSize)
{
	if (!lpUsername || !lpHomeDir || !dwHomeDirSize)
		return false;
	WCHAR szSid[256], szKey[256];
	if (!GetUserSid(lpUsername, szSid, 256))
		return false;
	wsprintfW(szKey, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s", szSid);
	HKEY hKey;
	if (ERROR_SUCCESS != RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		szKey,
		NULL,
		KEY_READ,
		&hKey
	))
	{
		return false;
	}
	DWORD cbData = dwHomeDirSize;
	if (ERROR_SUCCESS != RegQueryValueExW(
		hKey,
		L"ProfileImagePath",
		NULL,
		NULL,
		(LPBYTE)lpHomeDir,
		&cbData
	))
	{
		RegCloseKey(hKey);
		return false;
	}
	RegCloseKey(hKey);
	return true;
}

LSTATUS GetUserRegHive(REGSAM samDesired, PHKEY phkResult)
{
	if (!phkResult)
		return ERROR_INVALID_PARAMETER;
	if (IsSystemUser())
		return RegOpenKeyExW(HKEY_CURRENT_USER, NULL, 0, samDesired, phkResult);
	WCHAR lpUsername[256], szSid[256];
	if (!GetLoggedOnUserInfo(lpUsername, 256, NULL, 0))
		return ERROR_INVALID_PARAMETER;
	if (!GetUserSid(lpUsername, szSid, 256))
		return ERROR_INVALID_PARAMETER;
	return RegOpenKeyExW(HKEY_USERS, szSid, 0, samDesired, phkResult);
}

void EmergencyRestart()
{
	typedef ULONG32(WINAPI* lpNtShutdownSystem)(int Action);
	typedef ULONG32(WINAPI* lpNtSetSystemPowerState)(IN POWER_ACTION SystemAction, IN SYSTEM_POWER_STATE MinSystemState, IN ULONG32 Flags);

	HMODULE hNtDll;
	lpNtSetSystemPowerState NtSetSystemPowerState;
	lpNtShutdownSystem NtShutdownSystem;

	// Load ntdll.dll
	if ((hNtDll = LoadLibrary(L"ntdll.dll")) == 0) {
		return;
	}

	// Get functions
	NtShutdownSystem = (lpNtShutdownSystem)GetProcAddress(hNtDll, "NtShutdownSystem");
	if (NtShutdownSystem == NULL) {
		return;
	}
	NtSetSystemPowerState = (lpNtSetSystemPowerState)GetProcAddress(hNtDll, "NtSetSystemPowerState");
	if (NtSetSystemPowerState == NULL) {
		return;
	}

	if (!EnableShutdownPrivilege())
		return;

	NtShutdownSystem(1); // 1 = ShutdownReboot
	NtSetSystemPowerState((POWER_ACTION)PowerSystemShutdown, (SYSTEM_POWER_STATE)PowerActionShutdownReset, 0);
}