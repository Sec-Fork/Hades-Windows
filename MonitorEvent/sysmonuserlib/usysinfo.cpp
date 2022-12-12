#include <Windows.h>
#include "usysinfo.h"
#include "utilstool.h"
#include <sysinfo.h>
#include <BluetoothAPIs.h>  
#include <amvideo.h>
#include <vector>
#include <strmif.h>
#include <uuids.h>
#include <iostream>
#include <Vfw.h>
#include <DXGI.h> 

#pragma comment(lib,"Strmiids.lib")
#pragma comment(lib,"Bthprops.lib") 
#pragma comment(lib, "DXGI.lib")

#ifdef _WIN64
    extern "C" void __stdcall GetCpuid(ULONG64 * deax, ULONG64 * debx, ULONG64 * decx, ULONG64 * dedx);
    extern "C" void __stdcall GetCpuInfos(char* cProStr);
#else
    extern "C" void __stdcall GetCpuid(DWORD * deax, DWORD * debx, DWORD * decx, DWORD * dedx, char* cProStr);
#endif // _WIN64



// View: ϵͳ�汾
void USysBaseInfo::GetOSVersion(std::string& strOSVersion, int& verMajorVersion, int& verMinorVersion, bool& Is64)
{
    try
    {
        CStringA tmpbuffer;
        std::string str;
        OSVERSIONINFOEX osvi;
        SYSTEM_INFO si;
        
        ZeroMemory(&si, sizeof(SYSTEM_INFO));
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        BOOL bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*)&osvi);
        if (!bOsVersionInfoEx)
        {
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*)&osvi);
        }
        if (!bOsVersionInfoEx)
            return;

        GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
        GetSystemInfo(&si);
        verMajorVersion = osvi.dwMajorVersion;
        verMinorVersion = osvi.dwMinorVersion;
        switch (osvi.dwPlatformId)
        {
        case VER_PLATFORM_WIN32_NT:
            if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2)
            {
                str = "Windows 10 ";
            }
            if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
            {
                str = "Windows 7 ";
            }
            if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
            {
                if (osvi.wProductType == VER_NT_WORKSTATION)
                {
                    str = "Windows Vista ";
                }
                else
                {
                    str = "Windows Server \"Longhorn\" ";
                }
            }
            if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
            {
                if (GetSystemMetrics(SM_SERVERR2))
                {
                    str = "Microsoft Windows Server 2003 \"R2\" ";
                }
                else if (osvi.wProductType == VER_NT_WORKSTATION &&
                    si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
                {
                    str = "Microsoft Windows XP Professional x64 Edition ";
                }
                else
                {
                    str = "Microsoft Windows Server 2003, ";
                }
            }
            if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
            {
                str = "Microsoft Windows XP ";
            }
            if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
            {
                str = "Microsoft Windows 2000 ";
            }
            if (osvi.dwMajorVersion <= 4)
            {
                str = "Microsoft Windows NT ";
            }

            // Test for specific product on Windows NT 4.0 SP6 and later.  
            if (bOsVersionInfoEx)
            {
                //tmpbuffer.Format("Service Pack %d", osvi.wServicePackMajor);
                //strServiceVersion = tmpbuffer.GetBuffer();
                // Test for the workstation type.  
                if (osvi.wProductType == VER_NT_WORKSTATION &&
                    si.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64)
                {
                    if (osvi.dwMajorVersion == 4)
                        str = str + "Workstation 4.0";
                    else if (osvi.wSuiteMask & VER_SUITE_PERSONAL)
                        str = str + "Home Edition";
                    else str = str + "Professional";
                }

                // Test for the server type.  
                else if (osvi.wProductType == VER_NT_SERVER ||
                    osvi.wProductType == VER_NT_DOMAIN_CONTROLLER)
                {
                    if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
                    {
                        if (si.wProcessorArchitecture ==
                            PROCESSOR_ARCHITECTURE_IA64)
                        {
                            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                str = str + "Datacenter Edition for Itanium-based Systems";
                            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                str = str + "Enterprise Edition for Itanium-based Systems";
                        }

                        else if (si.wProcessorArchitecture ==
                            PROCESSOR_ARCHITECTURE_AMD64)
                        {
                            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                str = str + "Datacenter x64 Edition ";
                            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                str = str + "Enterprise x64 Edition ";
                            else str = str + "Standard x64 Edition ";
                        }

                        else
                        {
                            if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                                str = str + "Datacenter Edition ";
                            else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                                str = str + "Enterprise Edition ";
                            else if (osvi.wSuiteMask & VER_SUITE_BLADE)
                                str = str + "Web Edition ";
                            else str = str + "Standard Edition ";
                        }
                    }
                    else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
                    {
                        if (osvi.wSuiteMask & VER_SUITE_DATACENTER)
                            str = str + "Datacenter Server ";
                        else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                            str = str + "Advanced Server ";
                        else str = str + "Server ";
                    }
                    else  // Windows NT 4.0   
                    {
                        if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE)
                            str = str + "Server 4.0, Enterprise Edition ";
                        else str = str + "Server 4.0 ";
                    }
                }
            }
            // Test for specific product on Windows NT 4.0 SP5 and earlier  
            else
            {
                HKEY hKey;
                TCHAR szProductType[256];
                DWORD dwBufLen = 256 * sizeof(TCHAR);
                LONG lRet;

                lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    L"SYSTEM\\CurrentControlSet\\Control\\ProductOptions", 0, KEY_QUERY_VALUE, &hKey);
                if (lRet != ERROR_SUCCESS)
                    strOSVersion = str;
                return;

                lRet = RegQueryValueEx(hKey, TEXT("ProductType"),
                    NULL, NULL, (LPBYTE)szProductType, &dwBufLen);
                RegCloseKey(hKey);

                if ((lRet != ERROR_SUCCESS) ||
                    (dwBufLen > 256 * sizeof(TCHAR)))
                    strOSVersion = str;
                return;

                if (lstrcmpi(TEXT("WINNT"), szProductType) == 0)
                    str = str + "Workstation ";
                if (lstrcmpi(TEXT("LANMANNT"), szProductType) == 0)
                    str = str + "Server ";
                if (lstrcmpi(TEXT("SERVERNT"), szProductType) == 0)
                    str = str + "Advanced Server ";
                tmpbuffer.Format("%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion);
                str = tmpbuffer.GetString();
            }

            // Display service pack (if any) and build number.  

            if (osvi.dwMajorVersion == 4 &&
                lstrcmpi(osvi.szCSDVersion, L"Service Pack 6") == 0)
            {
                HKEY hKey;
                LONG lRet;

                // Test for SP6 versus SP6a.  
                lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009", 0, KEY_QUERY_VALUE, &hKey);
                if (lRet == ERROR_SUCCESS)
                {
                    tmpbuffer.Format(("Service Pack 6a (Build %d)\n"), osvi.dwBuildNumber & 0xFFFF);
                    str = tmpbuffer.GetBuffer();
                }
                else // Windows NT 4.0 prior to SP6a  
                {
                    _tprintf(TEXT("%s (Build %d)\n"),
                        osvi.szCSDVersion,
                        osvi.dwBuildNumber & 0xFFFF);
                }

                RegCloseKey(hKey);
            }
            else // not Windows NT 4.0   
            {
                _tprintf(TEXT("%s (Build %d)\n"),
                    osvi.szCSDVersion,
                    osvi.dwBuildNumber & 0xFFFF);
            }

            break;

            // Test for the Windows Me/98/95.  
        case VER_PLATFORM_WIN32_WINDOWS:

            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
            {
                str = "Microsoft Windows 95 ";
                if (osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B')
                    str = str + "OSR2 ";
            }
            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
            {
                str = "Microsoft Windows 98 ";
                if (osvi.szCSDVersion[1] == 'A' || osvi.szCSDVersion[1] == 'B')
                    str = str + "SE ";
            }
            if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
            {
                str = "Microsoft Windows Millennium Edition\n";
            }
            break;

        case VER_PLATFORM_WIN32s:
            str = "Microsoft Win32s\n";
            break;
        default:
            break;
        }

        GetNativeSystemInfo(&si);
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
            si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
        {
            Is64 = true;
            str += " x64";
        }
        else
        {
            Is64 = false;
            str += " x32";
        }

        strOSVersion = str;
    }
    catch (const std::exception&)
    {
    }
}
// View: ϵͳ����
void USysBaseInfo::GetDiskInfo(std::vector<std::string>& diskinfo)
{
    try
    {
        DWORD DiskCount = 0;
        //����GetLogicalDrives()�������Ի�ȡϵͳ���߼����������������������ص���һ��32λ�޷����������ݡ�  
        DWORD DiskInfo = GetLogicalDrives();
        //ͨ��ѭ�������鿴ÿһλ�����Ƿ�Ϊ1�����Ϊ1�����Ϊ��,���Ϊ0����̲����ڡ�  
        while (DiskInfo)
        {
            //ͨ��λ������߼���������ж��Ƿ�Ϊ1  
            Sleep(10);
            if (DiskInfo & 1)
            {
                DiskCount++;
            }
            DiskInfo = DiskInfo >> 1;//ͨ��λ��������Ʋ�����֤ÿѭ��һ��������λ�������ƶ�һλ��*/  
        }

        //-------------------------------------------------------------------//  
        //ͨ��GetLogicalDriveStrings()������ȡ�����������ַ�����Ϣ����  
        int DSLength = GetLogicalDriveStrings(0, NULL);

        WCHAR* DStr = new WCHAR[DSLength];
        if (!DStr)
            return;
        memset(DStr, 0, DSLength);

        //ͨ��GetLogicalDriveStrings���ַ�����Ϣ���Ƶ�����������,���б�������������������Ϣ��  
        GetLogicalDriveStrings(DSLength, DStr);

        int DType;
        int si = 0;
        BOOL fResult;
        unsigned _int64 i64FreeBytesToCaller;
        unsigned _int64 i64TotalBytes;
        unsigned _int64 i64FreeBytes;

        //��ȡ����������Ϣ������DStr�ڲ����ݸ�ʽ��A:\NULLB:\NULLC:\NULL������DSLength/4���Ի�þ����ѭ����Χ  
        for (int i = 0; i < DSLength / 4; ++i)
        {
            Sleep(10);
            CStringA strdriver = DStr + (i * 4);
            CStringA strTmp, strTotalBytes, strFreeBytes;
            DType = GetDriveTypeA(strdriver);//GetDriveType���������Ի�ȡ���������ͣ�����Ϊ�������ĸ�Ŀ¼  
            switch (DType)
            {
            case DRIVE_FIXED:
            {
                strTmp.Format("���ش���");
            }
            break;
            case DRIVE_CDROM:
            {
                strTmp.Format("DVD������");
            }
            break;
            case DRIVE_REMOVABLE:
            {
                strTmp.Format("���ƶ�����");
            }
            break;
            case DRIVE_REMOTE:
            {
                strTmp.Format("�������");
            }
            break;
            case DRIVE_RAMDISK:
            {
                strTmp.Format("����RAM����");
            }
            break;
            case DRIVE_UNKNOWN:
            {
                strTmp.Format("����RAMδ֪�豸");
            }
            break;
            default:
                strTmp.Format("δ֪�豸");
                break;
            }

            //GetDiskFreeSpaceEx���������Ի�ȡ���������̵Ŀռ�״̬,�������ص��Ǹ�BOOL��������  
            fResult = GetDiskFreeSpaceExA(strdriver,
                (PULARGE_INTEGER)&i64FreeBytesToCaller,
                (PULARGE_INTEGER)&i64TotalBytes,
                (PULARGE_INTEGER)&i64FreeBytes);

            if (fResult)
            {
                strTotalBytes.Format(("����������%fMB"), (float)i64TotalBytes / 1024 / 1024);
                strFreeBytes.Format(("����ʣ��ռ�%fMB"), (float)i64FreeBytesToCaller / 1024 / 1024);
            }
            else
            {
                strTotalBytes.Format("");
                strFreeBytes.Format("");
            }
            auto tmpstr = strTmp + _T("(") + strdriver + _T("):") + strTotalBytes + strFreeBytes;
            diskinfo.push_back(tmpstr.GetBuffer());
            si += 4;
        }
    }
    catch (const std::exception&)
    {
    }  
}
// View: ϵͳ����
void USysBaseInfo::GetDisplayCardInfo(std::vector<std::string>& Cardinfo)
{
    try
    {
        HKEY keyServ;
        HKEY keyEnum;
        HKEY key;
        HKEY key2;
        LONG lResult;//LONG�ͱ��������溯������ֵ  

        //��ѯ"SYSTEM\\CurrentControlSet\\Services"�µ������Ӽ����浽keyServ  
        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services"), 0, KEY_READ, &keyServ);
        if (ERROR_SUCCESS != lResult)
            return;


        //��ѯ"SYSTEM\\CurrentControlSet\\Enum"�µ������Ӽ����浽keyEnum  
        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Enum"), 0, KEY_READ, &keyEnum);
        if (ERROR_SUCCESS != lResult)
            return;

        int i = 0, count = 0;
        DWORD size = 0, type = 0;
        for (;; ++i)
        {
            Sleep(5);
            size = 512;
            TCHAR name[512] = { 0 };//����keyServ�¸�������ֶ�����  

            //���ö��keyServ�µĸ������ֶα��浽name��  
            lResult = RegEnumKeyEx(keyServ, i, name, &size, NULL, NULL, NULL, NULL);

            //Ҫ��ȡ��������ڣ���keyServ������ȫ��������ʱ����ѭ��  
            if (lResult == ERROR_NO_MORE_ITEMS)
                break;

            //��keyServ�������ֶ�Ϊname����ʶ���ֶε�ֵ���浽key  
            lResult = RegOpenKeyEx(keyServ, name, 0, KEY_READ, &key);
            if (lResult != ERROR_SUCCESS)
            {
                RegCloseKey(keyServ);
                return;
            }

            size = 512;
            //��ѯkey�µ��ֶ�ΪGroup���Ӽ��ֶ������浽name  
            lResult = RegQueryValueEx(key, TEXT("Group"), 0, &type, (LPBYTE)name, &size);
            if (lResult == ERROR_FILE_NOT_FOUND)
            {
                //?��������  
                RegCloseKey(key);
                continue;
            };

            //�����ѯ����name����Video��˵���ü������Կ�������  
            if (_tcscmp(TEXT("Video"), name) != 0)
            {
                RegCloseKey(key);
                continue;     //����forѭ��  
            };

            //��������������ִ�еĻ�˵���Ѿ��鵽���й��Կ�����Ϣ������������Ĵ���ִ����֮��Ҫbreak��һ��forѭ������������  
            lResult = RegOpenKeyEx(key, TEXT("Enum"), 0, KEY_READ, &key2);
            RegCloseKey(key);
            key = key2;
            size = sizeof(count);
            lResult = RegQueryValueEx(key, TEXT("Count"), 0, &type, (LPBYTE)&count, &size);//��ѯCount�ֶΣ��Կ���Ŀ��  

            for (int j = 0; j < count; ++j)
            {
                CHAR sz[512] = { 0 };
                CHAR name[64] = { 0 };
                sprintf(name, "%d", j);
                size = sizeof(sz);
                lResult = RegQueryValueExA(key, name, 0, &type, (LPBYTE)sz, &size);
                lResult = RegOpenKeyExA(keyEnum, sz, 0, KEY_READ, &key2);
                if (lResult == ERROR_SUCCESS)
                {
                    RegCloseKey(keyEnum);
                    return;
                }

                size = sizeof(sz);
                lResult = RegQueryValueExA(key2, "FriendlyName", 0, &type, (LPBYTE)sz, &size);
                if (lResult == ERROR_FILE_NOT_FOUND)
                {
                    size = sizeof(sz);
                    lResult = RegQueryValueExA(key2, "DeviceDesc", 0, &type, (LPBYTE)sz, &size);
                    Cardinfo.push_back(sz);
                };
                RegCloseKey(key2);
                key2 = NULL;
            };
            RegCloseKey(key);
            key = NULL;
            break;
        }
    }
    catch (const std::exception&)
    {
    }
}
void USysBaseInfo::GetDisplayCardInfoWmic(std::vector<std::string>& Cardinfo)
{
    const long MAX_COMMAND_SIZE = 1024 * 4;
    wchar_t szFetCmd[] = L"wmic baseboard get manufacturer,product";
    HANDLE hReadPipe = NULL;  //��ȡ�ܵ�
    HANDLE hWritePipe = NULL; //д��ܵ�	
    PROCESS_INFORMATION pi;   //������Ϣ	
    STARTUPINFO			si;	  //���������д�����Ϣ
    SECURITY_ATTRIBUTES sa;   //��ȫ����

    char			szBuffer[MAX_COMMAND_SIZE + 1] = { 0 };
    std::string		strBuffer;
    unsigned long	count = 0;
    long			ipos = 0;

    memset(&pi, 0, sizeof(pi));
    memset(&si, 0, sizeof(si));
    memset(&sa, 0, sizeof(sa));

    pi.hProcess = NULL;
    pi.hThread = NULL;
    si.cb = sizeof(STARTUPINFO);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    do
    {
        bool bret = false;
        bret = CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);
        if (!bret)
            break;

        GetStartupInfo(&si);
        si.hStdError = hWritePipe;
        si.hStdOutput = hWritePipe;
        si.wShowWindow = SW_HIDE; //���������д���
        si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        bret = CreateProcess(NULL, szFetCmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
        if (!bret)
            break;

        WaitForSingleObject(pi.hProcess, 500/*INFINITE*/);
        bret = ReadFile(hReadPipe, szBuffer, MAX_COMMAND_SIZE, &count, 0);
        if (!bret)
            break;
        bret = FALSE;
        strBuffer = szBuffer;

        //�и� \r\r\n
        std::vector<std::string> vecArray;
        {
            static std::string strSp;
            char* vector_routeip = strtok((char*)strBuffer.data(), "\r\r\n");
            while (vector_routeip != NULL)
            {
                strSp = vector_routeip;
                vecArray.push_back(strSp);
                vector_routeip = strtok(NULL, "\r\r\n");
            }
        }

        const int icount = vecArray.size();
        if (icount < 2)
            break;

        Cardinfo.push_back(vecArray[1]);
    } while (false);
    if (hWritePipe)
        CloseHandle(hWritePipe);
    if (hReadPipe)
        CloseHandle(hReadPipe);
    if (pi.hProcess)
        CloseHandle(pi.hProcess);
    if (pi.hThread)
        CloseHandle(pi.hThread);
}
// View: ϵͳCPU
#ifdef _WIN64
#else
long GetCPUFreq()
{//��ȡCPUƵ��,��λ: MHZ  
    int start, over;
    _asm
    {
        RDTSC
        mov start, eax
    }
    Sleep(50);
    _asm
    {
        RDTSC
        mov over, eax
    }
    return (over - start) / 50000;
}
#endif // _WIN64
void USysBaseInfo::GetSysCpuInfo(std::string& cpuinfo)
{//��������Ϣ
    try
    {
        ULONG64 deax = 0;
        ULONG64 debx = 0;
        ULONG64 decx = 0;
        ULONG64 dedx = 0;
        //����
        char ID[25] = { 0, };
        //�ִ�
        char cProStr[49] = { 0, };
        memset(ID, 0, sizeof(ID));
#ifdef _WIN64
        GetCpuid(&deax, &debx, &decx, &dedx);
        GetCpuInfos(cProStr);
#else
        GetCpuid((DWORD*)&deax, (DWORD*)&debx, (DWORD*)&decx, (DWORD*)&dedx, cProStr);
#endif // _WIN64
        memcpy(ID + 0, &debx, 4);
        memcpy(ID + 4, &dedx, 4);
        memcpy(ID + 8, &decx, 4);
        cpuinfo = ID;
        cpuinfo += " ";
        cpuinfo += cProStr;
    }
    catch (const std::exception&)
    {
    }
}
// View: ϵͳ��� - (�ʼǱ�)���ú����� - ̨ʽ Ҳ�����������ֱʼǱ���̨ʽ����
void USysBaseInfo::Getbattery(std::vector<std::string>& batteryinfo) {
    try
    {
        // ��ȡ������ʹ��ʱ��-����
        SYSTEM_POWER_STATUS powerStatus;
        if (GetSystemPowerStatus(&powerStatus) == 0)
            return;
        CStringA tmpstr, tmpstr1;
        tmpstr.Format("%d_", (int)powerStatus.BatteryLifePercent);  // ����
        tmpstr1 += tmpstr;
        tmpstr.Format("%d_", (int)powerStatus.ACLineStatus);    // ״̬
        tmpstr1 += tmpstr;
        tmpstr.Format("%d_", (int)powerStatus.BatteryLifeTime); // ʣ��ʹ��ʱ��
        tmpstr1 += tmpstr;
        tmpstr.Format("%d_", (int)powerStatus.BatteryFullLifeTime);
        tmpstr1 += tmpstr;
        batteryinfo.push_back(tmpstr1.GetBuffer());
    }
    catch (const std::exception&)
    {

    }
}
// View: ϵͳ����
void USysBaseInfo::GetNetworkCard(std::vector<std::string>& networkcar)
{

}
// View: ϵͳ�Կ�(GPU)
void USysBaseInfo::GetGPU(std::vector<std::string>& monitor) {
    try
    {
        // ��������  
        IDXGIFactory* pFactory;
        IDXGIAdapter* pAdapter;
        std::vector <IDXGIAdapter*> vAdapters;            // �Կ�  
        // �Կ�������  
        int iAdapterNum = 0;
        // ����һ��DXGI����  
        HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
        if (FAILED(hr))
            return;
        // ö��������  
        while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
        {
            vAdapters.push_back(pAdapter);
            ++iAdapterNum;
        }
        for (size_t i = 0; i < vAdapters.size(); i++)
        {
            //std::cout << "Video card" << i + 1 << ":" << std::endl;
            // ��ȡ��Ϣ  
            DXGI_ADAPTER_DESC adapterDesc;
            vAdapters[i]->GetDesc(&adapterDesc);
            std::string bb;
            Wchar_tToString(bb, adapterDesc.Description);
            monitor.push_back(bb.c_str());
            //std::string bb = WStringToString(aa);
            //std::cout << "Video card " << i + 1 << " DedicatedVideoMemory:" << adapterDesc.DedicatedVideoMemory / 1024 / 1024 << "M" << std::endl;
            //std::cout << "Video card " << i + 1 << " SharedSystemMemory:" << adapterDesc.SharedSystemMemory / 1024 / 1024 << "M" << std::endl;
            //std::cout << "ϵͳ��Ƶ�ڴ�:" << adapterDesc.DedicatedSystemMemory / 1024 / 1024 << "M" << std::endl;
            //std::cout << "ר����Ƶ�ڴ�:" << adapterDesc.DedicatedVideoMemory / 1024 / 1024 << "M" << std::endl;
            //std::cout << "����ϵͳ�ڴ�:" << adapterDesc.SharedSystemMemory / 1024 / 1024 << "M" << std::endl;
            //std::cout << "�豸����:" << bb.c_str() << std::endl;
            //std::cout << "�豸ID:" << adapterDesc.DeviceId << std::endl;
            //std::cout << "PCI ID�����汾:" << adapterDesc.Revision << std::endl;
            //std::cout << "��ϵͳPIC ID:" << adapterDesc.SubSysId << std::endl;
            //std::cout << "���̱��:" << adapterDesc.VendorId << std::endl

            // ����豸  
            IDXGIOutput* pOutput;
            std::vector<IDXGIOutput*> vOutputs;
            // ����豸����  
            int iOutputNum = 0;
            while (vAdapters[i]->EnumOutputs(iOutputNum, &pOutput) != DXGI_ERROR_NOT_FOUND)
            {
                vOutputs.push_back(pOutput);
                iOutputNum++;
            }

            //std::cout << std::endl;
            //std::cout << "���Կ���ȡ��" << iOutputNum << "����ʾ�豸:" << std::endl;

            for (size_t n = 0; n < vOutputs.size(); n++)
            {
                // ��ȡ��ʾ�豸��Ϣ  
                DXGI_OUTPUT_DESC outputDesc;
                vOutputs[n]->GetDesc(&outputDesc);

                // ��ȡ�豸֧��  
                UINT uModeNum = 0;
                DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
                UINT flags = DXGI_ENUM_MODES_INTERLACED;

                vOutputs[n]->GetDisplayModeList(format, flags, &uModeNum, 0);
                DXGI_MODE_DESC* pModeDescs = new DXGI_MODE_DESC[uModeNum];
                vOutputs[n]->GetDisplayModeList(format, flags, &uModeNum, pModeDescs);

                //std::cout << "DisplayDevice:" << n + 1 << " Name:" << outputDesc.DeviceName << std::endl;
                //std::cout << "DisplayDevice " << n + 1 << " Resolution ratio:" << outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left << "*" << outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top << std::endl;
                // ��֧�ֵķֱ�����Ϣ  
                //std::cout << "�ֱ�����Ϣ:" << std::endl;
                /*for (UINT m = 0; m < uModeNum; m++)
                {
                    std::cout << "== �ֱ���:" << pModeDescs[m].Width << "*" << pModeDescs[m].Height << "     ˢ����" << (pModeDescs[m].RefreshRate.Numerator) / (pModeDescs[m].RefreshRate.Denominator) << std::endl;
                }*/
            }
            vOutputs.clear();
        }
        vAdapters.clear();
    }
    catch (const std::exception&)
    {

    }
}

// ʱ��ת��
double FILETIMEDouble(const _FILETIME& filetime)
{
    return double(filetime.dwHighDateTime * 4.294967296e9) + double(filetime.dwLowDateTime);
}
// Monitor/View: ϵͳ����
void USysBaseInfo::GetBluetooth(std::vector<std::string>& blueinfo)
{
    HBLUETOOTH_RADIO_FIND hbf = NULL;
    HANDLE hbr = NULL;
    HBLUETOOTH_DEVICE_FIND hbdf = NULL;
    //����BluetoothFindFirstDevice�������������շ�������Ҫ��������������
    BLUETOOTH_FIND_RADIO_PARAMS btfrp = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
    //��ʼ��һ�����������շ�����Ϣ��BLUETOOTH_RADIO_INFO���Ķ���bri
    BLUETOOTH_RADIO_INFO bri = { sizeof(BLUETOOTH_RADIO_INFO) };
    //����BluetoothFindFirstDevice����������Ҫ��������������
    BLUETOOTH_DEVICE_SEARCH_PARAMS btsp = { sizeof(BLUETOOTH_DEVICE_SEARCH_PARAMS) };
    //��ʼ��һ��Զ�������豸��Ϣ��BLUETOOTH_DEVICE_INFO������btdi���Դ����������������豸��Ϣ
    BLUETOOTH_DEVICE_INFO btdi = { sizeof(BLUETOOTH_DEVICE_INFO) };
    //�õ���һ����ö�ٵ������շ����ľ��hbf������BluetoothFindNextRadio��hbr������BluetoothFindFirstDevice��
    //��û���ҵ������������շ�������õ��ľ��hbf=NULL
    //����ɲο�https://msdn.microsoft.com/en-us/library/aa362786(v=vs.85).aspx 
    // ���ʧ���˾��ǵ���û�п�������
    hbf = BluetoothFindFirstRadio(&btfrp, &hbr);
    // MAX 10(�������ֵ)
    int icount = 10;
    bool brfind = (hbf != NULL);
    while (brfind && --icount)
    {
        if (BluetoothGetRadioInfo(hbr, &bri) == ERROR_SUCCESS)//��ȡ�����շ�������Ϣ��������bri��  
        {
            // �����շ���������
            const std::wstring wsName = bri.szName; std::string sName;
            UtilsTool::WStr2Str(wsName, sName);
            if (!sName.empty())
                blueinfo.emplace_back();
            //std::cout << "Class of device: 0x" << uppercase << hex << bri.ulClassofDevice << endl;
            //wcout << "Name:" << bri.szName << endl;  //�����շ���������
            //cout << "Manufacture:0x" << uppercase << hex << bri.manufacturer << endl;
            //cout << "Subversion:0x" << uppercase << hex << bri.lmpSubversion << endl;

            // ������Χ���յ��������ź�
            //btsp.hRadio = hbr;  //����ִ�������豸���ڵľ����Ӧ��Ϊִ��BluetoothFindFirstRadio�������õ��ľ��
            //btsp.fReturnAuthenticated = TRUE;//�Ƿ���������Ե��豸  
            //btsp.fReturnConnected = FALSE;//�Ƿ����������ӵ��豸  
            //btsp.fReturnRemembered = TRUE;//�Ƿ������Ѽ�����豸  
            //btsp.fReturnUnknown = TRUE;//�Ƿ�����δ֪�豸  
            //btsp.fIssueInquiry = TRUE;//�Ƿ�����������True��ʱ���ִ���µ�������ʱ��ϳ���FALSE��ʱ���ֱ�ӷ����ϴε����������
            //btsp.cTimeoutMultiplier = 10;//ָʾ��ѯ��ʱ��ֵ����1.28��Ϊ������ ���磬12.8��Ĳ�ѯ��cTimeoutMultiplierֵΪ10.�˳�Ա�����ֵΪ48.��ʹ�ô���48��ֵʱ�����ú�������ʧ�ܲ����� 
            //hbdf = BluetoothFindFirstDevice(&btsp, &btdi);//ͨ���ҵ���һ���豸�õ���HBLUETOOTH_DEVICE_FIND���hbdf��ö��Զ�������豸���ѵ��ĵ�һ��Զ�������豸����Ϣ������btdi�����С���û��Զ�������豸��hdbf=NULL��  
            //bool bfind = hbdf != NULL;
            //while (bfind)
            //{
            //    //wcout << "[Name]:" << btdi.szName;  //Զ�������豸������
            //    //cout << ",[Address]:0x" << uppercase << hex << btdi.Address.ullLong << endl;
            //    bfind = BluetoothFindNextDevice(hbdf, &btdi);//ͨ��BluetoothFindFirstDevice�õ���HBLUETOOTH_DEVICE_FIND�����ö��������һ��Զ�������豸������Զ�������豸����Ϣ������btdi��  
            //}
            //if (hbdf)
            //    BluetoothFindDeviceClose(hbdf); //ʹ�����ǵùر�HBLUETOOTH_DEVICE_FIND���hbdf��  
        }
        if (hbf)
        {
            CloseHandle(hbr);
            brfind = BluetoothFindNextRadio(hbf, &hbr);//ͨ��BluetoothFindFirstRadio�õ���HBLUETOOTH_RADIO_FIND���hbf��ö��������һ�����������շ������õ�������BluetoothFindFirstDevice�ľ��hbr��    
        }
    }
    if (hbf)
        BluetoothFindRadioClose(hbf); //ʹ�����ǵùر�HBLUETOOTH_RADIO_FIND���hbf��  
    return;
}
// Monitor/View: ϵͳ����ͷ - ֧�ֵķֱ���
const std::vector<std::pair<int, int>> GetCameraSupportResolutions(IBaseFilter* pBaseFilter)
{
    std::vector<std::pair<int, int>> result;
    if (!pBaseFilter)
        return result;

    try
    {
        HRESULT hr = 0;
        std::vector<IPin*> pins;
        IEnumPins* EnumPins;
        pBaseFilter->EnumPins(&EnumPins);
        pins.clear();

        for (;;)
        {
            IPin* pin;
            hr = EnumPins->Next(1, &pin, NULL);
            if (hr != S_OK)
            {
                break;
            }
            pins.push_back(pin);
            pin->Release();
        }

        EnumPins->Release();

        PIN_INFO pInfo;
        for (int i = 0; i < pins.size(); i++)
        {
            if (nullptr == pins[i])
            {
                break;
            }
            pins[i]->QueryPinInfo(&pInfo);

            IEnumMediaTypes* emt = NULL;
            pins[i]->EnumMediaTypes(&emt);
            AM_MEDIA_TYPE* pmt;

            for (;;)
            {
                hr = emt->Next(1, &pmt, NULL);
                if (hr != S_OK)
                {
                    break;
                }
                if ((pmt->formattype == FORMAT_VideoInfo)
                    //&& (pmt->subtype == MEDIASUBTYPE_RGB24)
                    && (pmt->cbFormat >= sizeof(VIDEOINFOHEADER))
                    && (pmt->pbFormat != NULL)) {

                    VIDEOINFOHEADER* pVIH = (VIDEOINFOHEADER*)pmt->pbFormat;

                    auto insertParam = std::pair<int, int>{ pVIH->bmiHeader.biWidth, pVIH->bmiHeader.biHeight };
                    bool isSet = false;

                    for (auto param : result)
                    {
                        if (param.first == insertParam.first && param.second == insertParam.second)
                        {
                            isSet = true;
                            break;
                        }
                    }

                    if (!isSet)
                    {
                        result.push_back(insertParam);
                    }
                }

                if (pmt->cbFormat != 0)
                {
                    CoTaskMemFree((PVOID)pmt->pbFormat);
                    pmt->cbFormat = 0;
                    pmt->pbFormat = NULL;
                }
                if (pmt->pUnk != NULL)
                {
                    // pUnk should not be used.
                    pmt->pUnk->Release();
                    pmt->pUnk = NULL;
                }
            }
            break;
            emt->Release();
        }
        return result;
    }
    catch (const std::exception&)
    {
        result.clear();
        return result;
    }
}
void USysBaseInfo::GetCameraInfoList(std::vector<std::string>& cameraInfo)
{
    std::vector<CameraInfo> nameList;
    HRESULT hr;

    ICreateDevEnum* pSysDevEnum = NULL;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
        IID_ICreateDevEnum, (void**)&pSysDevEnum);
    if (FAILED(hr))
    {
        pSysDevEnum->Release();
        return;
    }

    IEnumMoniker* pEnumCat = NULL;
    hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);
    if (FAILED(hr) || !pEnumCat)
    {
        pSysDevEnum->Release();
        return;
    }

    try
    {
        IMoniker* pMoniker = NULL;
        ULONG cFetched;
        while (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
        {
            IPropertyBag* pPropBag;
            hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
            if (!SUCCEEDED(hr))
            {
                pMoniker->Release();
                continue;
            }
            IBaseFilter* pFilter;
            hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
            if (!pFilter)
            {
                pPropBag->Release();
                pMoniker->Release();
                continue;
            }
            VARIANT varName; VariantInit(&varName);
            hr = pPropBag->Read(L"FriendlyName", &varName, 0);
            if (SUCCEEDED(hr))
            {
                CameraInfo info;
                CStringA tmpstr;
                info.resolutionList = GetCameraSupportResolutions(pFilter);
                //tmpstr.Format("%ws", varName.bstrVal); [�¶�����] ע������ʱ����BUG 
                tmpstr = varName.bstrVal;
                info.cameraName = tmpstr.GetBuffer();
                nameList.push_back(info);
                cameraInfo.push_back(tmpstr.GetBuffer());
            }
            VariantClear(&varName);
            pFilter->Release();
            pPropBag->Release();
            pMoniker->Release();
        }
        pEnumCat->Release();
    }
    catch (const std::exception&)
    {
    }
}
void USysBaseInfo::GetCamerStatus()
{
    //const auto nStatus = SendMessage(NULL, WM_CAP_DRIVER_CONNECT, 0, 0);
    //if (false == nStatus)
    //{

    //}
    //else
    //{

    //}
}
// Monitor/View: ϵͳ��˷� - �ͺ� - ״̬
void USysBaseInfo::GetMicroPhone(std::vector<std::string>& micrphone)
{
}

// Monitor: �����¶�
void USysBaseInfo::GetSysDynManBoardTempera()
{

}
// Monitor: �����¶�
void USysBaseInfo::GetSysDynDiskTempera()
{

}
// Monitor: Cpu�¶�
void USysBaseInfo::GetSysDynCpuTempera()
{
}
// Monitor: Gpu�¶�
void USysBaseInfo::GetSysDynGpuTempera()
{

}

// Monitor: Cpuռ����
const double USysBaseInfo::GetSysDynCpuUtiliza()
{
    // ��ȡ����ʱ�� �ں� �û�
    _FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);
    // Creates or opens a named or unnamed event object.
    // �������һ�������Ļ��������¼�����
    // failure 0  | sucess handle
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    // �ȴ�1000���룬�ں˶�������ȷ
    if (hEvent)
        WaitForSingleObject(hEvent, 1000);
    // ��ȡ�µ�ʱ��
    _FILETIME newidleTime, newkernelTime, newuserTime;
    GetSystemTimes(&newidleTime, &newkernelTime, &newuserTime);
    // ת��ʱ��
    double	doldidleTime = FILETIMEDouble(idleTime);
    double	doldkernelTime = FILETIMEDouble(kernelTime);
    double	dolduserTime = FILETIMEDouble(userTime);
    double	dnewidleTime = FILETIMEDouble(newidleTime);
    double	dnewkernelTime = FILETIMEDouble(newkernelTime);
    double	dnewuserTime = FILETIMEDouble(newuserTime);
    double	Times = dnewidleTime - doldidleTime;
    double	Kerneltime = dnewkernelTime - doldkernelTime;
    double	usertime = dnewuserTime - dolduserTime;
    // ����ʹ����
    double Cpurate = (100.0 - Times / (Kerneltime + usertime) * 100.0);
    return Cpurate;
}
// Monitor: �ڴ�ռ����
const DWORD USysBaseInfo::GetSysDynSysMem()
{
    // �����ṹ����� ��ȡ�ڴ���Ϣ����
    MEMORYSTATUS memStatus;
    GlobalMemoryStatus(&memStatus);
    return memStatus.dwMemoryLoad;

    CString m_MemoryBFB, m_Pymemory, m_Pagesize, m_Memorysize, m_Kymemorysize;
    // ��ʹ�������ڴ��С Physical memory size
    size_t memPhysize = memStatus.dwTotalPhys - memStatus.dwAvailPhys;
    m_Pymemory.Format(L"%u", (memPhysize / 1024 / 1024 / 8));
    m_Pymemory += " MB";
    // �ļ�������С Size of the file exchange
    m_Pagesize.Format(L"%u", (memStatus.dwAvailPageFile / 1024 / 1024 / 8));
    m_Pagesize += " MB";
    // �����ڴ��С Virtual memory size
    m_Memorysize.Format(L"%u", (memStatus.dwTotalVirtual / 1024 / 1024 / 8));
    m_Memorysize += " MB";
    // ���������ڴ��С Available virtual memory size
    m_Kymemorysize.Format(L"%d", (memStatus.dwAvailVirtual / 1024 / 1024 / 8));
    m_Kymemorysize += " MB";
}
// Monitor: ����ռ����
void USysBaseInfo::GetSysDynDiskIo()
{

}
// Monitor: Gpuռ����
void USysBaseInfo::GetSysDynGpu()
{

}
// Mem�Ż�
void USysBaseInfo::MemSwap()
{
    CString str, str1;
    str = "һ�����ٳɹ��� ��ʡ�˿ռ䣺  ";
    // 1. ��ȡ��ǰ���������ڴ�״̬
    MEMORYSTATUSEX stcMemStatusEx = { 0 };
    stcMemStatusEx.dwLength = sizeof(stcMemStatusEx);
    GlobalMemoryStatusEx(&stcMemStatusEx);
    DWORDLONG preUsedMem = stcMemStatusEx.ullTotalPhys - stcMemStatusEx.ullAvailPhys;
    // 2. �����ڴ�
    DWORD dwPIDList[1000] = { 0 };
    DWORD bufSize = sizeof(dwPIDList);
    DWORD dwNeedSize = 0;
    // EnumProcesses(dwPIDList, bufSize, &dwNeedSize);
    for (DWORD i = 0; i < dwNeedSize / sizeof(DWORD); ++i)
    {
        HANDLE hProccess = OpenProcess(PROCESS_SET_QUOTA, false, dwPIDList[i]);
        SetProcessWorkingSetSize(hProccess, -1, -1);
    }
    // 3. ��ȡ�������ڴ�״̬
    GlobalMemoryStatusEx(&stcMemStatusEx);
    DWORDLONG afterCleanUserdMem = stcMemStatusEx.ullTotalPhys - stcMemStatusEx.ullAvailPhys;
    // 4. ���㲢��������ɹ�
    DWORDLONG CleanofSuccess = preUsedMem - afterCleanUserdMem;
    str1.Format(L"%d", (CleanofSuccess / 1024 / 1024 / 8));
    str = str + str1 + " MB";
}

USysBaseInfo::USysBaseInfo()
{
}
USysBaseInfo::~USysBaseInfo()
{
}


