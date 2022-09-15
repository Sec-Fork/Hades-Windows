/*
* Topic Class
* Mod: Sub <--> Topic <--> Pub
    uMsginterface.cpp����SysMonUserLib��˵��������,����Grpc����Iocp���ϱ��ӿ���˵�������ߡ�
    �������·���Grpc --> ����taskId --> uMsginterface�·� --> SysMonUserLib(��������)
    ������������Grpc <-- ��ɷ���  <--  uMsginterface��ȡ/��� <-- SysMonUserLib(��������)
ע: ΪʲôACK���̲�����ʹ������ָ�룿
    Pub��Ϣ����ȷ�����Ľ�����Ϣ����,������(Pub)�ͷ�,�����ط���Ϣ��
    ����ָ��Ҳ���Ա����ò��ͷ�,���Ƿ�ֹ�����ܵ��������,����Լ�����
���ԭ��
    Pub to Topic �Լ�����ָ��
    Topic to Sub ����ָ��
���Ż�
    1.����ж������,�����Զ�����Topic�����������ָ��.
    2.Topic���������������Ч��,û��ACK����,Pub��������Sub,������Topic����Sub�Ƿ������õ���������,Sub��������Topic.
*/
#include "msgassist.h"
#include "umsginterface.h"

#include "uautostart.h"
#include "unet.h"
#include "usysuser.h"
#include "uprocesstree.h"
#include "uservicesoftware.h"
#include "ufile.h"
#include "uetw.h"

//rapidjson
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

//nlohmannjson
#include <json.hpp>
using json_t = nlohmann::json;

// ������ȫ�ֶ���
static UAutoStart               g_user_uautostrobj;
static UNet                     g_user_unetobj;
static NSysUser                 g_user_usysuser;
static UProcess                 g_user_uprocesstree;
static UServerSoftware          g_user_userversoftware;
static UFile                    g_user_ufile;
static UEtw                     g_user_etw;
// Topic�������ָ��1
static std::mutex               g_RecvQueueCs;
static std::queue<UPubNode*>    g_RecvQueueData;
static HANDLE                   g_jobAvailableEvent;
static bool                     g_exit = false;
// Topic�������ָ��1����,����Etw����������
inline void uMsgInterface::uMsg_SetTopicQueuePtr() { g_user_etw.uf_setqueuetaskptr(g_RecvQueueData); }
inline void uMsgInterface::uMsg_SetTopicQueueLockPtr() { g_user_etw.uf_setqueuelockptr(g_RecvQueueCs); }
inline void uMsgInterface::uMsg_SetTopicEventPtr() { g_user_etw.uf_setqueueeventptr(g_jobAvailableEvent); }

// ����������ָ��(�������ߵ���)
static std::queue<std::shared_ptr<USubNode>>*       g_SendQueueData_Ptr = NULL;
static std::mutex*                                  g_SendQueueCs_Ptr = NULL;
static HANDLE                                       g_SendQueue_Event = NULL;
void uMsgInterface::uMsg_SetSubQueuePtr(std::queue<std::shared_ptr<USubNode>>& qptr) { g_SendQueueData_Ptr = &qptr; }
void uMsgInterface::uMsg_SetSubQueueLockPtr(std::mutex& qptrcs) { g_SendQueueCs_Ptr = &qptrcs; }
void uMsgInterface::uMsg_SetSubEventPtr(HANDLE& eventptr) { g_SendQueue_Event = eventptr; }
const int EtwSubLens = sizeof(USubNode);

uMsgInterface::uMsgInterface()
{
}

uMsgInterface::~uMsgInterface()
{
}

// Topic���ݴ�������ͷ���Sub
void uMsgInterface::uMsgEtwDataHandlerEx()
{
    static json_t j;
    static std::string tmpstr;
    UPubNode* etw_taskdata = nullptr;

    g_RecvQueueCs.lock();
    for (;;)
    {
        Sleep(10);
        if (g_RecvQueueData.empty())
        {
            g_RecvQueueCs.unlock();
            return;
        }
        etw_taskdata = g_RecvQueueData.front();
        g_RecvQueueData.pop();
        if (!etw_taskdata)
        {
            g_RecvQueueCs.unlock();
            return;
        }

        const int taskid = etw_taskdata->taskid;
        switch (taskid)
        {
        case UF_ETW_NETWORK:
        {
            UEtwNetWork* etwNet = (UEtwNetWork*)&(etw_taskdata->data[0]);
            if (!etwNet)
                break;
            //swprintf(output, L"[etw_network] protocol:%d pid:%d localport: %x:%d  remoteport: %x:%d", \
            //    etwNet->protocol,
            //    etwNet->processId,
            //    etwNet->ipv4LocalAddr, etwNet->toLocalPort, \
            //    etwNet->RemoteAddr, etwNet->toRemotePort);
            //OutputDebugString(output);
            j["win_network_addressfamily"] = to_string(etwNet->addressFamily);
            j["win_network_protocol"] = to_string(etwNet->protocol);
            j["win_network_processid"] = to_string(etwNet->processId);
            j["win_network_localaddr"] = to_string(etwNet->ipv4LocalAddr);
            j["win_network_toLocalport"] = to_string(etwNet->protocol);
            j["win_network_remoteaddr"] = to_string(etwNet->ipv4toRemoteAddr);
            j["win_network_toremoteport"] = to_string(etwNet->toRemotePort);
        }
        break;
        case UF_ETW_PROCESSINFO:
        {
            UEtwProcessInfo* etwProcess = (UEtwProcessInfo*)&(etw_taskdata->data[0]);
            if (!etwProcess)
                break;
            //swprintf(output, L"[etw_process] pid: %d  Path: ", etwProcess->processId);
            //lstrcatW(output, etwProcess->processPath);
            //OutputDebugString(output);
            j["win_etw_processinfo_pid"] = to_string(etwProcess->processId);
            Wchar_tToString(tmpstr, etwProcess->processPath);
            if (tmpstr.empty())
                break;
            tmpstr = String_ToUtf8(tmpstr);
            j["win_etw_processinfo_path"] = tmpstr.c_str();
        }
        break;
        case UF_ETW_THREADINFO:
        {
            UEtwThreadInfo* etwThread = (UEtwThreadInfo*)&(etw_taskdata->data[0]);
            if (!etwThread)
                break;
            //swprintf(output, L"[etw_thread] pid: %d  tid: %d ThreadEntryAddr: 0x%x status: %d", etwThread->processId, etwThread->threadId, etwThread->Win32StartAddr, etwThread->ThreadFlags);
            //OutputDebugString(output);
            j["win_etw_threadinfo_pid"] = to_string(etwThread->processId);
            j["win_etw_threadinfo_tid"] = to_string(etwThread->threadId);
            j["win_etw_threadinfo_win32startaddr"] = to_string(etwThread->Win32StartAddr);
            j["win_etw_threadinfo_flags"] = to_string(etwThread->ThreadFlags);
        }
        break;
        case UF_ETW_IMAGEMOD:
        {
            UEtwImageInfo* etwProcMod = (UEtwImageInfo*)&(etw_taskdata->data[0]);
            if (!etwProcMod)
                break;
            //swprintf(output, L"[etw_image] pid: %d  Address: 0x%d DLLName: ", etwProcMod->ProcessId, etwProcMod->ImageBase);
            //lstrcatW(output, etwProcMod->FileName);
            //OutputDebugString(output);
            j["win_etw_imageinfo_processId"] = to_string(etwProcMod->ProcessId);
            j["win_etw_imageinfo_imageBase"] = to_string(etwProcMod->ImageBase);
            j["win_etw_imageinfo_imageSize"] = to_string(etwProcMod->ImageSize);
            j["win_etw_imageinfo_signatureLevel"] = to_string(etwProcMod->SignatureLevel);
            j["win_etw_imageinfo_signatureType"] = to_string(etwProcMod->SignatureType);
            j["win_etw_imageinfo_imageChecksum"] = to_string(etwProcMod->ImageChecksum);
            j["win_etw_imageinfo_timeDateStamp"] = to_string(123);
            j["win_etw_imageinfo_defaultBase"] = to_string(etwProcMod->DefaultBase);
            Wchar_tToString(tmpstr, etwProcMod->FileName);
            if (tmpstr.empty())
                break;
            tmpstr = String_ToUtf8(tmpstr);
            j["win_etw_imageinfo_fileName"] = tmpstr.c_str();
        }
        break;
        case UF_ETW_REGISTERTAB:
        {
            UEtwRegisterTabInfo* etwregtab = (UEtwRegisterTabInfo*)&(etw_taskdata->data[0]);
            if (!etwregtab)
                break;
            //swprintf(output, L"[etw_register] Key: %d  KeyName: ", etwregtab->KeyHandle);
            //lstrcatW(output, etwregtab->KeyName);
            //OutputDebugString(output);
            //j["win_etw_regtab_initialTime"] = to_string(etwregtab->InitialTime);
            j["win_etw_regtab_status"] = to_string(etwregtab->Status);
            j["win_etw_regtab_index"] = to_string(etwregtab->Index);
            j["win_etw_regtab_keyHandle"] = to_string(etwregtab->KeyHandle);
            Wchar_tToString(tmpstr, etwregtab->KeyName);
            tmpstr = String_ToUtf8(tmpstr);
            j["win_etw_regtab_keyName"] = tmpstr.c_str();
        }
        break;
        case UF_ETW_FILEIO:
        {
            UEtwFileIoTabInfo* etwfileio = (UEtwFileIoTabInfo*)&(etw_taskdata->data[0]);
            if (!etwfileio)
                break;
            //swprintf(output, L"[etw_fileio] FileKey: %d  KeyName: ", etwfileio->FileKey);
            //OutputDebugString(output);
        }
        }

        // ע: Topic �ͷ� Pub������ָ��
        if (etw_taskdata)
        {
            delete[] etw_taskdata;
            etw_taskdata = nullptr;
        }

        try
        {
            // ���л�
            std::shared_ptr<std::string> data = nullptr;
            if (j.size())
            {
                data = std::make_shared<std::string>(j.dump());
            }
            else
                continue;
            
            if (!g_SendQueueData_Ptr && !g_SendQueueCs_Ptr && !g_SendQueue_Event)
            {
                OutputDebugString(L"Grpcû���ö���ָ��");
                g_RecvQueueCs.unlock();
                return;
            }

            std::shared_ptr<USubNode> sub = std::make_shared<USubNode>();
            if (!sub || !data)
            {
                g_RecvQueueCs.unlock();
                return;
            }
                
            sub->data = data;
            sub->taskid = taskid;
            g_SendQueueCs_Ptr->lock();
            g_SendQueueData_Ptr->push(sub);
            g_SendQueueCs_Ptr->unlock();
            SetEvent(g_SendQueue_Event);
            j.clear();
            tmpstr.clear();
        }
        catch (const std::exception&)
        {
            g_RecvQueueCs.unlock();
        }
    }
    g_RecvQueueCs.unlock();
}
// Topic���,�첽�¼��ȴ�
void uMsgInterface::uMsg_taskPopEtwLoop()
{
    try
    {
        if (!g_jobAvailableEvent)
            return;
        do
        {
            WaitForSingleObject(
                g_jobAvailableEvent,
                INFINITE
            );

            if (g_exit)
                break;

            uMsgEtwDataHandlerEx();

        } while (true);
    }
    catch (const std::exception&)
    {

    }
}
static unsigned WINAPI uMsg_taskPopThread(void* pData)
{
    (reinterpret_cast<uMsgInterface*>(pData))->uMsg_taskPopEtwLoop();
    return 0;
}
void uMsgInterface::uMsg_taskPopInit()
{
    int i = 0;
    HANDLE hThread;
    unsigned threadId;

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    DWORD threadCount = sysinfo.dwNumberOfProcessors;
    if (threadCount == 0)
    {
        threadCount = 4;
    }
    for (i = 0; i < threadCount; i++)
    {
        hThread =(HANDLE)_beginthreadex(0, 0, uMsg_taskPopThread, (LPVOID)this, 0, &threadId);
        if (hThread != 0 && hThread != (HANDLE)(-1L))
        {
            m_topicthread.push_back(hThread);
        }
    }
}

// �ӿڣ��û�̬TaskId�·���ȡ����,ͬ������
void uMsgInterface::uMsg_taskPush(const int taskcode, std::vector<std::string>& vec_task_string)
{
    std::string tmpstr; wstring catstr;
    int i = 0, index = 0;
    DWORD dwAllocateMemSize = 0;
    char* ptr_Getbuffer;
    bool nstatus = Choose_mem(ptr_Getbuffer, dwAllocateMemSize, taskcode);
    if (false == nstatus || nullptr == ptr_Getbuffer || dwAllocateMemSize == 0)
        return;
    json_t j;
    try
    {
        // ptr_Getbuffer
        do
        {
            switch (taskcode)
            {
            case UF_PROCESS_ENUM:
            {
                if (false == g_user_uprocesstree.uf_EnumProcess(ptr_Getbuffer))
                    break;
                PUProcessNode procesNode = (PUProcessNode)ptr_Getbuffer;
                if (!procesNode)
                    break;

                std::vector<std::string> test_vec;
                for (i = 0; i < procesNode->processcount; ++i)
                {   
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, procesNode->sysprocess[i].fullprocesspath);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_process_Path"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, procesNode->sysprocess[i].szExeFile);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_process_szExeFile"] = tmpstr.c_str();
                    j["win_user_process_pid"] = to_string(procesNode->sysprocess[i].pid).c_str();
                    tmpstr.clear();
                    tmpstr = String_ToUtf8(procesNode->sysprocess[i].priclassbase);
                    j["win_user_process_pribase"] = tmpstr.c_str();
                    j["win_user_process_parenid"] = to_string(procesNode->sysprocess[i].th32ParentProcessID).c_str();
                    j["win_user_process_thrcout"] = to_string(procesNode->sysprocess[i].threadcout).c_str();
                    vec_task_string.push_back(j.dump());
                }
                OutputDebugString(L"[User] Process Enum Success");
                std::cout << "[User] Process Enum Success" << std::endl;
            }
            break;
            case UF_PROCESS_PID_TREE:
            {
                // Command - pid
                if (false == g_user_uprocesstree.uf_GetProcessInfo(4, ptr_Getbuffer))
                    break;
            }
            break;
            case UF_SYSAUTO_START:
            {
                if (false == g_user_uautostrobj.uf_EnumAutoStartask(ptr_Getbuffer, dwAllocateMemSize))
                    break;

                PUAutoStartNode autorunnode = (PUAutoStartNode)ptr_Getbuffer;
                if (!autorunnode)
                    break;

                
                j["win_user_autorun_flag"] = "1";
                for (i = 0; i < autorunnode->regnumber; ++i)
                {
                    tmpstr.clear();
                    tmpstr = String_ToUtf8(autorunnode->regrun[i].szValueName);
                    j["win_user_autorun_regName"] = tmpstr.c_str();
                    tmpstr.clear();
                    tmpstr = String_ToUtf8(autorunnode->regrun[i].szValueKey);
                    j["win_user_autorun_regKey"] = tmpstr.c_str();
                    vec_task_string.push_back(j.dump());
                }

                j.clear();
                j["win_user_autorun_flag"] = "2";
                for (i = 0; i < autorunnode->taskrunnumber; ++i)
                {
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, autorunnode->taskschrun[i].szValueName);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_autorun_tschname"] = tmpstr.c_str();
                    j["win_user_autorun_tscState"] = to_string(autorunnode->taskschrun[i].State).c_str();
                    j["win_user_autorun_tscLastTime"] = to_string(autorunnode->taskschrun[i].LastTime).c_str();
                    j["win_user_autorun_tscNextTime"] = to_string(autorunnode->taskschrun[i].NextTime).c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, autorunnode->taskschrun[i].TaskCommand);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_autorun_tscCommand"] = tmpstr.c_str();
                    vec_task_string.push_back(j.dump());
                }
                OutputDebugString(L"[User] SystemAutoStartRun Enum Success");
                std::cout << "[User] SystemAutoStartRun Enum Success" << std::endl;
            }
            break;
            case UF_SYSNET_INFO:
            {
                if (false == g_user_unetobj.uf_EnumNetwork(ptr_Getbuffer))
                    break;

                PUNetNode netnode = (PUNetNode)ptr_Getbuffer;
                if (!netnode)
                    break;
                j["win_user_net_flag"] = "1";
                for (i = 0; i < netnode->tcpnumber; i++)
                {
                    j["win_user_net_src"] = netnode->tcpnode[i].szlip;
                    j["win_user_net_dst"] = netnode->tcpnode[i].szrip;
                    j["win_user_net_status"] = netnode->tcpnode[i].TcpState;
                    j["win_user_net_pid"] = netnode->tcpnode[i].PidString;
                    vec_task_string.push_back(j.dump());
                }

                j.clear();
                j["win_user_net_flag"] = "2";
                for (i = 0; i < netnode->udpnumber; i++)
                {

                    j["win_user_net_src"] = netnode->tcpnode[i].szlip;
                    j["win_user_net_pid"] = netnode->tcpnode[i].PidString;
                    vec_task_string.push_back(j.dump());
                }
                OutputDebugString(L"[User] EnumNetwork Enum Success");
                std::cout << "[User] EnumNetwork Enum Success" << std::endl;
            }
            break;
            case UF_SYSSESSION_INFO:    // v2.0
            {
            }
            break;
            case UF_SYSINFO_ID:         // v1.0 --> �Ƿ�����ʱ����������?�Ǳ����ɼ�
            {
            }
            break;
            case UF_SYSLOG_ID:          // ���� --> etw��ȫ����ȡ��
            {
            }
            break;
            case UF_SYSUSER_ID:
            {
                if (false == g_user_usysuser.uf_EnumSysUser(ptr_Getbuffer))
                    break;

                PUUserNode pusernode = (PUUserNode)ptr_Getbuffer;
                if (!pusernode)
                    break;

                for (i = 0; i < pusernode->usernumber; ++i)
                {
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pusernode->usernode[i].serveruser);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_sysuser_user"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pusernode->usernode[i].servername);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_sysuser_name"] = tmpstr.c_str();
                    j["win_user_sysuser_sid"] = to_string((ULONGLONG)pusernode->usernode[i].serverusid).c_str();
                    j["win_user_sysuser_flag"] = to_string(pusernode->usernode[i].serveruflag).c_str();
                    vec_task_string.push_back(j.dump());
                }
                OutputDebugString(L"[User] SysUser Enum Success");
                std::cout << "[User] SysUser Enum Success" << std::endl;
            }
            break;
            case UF_SYSSERVICE_SOFTWARE_ID:
            {
                if (false == g_user_userversoftware.EnumAll(ptr_Getbuffer))
                    break;

                PUAllServerSoftware pNode = (PUAllServerSoftware)ptr_Getbuffer;
                if (!pNode)
                    break;

                j["win_user_softwareserver_flag"] = "1";
                for (i = 0; i < pNode->servicenumber; ++i)
                {
                    j.clear();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uSericeinfo[i].lpServiceName);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_server_lpsName"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uSericeinfo[i].lpDisplayName);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_server_lpdName"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uSericeinfo[i].lpBinaryPathName);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_server_lpPath"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uSericeinfo[i].lpDescription);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_server_lpDescr"] = tmpstr.c_str();
                    j["win_user_server_status"] = pNode->uSericeinfo[i].dwCurrentState;
                    vec_task_string.push_back(j.dump());
                }

                j.clear();
                j["win_user_softwareserver_flag"] = "2";
                for (i = 0; i < pNode->softwarenumber; ++i)
                {
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uUsoinfo[i].szSoftName);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_software_lpsName"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uUsoinfo[i].szSoftSize);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_software_Size"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uUsoinfo[i].szSoftVer);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_software_Ver"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uUsoinfo[i].strSoftInsPath);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_software_installpath"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uUsoinfo[i].strSoftUniPath);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_software_uninstallpath"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uUsoinfo[i].szSoftDate);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_software_data"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, pNode->uUsoinfo[i].strSoftVenRel);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_software_venrel"] = tmpstr.c_str();
                    vec_task_string.push_back(j.dump());
                }
                OutputDebugString(L"[User] Software_Server Enum Success");
                std::cout << "[User] Software_Server Enum Success" << std::endl;
            }
            break;
            case UF_SYSFILE_ID:
            {
                // Command ��ȡ Ŀ¼·��
                if (false == g_user_ufile.uf_GetDirectoryFile((char*)"D:\\bin", ptr_Getbuffer))
                    break;

                PUDriectInfo directinfo = (PUDriectInfo)ptr_Getbuffer;
                if (!directinfo)
                    break;

                // �Ȼط���һ��cout����Ŀ¼��С
                j["win_user_driectinfo_flag"] = "1";
                j["win_user_driectinfo_filecout"] = to_string(directinfo->FileNumber).c_str();
                j["win_user_driectinfo_size"] = to_string(directinfo->DriectAllSize).c_str();
                vec_task_string.push_back(j.dump());
               
                
                // ö�ٵ��ļ�����
                j.clear();
                j["win_user_driectinfo_flag"] = "2";
                for (i = 0; i < directinfo->FileNumber; ++i)
                {
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, directinfo->fileEntry[i].filename);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_driectinfo_filename"] = tmpstr.c_str();
                    tmpstr.clear();
                    Wchar_tToString(tmpstr, directinfo->fileEntry[i].filepath);
                    tmpstr = String_ToUtf8(tmpstr);
                    j["win_user_driectinfo_filePath"] = tmpstr.c_str();
                    j["win_user_driectinfo_fileSize"] = to_string(directinfo->fileEntry[i].filesize).c_str();
                    vec_task_string.push_back(j.dump());
                }
                OutputDebugString(L"[User] GetDirectoryFile Enum Success");
                std::cout << "[User] GetDirectoryFile Enum Success" << std::endl;
            }
            break;
            case UF_FILE_INFO:
            {
                // Command ��ȡ �ļ�����·��
                if (false == g_user_ufile.uf_GetFileInfo((char*)"d:\\bin\\1.txt", ptr_Getbuffer))
                    break;

                PUFileInfo fileinfo = (PUFileInfo)ptr_Getbuffer;
                if (!fileinfo)
                    break;

                tmpstr.clear();
                Wchar_tToString(tmpstr, fileinfo->cFileName);
                tmpstr = String_ToUtf8(tmpstr);
                j["win_user_fileinfo_filename"] = tmpstr.c_str();
                tmpstr.clear();
                Wchar_tToString(tmpstr, fileinfo->dwFileAttributes);
                tmpstr = String_ToUtf8(tmpstr);
                j["win_user_fileinfo_dwFileAttributes"] = tmpstr.c_str();
                tmpstr.clear();
                Wchar_tToString(tmpstr, fileinfo->dwFileAttributesHide);
                tmpstr = String_ToUtf8(tmpstr);
                j["win_user_fileinfo_dwFileAttributesHide"] = tmpstr.c_str();
                j["win_user_fileinfo_md5"] = fileinfo->md5;
                tmpstr.clear();
                Wchar_tToString(tmpstr, fileinfo->m_seFileSizeof);
                j["win_user_fileinfo_m_seFileSizeof"] = tmpstr.c_str();
                tmpstr.clear();
                Wchar_tToString(tmpstr, fileinfo->seFileAccess);
                tmpstr = String_ToUtf8(tmpstr);
                j["win_user_fileinfo_seFileAccess"] = tmpstr.c_str();
                tmpstr.clear();
                Wchar_tToString(tmpstr, fileinfo->seFileCreate);
                j["win_user_fileinfo_seFileCreate"] = tmpstr.c_str();
                tmpstr.clear();
                Wchar_tToString(tmpstr, fileinfo->seFileModify);
                j["win_user_fileinfo_seFileModify"] = tmpstr.c_str();
                vec_task_string.push_back(j.dump());
                OutputDebugString(L"[User] GetFIleInfo Success");
                std::cout << "[User] GetFIleInfo Success" << std::endl;
            }
            break;
            case UF_ROOTKIT_ID:     // v2.0
            {
            }
            break;
            default:
                break;
            }
        } while (false);
    }
    catch (const std::exception&)
    {

    }

    if (ptr_Getbuffer)
    {
        delete[] ptr_Getbuffer;
        ptr_Getbuffer = nullptr;
    }

}

void uMsgInterface::uMsg_EtwInit()
{
    g_user_etw.uf_init();
    etwStatus = true;
}
void uMsgInterface::uMsg_EtwClose()
{
    g_user_etw.uf_close();
    etwStatus = false;
}
bool uMsgInterface::GetEtwMonStatus()
{
    return etwStatus;
}

void uMsgInterface::uMsg_Init() {
    // ��ʼ��Topic
    g_jobAvailableEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    this->uMsg_SetTopicQueuePtr();
    this->uMsg_SetTopicQueueLockPtr();
    this->uMsg_SetTopicEventPtr();
    // ������
    this->uMsg_taskPopInit();
};
void uMsgInterface::uMsg_Free()
{
    g_exit = true;
    for (size_t idx = 0; idx < m_topicthread.size(); ++idx)
    {
        SetEvent(g_jobAvailableEvent);
        WaitForSingleObject(m_topicthread[idx], 1000);
        CloseHandle(m_topicthread[idx]);
    }

    if (g_jobAvailableEvent != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(g_jobAvailableEvent);
        g_jobAvailableEvent = INVALID_HANDLE_VALUE;
    }
    m_topicthread.clear();
}