#include "../HpTcpSvc.h"
#include "MainWindow.h"
#include "../DriverManager.h"
#include "../Systeminfolib.h"
#include <usysinfo.h>
#include <TlHelp32.h>
#include <mutex>
#include <WinUser.h>
#include <UserEnv.h>
#include <stdio.h>
#include <time.h>
#include "../resource.h"
#pragma comment(lib,"Userenv.lib")

const int WM_SHOWTASK = WM_USER + 501;
const int WM_ONCLOSE = WM_USER + 502;
const int WM_ONOPEN = WM_USER + 503;
const int WM_IPS_PROCESS = WM_USER + 600;

// Hades״̬��
static std::mutex			g_hadesStatuscs;
// Start�߳���
static std::mutex			g_startprocesslock;
// ��̬��ʱ����Ҫ
static USysBaseInfo			g_DynSysBaseinfo;
// ��������
static DriverManager		g_DrvManager;
const std::wstring			g_drverName = L"sysmondriver";

static DWORD WINAPI StartIocpWorkNotify(LPVOID lpThreadParameter)
{
	HpTcpSvc tcpsvc;
	tcpsvc.hpsk_init();
	return 0;
}

void WindlgShow(HWND& hWnd)
{
	typedef void    (WINAPI* PROCSWITCHTOTHISWINDOW)    (HWND, BOOL);
	PROCSWITCHTOTHISWINDOW    SwitchToThisWindow;
	HMODULE    hUser32 = GetModuleHandle(L"user32");
	SwitchToThisWindow = (PROCSWITCHTOTHISWINDOW)GetProcAddress(hUser32, "SwitchToThisWindow");
	SwitchToThisWindow(hWnd, TRUE);
}
std::wstring GetWStringByChar(const char* szString)
{
	std::wstring wstrString = L"";
	try
	{
		if (szString != NULL)
		{
			std::string str(szString);
			wstrString.assign(str.begin(), str.end());
		}
	}
	catch (const std::exception&)
	{
		return wstrString;
	}

	return wstrString;
}
// �����ļ���ȡgrpc����
std::wstring ReadConfigtoIpPort(std::wstring& config_root)
{
	std::wstring ip_port;
	HANDLE hFile = CreateFile(config_root.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile)
		return ip_port;
	bool boolinit = false;
	BYTE* guardData = NULL;
	do {

		const int guardDataSize = GetFileSize(hFile, NULL);
		if (guardDataSize <= 0)
			break;

		guardData = (BYTE*)new char[guardDataSize];
		if (!guardData)
			break;

		DWORD redSize = 0;
		ReadFile(hFile, guardData, guardDataSize, &redSize, NULL);
		if (redSize != guardDataSize)
			break;

		boolinit = true;

	} while (false);

	if (hFile) {
		CloseHandle(hFile);
		hFile = NULL;
	}
	if (false == boolinit)
	{
		if (guardData)
			delete[] guardData;
		return ip_port;
	}

	// \r\n�и�
	std::vector<std::wstring> vector_;
	char* vector_routeip = strtok((char*)guardData, "\r\n");
	if (NULL == vector_routeip)
		vector_routeip = strtok((char*)guardData, "\n");
	if (NULL == vector_routeip)
		vector_routeip = strtok((char*)guardData, "\r");
	while (vector_routeip != NULL)
	{
		vector_.push_back(GetWStringByChar(vector_routeip));
		vector_routeip = strtok(NULL, "\r");
	}

	// find "xx"����������
	ip_port.clear();
	for (size_t idx = 0; idx < vector_.size(); ++idx)
	{
		const int start = vector_[idx].find_first_of('"');
		const int end = vector_[idx].find_last_of('"');
		if (idx == 0)
		{
			ip_port += L"-ip ";
		}
		else if (idx == 1)
		{
			ip_port += L" -p ";
		}
		else
			break;
		ip_port += vector_[idx].substr(start + 1, end - start - 1);

	}

	if (guardData)
		delete[] guardData;
	return ip_port;
}
// ��������Ƿ�װ
bool DrvCheckStart()
{
	std::wstring pszCmd = L"sc start sysmondriver";
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	int nSeriverstatus = g_DrvManager.nf_GetServicesStatus(g_drverName.c_str());
	switch (nSeriverstatus)
	{
		// ��������
	case SERVICE_CONTINUE_PENDING:
	case SERVICE_RUNNING:
	case SERVICE_START_PENDING:
	{
		OutputDebugString(L"Driver Running");
		break;
	}
	break;
	// �Ѱ�װ - δ����
	case SERVICE_STOPPED:
	case SERVICE_STOP_PENDING:
	{
		GetStartupInfo(&si);
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.wShowWindow = SW_HIDE;
		// ����������
		PROCESS_INFORMATION pi;
		CreateProcess(NULL, (LPWSTR)pszCmd.c_str(), NULL, NULL, TRUE, NULL, NULL, NULL, &si, &pi);
		Sleep(3000);
		nSeriverstatus = g_DrvManager.nf_GetServicesStatus(g_drverName.c_str());
		if (SERVICE_RUNNING == nSeriverstatus)
		{
			OutputDebugString(L"sc Driver Running");
			break;
		}
		else
		{
			OutputDebugString(L"sc Driver Install Failuer");
			return false;
		}
	}
	break;
	default:
	{	//��δ��װ������ʱ������
		const int nret = MessageBox(NULL, L"�����ں˲ɼ���Ҫ��װ������ϵͳ��δ��װ\nʾ������û��ǩ��,�����д�ǩ�����߹ر�ϵͳ����ǩ����֤��װ.\n�Ƿ����������װ�����ں�̬�ɼ�\n", L"��ʾ", MB_OKCANCEL | MB_ICONWARNING);
		if (nret == 1)
		{
			wchar_t output[MAX_PATH] = { 0, };
			wsprintf(output, L"[Hades] SysMaver: %d SysMiver: %d", SYSTEMPUBLIC::sysattriinfo.verMajorVersion, SYSTEMPUBLIC::sysattriinfo.verMinorVersion);
			OutputDebugStringW(output);
			
			if (!g_DrvManager.nf_DriverInstall_Start(SYSTEMPUBLIC::sysattriinfo.verMajorVersion, SYSTEMPUBLIC::sysattriinfo.verMinorVersion, SYSTEMPUBLIC::sysattriinfo.Is64))
			{
				MessageBox(NULL, L"������װʧ�ܣ������ֶ���װ�ٴο����ں�̬�ɼ�", L"��ʾ", MB_OKCANCEL);
				return false;
			}
		}
		else
			return false;
	}
	break;
	}

	return true;
}
// ��������
void killProcess(const wchar_t* const processname)
{

	HANDLE hSnapshort = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshort == INVALID_HANDLE_VALUE)
	{
		return;
	}

	// ����߳��б�  
	PROCESSENTRY32 stcProcessInfo;
	stcProcessInfo.dwSize = sizeof(stcProcessInfo);
	BOOL  bRet = Process32First(hSnapshort, &stcProcessInfo);
	while (bRet)
	{
		if (lstrcatW(stcProcessInfo.szExeFile, processname) == 0)
		{
			HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, stcProcessInfo.th32ProcessID);
			::TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
			break;
		}
		bRet = Process32Next(hSnapshort, &stcProcessInfo);
	}

	CloseHandle(hSnapshort);
}
// ��������
void StartProcess(std::wstring& cmdline)
{
	// ����
	wchar_t szModule[1024] = { 0, };
	GetModuleFileName(NULL, szModule, sizeof(szModule) / sizeof(char));
	std::wstring dirpath = szModule;
	if (0 >= dirpath.size())
		return;
	int offset = dirpath.rfind(L"\\");
	if (0 >= offset)
		return;
	dirpath = dirpath.substr(0, offset + 1);

	cmdline = L"\"";
	cmdline += dirpath;

	HANDLE hToken = NULL;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	void* Environ;
	if (!CreateEnvironmentBlock(&Environ, hToken, FALSE))
		Environ = NULL;

	RtlZeroMemory(&si, sizeof(STARTUPINFO));
	RtlZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);
	si.lpReserved = NULL;
	si.lpDesktop = NULL;
	si.lpTitle = NULL;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.cbReserved2 = NULL;
	si.lpReserved2 = NULL;
	// Read Local Config
	std::wstring filepath = dirpath + L"config\\client_config";
	std::wstring ip_port_cmdline = ReadConfigtoIpPort(filepath);
	if (ip_port_cmdline.empty())
		ip_port_cmdline = L"-ip localhost -p 8888";

	// Start HadesSvc.exe
#ifdef _WIN64
#ifdef _DEBUG
	cmdline += L"HadesSvc_d64.exe\" ";
#else
	cmdline += L"HadesSvc64.exe\" ";
#endif
#else
#ifdef _DEBUG
	cmdline += L"HadesSvc_d.exe\" ";
#else
	cmdline += L"HadesSvc.exe\" ";
#endif
#endif

	// "HadesSvc_d.exe" -ip localhost -p 8888
	cmdline += ip_port_cmdline;
	wchar_t ipport_arg[MAX_PATH] = { 0, };
	if (cmdline.size() <= MAX_PATH)
		lstrcpyW(ipport_arg, cmdline.c_str());
	//BOOL ok = CreateProcessAsUser(
	//	hToken, cmdline.c_str(), NULL, NULL, NULL, FALSE,
	//	(Environ ? CREATE_UNICODE_ENVIRONMENT : 0),
	//	Environ, NULL, &si, &pi);
	BOOL ok = CreateProcess(NULL, (LPWSTR)cmdline.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);

	if (Environ)
		DestroyEnvironmentBlock(Environ);

	if (ok) {

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

// HadesSvc���̣���ֹ������HadesSvc�ҵ�������û�и�֪
void MainWindow::GetHadesSvctStatus()
{
	//���HadesSvc�Ƿ����
	for (;;)
	{
		auto active_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\HadesSvc_EVENT");
		if (0 >= (int)active_event)
		{
			if (true == m_hadesSvcStatus)
			{
				// HadesSvc���ߣ�δ����
				// ���½���״̬
				m_pImage_lab = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("ServerConnectImg")));
				m_pImage_lab->SetBkImage(L"img/normal/winmain_connectfailuer1.png");
				m_pConnectSvc_lab->SetText(L"δ����ƽ̨");
				g_hadesStatuscs.lock();
				m_hadesSvcStatus = false;
				g_hadesStatuscs.unlock();
			}	
		}
		CloseHandle(active_event);
		Sleep(1000);
	}
}
static DWORD WINAPI HadesSvcActiveEventNotify(LPVOID lpThreadParameter)
{
	(reinterpret_cast<MainWindow*>(lpThreadParameter))->GetHadesSvctStatus();
	return 0;
}

// �ȴ�HadesSvc�Ƿ�����GRPC�ɹ�����
void MainWindow::GetHadesSvcConnectStatus()
{
	m_pImage_lab = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("ServerConnectImg")));
	m_pConnectSvc_lab = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("ServerConnectStatus")));
	for (;;)
	{
		WaitForSingleObject(m_HadesControlEvent, INFINITE);
		// ���½���״̬
		m_pImage_lab->SetBkImage(L"img/normal/winmain_connectsuccess.png");
		m_pConnectSvc_lab->SetText(L"������ƽ̨");
		g_hadesStatuscs.lock();
		m_hadesSvcStatus = true;
		g_hadesStatuscs.unlock();
	}
}
static DWORD WINAPI HadesConnectEventNotify(LPVOID lpThreadParameter)
{
	//�ȴ�Grpc���� - ���򲻼���
	(reinterpret_cast<MainWindow*>(lpThreadParameter))->GetHadesSvcConnectStatus();
	return 0;
}

// HadesSvc�ػ�����
void MainWindow::HadesSvcDaemon()
{
	// ��Ϊ���̸߳�����������m_hadesSvcStatus��־λ����˲����£���Ҫ�ȴ�5s����
	Sleep(5000);
	while (true)
	{
		if (false == m_hadesSvcStatus)
		{
			StartProcess(m_cmdline);
		}
		Sleep(5000);
	}
}
static DWORD WINAPI HadesSvcDeamonNotify(LPVOID lpThreadParameter)
{
	(reinterpret_cast<MainWindow*>(lpThreadParameter))->HadesSvcDaemon();
	return 0;
}

CDuiString MainWindow::GetSkinFile()
{
	return _T("MainWindow.xml");
}
CDuiString MainWindow::GetSkinFolder()
{
	return _T("");
}
LPCTSTR MainWindow::GetWindowClassName() const
{
	return _T("HadesMainWindow");
}

void MainWindow::InitWindows()
{
	try
	{
		//��ʼ������
		Systeminfolib libobj;
		CLabelUI* pCurrentUser_lab = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("mainwin_currentuser_lab")));
		pCurrentUser_lab->SetText(GetWStringByChar(SYSTEMPUBLIC::sysattriinfo.currentUser.c_str()).c_str());
		CLabelUI* pCpu_lab = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("mainwin_cpu_lab")));
		pCpu_lab->SetText(GetWStringByChar(SYSTEMPUBLIC::sysattriinfo.cpuinfo.c_str()).c_str());
		CLabelUI* pSysver_lab = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("mainwin_sysver_lab")));
		pSysver_lab->SetText(GetWStringByChar(SYSTEMPUBLIC::sysattriinfo.verkerlinfo.c_str()).c_str());
		CLabelUI* pMainbocard_lab = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("mainwin_mainbocard_lab")));
		if (!SYSTEMPUBLIC::sysattriinfo.mainboard.empty())
			pMainbocard_lab->SetText(GetWStringByChar(SYSTEMPUBLIC::sysattriinfo.mainboard[0].c_str()).c_str());
		CLabelUI* pBattery_lab = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("mainwin_battery_lab")));
		if (!SYSTEMPUBLIC::sysattriinfo.monitor.empty())
			pBattery_lab->SetText(GetWStringByChar(SYSTEMPUBLIC::sysattriinfo.monitor[0].c_str()).c_str());

		pMainOptemp = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(_T("MainOptemperature_VLayout")));
		pMainOpcpu = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(_T("MainOpCpu_VLayout")));
		pMainOpbox = static_cast<CHorizontalLayoutUI*>(m_PaintManager.FindControl(_T("MainOpBox_VLayout")));
		pMainOptemp->SetVisible(false);
		pMainOpbox->SetVisible(false);
		pMainOpcpu->SetVisible(true);
	}
	catch (const std::exception&)
	{

	}
}
void MainWindow::AddTrayIcon() {
	memset(&m_trayInfo, 0, sizeof(NOTIFYICONDATA));
	m_trayInfo.cbSize = sizeof(NOTIFYICONDATA);
	m_trayInfo.hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_SMALL));
	m_trayInfo.hWnd = m_hWnd;
	lstrcpy(m_trayInfo.szTip, _T("Hades"));
	m_trayInfo.uCallbackMessage = WM_SHOWTASK;
	m_trayInfo.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	Shell_NotifyIcon(NIM_ADD, &m_trayInfo);
	ShowWindow(SW_HIDE);
}
LRESULT MainWindow::OnTrayIcon(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (lParam == WM_LBUTTONDOWN)
	{
		Shell_NotifyIcon(NIM_DELETE, &m_trayInfo);
		ShowWindow(SW_SHOWNORMAL);
	}
	if (lParam == WM_RBUTTONDOWN)
	{
		POINT pt; 
		GetCursorPos(&pt);
		SetForegroundWindow(m_hWnd);
		HMENU hMenu;
		hMenu = CreatePopupMenu();
		AppendMenu(hMenu, MF_STRING, WM_ONCLOSE, _T("�˳�"));
		AppendMenu(hMenu, MF_STRING, WM_ONOPEN, _T("��������"));
		int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, NULL, m_hWnd, NULL);
		if (cmd == WM_ONCLOSE)
		{
			m_trayInfo.hIcon = NULL;	
			::PostQuitMessage(0);
		}
		else if (cmd == WM_ONOPEN)
		{
			ShowWindow(SW_SHOW);
		}
		Shell_NotifyIcon(NIM_DELETE, &m_trayInfo);
	}
	bHandled = true;
	return 0;
}
LRESULT MainWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lRes = __super::OnCreate(uMsg, wParam, lParam, bHandled);

	m_HadesControlEvent = CreateEvent(NULL, FALSE, FALSE, L"Global\\HadesContrl_Event");

	// Create Meue
	m_pMenu = new Menu();
	m_pMenu->Create(m_hWnd, _T(""), WS_POPUP, WS_EX_TOOLWINDOW);
	m_pMenu->ShowWindow(false);

	// ��ʼ����������
	InitWindows();

	// ��������֮ǰHadesSvc����������Ҫǿ���˳�Svc
	do {
		HANDLE active_event = nullptr;
		active_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\HadesSvc_EVENT");
		if (active_event)
		{
			CloseHandle(active_event);
			active_event = nullptr;
			auto exithandSvc = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\HadesSvc_EVNET_EXIT");
			if (exithandSvc)
			{
				SetEvent(exithandSvc);
				CloseHandle(exithandSvc);
				Sleep(500);
			}
#ifdef _WIN64
#ifdef _DEBUG
			const wchar_t* killname = L"HadesSvc_d64.exe";
#else
			const wchar_t* killname = L"HadesSvc64.exe";
#endif
#else
#ifdef _DEBUG
			const wchar_t* killname = L"HadesSvc_d.exe";
#else
			const wchar_t* killname = L"HadesSvc.exe";
#endif
#endif
			killProcess(killname);
			active_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\HadesSvc_EVENT");
			if (active_event)
			{
				OutputDebugString(L"HadesSvc�Ѿ����������ֶ�����������������");
				CloseHandle(active_event);
				active_event = nullptr;
				return lRes;
			}
		}
	} while (false);
	
	// �ȴ�HadesSvc����Grpc����ɹ�״̬����
	CreateThread(NULL, NULL, HadesConnectEventNotify, this, 0, 0);
	Sleep(100);
	// ���HadesSvc�Ƿ��Ծ������GRPC����״̬
	CreateThread(NULL, NULL, HadesSvcActiveEventNotify, this, 0, 0);
	Sleep(100);
	// ����HadesSvc����
	if (false == m_hadesSvcStatus)
		StartProcess(m_cmdline);
	Sleep(100);
	// ����HadesSvc�ػ�����
	CreateThread(NULL, NULL, HadesSvcDeamonNotify, this, 0, 0);
	// ���ö�ʱ��
	SetTimer(m_hWnd, 1, 1000, NULL);
	// ����SocketServer�ȴ�HadesSvc
	CreateThread(NULL, NULL, StartIocpWorkNotify, this, 0, 0);
	return lRes;
}
LRESULT MainWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	KillTimer(m_hWnd, 1);
	auto exithandSvc = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"Global\\HadesSvc_EVNET_EXIT");
	if (exithandSvc)
	{
		SetEvent(exithandSvc);
		CloseHandle(exithandSvc);
	}
	if (m_HadesControlEvent)
		CloseHandle(m_HadesControlEvent);
	Sleep(100);
#ifdef _WIN64
#ifdef _DEBUG
	const wchar_t killname[] = L"HadesSvc_d64.exe";
#else
	const wchar_t killname[] = L"HadesSvc64.exe";
#endif
#else
#ifdef _DEBUG
	const wchar_t killname[] = L"HadesSvc_d.exe";
#else
	const wchar_t killname[] = L"HadesSvc.exe";
#endif
#endif
	
	killProcess(killname);
	auto IocpExEvt = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"HpStopTcpSvcEvent");
	if (IocpExEvt)
	{
		SetEvent(IocpExEvt);
		Sleep(100);
		CloseHandle(IocpExEvt);
	}
	return __super::OnClose(uMsg, wParam, lParam, bHandled);
}

void MainWindow::FlushData()
{
	try
	{
		//cpu
		const double cpuutilize = g_DynSysBaseinfo.GetSysDynCpuUtiliza();
		CString m_Cpusyl;
		m_Cpusyl.Format(L"CPU: %0.2lf", cpuutilize);
		m_Cpusyl += "%";
		CLabelUI* pCpuut = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("winmain_layout_cpuinfo")));
		pCpuut->SetText(m_Cpusyl.GetBuffer());

		//memory
		const DWORD dwMem = g_DynSysBaseinfo.GetSysDynSysMem();
		// ��ǰռ���� Occupancy rate
		CString m_MemoryBFB;
		m_MemoryBFB.Format(L"�ڴ�: %u", dwMem);
		m_MemoryBFB += "%";
		CLabelUI* pMem = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("winmain_layout_memory")));
		pMem->SetText(m_MemoryBFB.GetBuffer());
	}
	catch (const std::exception&)
	{

	}
}
static DWORD WINAPI ThreadFlush(LPVOID lpThreadParameter)
{
	(reinterpret_cast<MainWindow*>(lpThreadParameter))->FlushData();
	return 0;
}
LRESULT MainWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	QueueUserWorkItem(ThreadFlush, this, WT_EXECUTEDEFAULT);
	bHandled = false;
	return 0;
}

void MainWindow::Notify(TNotifyUI& msg)
{
	CDuiString strClassName = msg.pSender->GetClass();
	CDuiString strControlName = msg.pSender->GetName();

	if (msg.sType == DUI_MSGTYPE_WINDOWINIT);
	else if (msg.sType == DUI_MSGTYPE_CLICK)
	{
		if (strClassName == DUI_CTR_BUTTON)
		{
			if (strControlName == _T("MainCloseBtn"))
			{
				const int nret = MessageBox(m_hWnd, L"����ر�,��ϣ���Ƿ����������̣�", L"��ʾ", MB_OKCANCEL | MB_ICONWARNING);
				if (1 == nret)
					AddTrayIcon();
				else
					Close();
			}
			else if (strControlName == _T("MainMenuBtn"))
			{//�˵�
				int xPos = msg.pSender->GetPos().left - 36;
				int yPos = msg.pSender->GetPos().bottom;
				POINT pt = { xPos, yPos };
				ClientToScreen(m_hWnd, &pt);
				m_pMenu->ShowWindow(true);
				::SetWindowPos(m_pMenu->GetHWND(), NULL, pt.x, pt.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
			}
			//��С��
			else if (strControlName == _T("MainMinsizeBtn"))
			{
				::ShowWindow(m_hWnd, SW_MINIMIZE);
			}
		}
		else if (strClassName == DUI_CTR_OPTION)
		{
			if (_tcscmp(static_cast<COptionUI*>(msg.pSender)->GetGroup(), _T("MainOpView")) == 0)
			{

				if (strControlName == _T("MainMontemperatureOpt"))
				{
					// MainOptemperature_VLayout
					pMainOptemp->SetVisible(true);
					pMainOpcpu->SetVisible(false);
					pMainOpbox->SetVisible(false);
				}
				else if (strControlName == _T("MainMonCpuOpt"))
				{
					// MainOpCpu_VLayout
					pMainOptemp->SetVisible(false);
					pMainOpcpu->SetVisible(true);
					pMainOpbox->SetVisible(false);
				}
				else if (strControlName == _T("MainMonBoxOpt"))
				{
					// MainOpBox_VLayout
					pMainOptemp->SetVisible(false);
					pMainOpcpu->SetVisible(false);
					pMainOpbox->SetVisible(true);
				}
			}
			else if (strControlName == _T("MainMonUserBtn"))
			{//�·��û�̬���ָ��
				COptionUI* pOption = static_cast<COptionUI*>(m_PaintManager.FindControl(_T("MainMonUserBtn")));
				if (!pOption)
					return;
				if (false == m_hadesSvcStatus)
				{
					pOption->Selected(true);
					MessageBox(m_hWnd, L"��������Grpc�ϱ�ƽ̨�������ɼ�", L"��ʾ", MB_OK);
					return;
				}
				HWND m_SvcHwnd = FindWindow(L"HadesSvc", L"HadesSvc");
				COPYDATASTRUCT c2_;
				c2_.dwData = 1;
				c2_.cbData = 0;
				c2_.lpData = NULL;
				//������Ϣ
				::SendMessage(m_SvcHwnd, WM_COPYDATA, NULL, (LPARAM)&c2_);
			}
			else if (strControlName == _T("MainMonKerBtn"))
			{//�·��ں�̬���ָ��
				COptionUI* pOption = static_cast<COptionUI*>(m_PaintManager.FindControl(_T("MainMonKerBtn")));
				if (!pOption)
					return;
				if (false == m_hadesSvcStatus)
				{
					pOption->Selected(true);
					MessageBox(m_hWnd, L"��������Grpc�ϱ�ƽ̨�������ɼ�", L"��ʾ", MB_OK);
					return;
				}
				if (SYSTEMPUBLIC::sysattriinfo.verMajorVersion < 6)
				{
					pOption->Selected(true);
					MessageBox(m_hWnd, L"��ǰϵͳ����ģʽ�����ݣ��뱣֤����ϵͳwin7~win10֮��", L"��ʾ", MB_OK);
					return;
				}
				const bool nret = DrvCheckStart();
				if (true == nret)
				{
					HWND m_SvcHwnd = FindWindow(L"HadesSvc", L"HadesSvc");
					COPYDATASTRUCT c2_;
					c2_.dwData = 2;
					c2_.cbData = 0;
					c2_.lpData = NULL;
					::SendMessage(m_SvcHwnd, WM_COPYDATA, NULL, (LPARAM)&c2_);
				}
				else {
					pOption->Selected(true);
					MessageBox(m_hWnd, L"�ں�̬�������ʧ��\n��ʹ��cmd: sc query/delete hadesmondrv�鿴����״̬\ndeleteɾ���������¿�����", L"��ʾ", MB_OK);
				}
			}
			else if (strControlName == _T("MainMonBeSnipingBtn"))
			{//���ض�����Ϊ
				COptionUI* pOption = static_cast<COptionUI*>(m_PaintManager.FindControl(_T("MainMonBeSnipingBtn")));
				if (!pOption)
					return;
				if (false == m_hadesSvcStatus)
				{
					pOption->Selected(true);
					MessageBox(m_hWnd, L"��������Grpc�ϱ�ƽ̨�������ɼ�", L"��ʾ", MB_OK);
					return;
				}
				if (SYSTEMPUBLIC::sysattriinfo.verMajorVersion < 6)
				{
					pOption->Selected(true);
					MessageBox(m_hWnd, L"��ǰϵͳ����ģʽ�����ݣ��뱣֤����ϵͳwin7~win10֮��", L"��ʾ", MB_OK);
					return;
				}
				const bool nret = DrvCheckStart();
				if (true == nret)
				{
					HWND m_SvcHwnd = FindWindow(L"HadesSvc", L"HadesSvc");
					COPYDATASTRUCT c2_;
					c2_.dwData = 3;
					c2_.cbData = 0;
					c2_.lpData = NULL;
					::SendMessage(m_SvcHwnd, WM_COPYDATA, NULL, (LPARAM)&c2_);
				}
				else {
					pOption->Selected(true);
					MessageBox(m_hWnd, L"�ں�̬�������ʧ��\n��ʹ��cmd: sc query/delete hadesmondrv�鿴����״̬\ndeleteɾ���������¿�����", L"��ʾ", MB_OK);
				}
			}
		}
	}
}
LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRes = 0;
	BOOL bHandled = TRUE;
	return __super::HandleMessage(uMsg, wParam, lParam);
}
LRESULT MainWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	LRESULT lRes = 0;
	bHandled = TRUE;

	switch (uMsg) {
	case WM_TIMER: lRes = OnTimer(uMsg, wParam, lParam, bHandled); break;	// ˢ�½�������
	case WM_SHOWTASK: OnTrayIcon(uMsg, wParam, lParam, bHandled); break;	// ���̴���
	case WM_IPS_PROCESS: break;
	default:
		bHandled = FALSE;
		break;
	}
	if (bHandled) return lRes;
	return __super::HandleCustomMessage(uMsg, wParam, lParam, bHandled);
}