#include <Windows.h>
#include "ufile.h"
#include <string>
#include <atlstr.h>
#include <vector>
#include <sysinfo.h>
#include "MD5.h"

using namespace std;

static DWORD g_FileCount = 0, dwAllSize = 0;
static PUDriectFile g_driectfileinfo = NULL;

UFile::UFile()
{
}

UFile::~UFile()
{
}

/*
*	�ݹ��������Ŀ¼����
	std::vector<std::string> vecHitDirectory;
	for (const auto& vDrvIter : vecDiskH)
	{
		for (const auto& folderKeyIter : strDirectory)
		{
			CodeTool::SearchDir(vDrvIter, 4, vecHitDirectory, folderKeyIter);
		}
	}
	void SearchDirEx(const std::string& strDir, int bSub, std::vector<std::string>& paths, const std::string& strFolderKey)
{
	int i = 0; WIN32_FIND_DATA FileData = { 0 };
	const HANDLE hFile = FindFirstFile((strDir + "*").c_str(), &FileData);
	// 1. �ж�
	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (bSub <= 0)
			return;
		// 2. �ݹ�ѭ��
		do
		{
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				const std::string strTmp = FileData.cFileName;
				if (std::string(".") == strTmp || std::string("..") == strTmp)
					continue;
				const auto& iter = strTmp.find("$");
				if (string::npos != iter)
					continue;
				if (strFolderKey == FileData.cFileName)
				{
					//OutputDebugString(("[LoadPP] EnumDirectory FolderKey Success: " + strDir + FileData.cFileName + "\\" + "\n").c_str());
					paths.push_back(strDir + FileData.cFileName);
					break;
				}
				SearchDirEx(strDir + FileData.cFileName + "\\", --bSub, paths, strFolderKey);
			}
		} while (FindNextFile(hFile, &FileData));
		if (hFile)
			FindClose(hFile);
	}
	return;
}
void SearchDir(const std::string& strDir, int bSub, std::vector<std::string>& paths, const std::string& strFolderKey)
{
	int i = 0;  WIN32_FIND_DATA FileData = { 0 };
	const HANDLE hFile = FindFirstFile((strDir + "*").c_str(), &FileData);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		std::string strTmpcFileName, strFullTmpcFileName;
		do
		{
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				strTmpcFileName = FileData.cFileName;
				if ("." == strTmpcFileName || ".." == strTmpcFileName)
					continue;
				const auto& iter = strTmpcFileName.find("$");
				if (string::npos != iter)
					continue;
				if (strFolderKey == FileData.cFileName)
				{
					OutputDebugString(("EnumDirectory FolderKey Success: " + strDir + FileData.cFileName + "\\" + "\n").c_str());
					paths.push_back(strDir + FileData.cFileName);
					break;
				}
				strFullTmpcFileName = strDir + FileData.cFileName + "\\";
				if ("C:\\Windows\\" == strFullTmpcFileName)
					continue;
				SearchDirEx(strFullTmpcFileName, bSub, paths, strFolderKey);
				Sleep(1);
			}
		} while (FindNextFile(hFile, &FileData));
		if (hFile)
			FindClose(hFile);
	}
	return;
}
*/
bool EnumDriectFile(CString Path, LPVOID outbuf)
{
	if (!g_driectfileinfo)
		g_driectfileinfo = (PUDriectFile)outbuf;

	int i = 0;
	WIN32_FIND_DATA FileData = { 0 };
	const HANDLE hFile = FindFirstFile(Path + L"\\*", &FileData);
	// 1. �ж�
	if (hFile != INVALID_HANDLE_VALUE)
	{
		// 2. �ݹ�ѭ��
		do
		{
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (CString(".") != FileData.cFileName && CString("..") != FileData.cFileName)
					EnumDriectFile(Path + CString("\\") + FileData.cFileName, outbuf);
			}
			// 3. �����ļ�������ļ���Ϣ
			else
			{
				// file path
				lstrcpyW(g_driectfileinfo[g_FileCount].filename, FileData.cFileName);
				lstrcpyW(g_driectfileinfo[g_FileCount].filepath, Path + CString("\\") + FileData.cFileName);
				// file all Size
				g_driectfileinfo[g_FileCount].filesize = FileData.nFileSizeLow;
				dwAllSize += FileData.nFileSizeLow;
				// file number
				++g_FileCount;
			}
		} while (FindNextFile(hFile, &FileData));
		if (hFile)
			FindClose(hFile);
	}
	return true;
}

bool UFile::uf_GetFileInfo(char* filepath,LPVOID outbuf)
{
	CString filestr = filepath;
	if (!outbuf && (0 >= filestr.GetLength()))
		return false;

	// ��ȡ�ļ�·��
	TCHAR Path[MAX_PATH] = {};
	SYSTEMTIME System = { 0 };
	// ���ڱ�����ʱ�ַ����Ļ�����
	TCHAR TempBuffer[MAX_PATH] = { 0 };
	// VS_FIXEDFILEINFO 
	WIN32_FIND_DATA stFileData = { 0 };
	const HANDLE hFile = FindFirstFile(filestr, &stFileData);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	// 1. ����ļ���
	stFileData.cFileName;
	// 2. ��Ӵ���ʱ��
	FileTimeToSystemTime(&stFileData.ftCreationTime, &System);
	_stprintf_s(TempBuffer, TEXT("%d/%d/%d %d:%d:%d"), System.wYear,
		System.wMonth, System.wDay, System.wHour, System.wMinute, System.wSecond);
	// 3. ����ļ��޸�ʱ��
	FileTimeToSystemTime(&stFileData.ftLastWriteTime, &System);
	_stprintf_s(TempBuffer, TEXT("%d/%d/%d %d:%d:%d"), System.wYear,
		System.wMonth, System.wDay, System.wHour, System.wMinute, System.wSecond);
	// 4. ���������
	FileTimeToSystemTime(&stFileData.ftLastAccessTime, &System);
	_stprintf_s(TempBuffer, TEXT("%d/%d/%d %d:%d:%d"), System.wYear,
		System.wMonth, System.wDay, System.wHour, System.wMinute, System.wSecond);
	TempBuffer[0] = 0;
	// 5. �ж��ǲ���Ŀ¼  
	if (stFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		_tcscat_s(TempBuffer, TEXT("Ŀ¼ "));
	else
		_tcscat_s(TempBuffer, TEXT("�ļ� "));
	// 6. ��ʾ��ǰ�ļ��Ĵ�С
	TempBuffer[0] = 0;
	if (stFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		_tcscat_s(TempBuffer, TEXT("-"));
	else
	{
		if (stFileData.nFileSizeLow > 1073741824)
			_stprintf_s(TempBuffer, TEXT("%.2lfGB"), stFileData.nFileSizeLow / 1024.0 / 1024.0 / 1024.0);
		else if (stFileData.nFileSizeLow > 1048576)
			_stprintf_s(TempBuffer, TEXT("%.2lfMB"), stFileData.nFileSizeLow / 1024.0 / 1024.0);
		else
			_stprintf_s(TempBuffer, TEXT("%.2lfKB"), stFileData.nFileSizeLow / 1024.0 / 1024.0);
	}
	// 7. ����
	// �ж��ǲ������ص�
	if (stFileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
		_tcscat_s(TempBuffer, TEXT("���� "));
	// �ж��ǲ���ֻ����
	if (stFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		_tcscat_s(TempBuffer, TEXT("ֻ��"));

	// MD5����
	char FileName[MAX_PATH] = { 0 };
	memset(FileName, 0, sizeof(FileName));
	sprintf_s(FileName, "%ws", filestr.GetBuffer());
	md5FileValue(FileName);

	// ��ȡ��Ϣ
	//GetModuleFileNameEx(hFile, NULL, Path, MAX_PATH);
	//SHFILEINFOW shfileinfo;
	//SHGetFileInfo(Path, 0, &shfileinfo, sizeof(SHFILEINFOW), SHGFI_ICON);
	if (hFile)
		FindClose(hFile);
	return true;
}
bool UFile::uf_GetDirectoryFile(char* DriPath, LPVOID outbuf)
{
	g_driectfileinfo = NULL;
	g_FileCount = 0, dwAllSize = 0;
	PUDriectInfo dirinfo = (PUDriectInfo)outbuf;
	if (!dirinfo)
		return false;
	EnumDriectFile(DriPath, dirinfo->fileEntry);
	dirinfo->DriectAllSize = dwAllSize;
	dirinfo->FileNumber = g_FileCount;

	if (g_FileCount)
		return true;
	else
		return false;
}