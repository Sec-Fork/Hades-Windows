#pragma once
class ArkSsdt
{
public:
	ArkSsdt();
	~ArkSsdt();

	bool nf_init();
	bool nf_GetSysCurrentSsdtData(LPVOID pData, const DWORD SSdtinfoSize);
};

