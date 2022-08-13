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
				uStatus = g_ulib->GetEtwMonStatus();
				if (false == uStatus)
					g_ulib->uMsg_EtwInit();
				else if (true == uStatus)
					g_ulib->uMsg_EtwClose();
			}
			break;
			case 2:
			{//�ں�̬����
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