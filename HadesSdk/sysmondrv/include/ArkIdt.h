#pragma once

class ArkIdt
{
public:
	ArkIdt();
	~ArkIdt();

	bool nf_init();
	bool nf_GetIdtData(LPVOID pData, const DWORD IdtinfoSize);
};

