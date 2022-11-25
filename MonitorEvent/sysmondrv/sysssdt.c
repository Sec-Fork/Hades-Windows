#include "public.h"
#include "sysssdt.h"

const ULONGLONG HelpSsdtBaseAddr64(PUCHAR StartSearchAddress)
{
	PUCHAR EndSearchAddress = StartSearchAddress + 0x500;
	PUCHAR i = NULL;
	ULONG templong = 0;
	ULONGLONG addr = 0;
	for (i = StartSearchAddress; i < EndSearchAddress; i++)
	{
		if (MmIsAddressValid(i) && MmIsAddressValid(i + 1) && MmIsAddressValid(i + 2))
		{
			//4c8d15
			if ((*i == 0x4c) &&
				(*(i + 1) == 0x8d) &&
				(*(i + 2) == 0x15)) 
			{
				memcpy(&templong, i + 3, 4);
				addr = (ULONGLONG)templong + (ULONGLONG)i + 7;
				return addr;
			}
		}
	}
	return 0;
}

const ULONGLONG SysGetSsdtBaseAddrto64()
{
	PUCHAR StartSearchAddress = (PUCHAR)__readmsr(0xC0000082);

	if (0x00 == *(StartSearchAddress + 0x9))
	{
		return HelpSsdtBaseAddr64(StartSearchAddress);
	}
	// KiSystemCall64Shadow
	else if (
		(0x70 == *(StartSearchAddress + 0x9)) ||
		// 0f01f8          swapgs
		((0x0f == *StartSearchAddress) && (0x01 == *(StartSearchAddress + 1)) && (0xf8 == *(StartSearchAddress + 2))))
	{
		PUCHAR EndSearchAddress = StartSearchAddress + 0x500;
		PUCHAR i = NULL;
		INT temp = 0;
		for (i = StartSearchAddress; i < EndSearchAddress; i++)
		{
			//e9e35ce9ff	jmp     nt!KiSystemServiceUser
			//c3            ret
			if (MmIsAddressValid(i) && MmIsAddressValid(i + 5))
			{
				if ((*i == 0xe9) && (*(i + 5) == 0xc3))
				{
					memcpy(&temp, i + 1, 4);
					PUCHAR pKiSystemServiceUser = temp + (i + 5);
					return HelpSsdtBaseAddr64(pKiSystemServiceUser);
				}
			}
		}
	}
	return 0;
}

const ULONGLONG SysGetSsdtBaseAddrto32()
{
	// 1.1 ��ȡ��ǰ�߳�
	PETHREAD pThread = PsGetCurrentThread();
	// 1.2 �߳̽ṹ�� +0xbc ��ȡ���� ServiceTable 
	// Ӳ������Ҫά��һ�°汾����ȱ��x86��
	return (*(ULONG*)((ULONG_PTR)pThread + 0xbc));
}

int Sstd_Init()
{
#ifdef _WIN64
	KeServiceDescriptorTable = (PSYSTEM_SERVICE_TABLE)SysGetSsdtBaseAddrto64();
#else
	KeServiceDescriptorTable = (PSYSTEM_SERVICE_TABLE)SysGetSsdtBaseAddrto32();
#endif // _WIN32
	if (KeServiceDescriptorTable)
		return 1;
	else
		return 0;
}

int Sstd_GetTableInfo(SSDTINFO* MemBuffer)
{
	if (!KeServiceDescriptorTable)
		return -1;

	ULONGLONG SsdtFunNumber = KeServiceDescriptorTable->NumberOfServices;
	if (0 >= SsdtFunNumber)
		return -1;

	PULONG ServiceTableBase = NULL;
	ServiceTableBase = (PULONG)KeServiceDescriptorTable->ServiceTableBase;
	if (0 >= ServiceTableBase)
		return -1;

	DWORD32		offset = 0;
	ULONGLONG	funaddr = 0;
	PSSDTINFO	ssdtinfo = ExAllocatePoolWithTag(NonPagedPool, sizeof(SSDTINFO), 'STMM');
	if (!ssdtinfo)
		return -1;
	RtlSecureZeroMemory(ssdtinfo, sizeof(SSDTINFO));
	int			i = 0;
	for (i = 0; i < SsdtFunNumber; ++i)
	{
		ssdtinfo->ssdt_id = i;
		offset = ((PDWORD32)ServiceTableBase)[i];
		ssdtinfo->sstd_memoffset = offset;
#ifndef _WIN64
		// x86
		ssdtinfo->sstd_memaddr = offset;
#else
		// x64
		if (offset & 0x80000000)
			offset = (offset >> 4) | 0xF0000000;
		else
			offset = offset >> 4;
		funaddr = (ULONGLONG)ServiceTableBase + (ULONGLONG)offset;
		ssdtinfo->sstd_memaddr = funaddr;
#endif // !_WIN64
		RtlCopyMemory(&MemBuffer[i], ssdtinfo, sizeof(SSDTINFO));
		RtlSecureZeroMemory(ssdtinfo, sizeof(SSDTINFO));
	}

	if (ssdtinfo)
	{
		ExFreePoolWithTag(ssdtinfo, 'STMM');
		ssdtinfo = NULL;
	}

	return 1;
}