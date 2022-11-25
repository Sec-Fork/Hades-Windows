#ifndef _SYSSSDT_H
#define _SYSSSDT_H

typedef struct _SSDTINFO
{
	short			ssdt_id;
	ULONGLONG		sstd_memaddr;
	LONG			sstd_memoffset;
	// ��ַ�Ƿ���Ч
	BOOLEAN			ssdt_addrstatus;
}SSDTINFO, * PSSDTINFO;

#ifndef _WIN64
typedef  struct  _KSYSTEM_SERVICE_TABLE
{
	PULONG  ServiceTableBase;           // ������ַ����׵�ַ
	PULONG  ServiceCounterTableBase;    // ��������ÿ�����������õĴ���
	ULONG   NumberOfService;            // �������ĸ���, NumberOfService * 4 ����������ַ��Ĵ�С
	UCHAR* ParamTableBase;				// �����������׵�ַ
} KSYSTEM_SERVICE_TABLE, * PKSYSTEM_SERVICE_TABLE;
#else
/*x64 UINT64*/
typedef  struct  _KSYSTEM_SERVICE_TABLE
{
	UINT64  ServiceTableBase;           // ������ַ����׵�ַ
	UINT64  ServiceCounterTableBase;    // ��������ÿ�����������õĴ���
	UINT64  NumberOfService;            // �������ĸ���, NumberOfService * 4 ����������ַ��Ĵ�С
	UCHAR* ParamTableBase;				// �����������׵�ַ
} KSYSTEM_SERVICE_TABLE, * PKSYSTEM_SERVICE_TABLE;
#endif // !_WIN64

extern PKSYSTEM_SERVICE_TABLE KeServiceDescriptorTable;

int Sstd_Init();
int Sstd_GetTableInfo(SSDTINFO* MemBuffer);

#endif // !_SYSSSDT_H

