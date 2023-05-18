#include <Windows.h>
#include "uautostart.h"
#include <comdef.h>
//  Include the task header file.
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#include <wchar.h>
#include <sysinfo.h>

UAutoStart::UAutoStart()
{

}
UAutoStart::~UAutoStart()
{
}

// Ĭ�ϲ�����1000��ע�������
ULONG CheckRegisterRun(RegRun* pRegRun)
{
	if (!pRegRun)
		return 0;

	HKEY hKey;
	DWORD dwType = 0;
	DWORD dwBufferSize = MAXBYTE;
	DWORD dwKeySize = MAXBYTE;
	CHAR szValueName[MAXBYTE] = { 0 };
	CHAR szValueKey[MAXBYTE] = { 0 };
	int i = 0, j = 0;
	int index = 0;

	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey) == ERROR_SUCCESS)
	{
		while (TRUE)
		{
			int rect = RegEnumValueA(hKey, i, szValueName, &dwBufferSize, NULL, &dwType, (LPBYTE)szValueKey, &dwKeySize);
			if (rect == ERROR_NO_MORE_ITEMS)
			{
				break;
			}

			RtlCopyMemory(pRegRun[index].szValueName, szValueName, MAXBYTE);
			RtlCopyMemory(pRegRun[index].szValueKey, szValueKey, MAXBYTE);
			if (1000 >= (++index))
				break;

			i++;
			j++;
			dwBufferSize = MAXBYTE;
			dwKeySize = MAXBYTE;
			ZeroMemory(szValueName, MAXBYTE);
			ZeroMemory(szValueKey, MAXBYTE);
		}
		RegCloseKey(hKey);
	}

	i = 0; j = 0; dwType = 0; hKey = 0;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Runonce", &hKey) == ERROR_SUCCESS)
	{
		while (TRUE)
		{
			int rect = RegEnumValueA(hKey, i, szValueName, &dwBufferSize, NULL, &dwType, (LPBYTE)szValueKey, &dwKeySize);
			if (rect == ERROR_NO_MORE_ITEMS)
			{
				break;
			}

			RtlCopyMemory(pRegRun[index].szValueName, szValueName, MAXBYTE);
			RtlCopyMemory(pRegRun[index].szValueKey, szValueKey, MAXBYTE);
			if (1000 >= (++index))
				break;

			i++;
			j++;
			dwBufferSize = MAXBYTE;
			dwKeySize = MAXBYTE;
			ZeroMemory(szValueName, MAXBYTE);
			ZeroMemory(szValueKey, MAXBYTE);
		}
		RegCloseKey(hKey);
	}

	i = 0; j = 0; dwType = 0; hKey = 0;
	if (RegOpenKeyA(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey) == ERROR_SUCCESS)
	{
		while (TRUE)
		{
			int rect = RegEnumValueA(hKey, i, szValueName, &dwBufferSize, NULL, &dwType, (LPBYTE)szValueKey, &dwKeySize);
			if (rect == ERROR_NO_MORE_ITEMS)
			{
				break;
			}

			RtlCopyMemory(pRegRun[index].szValueName, szValueName, MAXBYTE);
			RtlCopyMemory(pRegRun[index].szValueKey, szValueKey, MAXBYTE);
			if (1000 >= (++index))
				break;

			i++;
			j++;
			dwBufferSize = MAXBYTE;
			dwKeySize = MAXBYTE;
			ZeroMemory(szValueName, MAXBYTE);
			ZeroMemory(szValueKey, MAXBYTE);
		}
		RegCloseKey(hKey);
	}

	i = 0; j = 0; hKey = 0;
	dwType = REG_SZ | REG_EXPAND_SZ;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey) == ERROR_SUCCESS)
	{
		while (TRUE)
		{
			int rect = RegEnumValueA(hKey, i, szValueName, &dwBufferSize, NULL, &dwType, (LPBYTE)szValueKey, &dwKeySize);
			if (rect == ERROR_NO_MORE_ITEMS)
			{
				break;
			}

			RtlCopyMemory(pRegRun[index].szValueName, szValueName, MAXBYTE);
			RtlCopyMemory(pRegRun[index].szValueKey, szValueKey, MAXBYTE);
			if (1000 >= (++index))
				break;

			i++;
			j++;
			dwBufferSize = MAXBYTE;
			dwKeySize = MAXBYTE;
			ZeroMemory(szValueName, MAXBYTE);
			ZeroMemory(szValueKey, MAXBYTE);
		}
		RegCloseKey(hKey);
	}

	return index;
}

// ����Xml��ĳ����������ĸ�ʽ
void XmlCommandAn(const wchar_t* source, wchar_t* deststr)
{
	DWORD index_head = 0;
	DWORD index_tail = 0;
	// ����<Command
	for (int i = 0; i < 0x1024; i++)
	{
		// <Command>F:\\360zip\360zipUpdate.exe</Command>
		if ((source[i] == '<') && (source[i + 1] == 'C') && (source[i + 2] == 'o') && (source[i + 3] == 'm'))
		{
			// ��ȡִ�в�����ʵλ��
			index_head = i + 9;
		}

		if ((source[i] == '<') && (source[i + 1] == '/') && (source[i + 2] == 'C') && (source[i + 3] == 'o'))
		{
			index_tail = i;
			if ((index_tail - index_head) >= 3)
			{
				// ����<Command> xxxxx </Command>
				wmemcpy(deststr, &source[index_head], (index_tail - index_head));
				break;
			}
		}
	}

	// ���Ҳ��� <Arguments>/detectupdate</Arguments>
	for (int i = index_tail; i < 0x1024; i++)
	{
		// <Command>F:\\360zip\360zipUpdate.exe</Command>
		if ((source[i] == '<') && (source[i + 1] == 'A') && (source[i + 2] == 'r') && (source[i + 3] == 'g'))
		{
			// ��ȡִ�в�����ʵλ��
			index_head = i + 11;
		}

		if ((source[i] == '<') && (source[i + 1] == '/') && (source[i + 2] == 'A') && (source[i + 3] == 'r'))
		{
			index_tail = i;
			if ((index_tail - index_head) >= 1)
			{
				// ƴ�� <Command> + <Arguments>
				int nCommandSize = lstrlenW(deststr);
				deststr[nCommandSize] = ' ';
				wmemcpy(&deststr[nCommandSize + 1], &source[index_head], (index_tail - index_head));
				break;
			}
		}
	}
}
/*
	ͨ��WMIֻ��ö��ʹ��Win32_ScheduledJob����At.exeʵ�ó��򴴽��ļƻ�����NetScheduleJobEnum(); Win8���ϾͲ�֧����(����)
	Ues: Task Scheduler 2.0
*/
ULONG CheckTaskSchedulerRun(UTaskSchedulerRun* pTaskRun)
{
	if (!pTaskRun)
		return 0;
	//  ------------------------------------------------------
	//  Initialize COM.
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
		return 0;

	//  Set general COM security levels.
	hr = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		0,
		NULL);

	if (FAILED(hr))
	{
		CoUninitialize();
		return 0;
	}

	//  ------------------------------------------------------
	//  Create an instance of the Task Service. 
	ITaskService* pService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskService,
		(void**)&pService);
	if (FAILED(hr))
	{
		CoUninitialize();
		return 0;
	}

	//  Connect to the task service.
	hr = pService->Connect(_variant_t(), _variant_t(),
		_variant_t(), _variant_t());
	if (FAILED(hr))
	{
		pService->Release();
		CoUninitialize();
		return 0;
	}

	//  ------------------------------------------------------
	//  Get the pointer to the root task folder.
	ITaskFolder* pRootFolder = NULL;
	hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);

	pService->Release();
	if (FAILED(hr))
	{
		CoUninitialize();
		return 0;
	}

	//  -------------------------------------------------------
	//  Get the registered tasks in the folder.
	IRegisteredTaskCollection* pTaskCollection = NULL;
	hr = pRootFolder->GetTasks(NULL, &pTaskCollection);

	pRootFolder->Release();
	if (FAILED(hr))
	{
		CoUninitialize();
		return 0;
	}

	LONG numTasks = 0;
	hr = pTaskCollection->get_Count(&numTasks);

	if (numTasks == 0)
	{
		pTaskCollection->Release();
		CoUninitialize();
		return 0;
	}

	TASK_STATE taskState;
	wchar_t TaskCommand[1024] = { 0 };

	DWORD iCouent = 0;
	for (LONG i = 0; i < numTasks; i++)
	{
		IRegisteredTask* pRegisteredTask = NULL;
		hr = pTaskCollection->get_Item(_variant_t(i + 1), &pRegisteredTask);

		if (SUCCEEDED(hr))
		{
			BSTR taskName = NULL;
			hr = pRegisteredTask->get_Name(&taskName);
			if (SUCCEEDED(hr))
			{
				
				lstrcpyW(pTaskRun[i].szValueName, taskName);

				hr = pRegisteredTask->get_State(&taskState);
				if (SUCCEEDED(hr))
					pTaskRun[i].State = taskState;

				DATE* pLastTime = NULL;
				hr = pRegisteredTask->get_LastRunTime(pLastTime);
				if (SUCCEEDED(hr) && pLastTime)
					pTaskRun[i].LastTime = *pLastTime;

				hr = pRegisteredTask->get_NextRunTime(pLastTime);
				if (SUCCEEDED(hr) && pLastTime)
					pTaskRun[i].NextTime = *pLastTime;

				hr = pRegisteredTask->get_Xml(&taskName);
				if (SUCCEEDED(hr))
				{
					XmlCommandAn((wchar_t*)taskName, TaskCommand);
					lstrcpyW(pTaskRun[i].TaskCommand, TaskCommand);
				}

				SysFreeString(taskName);

				iCouent++;
			}
			pRegisteredTask->Release();
		}
	}

	pTaskCollection->Release();
	CoUninitialize();
	return iCouent;
}

bool UAutoStart::uf_EnumAutoStartask(LPVOID pData, const DWORD dwSize)
{
	if (!pData || 0 >= dwSize)
		return false;

	PUAutoStartNode Autorun = (PUAutoStartNode)pData;
	if (!Autorun)
		return false;

	// ע���
	Autorun->regnumber = CheckRegisterRun((RegRun*)Autorun->regrun);

	// �ƻ�����
	Autorun->taskrunnumber = CheckTaskSchedulerRun((UTaskSchedulerRun*)Autorun->taskschrun);

	return true;
}