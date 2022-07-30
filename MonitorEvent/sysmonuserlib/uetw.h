#ifndef _UETW_H
#define _UETW_H

#include <mutex>
#include <functional>

#define INITGUID
#include <evntcons.h>

typedef struct _PROCESSINFO
{
	int processsid;					// ����pid
	std::wstring processName;		// ������
	std::wstring processCommLine;	// ����
	bool processStatus;				// ����״̬(����/�˳�)
}PROCESSINFO, *PPROCESSINFO;

class UEtw
{
public:
	UEtw();
	~UEtw();

	bool uf_init();
	bool uf_close();
	bool uf_init(const bool flag);
	bool uf_close(const bool flag);
	void set_on_processMonitor(const std::function<void(const PROCESSINFO&)>& on_processinfo_data);

protected:
	bool uf_RegisterTraceFile();
	bool uf_RegisterTrace(const int dwEnableFlags);
	unsigned long uf_setmonitor(const unsigned __int64 hSession, PVOID64 m_traceconfig, const int ioct);

private:
	TRACEHANDLE m_hFileSession;
};

#endif // !_UETW_H
