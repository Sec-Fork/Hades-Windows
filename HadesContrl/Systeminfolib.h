#pragma once
#include <string>
#include <vector>

/*
	����ϵͳ������
*/
// OS�̶�����
typedef struct _SystemAttributesNode
{
	std::string currentUser;	//��ǰ�û�	
	std::string cpuinfo;		//cpu��Ϣ
	std::string verkerlinfo;	//�汾
	int verMajorVersion;
	int verMinorVersion;
	bool Is64;
	std::vector<std::string> mainboard;		//����
	std::vector<std::string> sysdisk;		//����
	std::vector<std::string> monitor;		//�Կ�
	std::vector<std::string> netcard;		//����
	std::vector<std::string> battery;		//���
	std::vector<std::string> camera;		//����ͷ
	std::vector<std::string> bluetooth;		//����
	std::vector<std::string> voice;			//��Ƶ
	std::vector<std::string> microphone;	//��˷�
}SystemAttributesNode, * PSystemAttributesNode;
// OS�䶯����
typedef struct _SystemDynamicNode
{
	std::string cpu_temperature;
	std::string monitor_temperature;
	std::string mainboard_temperature;
	std::string disk_temperature;
	std::string cpu_utilization;
	std::string sys_memory;
	std::string disk_io;
	std::string GPU;
}SystemDynamicNode, * PSystemDynamicNode;

namespace SYSTEMPUBLIC {
	extern SystemAttributesNode sysattriinfo;
	extern SystemDynamicNode sysdynamicinfo;
}

class Systeminfolib
{
public:
	Systeminfolib();
	~Systeminfolib();

private:
};

