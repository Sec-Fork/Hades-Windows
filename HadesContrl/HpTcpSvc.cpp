#include "framework.h"
#include "HpTcpSvc.h"
#include "MessageBoxDlg.h"

HpTcpSvc::HpTcpSvc()
{
}

HpTcpSvc::~HpTcpSvc()
{
}

bool HpTcpSvc::hpsk_init()
{
	try
	{
		// 1. Create listener object
		m_listener = new HpTcpSvc;
		if (!m_listener)
			return false;
		// 2. Create component object (and binding with listener object)
		//CTcpPullServerPtr s_pserver(m_listener);
		CTcpServerPtr s_pserver(m_listener);
		// 3. Start component object
		if (!s_pserver->Start(L"127.0.0.1", 10246))
			exit(1);
		// 4. wating stop event
		const HANDLE stopevent = CreateEvent(NULL, FALSE, FALSE, L"HpStopTcpSvcEvent");
		if (!stopevent)
		{
			if (s_pserver)
				s_pserver->Stop();
			return false;
		}
		WaitForSingleObject(stopevent, INFINITE);
		if (s_pserver)
			s_pserver->Stop();
		if (m_listener)
			delete m_listener;
		CloseHandle(stopevent);
		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

EnHandleResult HpTcpSvc::OnPrepareListen(ITcpServer* pSender, SOCKET soListen)
{
	return HR_OK;
}
EnHandleResult HpTcpSvc::OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient)
{
	return HR_OK;
}
EnHandleResult HpTcpSvc::OnHandShake(ITcpServer* pSender, CONNID dwConnID)
{
	return HR_OK;
}
EnHandleResult HpTcpSvc::OnReceive(ITcpServer* pSender, CONNID dwConnID, int iLength)
{
	//TPkgInfo* pInfo = FindPkgInfo(pSender, dwConnID);
	//ITcpPullServer* pServer = ITcpPullServer::FromS(pSender);

	//if (pInfo != nullptr)
	//{
	//	int required = pInfo->length;
	//	int remain = iLength;

	//	while (remain >= required)
	//	{
	//		remain -= required;
	//		CBufferPtr buffer(required);

	//		EnFetchResult result = pServer->Fetch(dwConnID, buffer, (int)buffer.Size());
	//		if (result == FR_OK)
	//		{
	//			if (pInfo->is_header)
	//			{
	//				TPkgHeader* pHeader = (TPkgHeader*)buffer.Ptr();
	//				//TRACE("[Server] head -> seq: %d, body_len: %d\n", pHeader->seq, pHeader->body_len);

	//				required = pHeader->body_len;
	//			}
	//			else
	//			{
	//				TPkgBody* pBody = (TPkgBody*)(BYTE*)buffer;

	//				char* ch = pBody->name;
	//				int num = MultiByteToWideChar(0, 0, ch, -1, NULL, 0);
	//				//��ó��ֽ�����Ŀռ�
	//				wchar_t* wide = new wchar_t[num];
	//				MultiByteToWideChar(0, 0, ch, -1, wide, num);
	//				m_Info.AddString(wide);   //�ڿؼ�����ʾname

	//				itoa(int(pBody->age), ch, 10);   //��intת��Ϊchar*
	//				num = MultiByteToWideChar(0, 0, ch, -1, NULL, 0);
	//				//��ó��ֽ�����Ŀռ�
	//				wide = new wchar_t[num];
	//				MultiByteToWideChar(0, 0, ch, -1, wide, num);
	//				m_Info.AddString(wide);  //�ڿؼ�����ʾage

	//				ch = pBody->desc;
	//				num = MultiByteToWideChar(0, 0, ch, -1, NULL, 0);
	//				//��ó��ֽ�����Ŀռ�
	//				wide = new wchar_t[num];
	//				MultiByteToWideChar(0, 0, ch, -1, wide, num);
	//				m_Info.AddString(wide); //�ڿؼ�����ʾdesc

	//				//TRACE("[Server] body -> name: %s, age: %d, desc: %s\n", pBody->name, pBody->age, pBody->desc);

	//				required = sizeof(TPkgHeader);
	//			}

	//			pInfo->is_header = !pInfo->is_header;
	//			pInfo->length = required;

	//			::PostOnReceive(dwConnID, buffer, (int)buffer.Size());

	//			if (!pSender->Send(dwConnID, buffer, (int)buffer.Size()))  //��������
	//				return HR_ERROR;
	//		}
	//	}
	//}
	return HR_OK;
}
EnHandleResult HpTcpSvc::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	if (!pSender || !dwConnID || !pData || (sizeof(int) > iLength))
		return HR_ERROR;
	
	char* Msginfo = nullptr;
	MSG_DLGBUFFER* pMsgbuf = nullptr;
	const int msglen = iLength - sizeof(int);

	do {
		Msginfo = new char[msglen];
		pMsgbuf = (MSG_DLGBUFFER*)new MSG_DLGBUFFER;
		if (!Msginfo || !pMsgbuf)
			break;
		RtlSecureZeroMemory(Msginfo, msglen);
		RtlSecureZeroMemory(pMsgbuf, sizeof(MSG_DLGBUFFER));

		memcpy(Msginfo, pData + sizeof(int), msglen);
		const int taskid = *((int*)pData);

		switch (taskid)
		{
		case IPS_PROCESSSTART:
		{
			int options = 2;
			do {
				pMsgbuf->options = IPS_PROCESSSTART;
				MessageBoxDlg msgdlg(pMsgbuf, Msginfo);
				msgdlg.Create(NULL, L"", UI_WNDSTYLE_DIALOG, WS_EX_WINDOWEDGE);
				msgdlg.CenterWindow();
				msgdlg.ShowModal();
				if (!pMsgbuf)
					break;
				options = pMsgbuf->options;
			} while (false);
			pSender->Send(dwConnID, (const BYTE*)&options, sizeof(int));
		}
		break;
		}
	} while (false);
	
	if (Msginfo)
	{
		delete[] Msginfo;
		Msginfo = nullptr;
	}
	if (pMsgbuf)
	{
		delete pMsgbuf;
		pMsgbuf = nullptr;
	}

	return HR_OK;
}
EnHandleResult HpTcpSvc::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	return HR_OK;
}
EnHandleResult HpTcpSvc::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
	return HR_OK;
}
EnHandleResult HpTcpSvc::OnShutdown(ITcpServer* pSender)
{
	return HR_OK;
}