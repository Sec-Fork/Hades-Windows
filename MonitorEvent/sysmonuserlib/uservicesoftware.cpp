#include <Windows.h>
#include <string>
#include <msi.h>
#pragma comment(lib, "msi.lib")
#include "uservicesoftware.h"

#include <sysinfo.h>
using namespace std;

#define MAX_SERVICE_SIZE 1024 * 64
#define MAX_QUERY_SIZE   1024 * 8
static const HKEY RootKey = HKEY_LOCAL_MACHINE;
static const LPCTSTR lpSubKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
static const LPCTSTR lpSubKey64 = L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

UServerSoftware::UServerSoftware()
{
}
UServerSoftware::~UServerSoftware()
{
}

const DWORD UServerSoftware::EnumService(LPVOID outbuf)
{
	if (!outbuf)
		return FALSE;

	PUServicesNode const serinfo = (PUServicesNode)outbuf;
	DWORD count = 0;

	do {
		SC_HANDLE SCMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (SCMan == NULL) {
			break;
		}
		LPENUM_SERVICE_STATUS service_status = nullptr;
		DWORD cbBytesNeeded = NULL;
		DWORD ServicesReturned = NULL;
		DWORD ResumeHandle = NULL;

		service_status = (LPENUM_SERVICE_STATUS)LocalAlloc(LPTR, MAX_SERVICE_SIZE);
		if (!service_status)
			break;

		BOOL ESS = EnumServicesStatus(SCMan,						// ���
			SERVICE_WIN32,                                          // ��������
			SERVICE_STATE_ALL,                                      // �����״̬
			(LPENUM_SERVICE_STATUS)service_status,                  // ���������ϵͳ����Ľṹ
			MAX_SERVICE_SIZE,                                       // �ṹ�Ĵ�С
			&cbBytesNeeded,                                         // ������������շ�������ķ���
			&ServicesReturned,                                      // ������������շ��ط��������
			&ResumeHandle);                                         // ���������������һ�ε��ñ���Ϊ0������Ϊ0����ɹ�
		if (ESS == NULL) {
			break;
		}
		for (int i = 0; i < static_cast<int>(ServicesReturned); i++) {

			lstrcpyW(serinfo[count].lpDisplayName, service_status[i].lpDisplayName);
			
			switch (service_status[i].ServiceStatus.dwCurrentState) { // ����״̬
			case SERVICE_CONTINUE_PENDING:
				strcpy_s(serinfo[count].dwCurrentState, "CONTINUE_PENDING");
				break;
			case SERVICE_PAUSE_PENDING:
				strcpy_s(serinfo[count].dwCurrentState, "PAUSE_PENDING");
				break;
			case SERVICE_PAUSED:
				strcpy_s(serinfo[count].dwCurrentState, "PAUSED");
				break;
			case SERVICE_RUNNING:
				strcpy_s(serinfo[count].dwCurrentState, "RUNNING");
				break;
			case SERVICE_START_PENDING:
				strcpy_s(serinfo[count].dwCurrentState, "START_PENDING");
				break;
			case SERVICE_STOPPED:
				strcpy_s(serinfo[count].dwCurrentState, "STOPPED");
				break;
			default:
				strcpy_s(serinfo[count].dwCurrentState, "UNKNOWN");
				break;
			}
			LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;          // ������ϸ��Ϣ�ṹ
			SC_HANDLE service_curren = NULL;                        // ��ǰ�ķ�����
			LPSERVICE_DESCRIPTION lpqscBuf2 = NULL;					// ����������Ϣ
			service_curren = OpenService(SCMan, service_status[i].lpServiceName, SERVICE_QUERY_CONFIG);        // �򿪵�ǰ����
			lpServiceConfig = (LPQUERY_SERVICE_CONFIG)LocalAlloc(LPTR, MAX_QUERY_SIZE);                        // �����ڴ棬 ���Ϊ8kb 

			if (NULL == QueryServiceConfig(service_curren, lpServiceConfig, MAX_QUERY_SIZE, &ResumeHandle)) {
				break;
			}

			lstrcpyW(serinfo[count].lpServiceName, service_status[i].lpServiceName);
			lstrcpyW(serinfo[count].lpBinaryPathName, lpServiceConfig->lpBinaryPathName);
			// fwprintf(g_pFile, L"Path: %s\n", lpServiceConfig->lpBinaryPathName);

			DWORD dwNeeded = 0;
			if (QueryServiceConfig2(service_curren, SERVICE_CONFIG_DESCRIPTION, NULL, 0,
				&dwNeeded) == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				lpqscBuf2 = (LPSERVICE_DESCRIPTION)LocalAlloc(LPTR, MAX_QUERY_SIZE);
				if (QueryServiceConfig2(service_curren, SERVICE_CONFIG_DESCRIPTION,
					(BYTE*)lpqscBuf2, dwNeeded, &dwNeeded))
				{
					if (lstrlenW(lpqscBuf2->lpDescription))
						lstrcpynW(serinfo[count].lpDescription, lpqscBuf2->lpDescription, MAX_PATH);
				}
				if (lpqscBuf2)
				{
					LocalFree(lpqscBuf2);
					lpqscBuf2 = NULL;
				}
			}
			count++;
			CloseServiceHandle(service_curren);
		}
		CloseServiceHandle(SCMan);
		if (service_status)
			LocalFree(service_status);
	} while (0);
	return count;
}
const DWORD UServerSoftware::EnumSoftware(LPVOID outbuf)
{
	if (!outbuf)
		return 0;

	PUSOFTINFO const softwareinfo = (PUSOFTINFO)outbuf;
	if (!softwareinfo)
		return 0;

	HKEY hkResult = 0;
	USOFTINFO SoftInfo = { 0 };
	FILETIME ftLastWriteTimeA;					// last write time 
	// 1. ��һ���Ѵ��ڵ�ע����
	LSTATUS retCode = RegOpenKeyEx(RootKey, lpSubKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_32KEY | KEY_QUERY_VALUE, &hkResult);
	if (retCode != ERROR_SUCCESS)
		return false;

	// 2. �����Сע���
	// TCHAR    achKey[MAX_PATH] = {};			// buffer for subkey name
	DWORD    cbName = 0;						// size of name string 
	TCHAR    achClass[MAX_PATH] = TEXT("");		// buffer for class name 
	DWORD    cchClassName = MAX_PATH;			// size of class string 
	DWORD    cSubKeys = 0;						// number of subkeys 
	DWORD    cbMaxSubKey;						// longest subkey size 
	DWORD    cchMaxClass;						// longest class string 
	DWORD    cValues;							// number of values for key 
	DWORD    cchMaxValue;						// longest value name 
	DWORD    cbMaxValueData;					// longest value data 
	DWORD    cbSecurityDescriptor;				// size of security descriptor 
	FILETIME ftLastWriteTime;					// last write time 
	DWORD cchValue = MAX_PATH;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hkResult,                // key handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 
	if (retCode != ERROR_SUCCESS)
	{
		if (hkResult)
			RegCloseKey(hkResult);
		return 0;
	}

	// 3. ѭ������UninstallĿ¼�µ��ӽ�
	int nCount = 1;
	DWORD dwIndex = 0; DWORD dwKeyLen = 255; DWORD dwType = 0;
	WCHAR szNewKeyName[MAX_PATH] = { 0, }; WCHAR strMidReg[MAX_PATH] = { 0, };
	DWORD dwNamLen = 255; DWORD countnumber = 0;
	HKEY hkValueKey = 0; LSTATUS lRet = ERROR_SUCCESS;
	for (SIZE_T i = 0; i < cSubKeys; i++)
	{
		dwKeyLen = MAX_PATH;
		lRet = RegEnumKeyEx(hkResult, dwIndex, szNewKeyName, &dwKeyLen, 0, NULL, NULL, &ftLastWriteTimeA);
		if (lRet != ERROR_SUCCESS)
			continue;

		// 2.1 ͨ���õ��ӽ�������������ϳ��µ��ӽ�·��
		swprintf_s(strMidReg, L"%s%s%s", lpSubKey, L"\\", szNewKeyName);
		// 2.2 ���µ��ӽ�, ��ȡ����
		lRet = RegOpenKeyEx(RootKey, strMidReg, 0, KEY_QUERY_VALUE, &hkValueKey);
		if (lRet != ERROR_SUCCESS)
			continue;

		SoftInfo.clear();
		// ����
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"DisplayName", 0, &dwType, (LPBYTE)SoftInfo.szSoftName, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].szSoftName, SoftInfo.szSoftName);
		// �汾��
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"VersionNumber", 0, &dwType, (LPBYTE)SoftInfo.szSoftVer, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].szSoftVer, SoftInfo.szSoftVer);
		// ��װʱ��
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"Time", 0, &dwType, (LPBYTE)SoftInfo.szSoftDate, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].szSoftDate, SoftInfo.szSoftDate);
		// ��С
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"Sizeof", 0, &dwType, (LPBYTE)SoftInfo.szSoftSize, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].szSoftSize, SoftInfo.szSoftSize);
		// ������
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"Sizeof", 0, &dwType, (LPBYTE)SoftInfo.strSoftVenRel, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].strSoftVenRel, SoftInfo.strSoftVenRel);
		// ж��·��
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"UninstallString", 0, &dwType, (LPBYTE)SoftInfo.strSoftUniPath, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].strSoftUniPath, SoftInfo.strSoftUniPath);

		if (hkValueKey)
		{
			RegCloseKey(hkValueKey);
			hkValueKey = 0;
		}

		++dwIndex;
		++countnumber;
		RtlZeroMemory(szNewKeyName, MAX_PATH);
		RtlZeroMemory(strMidReg, MAX_PATH);
		if (0x1000 >= countnumber)
			break;
	}
	if (hkResult)
		RegCloseKey(hkResult);
	return countnumber;
}
const DWORD UServerSoftware::EnumSoftwareWo64(LPVOID outbuf, const int icount)
{
	if (!outbuf)
		return 0;

	PUSOFTINFO const softwareinfo = (PUSOFTINFO)outbuf;
	if (!softwareinfo)
		return 0;

	HKEY hkResult = 0;
	USOFTINFO SoftInfo = { 0 };
	FILETIME ftLastWriteTimeA;					// last write time 
	// 1. ��һ���Ѵ��ڵ�ע����
	LSTATUS retCode = RegOpenKeyEx(RootKey, lpSubKey64, 0, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_32KEY | KEY_QUERY_VALUE, &hkResult);
	if (retCode != ERROR_SUCCESS)
		return false;

	// 2. �����Сע���
	// TCHAR    achKey[MAX_PATH] = {};			// buffer for subkey name
	DWORD    cbName = 0;						// size of name string 
	TCHAR    achClass[MAX_PATH] = TEXT("");		// buffer for class name 
	DWORD    cchClassName = MAX_PATH;			// size of class string 
	DWORD    cSubKeys = 0;						// number of subkeys 
	DWORD    cbMaxSubKey;						// longest subkey size 
	DWORD    cchMaxClass;						// longest class string 
	DWORD    cValues;							// number of values for key 
	DWORD    cchMaxValue;						// longest value name 
	DWORD    cbMaxValueData;					// longest value data 
	DWORD    cbSecurityDescriptor;				// size of security descriptor 
	FILETIME ftLastWriteTime;					// last write time 
	DWORD cchValue = MAX_PATH;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hkResult,                // key handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 
	if (retCode != ERROR_SUCCESS)
	{
		if (hkResult)
			RegCloseKey(hkResult);
		return 0;
	}

	// 3. ѭ������UninstallĿ¼�µ��ӽ�
	DWORD dwIndex = 0; DWORD dwKeyLen = 255; DWORD dwType = 0;
	WCHAR szNewKeyName[MAX_PATH] = { 0, }; WCHAR strMidReg[MAX_PATH] = { 0, };
	DWORD dwNamLen = 255; DWORD countnumber = icount;
	HKEY hkValueKey = 0; LSTATUS lRet = ERROR_SUCCESS;
	for (SIZE_T i = 0; i < cSubKeys; i++)
	{
		dwKeyLen = MAX_PATH;
		lRet = RegEnumKeyEx(hkResult, dwIndex, szNewKeyName, &dwKeyLen, 0, NULL, NULL, &ftLastWriteTimeA);
		if (lRet != ERROR_SUCCESS)
			continue;

		// 2.1 ͨ���õ��ӽ�������������ϳ��µ��ӽ�·��
		swprintf_s(strMidReg, L"%s%s%s", lpSubKey, L"\\", szNewKeyName);
		// 2.2 ���µ��ӽ�, ��ȡ����
		lRet = RegOpenKeyEx(RootKey, strMidReg, 0, KEY_QUERY_VALUE, &hkValueKey);
		if (lRet != ERROR_SUCCESS)
			continue;

		SoftInfo.clear();
		// ����
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"DisplayName", 0, &dwType, (LPBYTE)SoftInfo.szSoftName, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].szSoftName, SoftInfo.szSoftName);
		// �汾��
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"VersionNumber", 0, &dwType, (LPBYTE)SoftInfo.szSoftVer, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].szSoftVer, SoftInfo.szSoftVer);
		// ��װʱ��
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"Time", 0, &dwType, (LPBYTE)SoftInfo.szSoftDate, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].szSoftDate, SoftInfo.szSoftDate);
		// ��С
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"Sizeof", 0, &dwType, (LPBYTE)SoftInfo.szSoftSize, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].szSoftSize, SoftInfo.szSoftSize);
		// ������
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"Sizeof", 0, &dwType, (LPBYTE)SoftInfo.strSoftVenRel, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].strSoftVenRel, SoftInfo.strSoftVenRel);
		// ж��·��
		dwNamLen = 255;
		RegQueryValueEx(hkValueKey, L"UninstallString", 0, &dwType, (LPBYTE)SoftInfo.strSoftUniPath, &dwNamLen);
		lstrcpyW(softwareinfo[countnumber].strSoftUniPath, SoftInfo.strSoftUniPath);

		if (hkValueKey)
		{
			RegCloseKey(hkValueKey);
			hkValueKey = 0;
		}

		++dwIndex;
		++countnumber;
		RtlZeroMemory(szNewKeyName, MAX_PATH);
		RtlZeroMemory(strMidReg, MAX_PATH);
		if (0x1000 >= countnumber)
			break;
	}
	if (hkResult)
		RegCloseKey(hkResult);
	return countnumber;
}
const UINT UServerSoftware::DetermineContextForAllProducts()
{
	const int cchGUID = 38;
	WCHAR wszProductCode[cchGUID + 1] = { 0 };
	WCHAR wszAssignmentType[10] = { 0 };
	DWORD cchAssignmentType =
		sizeof(wszAssignmentType) / sizeof(wszAssignmentType[0]);
	DWORD dwIndex = 0;

	DWORD cchProductName = MAX_PATH;
	WCHAR* lpProductName = new WCHAR[cchProductName];
	if (!lpProductName)
	{
		return ERROR_OUTOFMEMORY;
	}

	UINT uiStatus = ERROR_SUCCESS;

	// enumerate all visible products
	do
	{
		uiStatus = MsiEnumProducts(dwIndex,
			wszProductCode);
		if (ERROR_SUCCESS == uiStatus)
		{
			cchAssignmentType =
				sizeof(wszAssignmentType) / sizeof(wszAssignmentType[0]);
			BOOL fPerMachine = FALSE;
			BOOL fManaged = FALSE;

			// Determine assignment type of product
			// This indicates whether the product
			// instance is per-user or per-machine
			if (ERROR_SUCCESS ==
				MsiGetProductInfo(wszProductCode, INSTALLPROPERTY_ASSIGNMENTTYPE, wszAssignmentType, &cchAssignmentType))
			{
				if (L'1' == wszAssignmentType[0])
					fPerMachine = TRUE;
			}
			else
			{
				// This halts the enumeration and fails. Alternatively the error
				// could be logged and enumeration continued for the
				// remainder of the products
				uiStatus = ERROR_FUNCTION_FAILED;
				break;
			}

			// determine the "managed" status of the product.
			// If fManaged is TRUE, product is installed managed
			// and runs with elevated privileges.
			// If fManaged is FALSE, product installation operations
			// run as the user.
			if (ERROR_SUCCESS != MsiIsProductElevated(wszProductCode,
				&fManaged))
			{
				// This halts the enumeration and fails. Alternatively the error
				// could be logged and enumeration continued for the
				// remainder of the products
				uiStatus = ERROR_FUNCTION_FAILED;
				break;
			}

			// obtain the user friendly name of the product
			UINT uiReturn = MsiGetProductInfo(wszProductCode, INSTALLPROPERTY_PRODUCTNAME, lpProductName, &cchProductName);
			if (ERROR_MORE_DATA == uiReturn)
			{
				// try again, but with a larger product name buffer
				delete[] lpProductName;

				// returned character count does not include
				// terminating NULL
				++cchProductName;

				lpProductName = new WCHAR[cchProductName];
				if (!lpProductName)
				{
					uiStatus = ERROR_OUTOFMEMORY;
					break;
				}

				uiReturn = MsiGetProductInfo(wszProductCode, INSTALLPROPERTY_VERSIONSTRING, lpProductName, &cchProductName);
			}

			if (ERROR_SUCCESS != uiReturn)
			{
				// This halts the enumeration and fails. Alternatively the error
				// could be logged and enumeration continued for the
				// remainder of the products
				uiStatus = ERROR_FUNCTION_FAILED;
				break;
			}

			// output information
			//wprintf(L" Product %s:\n", lpProductName);
			//wprintf(L"\t%s\n", wszProductCode);
			//wprintf(L"\tInstalled %s %s\n",
			//	fPerMachine ? L"per-machine" : L"per-user",
			//	fManaged ? L"managed" : L"non-managed");
			//std::wstring output = lpProductName;
			//output.append(L" ").append(wszProductCode).append(L"\r\n");
			//OutputDebugString(output.c_str());
		}
		dwIndex++;
	} while (ERROR_SUCCESS == uiStatus);

	if (lpProductName)
	{
		delete[] lpProductName;
		lpProductName = NULL;
	}

	return (ERROR_NO_MORE_ITEMS == uiStatus) ? ERROR_SUCCESS : uiStatus;
}

bool UServerSoftware::uf_EnumAll(LPVOID outbuf)
{
	if (!outbuf)
		return false;

	PUAllServerSoftware const psinfo = PUAllServerSoftware(outbuf);
	if (psinfo)
	{
		psinfo->softwarenumber = this->EnumSoftware(psinfo->uUsoinfo);
		psinfo->softwarenumber = this->EnumSoftwareWo64(psinfo->uSericeinfo, psinfo->softwarenumber);
		psinfo->servicenumber = this->EnumService(psinfo->uSericeinfo);
	}

	return true;
}