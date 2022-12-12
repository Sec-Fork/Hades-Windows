#include <Windows.h>
#include <string>
#include "uservicesoftware.h"

#include <sysinfo.h>
using namespace std;

#define MAX_SERVICE_SIZE 1024 * 64
#define MAX_QUERY_SIZE   1024 * 8
static const HKEY RootKey = HKEY_LOCAL_MACHINE;
static const LPCTSTR lpSubKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";

UServerSoftware::UServerSoftware()
{
}
UServerSoftware::~UServerSoftware()
{
}

DWORD UServerSoftware::EnumService(LPVOID outbuf)
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
DWORD UServerSoftware::EnumSoftware(LPVOID outbuf)
{
	if (!outbuf)
		return false;

	PUSOFTINFO const softwareinfo = (PUSOFTINFO)outbuf;
	HKEY hkResult = 0;
	USOFTINFO SoftInfo = { 0 };
	FILETIME ftLastWriteTimeA;					// last write time 
	// 1. ��һ���Ѵ��ڵ�ע����
	LONG LReturn = RegOpenKeyEx(RootKey, lpSubKey, 0, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_32KEY | KEY_QUERY_VALUE, &hkResult);
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
	DWORD	retCode;
	// TCHAR  achValue[MAX_PATH] = {};
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
	// 3. ѭ������UninstallĿ¼�µ��ӽ�
	int nCount = 1;
	DWORD dwIndex = 0;
	DWORD dwKeyLen = 255;
	DWORD dwType = 0;
	WCHAR szNewKeyName[MAX_PATH] = {};		// ע�������
	WCHAR strMidReg[MAX_PATH] = {};
	DWORD dwNamLen = 255;					// ��ȡ��ֵ
	HKEY hkValueKey = 0;
	LONG lRrturn = ERROR_SUCCESS;
	DWORD countnumber = 0;
	for (SIZE_T i = 0; i < cSubKeys; i++)
	{
		dwKeyLen = MAX_PATH;
		lRrturn = RegEnumKeyEx(hkResult, dwIndex, szNewKeyName, &dwKeyLen, 0, NULL, NULL, &ftLastWriteTimeA);
		// 2.1 ͨ���õ��ӽ�������������ϳ��µ��ӽ�·��
		swprintf_s(strMidReg, L"%s%s%s", lpSubKey, L"\\", szNewKeyName);
		// 2.2 ���µ��ӽ�, ��ȡ����
		RegOpenKeyEx(RootKey, strMidReg, 0, KEY_QUERY_VALUE, &hkValueKey);
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
		dwNamLen = 255;
		++dwIndex;
		++countnumber;
		if (0x1000 >= countnumber)
			break;
	}
	if (hkResult)
		RegCloseKey(hkResult);
	return countnumber;
}

bool UServerSoftware::uf_EnumAll(LPVOID outbuf)
{
	if (!outbuf)
		return false;

	PUAllServerSoftware psinfo = PUAllServerSoftware(outbuf);
	psinfo->softwarenumber = this->EnumSoftware(psinfo->uUsoinfo);
	psinfo->servicenumber = this->EnumService(psinfo->uSericeinfo);

	return true;
}