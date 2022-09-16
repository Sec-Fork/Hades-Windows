/*
* ���ս���HadesContrl����Msg������ؿ��غ͹���ʹ��
* HadesContrlMsg --> HadesSvc
*/
#include <Windows.h>
#include "msgloop.h"
#include <exception>

// ���������Lib
#include "kmsginterface.h"
#include "umsginterface.h"

const int WM_GETMONITORSTATUS = WM_USER + 504;
static kMsgInterface* g_klib = nullptr;
static uMsgInterface* g_ulib = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static bool kStatus = false;
	static bool uStatus = false;
	try
	{
		if (Message == WM_COPYDATA)
		{
			COPYDATASTRUCT* pCopyData = (COPYDATASTRUCT*)lParam;
			switch (pCopyData->dwData)
			{
			case 1:
			{//�û�̬����
				if (!g_ulib)
					break;
				uStatus = g_ulib->GetEtwMonStatus();
				if (false == uStatus)
					g_ulib->uMsg_EtwInit();
				else if (true == uStatus)
					g_ulib->uMsg_EtwClose();
			}
			break;
			case 2:
			{//�ں�̬����
				if (!g_klib)
					break;
				kStatus = g_klib->GetKerMonStatus();
				if (false == g_klib->GetKerInitStatus())
					g_klib->DriverInit(false); // ��ʼ������read i/o�߳�
				else
				{
					if (false == kStatus)
						g_klib->StartReadFileThread();//�������Ҫ��ʼ������Ϊ�������ڹ��� - ֻ�����߳�
				}
				if (false == kStatus)
				{
					OutputDebugString(L"[HadesSvc] GetKerMonStatus Send Enable KernelMonitor Command");
					g_klib->OnMonitor();
					OutputDebugString(L"[HadesSvc] GetKerMonStatus Enable KernelMonitor Success");
				}
				else if (true == kStatus)
				{
					OutputDebugString(L"[HadesSvc] GetKerMonStatus Send Disable KernelMonitor Command");
					g_klib->OffMonitor();
					OutputDebugString(L"[HadesSvc] GetKerMonStatus Disable KernelMonitor Success");
					if ((true == g_klib->GetKerInitStatus()) && (false == g_klib->GetKerBeSnipingStatus()))
						g_klib->DriverFree();
					else
						g_klib->StopReadFileThread(); // ������Ϊ����״̬�£��ر��߳� - ��ֹ�·�I/O
				}
			}
			break;
			case 3:
			{//��Ϊ����
				if (!g_klib)
					break;
				if (false == g_klib->GetKerInitStatus())
					g_klib->DriverInit(true);// ��ʼ��������read i/o�߳�
				kStatus = g_klib->GetKerBeSnipingStatus();
				if (false == kStatus)
				{
					OutputDebugString(L"[HadesSvc] OnBeSnipingMonitor Send Enable KernelMonitor Command");
					g_klib->OnBeSnipingMonitor();
					OutputDebugString(L"[HadesSvc] OnBeSnipingMonitor Enable KernelMonitor Success");
				}
				else if (true == kStatus)
				{
					OutputDebugString(L"[HadesSvc] OnBeSnipingMonitor Disable Disable KernelMonitor Command");
					g_klib->OffBeSnipingMonitor();
					OutputDebugString(L"[HadesSvc] OnBeSnipingMonitor Disable KernelMonitor Success");
					if ((true == g_klib->GetKerInitStatus()) && (false == g_klib->GetKerMonStatus()))
						g_klib->DriverFree();
				}
			}
			break;
			default:
				break;
			}
		}
		else if (Message == WM_GETMONITORSTATUS)
		{// ��ȡ��ǰ���״̬
			do
			{
				if (!g_klib || !g_ulib)
					break;
				HWND m_ControlHwnd = FindWindow(L"HadesMainWindow", NULL);
				if (!m_ControlHwnd)
					break;
				const bool bUStatus = g_ulib->GetEtwMonStatus();
				const DWORD dwId = bUStatus ? 0x20 : 0x21;
				::PostMessage(m_ControlHwnd, WM_GETMONITORSTATUS, dwId, NULL);
				const bool bkStatus = g_klib->GetKerMonStatus();
				const DWORD dwId1 = bkStatus ? 0x22 : 0x23;
				::PostMessage(m_ControlHwnd, WM_GETMONITORSTATUS, dwId1, NULL);
				const bool bkMStatus = g_klib->GetKerBeSnipingStatus();
				const DWORD dwId2 = bkMStatus ? 0x24 : 0x25;
				::PostMessage(m_ControlHwnd, WM_GETMONITORSTATUS, dwId2, NULL);
			} while (false);				
		}
	}
	catch (const std::exception&)
	{
	}
	return DefWindowProc(hWnd, Message, wParam, lParam);
}
static DWORD WINAPI pInitWinReg(LPVOID lpThreadParameter)
{
	WNDCLASS wnd;
	wnd.style = CS_VREDRAW | CS_HREDRAW;
	wnd.lpfnWndProc = WndProc;
	wnd.cbClsExtra = NULL;
	wnd.cbWndExtra = NULL;
	wnd.hInstance = NULL;
	wnd.hIcon = NULL;
	wnd.hCursor = NULL;
	wnd.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wnd.lpszMenuName = NULL;
	wnd.lpszClassName = TEXT("HadesSvc");
	RegisterClass(&wnd);
	HWND hWnd = CreateWindow(
		TEXT("HadesSvc"),
		TEXT("HadesSvc"),
		WS_OVERLAPPEDWINDOW,
		10, 10, 500, 300,
		NULL,
		NULL,
		NULL,
		NULL
	);
	if (!hWnd)
	{
		OutputDebugString(L"HadesSvc���ڴ���ʧ��");
		return 0;
	}
	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);
	MSG  msg = {};
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

bool WinMsgLoop::setKmsgLib(LPVOID ptrlib)
{
	g_klib = (kMsgInterface*)ptrlib;
	return g_klib ? true : false;
}
bool WinMsgLoop::setUmsgLib(LPVOID ptrlib)
{
	g_ulib = (uMsgInterface*)ptrlib;
	return g_ulib ? true : false;
}

WinMsgLoop::WinMsgLoop()
{
	CreateThread(NULL, 0, pInitWinReg, 0, 0, 0);
}
WinMsgLoop::~WinMsgLoop()
{
}