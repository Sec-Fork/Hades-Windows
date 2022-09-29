/*
* �������SysMonDrv����ͬ�����ݴ���
* SysMonDrv <--> HadesSvc <--> HadesContrl
*/
#include "HlprMiniCom.h"
#include <fltuser.h>
#include <sysinfo.h>

#include "socketMsg.h"

#define NOTIFICATION_KEY ((ULONG_PTR)-1)

static HANDLE g_hPort = nullptr;
static HANDLE g_comPletion = nullptr;
static BOOL   g_InitPortStatus = FALSE;

#define HADES_READ_BUFFER_SIZE  4096 
typedef struct _HADES_NOTIFICATION {

	ULONG CommandId;
	ULONG Reserved;
	UCHAR Contents[HADES_READ_BUFFER_SIZE];
} HADES_NOTIFICATION, * PHADES_NOTIFICATION;
typedef struct _HADES_REPLY {
	DWORD SafeToOpen;
} HADES_REPLY, * PHADES_REPLY;
// GetMsg
typedef struct _COMAND_MESSAGE
{
	FILTER_MESSAGE_HEADER MessageHeader;
	HADES_NOTIFICATION Notification;
	OVERLAPPED Overlapped;
} COMMAND_MESSAGE, * PCOMMAND_MESSAGE;
// Reply
typedef struct _REPLY_MESSAGE
{
	FILTER_REPLY_HEADER ReplyHeader;
	HADES_REPLY			Reply;
}REPLY_MESSAGE, * PREPLY_MESSAGE;

static DWORD WINAPI ThreadMiniPortConnectNotify(LPVOID pData)
{
	(reinterpret_cast<HlprMiniPortIpc*>(pData))->StartMiniPortWaitConnectWork();
	return 0;
}
static DWORD WINAPI ThreadMiniPortGetMsgNotify(LPVOID pData)
{
	(reinterpret_cast<HlprMiniPortIpc*>(pData))->GetMsgNotifyWork();
	return 0;
}

HlprMiniPortIpc::HlprMiniPortIpc()
{
	// �̻߳ص����Ǿ�̬ȫ��/���ﲻ����дInit������
	// ������ʳ�Ա���������װ�ɺ������ã���Ҫ�ڹ������߳�
	DWORD threadid = 0;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadMiniPortConnectNotify, NULL, 0, &threadid);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadMiniPortGetMsgNotify, NULL, 0, &threadid);
}
HlprMiniPortIpc::~HlprMiniPortIpc()
{
	if (g_hPort)
		CloseHandle(g_hPort);
	if (g_comPletion)
		CloseHandle(g_comPletion);
	g_hPort = nullptr;
	g_comPletion = nullptr;
}

// TestOutpuDebug
void RegisterOutPut(const REGISTERINFO* const registerinfo)
{
	try
	{
		std::wstring OutPut;
		switch (registerinfo->opeararg)
		{// Ĭ��Ex�����ṹ��>=Win7
			// ����Key - ��Key
		case RegNtPreCreateKey:
		case RegNtPreOpenKey:
		{
			OutPut = L"[HadesSvc] RegNtPreCreateKey/RegNtPreOpenKey CompleteName: ";
			OutPut.append(registerinfo->CompleteName);
			OutputDebugString(OutPut.c_str());
		}
		break;

		case RegNtPreCreateKeyEx:
		case RegNtPreOpenKeyEx:
		{
			OutPut = L"[HadesSvc] RegNtPreCreateKeyEx/RegNtPreOpenKeyEx CompleteName: ";
			OutPut.append(registerinfo->CompleteName);
			OutputDebugString(OutPut.c_str());
		}
		break;

		case RegNtPostCreateKeyEx:
		case RegNtPostOpenKeyEx:
		{
			OutPut = L"[HadesSvc] RegNtPostCreateKeyEx/RegNtPostOpenKeyEx Object: ";
			OutPut.append(std::to_wstring((long)registerinfo->Object));
			OutPut.append(L" CompleteName: ");
			OutPut.append(registerinfo->CompleteName);
			OutputDebugString(OutPut.c_str());
		}
		break;

		case RegNtPostCreateKey:
		case RegNtPostOpenKey:
		{
			OutPut = L"[HadesSvc] RegNtPostCreateKey/RegNtPostOpenKey CompleteName: ";
			OutPut.append(std::to_wstring((long)registerinfo->Object));
			OutPut.append(L" CompleteName: ");
			OutPut.append(registerinfo->CompleteName);
			OutputDebugString(OutPut.c_str());
		}
		break;

		// �޸�Key
		case RegNtSetValueKey:
		{
			OutPut = L"[HadesSvc] RegNtSetValueKey CompleteName: ";
			OutPut.append(std::to_wstring((long)registerinfo->Object));
			OutPut.append(L" CompleteName: ");
			OutPut.append(registerinfo->CompleteName);
			OutputDebugString(OutPut.c_str());
		}
		break;

		// ɾ��Key
		case RegNtPreDeleteKey:
		{

		}
		break;

		// ö��Key
		case RegNtEnumerateKey:
		{
		}
		break;

		// ������ע���
		case RegNtRenameKey:
		//case RegNtPostRenameKey:
		{
			OutPut = L"[HadesSvc] RegNtRenameKey CompleteName: ";
			OutPut.append(std::to_wstring((long)registerinfo->Object));
			OutPut.append(L" CompleteName(NewName): ");
			OutPut.append(registerinfo->CompleteName);
			
			OutputDebugString(OutPut.c_str());
		}
		break;

		// ��ѯ
		case RegNtQueryValueKey:
		{			
			OutPut = L"[HadesSvc] RegNtQueryValueKey CompleteName: ";
			OutPut.append(std::to_wstring((long)registerinfo->Object));
			OutPut.append(L" CompleteName: ");
			OutPut.append(registerinfo->CompleteName);
			OutputDebugString(OutPut.c_str());
		}
		break;
		}
	}
	catch (const std::exception&)
	{

	}

}

bool HlprMiniPortIpc::SetRuleProcess(PVOID64 rulebuffer, unsigned int buflen, unsigned int processnamelen) {
	if (FALSE == g_InitPortStatus)
		return false;
	
	DWORD bytesReturned = 0;
	DWORD hResult = 0;
	unsigned int total = sizeof(COMMAND_MESSAGE) + buflen + 1;
	auto InputBuffer = VirtualAlloc(NULL, total, MEM_RESERVE, PAGE_READWRITE);

	COMMAND_MESSAGE command_message;
	//command_message.Command = MIN_COMMAND::SET_PROCESSNAME;
	memcpy(InputBuffer, &command_message, sizeof(COMMAND_MESSAGE));
	memcpy((void*)((DWORD64)InputBuffer + sizeof(COMMAND_MESSAGE)), rulebuffer, buflen);

	if (g_hPort)
	{
		hResult = FilterSendMessage(g_hPort, InputBuffer, total, NULL, NULL, &bytesReturned);
		if (hResult != S_OK)
		{
			return hResult;
		}
	}

	return true;
}
void HlprMiniPortIpc::StartMiniPortWaitConnectWork()
{
	HRESULT Status = 0;
	g_hPort = nullptr;
	g_comPletion = nullptr;
	PCOMMAND_MESSAGE msg = nullptr;

	do {
		Status = FilterConnectCommunicationPort(
			L"\\HadesEventFltPort",
			0,
			NULL,
			0,
			NULL,
			&g_hPort);
		if (Status == HRESULT_FROM_WIN32(S_OK))
		{
			// ��IoComplet
			g_comPletion = CreateIoCompletionPort(g_hPort, NULL, 0, 4);
			if (nullptr == g_comPletion)
			{
				CloseHandle(g_hPort);
				g_hPort = nullptr;
				continue;
			}

			// ��ʼ����GetMsg, Notify�̵߳ȴ�����  
			// ��ҪֻGetMsgһ�Σ���ΪIOCP�˿ڿ���error���˷ѵ��������ٴ�SendMsgû��GetMsg�ͻ�һֱ����
			
			for (size_t idx = 0; idx < 4; ++idx)
			{
				msg = (PCOMMAND_MESSAGE)malloc(sizeof(COMMAND_MESSAGE));
				if (nullptr == msg)
				{
					Status = ERROR;
					break;
				}
					
				RtlSecureZeroMemory(&msg->Overlapped, sizeof(OVERLAPPED));
				Status = FilterGetMessage(
					g_hPort,
					&msg->MessageHeader,
					FIELD_OFFSET(COMMAND_MESSAGE, Overlapped),
					&msg->Overlapped
				);
				// Pending״̬�ɹ�
				if (Status != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
				{
					Status = ERROR;
					break;
				}
			}

			if (Status == ERROR)
			{
				if (msg)
					free(msg);
				CloseHandle(g_hPort);
				CloseHandle(g_comPletion);
				g_hPort = nullptr;
				g_comPletion = nullptr;
				msg = nullptr;
				g_InitPortStatus = false;
				return;
			}

			g_InitPortStatus = true;
			OutputDebugString(L"Connect sysmondriver miniPort Success");
			break;
		}		
		else
			Sleep(2000);
	} while (TRUE);
}
void HlprMiniPortIpc::GetMsgNotifyWork()
{
	DWORD outSize = 0;
	ULONG_PTR key = 0;
	BOOL nRet = FALSE;
	LPOVERLAPPED pOvlp = nullptr;
	HRESULT result = FALSE;
	PCOMMAND_MESSAGE message = nullptr;
	REPLY_MESSAGE replyMessage;
	PHADES_NOTIFICATION notification = nullptr;
	RtlSecureZeroMemory(&replyMessage, sizeof(REPLY_MESSAGE));

	// Waiting Connect Driver MiniPort_Server - Modify EventWaiting
	do {
		if (g_InitPortStatus && (nullptr != g_hPort) && (nullptr != g_comPletion))
			break;
		else
			Sleep(2000);
	} while (1);

	// Recv While Driver Send to Client Msg_Handler
	DWORD error_code = 0;
	do {

		nRet = GetQueuedCompletionStatus(g_comPletion, &outSize, &key, &pOvlp, INFINITE);
		if (FALSE == nRet) {
			OutputDebugString(L"GetQueuedCompletionStatus sysmondriver miniPort Error");
			if (!g_comPletion)
				break;
			continue;
		}
		else if (!pOvlp || key == NOTIFICATION_KEY)
			continue;
		message = CONTAINING_RECORD(pOvlp, COMMAND_MESSAGE, Overlapped);
		// handler buffer
		notification = &message->Notification;
		// Ĭ�Ϸ���
		replyMessage.Reply.SafeToOpen = 2;
		switch (notification->CommandId)
		{
		case MIN_COMMAND::IPS_PROCESSSTART:
		{
			const PROCESSINFO* const processinfo = (PROCESSINFO*)notification->Contents;
			OutputDebugString(processinfo->commandLine);
			// ��������������͵�����ȴ��û�����
			socketMsg socketPip;
			if (false == socketPip.sendDlgMsg(IPS_PROCESSSTART, (char*)processinfo, sizeof(PROCESSINFO)))
				break;
			replyMessage.Reply.SafeToOpen = socketPip.recv();
		}
		break;
		case MIN_COMMAND::IPS_REGISTERTAB: 
		{
			// ����Ĭ��
			const REGISTERINFO* const registerinfo = (REGISTERINFO*)notification->Contents;
			if (registerinfo)
				RegisterOutPut(registerinfo);
			replyMessage.Reply.SafeToOpen = 2;
		}
		break;
		case MIN_COMMAND::IPS_IMAGEDLL: break;
		}

		if (!g_hPort)
			break;
		replyMessage.ReplyHeader.Status = 0;
		replyMessage.ReplyHeader.MessageId = message->MessageHeader.MessageId;
		result = FilterReplyMessage(
			g_hPort,
			(PFILTER_REPLY_HEADER)&replyMessage,
			sizeof(replyMessage)
		);
		//if (S_OK != result)
		//{
		//	break;
		//}
		memset(&message->Overlapped, 0, sizeof(OVERLAPPED));
		result = FilterGetMessage(
			g_hPort,
			&message->MessageHeader,
			FIELD_OFFSET(COMMAND_MESSAGE, Overlapped),
			&message->Overlapped
		);
		if (result != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
			break;
		//OutputDebugString(L"FilterReplyMessage Message & FilterGetMessage");
#pragma warning(push)
#pragma warning(disable:4127)
	} while (TRUE);
#pragma warning(pop)

}
void HlprMiniPortIpc::MiniPortActiveCheck()
{
}