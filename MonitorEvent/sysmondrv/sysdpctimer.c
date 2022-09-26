#include "public.h"
#include "sysdpctimer.h"

/*
	See Csnd: https://blog.csdn.net/Simon798/article/details/106251920
		ULONG64 ul_Dpc = (ULONG64)pTimer->Dpc;
		INT i_Shift = (*((PULONG64)ukiwaNever) & 0xFF);
		// ���� Dpc ����
		ul_Dpc ^= *((ULONG_PTR*)ukiwaNever);		// ���
		ul_Dpc = _rotl64(ul_Dpc, i_Shift);			// ѭ������
		ul_Dpc ^= (ULONG_PTR)pTimer;				// ���
		ul_Dpc = _byteswap_uint64(ul_Dpc);			// �ߵ�˳��
		ul_Dpc ^= *((ULONG_PTR*)ukiwaAlways);		// ���
		// ��������ת��
		pkDpc = (PKDPC)ul_Dpc;
*/
BOOLEAN get_KiWait(PULONG64 never, PULONG64 always) {
	// ��ȡ KeSetTimer ��ַ
	ULONG64 ul_KeSetTimer = 0;
	UNICODE_STRING	uc_KeSetTimer = { 0 };
	RtlInitUnicodeString(&uc_KeSetTimer, L"KeSetTimer");
	ul_KeSetTimer = (ULONG64)MmGetSystemRoutineAddress(&uc_KeSetTimer);
	if (!ul_KeSetTimer) {
		return FALSE;
	}

	// ǰ 30 �ֽ��� call ָ��
	BOOLEAN b_e8 = FALSE;
	ULONG64 ul_e8Addr = 0;
	for (INT i = 0; i < 30; i++) {
		if (!MmIsAddressValid((PVOID64)ul_KeSetTimer)) { continue; }
		if (*(PUCHAR)(ul_KeSetTimer + i) == 0xe8) {
			b_e8 = TRUE;
			ul_e8Addr = ul_KeSetTimer + i;
			break;
		}
	}

	// �ҵ� call �����Ŀ�ĵ�ַ
	ULONG64 ul_KiSetTimerEx = 0;
	if (b_e8 == TRUE) {
		if (!MmIsAddressValid((PVOID64)ul_e8Addr)) { return FALSE; }
		INT ul_callCode = *(INT*)(ul_e8Addr + 1);
		ULONG64 ul_callAddr = ul_e8Addr + ul_callCode + 5;
		ul_KiSetTimerEx = ul_callAddr;
	}

	// û�� call ��ֱ���ڵ�ǰ������ 
	else {
		ul_KiSetTimerEx = ul_KeSetTimer;
	}

	// ǰ 50 �ֽ��� �� KiWaitNever �� KiWaitAlways
	if (!MmIsAddressValid((PVOID64)ul_KiSetTimerEx)) { return FALSE; }
	ULONG64 ul_arr_ret[2];			// ��� KiWaitNever �� KiWaitAlways �ĵ�ַ
	INT i_sub = 0;					// ��Ӧ ul_arr_ret ���±�
	for (INT i = 0; i < 50; i++) {
		if (*(PUCHAR)(ul_KiSetTimerEx + i) == 0x48 &&
			*(PUCHAR)(ul_KiSetTimerEx + i + 1) == 0x8b &&
			*(PUCHAR)(ul_KiSetTimerEx + i + 6) == 0x00
			) {
			ULONG64 ul_movCode = *(UINT32*)(ul_KiSetTimerEx + i + 3);
			ULONG64 ul_movAddr = ul_KiSetTimerEx + i + ul_movCode + 7;
			// ֻ�÷���������ǰ����ֵ
			if (i_sub < 2) {
				ul_arr_ret[i_sub++] = ul_movAddr;
			}
		}
	}
	*never = ul_arr_ret[0];
	*always = ul_arr_ret[1];

	return TRUE;
}

static VOID get_KiWait1(PULONG64* ukiwaAlways, PULONG64* ukiwaNever)
{
	long			opcodetmp = 0;
	PUCHAR			StartAddress, i;
	UNICODE_STRING  KeTimterName;
	RtlInitUnicodeString(&KeTimterName, L"KeSetTimer");
	StartAddress = (PUCHAR)MmGetSystemRoutineAddress(&KeTimterName);
	for (i = StartAddress; i < StartAddress + 0xFF; i++)
	{
		if (*i == 0x48 && *(i + 1) == 0x8B && *(i + 2) == 0x05)
		{
			memcpy(&opcodetmp, i + 3, 4);
			ukiwaNever = (PULONG64)((ULONGLONG)opcodetmp + (ULONGLONG)i + 7);
			i = i + 7;
			memcpy(&opcodetmp, i + 3, 4);
			ukiwaAlways = (PULONG64)((ULONGLONG)opcodetmp + (ULONGLONG)i + 7);
			return;
		}
	}
}

static KDPC* DpcDecode(
	IN PKTIMER InTimer,
	IN ULONGLONG InKiWaitNever,
	IN ULONGLONG InKiWaitAlways)
{
	ULONGLONG RDX = (ULONGLONG)InTimer->Dpc;
	RDX ^= InKiWaitNever;
	RDX = _rotl64(RDX, (UCHAR)(InKiWaitNever & 0xFF));
	RDX ^= (ULONGLONG)InTimer;
	RDX = _byteswap_uint64(RDX);
	RDX ^= InKiWaitAlways;
	return (KDPC*)RDX;
}

int nf_GetDpcTimerInfoData(DPC_TIMERINFO* MemBuffer)
{
    ULONG64			uPrcbAddress = 0;
	PLIST_ENTRY		pCurrListEntry = NULL, pNextListEntry = NULL;
    PUCHAR			pCurrentTimerTableEntry = NULL;
    PULONG64		ukiwaAlways = NULL;
    PULONG64		ukiwaNever = NULL;
	PDPC_TIMERINFO	pDpcTimerInfo = NULL;
	PKTIMER			pTimer = NULL;
	PKDPC			pkDpc = NULL;
	DPC_TIMERINFO*	dpcinfo = NULL;
	int i = 0, index = 0;

    for (i = 0; i < KeNumberProcessors; ++i)
    {
        KeSetSystemAffinityThread(i + 1);
        uPrcbAddress = (ULONG64)__readmsr(0xC0000101) + 0x20;
        KeRevertToUserAffinityThread();

		// Win7 = 0x2200
		// Win10 = 0x3680
		// Get _KPRCB --> KTIMER
        pCurrentTimerTableEntry = (PUCHAR)(*(ULONG64*)uPrcbAddress + 0x3680 + 0x200);
		get_KiWait(&ukiwaNever, &ukiwaAlways);
		if (!ukiwaAlways)
		{
			get_KiWait1(&ukiwaAlways, &ukiwaNever);
			if (!ukiwaAlways)
				continue;
		}


		int j = 0;


		for (j = 0; j < 0x100; j++)
		{
			pCurrListEntry = (PLIST_ENTRY)(pCurrentTimerTableEntry + sizeof(KTIMER_TABLE_ENTRY) * j + 8);
			if (MmIsAddressValid(pCurrListEntry) == FALSE)
				continue;

			pNextListEntry = pCurrListEntry->Blink;
			if (MmIsAddressValid(pNextListEntry) == FALSE)
				continue;

			// ˫������
			while (pNextListEntry != pCurrListEntry)
			{
				//����׵�ַ
				pTimer = CONTAINING_RECORD(pNextListEntry, KTIMER, TimerListEntry);
				if (MmIsAddressValid(pTimer) == FALSE)
				{
					pNextListEntry = pNextListEntry->Blink;
					continue;
				}

				pkDpc = NULL;
				if (pkDpc == NULL)
				{
					pkDpc = DpcDecode(pTimer, *ukiwaNever, *ukiwaAlways);
					if (FALSE == MmIsAddressValid(pkDpc))
					{
						pNextListEntry = pNextListEntry->Blink;
						continue;
					}
				}

				MemBuffer[index].dpc = pkDpc;
				MemBuffer[index].timerobject = pTimer;
				MemBuffer[index].period = pTimer->Period;
				MemBuffer[index].timeroutine = pkDpc->DeferredRoutine;
				index++;
				pNextListEntry = pNextListEntry->Blink;
			}
		}
    }
	
	return 1;
}