//---------------------------------------------------------------------------


#pragma hdrstop

#include "stdafx.h"
#include "checklicense.h"
#include "md5.h"
#include <sys/stat.h>
#include <time.h>
#include <windows.h>

//---------------------------------------------------------------------------
// this function is modified to handel wide strings	(John Liu, 2011/05/08)
//
bool GetSystemDirTime(char *pcSysDirTime, int nBufLen)
{
	struct _stat buf;
	char szSystemDirPath[256];
	char *pcTime;
	bool bRet = false;

	//pcSysDirTime[0] = '\0';

	if (!GetWindowsDirectory((LPWSTR)szSystemDirPath, 256))
		goto err_out;

	if (0 != _wstat((const wchar_t *)szSystemDirPath, &buf ))
		goto err_out;

	pcTime = ctime(&buf.st_ctime);
	if (strlen(pcTime) > nBufLen)
		goto err_out;

	strcpy(pcSysDirTime, pcTime);

	bRet = true;

err_out:
	return bRet;
}
//---------------------------------------------------------------------------
string GetMachineCode()
{
	char szSysTime[256];
	if (!GetSystemDirTime(szSysTime, 256))
		return "";

	return md5(szSysTime);
}
//---------------------------------------------------------------------------
string GetSerial(string strMachineCode)
{
	return md5(strMachineCode + "NetExplorer8");
}
//---------------------------------------------------------------------------
bool CheckLicense(string strSerial)
{
	string strMachineCode = GetMachineCode();
	string strCmpMd5 = md5(strMachineCode + "NetExplorer8");

	return strCmpMd5 == strSerial;
}
//---------------------------------------------------------------------------

#pragma package(smart_init)
