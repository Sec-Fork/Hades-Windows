#pragma once
#include <functional>

#define PIPE_BUFSIZE		40960

typedef std::function<void(const std::string&)>	DealFunc;
// �ܵ��ͻ��˽ṹ
typedef struct _PIPECLIENT {
	std::string			name;		// �ܵ�����
	HANDLE				thread;		// ���߳�
	HANDLE				handle;		// �ܵ�
	BOOL				exit;		// �Ƿ��˳�
	BOOL				connect;	// �Ƿ��Ѿ�����
	DealFunc			dealFunc;	// ������
	LIST_ENTRY          link;		// �ͻ�������
}PIPECLIENT, * PPIPECLIENT;

// �ܵ��������ṹ
typedef struct _PIPESERVER {
	std::string			name;		//  �ܵ�����
	HANDLE				thread;		//	�����߳�
	PIPECLIENT			client;		//	�ܵ�����
	BOOL				exit;		//	�Ƿ��˳�
	DealFunc			dealFunc;	//  ������
	LIST_ENTRY          link;		//	����������
}PIPESERVER, * PPIPESERVER;

void	InitializeList(PLIST_ENTRY ListHead);
bool	IsListEmpty(PLIST_ENTRY ListHead);
void	InsertList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry);
void	RemoveEntry(PLIST_ENTRY Entry);

// ��ȡ�б�ĩβ�ṹ��
PPIPESERVER	GetTailSERVER(PLIST_ENTRY ListHead);
PPIPECLIENT	GetTailCLIENT(PLIST_ENTRY ListHead);

// ���ҽṹ��
PPIPECLIENT FindCLIENT(const std::string& name, PLIST_ENTRY ListHead);
PPIPESERVER FindSERVER(const std::string& name, PLIST_ENTRY ListHead);

class PipeManage
{
public:
	static PipeManage* Instance();
	static void Uninstance();

private:
	PipeManage();
	~PipeManage();

public:
	bool ServerStart(const std::string& name, DealFunc dealFunc = nullptr);

	void ServerStop();

	bool ServerSend(const std::string& name, const std::string& str);

private:
	bool PipeTest(const std::string& name);

private:
	// ������N�Զ�ṹ
	PIPESERVER	m_serverHead;

	// �رռӸ���
	CRITICAL_SECTION m_cs;
};