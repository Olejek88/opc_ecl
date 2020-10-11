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
  SHORT	prm;		// идентификатор
  CHAR  name[100];	// название 
};

typedef struct _DataR DataR;		// device info

struct _DataR {
  UINT	device;		// идентификатор устройства
  CHAR  name[100];	// название модуля устройства
  SHORT prm;		// номер параметра
  SHORT status;		// текущий статус, (0-нет связи, 1-нормально, 2-:).
  UINT	tn;			// номер тега
  DOUBLE value;		// текущее значение
  UINT	adr;		// адрес по Modbus
  FLOAT	mn;			// множитель
  SHORT func;		// 
  SHORT	type;		// тип параметра (0-int, 1-double)
};

SerialPort port[PORT_NUM_MAX];	// com-port
struct ComPorts {
 SHORT	num;		// номер порта
 UINT	speed;		// скорость обмена
 UINT	parity;		// четность
 SHORT	nm;			// порядковый номер порта
};
ComPorts Com [PORT_NUM_MAX];

typedef struct _DeviceR DeviceR;	// controllers info
//-----------------------------------------------------------------------------
struct _DeviceR {  
  SHORT device;		// порядковый номер устройства. (адрес на шине или адрес IP (порядковый))
  CHAR	name[50];	// название прибора 
  SHORT status;		// текущий статус, (0-нет связи, 1-нормально, 2-:).
  SHORT	com;		// коммуникационный интерфейс (номер порта)
  SHORT	adr;		// адрес на шине
  UINT	tags_num;	// количество тегов на контроллере
  CHAR	ver[20];	// версия прибора
  SHORT	connect;	// связь (к-во ошибок)
  SHORT	nm;			// порядковый номер порта
};
DeviceR DeviceRU [MAX_DEVICE_NUM];

struct ECL_Tags {
 CHAR	tag[100];	// название тега	
 CHAR	name[100];	// описание тега	
 DOUBLE	min_value;	// минимальное значение тега
 DOUBLE	max_value;	// максимальное значение тега
 SHORT	mode;		// режим работы 1-чтение, 2 - запись, 3-чтение/запись
 CHAR	ediz[10];	// единицы измерения
 UINT	adr;		// адрес по Modbus
 DOUBLE	mn;			// множитель
 SHORT	func;		// функция для чтения
 SHORT	type;		// тип параметра (0-int, 1-double)
};
//-----------------------------------------------------------------------------
ECL_Tags ECLTag[] =	// тепловая энергия
 {  
	{"Controller mode.Cir  1 mode","Режим контура - отопление контроллера",0,4,3,"",4201,1,4,0},
	{"Controller mode.Cir  2 mode","Режим контура - ГВС контроллера",0,4,3,"",4202,1,4,0},
	{"Controller mode.Cir  1 status","Состояние контура - отопление контроллера",0,3,1,"",4211,1,3,0},
	{"Controller mode.Cir  2 status","Состояние контура - ГВС контроллера",0,3,1,"",4212,1,3,0},
	
	{"Circuit 1 Sensor and ref.S1 sensor","Температура воздуха на улице",-64,192,3,"C",11201,0.1,4,1},
	{"Circuit 1 Sensor and ref.S3 sensor","Температура отопления подача от Данфосс",-64,192,1,"C",11203,0.1,3,1},
	{"Circuit 1 Sensor and ref.S4 sensor","Температура отопления обратка от Данфосс",-64,192,1,"C",11204,0.1,3,1},
	{"Circuit 2 Sensor and ref.S5 sensor","Температура ГВС подача от Данфосс",-64,192,1,"C",12205,0.1,3,1},
	
	{"Circuit 1 primary par._180 Day setpt","Задание комнатной температуры - комфортная",10,30,3,"C",11180,1,4,0},
	{"Circuit 1 primary par._181 Night setpt","Задание комнатной температуры - пониженная",10,30,3,"C",11181,1,4,0},
	{"Circuit 1 primary par._175 Heat curve","Установка графика отопления (наклона линии)",0.2,3.4,3,"",11175,0.1,4,1},
	{"Circuit 1 primary par._177 Flow tmp min","Задание минимальной температуры подачи отопления",10,110,3,"C",11177,1,4,0},
	{"Circuit 1 primary par._178 Flow tmp max","Задание максимальной температуры подачи отопления",10,110,3,"C",11178,1,4,0},

	{"Time and date.Hour","Текущее время в приборе - часы",0,23,3,"час",64045,1,4,0},
	{"Time and date.Minutes","Текущее время в приборе - минуты",0,59,3,"мин",64046,1,4,0},
	{"Time and date.DayMonth","Текущее время в приборе - день",1,31,3,"",64047,1,4,0},
	{"Time and date.Month","Текущее время в приборе - месяц",1,12,3,"",64048,1,4,0},
	{"Time and date.Year","Текущее время в приборе - год",0,99,3,"",64049,1,4,0},

	{"Connect_break_Danfoss","Индикатор обрыва связи с Данфоссом",0,1,1,"",0,1,0,0},	
	{"Dummy tag","Резервный тег",0,1,1,"",0,1,0,0}};
