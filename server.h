//-----------------------------------------------------------------------------
//#include <windows.h>
#include "unilog.h"
#include "opcda.h"
#include <time.h>
#include "serialport.h"
//-----------------------------------------------------------------------------
#define LOGID logg,0				// log identifiner
#define LOG_FNAME	"ecl.log"		// log name
#define CFG_FILE	"ecl.xml"		// cfg name
#define TAGS_NUM_MAX		7500	// maximum number of tags
#define DATALEN_MAX			200		// maximum lenght of the tags
#define MAX_DEVICE_NUM		15
#define MAX_ECL_PARAM		30		// maximum number of parametrs on one controller
#define PORTNAME_LEN		15		// port name lenght
#define PORT_NUM_MAX		50		// maximum number of ports	
#define COMMANDS_NUM_MAX	100		// maximum number of commands
#define DEVICE_NUM_MAX		10		// maximum number of ecl-300 on one port
#define COMM_BUFFER_SIZE	500		// com-port buffer size
//-----------------------------------------------------------------------------
typedef unsigned UINT;

typedef struct _Prm PrmR;		// device info

struct _Prm {
  SHORT	prm;		// �������������
  CHAR  name[100];	// �������� 
};

typedef struct _DataR DataR;		// device info

struct _DataR {
  UINT	device;		// ������������� ����������
  CHAR  name[100];	// �������� ������ ����������
  SHORT prm;		// ����� ���������
  SHORT status;		// ������� ������, (0-��� �����, 1-���������, 2-:).
  UINT	tn;			// ����� ����
  DOUBLE value;		// ������� ��������
  UINT	adr;		// ����� �� Modbus
  FLOAT	mn;			// ���������
  SHORT func;		// 
  SHORT	type;		// ��� ��������� (0-int, 1-double)
};

SerialPort port[PORT_NUM_MAX];	// com-port
struct ComPorts {
 SHORT	num;		// ����� �����
 UINT	speed;		// �������� ������
 UINT	parity;		// ��������
 SHORT	nm;			// ���������� ����� �����
};
ComPorts Com [PORT_NUM_MAX];

typedef struct _DeviceR DeviceR;	// controllers info
//-----------------------------------------------------------------------------
struct _DeviceR {  
  SHORT device;		// ���������� ����� ����������. (����� �� ���� ��� ����� IP (����������))
  CHAR	name[50];	// �������� ������� 
  SHORT status;		// ������� ������, (0-��� �����, 1-���������, 2-:).
  SHORT	com;		// ���������������� ��������� (����� �����)
  SHORT	adr;		// ����� �� ����
  UINT	tags_num;	// ���������� ����� �� �����������
  CHAR	ver[20];	// ������ �������
  SHORT	connect;	// ����� (�-�� ������)
  SHORT	nm;			// ���������� ����� �����
};
DeviceR DeviceRU [MAX_DEVICE_NUM];

struct ECL_Tags {
 CHAR	tag[100];	// �������� ����	
 CHAR	name[100];	// �������� ����	
 DOUBLE	min_value;	// ����������� �������� ����
 DOUBLE	max_value;	// ������������ �������� ����
 SHORT	mode;		// ����� ������ 1-������, 2 - ������, 3-������/������
 CHAR	ediz[10];	// ������� ���������
 UINT	adr;		// ����� �� Modbus
 DOUBLE	mn;			// ���������
 SHORT	func;		// ������� ��� ������
 SHORT	type;		// ��� ��������� (0-int, 1-double)
};
//-----------------------------------------------------------------------------
ECL_Tags ECLTag[] =	// �������� �������
 {  
	{"Controller mode.Cir  1 mode","����� ������� - ��������� �����������",0,4,3,"",4201,1,4,0},
	{"Controller mode.Cir  2 mode","����� ������� - ��� �����������",0,4,3,"",4202,1,4,0},
	{"Controller mode.Cir  1 status","��������� ������� - ��������� �����������",0,3,1,"",4211,1,3,0},
	{"Controller mode.Cir  2 status","��������� ������� - ��� �����������",0,3,1,"",4212,1,3,0},
	
	{"Circuit 1 Sensor and ref.S1 sensor","����������� ������� �� �����",-64,192,3,"C",11201,0.1,4,1},
	{"Circuit 1 Sensor and ref.S3 sensor","����������� ��������� ������ �� �������",-64,192,1,"C",11203,0.1,3,1},
	{"Circuit 1 Sensor and ref.S4 sensor","����������� ��������� ������� �� �������",-64,192,1,"C",11204,0.1,3,1},
	{"Circuit 2 Sensor and ref.S5 sensor","����������� ��� ������ �� �������",-64,192,1,"C",12205,0.1,3,1},
	
	{"Circuit 1 primary par._180 Day setpt","������� ��������� ����������� - ����������",10,30,3,"C",11180,1,4,0},
	{"Circuit 1 primary par._181 Night setpt","������� ��������� ����������� - ����������",10,30,3,"C",11181,1,4,0},
	{"Circuit 1 primary par._175 Heat curve","��������� ������� ��������� (������� �����)",0.2,3.4,3,"",11175,0.1,4,1},
	{"Circuit 1 primary par._177 Flow tmp min","������� ����������� ����������� ������ ���������",10,110,3,"C",11177,1,4,0},
	{"Circuit 1 primary par._178 Flow tmp max","������� ������������ ����������� ������ ���������",10,110,3,"C",11178,1,4,0},

	{"Time and date.Hour","������� ����� � ������� - ����",0,23,3,"���",64045,1,4,0},
	{"Time and date.Minutes","������� ����� � ������� - ������",0,59,3,"���",64046,1,4,0},
	{"Time and date.DayMonth","������� ����� � ������� - ����",1,31,3,"",64047,1,4,0},
	{"Time and date.Month","������� ����� � ������� - �����",1,12,3,"",64048,1,4,0},
	{"Time and date.Year","������� ����� � ������� - ���",0,99,3,"",64049,1,4,0},

	{"Connect_break_Danfoss","��������� ������ ����� � ���������",0,1,1,"",0,1,0,0},	
	{"Dummy tag","��������� ���",0,1,1,"",0,1,0,0}};
