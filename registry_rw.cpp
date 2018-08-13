//
// registry_rw.cpp
//
// the software key information is written in "Software\MainPath" of HKEY_CURRENT_USER directory
//
// Reminder: one can use "regedit" to read the content in registry
//

#include "stdafx.h"
#include "resource.h"
#include <windows.h>

#pragma comment(lib, "Advapi32.lib")	// tell the linker to link-in the specified library

//
// write data to registry
//
int SetKeyData(HKEY hRootKey, LPCWSTR subKey, DWORD dwType, LPWSTR valuename, LPBYTE data, DWORD cbData)
{
        HKEY hKey;

		if(RegCreateKey(hRootKey, subKey, &hKey) != ERROR_SUCCESS)
			return -1;
       
		if(RegSetValueEx(hKey, valuename, 0, dwType, data, cbData) != ERROR_SUCCESS)
        {
			RegCloseKey(hKey);
                return -1;
        }
       
		RegCloseKey(hKey);

        return 0;
}

//
// read data from registry
//
int GetKeyData(HKEY hRootKey, LPCWSTR subKey, LPCWSTR valuename, LPBYTE data, DWORD *cbData)
{
        HKEY hKey;
		int ret;

		if(RegOpenKeyEx(hRootKey, subKey, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
			return -1;
       
		if(ret = RegQueryValueEx(hKey, valuename, NULL, NULL, data, cbData) != ERROR_SUCCESS)
        {
			RegCloseKey(hKey);
                return -1;
        }
       
		RegCloseKey(hKey);

        return 0;
}

//
// read protection key from registry
//
int read_protection_key_from_registry(wchar_t *username, wchar_t *userorg, char *softwarekey)
{
	int ret, i;
	int dsize;
    wchar_t bfr1[BLOCK_SIZE], bfr2[BLOCK_SIZE];
	wchar_t bfr3[BLOCK_SIZE];
	
	dsize = BLOCK_SIZE;
    ret = GetKeyData(HKEY_CURRENT_USER, L"Software\\MainPath", L"User Name", (LPBYTE)bfr1, (DWORD *)&dsize);
	if (ret != 0) return -1;
	wcscpy(username, bfr1);

	dsize = BLOCK_SIZE;
    ret = GetKeyData(HKEY_CURRENT_USER, L"Software\\MainPath", L"User Organization", (LPBYTE)bfr2, (DWORD *)&dsize);
	if (ret != 0) return -1;
	wcscpy(userorg, bfr2);	

	dsize = BLOCK_SIZE;
    ret = GetKeyData(HKEY_CURRENT_USER, L"Software\\MainPath", L"SoftwareKey", (LPBYTE)bfr3, (DWORD *)&dsize);
	if (ret != 0) return -1;
	for (i = 0; i < dsize/sizeof(wchar_t); i++)
		softwarekey[i] = bfr3[i];

	return ret;
}


//
// wrtie the last usage time to registry
//
int write_last_usage_time(string last_usage_time)
{
	int i;
	int ret, slen;
	FILE *stream;

	unsigned char bfr1[BLOCK_SIZE];	// for first usage time

	for (i = 0; i < BLOCK_SIZE; i++)
		bfr1[i] = 0;

	for (i = 0; i < BLOCK_SIZE; i++)
	{
		if (last_usage_time[i] != '\0')
			bfr1[i] = last_usage_time[i];
		else
			break;
	}
	slen = i;

    ret = SetKeyData(HKEY_CURRENT_USER, L"Software\\MainPath", REG_SZ, L"Last Usage Time", (LPBYTE)bfr1, 1+slen);
	if (ret == 0) return -1;

	return 0;
}

//
// read last usage time from registry
//
int read_last_usage_time(char *last_usage_time)
{
	int ret, i;
	int dsize;
    char bfr1[BLOCK_SIZE];
	
	dsize = BLOCK_SIZE;
    ret = GetKeyData(HKEY_CURRENT_USER, L"Software\\MainPath", L"Last Usage Time", (LPBYTE)bfr1, (DWORD *)&dsize);
	if (ret != 0) return -1;
	for (i = 0; i < dsize; i++)
		last_usage_time[i] = bfr1[i];
	last_usage_time[i] = '\0';

	return ret;
}

