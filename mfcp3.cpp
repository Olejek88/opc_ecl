#define _CRT_SECURE_NO_WARNINGS 1 
#define _WIN32_DCOM				// Enables DCOM extensions
#define INITGUID				// Initialize OLE constants
#define ECL_SID "opc.ecl"		// identificator of OPC server

#include <stdio.h>
#include <math.h>				// some mathematical function
#include "server.h"				// server variable
#include "unilog.h"				// universal utilites for creating log-files
#include <locale.h>				// set russian codepage
#include "opcda.h"				// basic function for OPC:DA
#include "lightopc.h"			// light OPC library header file
#include "nport.h"				// Nport funcs
#include "tinyxml/tinyxml.h"			// XML parser lib

#include <TCHAR.H >
#include <conio.h>

//---------------------------------------------------------------------------------
static const loVendorInfo vendor = {0,1,8,0,"ECL300 OPC Server" };	// OPC vendor info (Major/Minor/Build/Reserv)
static void a_server_finished(VOID*, loService*, loClient*);	// OnServer finish his work
static int OPCstatus=OPC_STATUS_RUNNING;						// status of OPC server
loService *my_service;											// name of light OPC Service
//---------------------------------------------------------------------------------
UINT devNum=0;					// main device nums
UINT tag_num=0;					// tags counter
UINT prm_num=0;					// prm counter
UINT com_num=0;					// prm counter
UINT tTotal=0;					// total quantity of tags
BOOL WorkEnable=TRUE;
UINT pos=0;
//---------------------------------------------------------------------------------
DataR DR[TAGS_NUM_MAX];			// data
//Dev	DV[MAX_DK_NUM][MAX_DEVICE_NUM];		// data
//CHAR data[4600][120];
void EnumerateSerialPorts();
//-----------------------------------------------------------------------------
unilog	*logg=NULL;				// new structure of unilog
FILE	*CfgFile;				// pointer to .xml file
//-----------------------------------------------------------------------------
HANDLE	hThrd[PORT_NUM_MAX+1];	// Newlycreated thread handle
BOOL	ReadEndFlag[PORT_NUM_MAX+1];	// CurrentProgressThread
VOID	PollDeviceCOM (LPVOID lpParam);	// 
DWORD	dwThrdID;				// Newlycreated thread ID value
HANDLE  hReadEndEvents [PORT_NUM_MAX+1];	// Events signal about thread ended
//unsigned short Crc16( unsigned char *pcBlock, unsigned short len );
unsigned short CRC16 (UCHAR* puchMsg, unsigned short usDataLen );
CHAR* FormSequence (UCHAR adr, UCHAR nFunc, UINT nAdr);
//---------------------------------------------------------------------------------
UINT ScanBus();					// bus scanned programm
VOID poll_device();				// poll device func
INT	 init_tags(UINT nSock);		// Init tags
UINT InitDriver();				// func of initialising port and creating service
UINT DestroyDriver();			// function of detroying driver and service
UINT ParseInput (CHAR* buf, UINT nSock, UINT type);
BOOL ConnectToServer (SOCKET server_sock, CHAR tP, UINT sport);
//---------------------------------------------------------------------------------
static CRITICAL_SECTION lk_values;	// protects ti[] from simultaneous access 
static INT mymain(HINSTANCE hInstance, INT argc, CHAR *argv[]);
static INT show_error(LPCTSTR msg);		// just show messagebox with error
static INT show_msg(LPCTSTR msg);		// just show messagebox with message
CHAR* ReadParam (CHAR *SectionName,CHAR *Value);	// read parametr from .ini file
CRITICAL_SECTION drv_access;
//---------------------------------------------------------------------------------
WCHAR wargv0[FILENAME_MAX + 32];		// lenght of command line (file+path (260+32))
CHAR argv0[FILENAME_MAX + 32];			// lenght of command line (file+path (260+32))
static CHAR *tn[TAGS_NUM_MAX];		// Tag name
static loTagValue tv[TAGS_NUM_MAX];	// Tag value
static loTagId ti[TAGS_NUM_MAX];	// Tag id
//---------------------------------------------------------------------------------
// {59779987-9C0B-4107-8EB4-47B93B932A92}
DEFINE_GUID(GID_ECLOPCserverDll, 
0x59779987, 0x9c0b, 0x4107, 0x8e, 0xb4, 0x47, 0xb9, 0x3b, 0x93, 0x2a, 0x92);
// {76B8B688-38B1-4802-9C21-1E34A3097778}
DEFINE_GUID(GID_ECLOPCserverExe, 
0x76b8b688, 0x38b1, 0x4802, 0x9c, 0x21, 0x1e, 0x34, 0xa3, 0x9, 0x77, 0x78);
//---------------------------------------------------------------------------------
#include "serv_main.h"	//	main server 
#include "opc_main.h"	//	main server 
//---------------------------------------------------------------------------------
INT APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,INT nCmdShow)
{ static char *argv[3] = { "dummy.exe", NULL, NULL };	// defaults arguments
  argv[1] = lpCmdLine;									// comandline - progs keys
  return mymain(hInstance, 2, argv);}

INT main(INT argc, CHAR *argv[])
{  return mymain(GetModuleHandle(NULL), argc, argv); }
//---------------------------------------------------------------------------------
INT mymain(HINSTANCE hInstance, INT argc, CHAR *argv[]) 
{
const char eClsidName [] = ECL_SID;				// desription 
const char eProgID [] = ECL_SID;				// name
CHAR *cp;
DWORD objid=::GetModuleFileName(NULL, wargv0, sizeof(wargv0));	// function retrieves the fully qualified path for the specified module
WideCharToMultiByte(CP_ACP,0,wargv0,-1,argv0,300,NULL, NULL);
if(objid==0 || objid+50 > sizeof(argv0)) return 0;		// not in border
init_common();									// create log-file
UL_ERROR((LOGID, "system path [%s]",argv0));	// in bad case write error in log
if(NULL==(cp = setlocale(LC_ALL, ".1251")))		// sets all categories, returning only the string cp-1251
//if(NULL==(cp = setlocale(LC_ALL, "Russian_Russia.20866")))		// sets all categories, returning only the string cp-1251
	{ 
	UL_ERROR((LOGID, "setlocale() - Can't set 1251 code page"));	// in bad case write error in log
	cleanup_common();							// delete log-file
    return 0;
	}
cp = argv[1];		
if(cp)						// check keys of command line 
	{
    INT finish = 1;			// flag of comlection
    if (strstr(cp, "/r"))	//	attempt registred server
		{
	     if (loServerRegister(&GID_ECLOPCserverExe, eProgID, eClsidName, argv0, 0)) 
			{ show_error(L"Registration Failed");  UL_ERROR((LOGID, "Registration <%s> <%s> Failed", eProgID, argv0));  } 
		 else 
			{ show_msg(L"ECL300 OPC Registration Ok"); UL_INFO((LOGID, "Registration <%s> <%s> Ok", eProgID, argv0));  }
		} 
	else 
		if (strstr(cp, "/u")) 
			{
			 if (loServerUnregister(&GID_ECLOPCserverExe, eClsidName)) 
				{ show_error(L"UnRegistration Failed"); UL_ERROR((LOGID, "UnReg <%s> <%s> Failed", eClsidName, argv0)); } 
			 else 
				{ show_msg(L"ECL300 OPC Server Unregistered"); UL_INFO((LOGID, "UnReg <%s> <%s> Ok", eClsidName, argv0)); }
			} 
		else  // only /r and /u options
			if (strstr(cp, "/?")) 
				 show_msg(L"Use: \nKey /r to register server.\nKey /u to unregister server.\nKey /? to show this help.");
				 else
					{
					 UL_WARNING((LOGID, "Ignore unknown option <%s>", cp));
					 finish = 0;		// nehren delat
					}
		if (finish) {      cleanup_common();      return 0;    } 
	}
if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))) 
	{	// Initializes the COM library for use by the calling thread
     UL_ERROR((LOGID, "CoInitializeEx() failed. Exiting..."));
     cleanup_common();	// close log-file
     return 0;
	}
UL_INFO((LOGID, "CoInitializeEx() [ok]"));	// write to log

if (InitDriver()) {		// open sockets and attempt connect to servers
    CoUninitialize();	// Closes the COM library on the current thread
    cleanup_common();	// close log-file
    return 0;
  }
UL_INFO((LOGID, "InitDriver() [ok]"));	// write to log
	
if (FAILED(CoRegisterClassObject(GID_ECLOPCserverExe, &my_CF, 
				   CLSCTX_LOCAL_SERVER|CLSCTX_REMOTE_SERVER|CLSCTX_INPROC_SERVER, 
				   REGCLS_MULTIPLEUSE, &objid)))
    { UL_ERROR((LOGID, "CoRegisterClassObject() [failed]"));
      cleanup_all(objid);		// close socket and unload all librares
      return 0; }
UL_INFO((LOGID, "CoRegisterClassObject() [ok]"));	// write to log

Sleep(5000);
my_CF.Release();		// avoid locking by CoRegisterClassObject() 

if (OPCstatus!=OPC_STATUS_RUNNING)	// ???? maybe Status changed and OPC not currently running??
	{	while(my_CF.in_use())      Sleep(1000);	// wait
		cleanup_all(objid);
		return 0;	}
while(my_CF.in_use())	// while server created or client connected
	{
	 if (WorkEnable)
	     poll_device();		// polling devices else do nothing (and be nothing)	 
	}
UL_INFO((LOGID, "end cleanup_all()"));	// write to log
cleanup_all(objid);		// destroy himself
return 0;
}
//-------------------------------------------------------------------
loTrid ReadTags(const loCaller *, unsigned  count, loTagPair taglist[],
		VARIANT   values[],	WORD      qualities[],	FILETIME  stamps[],
		HRESULT   errs[],	HRESULT  *master_err,	HRESULT  *master_qual,
		const VARTYPE vtype[],	LCID lcid)
{  return loDR_STORED; }
//-------------------------------------------------------------------
INT WriteTags(const loCaller *ca,
              unsigned count, loTagPair taglist[],
              VARIANT values[], HRESULT error[], HRESULT *master, LCID lcid)
{
 unsigned ii,i;
 VARIANT v;				// input data - variant type
 char ldm;				
 CHAR Out[500]; CHAR sBuf1[5000];
 UINT bytes;
 struct lconv *lcp;		// Contains formatting rules for numeric values in different countries/regions
 lcp = localeconv();	// Gets detailed information on locale settings.	
 ldm = *(lcp->decimal_point);	// decimal point (i nah ona nujna?)
 VariantInit(&v);				// Init variant type
 UL_TRACE((LOGID, "WriteTags (%d) invoked", count));	
 
 EnterCriticalSection(&lk_values);	
 for(ii = 0; ii < count; ii++) 
	{
      HRESULT hr = 0;
	  loTagId clean = 0;
      i = (unsigned)taglist[ii].tpRt;
	  UL_TRACE((LOGID, "WriteTags(i=%u tpRt=%u tpTi=%u)",i,taglist[ii].tpRt,taglist[ii].tpTi));
      //ci = i % devp->cv_size;
      //devi = i / devp->cv_size;
      if (!taglist[ii].tpTi || !taglist[ii].tpRt || i >= tag_num) continue;
      VARTYPE tvVt = tv[i].tvValue.vt;
      hr = VariantChangeType(&v, &values[ii], 0, tvVt);
      if (hr == S_OK) 
		{
		 UL_TRACE((LOGID, "%!l WriteTags(Rt=%u Ti=%u %s)",hr, taglist[ii].tpRt, taglist[ii].tpTi, tn[i]));
		 DWORD dwBytesRead=0, enddt=0;
		 DWORD dwbr1=dwBytesRead;
		 COMSTAT stat;	 
		 DWORD dwErrors=CE_FRAME|CE_IOE|CE_TXFULL|CE_RXPARITY|CE_RXOVER|CE_OVERRUN|CE_MODE|CE_BREAK;	
		 port[DR[i].device].ClearWriteBuffer();
		 port[DR[i].device].Purge(PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
		 port[DR[i].device].ClearError(dwErrors);
		 port[DR[i].device].Read(sBuf1, 100);

		 Out[0]=(UCHAR)DeviceRU[DR[i].device].adr; Out[1]=0x6;
		 UINT addr=DR[i].adr-1;
		 Out[2]=(UCHAR)(addr/256); Out[3]=(UCHAR)(addr%256);
		 if (DR[i].type==1 || DR[i].adr==11175) { UL_INFO ((LOGID,"write to [%f]",v.fltVal)); v.fltVal=v.fltVal/DR[i].mn; UL_INFO ((LOGID,"write to [%f]",v.fltVal)); Out[4]=(UINT)v.fltVal/256; Out[5]=(UINT)v.fltVal%256; UL_INFO ((LOGID,"write to [%d][%d][%x][%x]",Out[4],Out[5],Out[4],Out[5]));}
		 else { Out[4]=(UINT)v.iVal/256; Out[5]=(UINT)v.iVal%256; }
		 UL_INFO ((LOGID,"write to [%d] nAdr=(%d)[%d] | nFunc=(%d)",DR[i].device,DR[i].adr,DR[i].adr,6));
		 unsigned short crc=CRC16((UCHAR *)Out, 6); 
		 Out[6]=crc%256; Out[7]=crc/256;
		 UL_INFO ((LOGID,"send to ecl-300 [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",(UCHAR)Out[0],(UCHAR)Out[1],(UCHAR)Out[2],(UCHAR)Out[3],(UCHAR)Out[4],(UCHAR)Out[5],(UCHAR)Out[6],(UCHAR)Out[7]));

		 dwbr1=0; dwBytesRead=0, enddt=0;
		 port[DR[i].device].Write (Out, 8);
		 Sleep (3000);			 
		 BOOL AS=FALSE; 
		 port[DR[i].device].GetStatus(stat);
		 if (stat.cbInQue) dwBytesRead = port[DR[i].device].Read(sBuf1, 70);
		 if (dwBytesRead>30) dwBytesRead=30;
		 for (UINT po=0;po<dwBytesRead;po++) UL_INFO((LOGID,"I[%d] = 0x%x",po,(UCHAR)sBuf1[po]));
		 bytes=sBuf1[2];
		 if (dwBytesRead>5)
			{
			 if (bytes>0) UL_INFO((LOGID,"recieved %d bytes",bytes));
			 else UL_INFO((LOGID,"wrong pack %d byte recieved",bytes));

			 unsigned short crc= CRC16((UCHAR *)sBuf1, (unsigned short)dwBytesRead-2);
			 if ((UCHAR)sBuf1[bytes+5]==(UCHAR)(crc%256) && (UCHAR)sBuf1[bytes+6]==(UCHAR)(crc/256)) UL_INFO((LOGID,"crc | recieved [0x%x 0x%x] calculated [0x%x 0x%x]",(UCHAR)sBuf1[bytes+3],(UCHAR)sBuf1[bytes+4],(UCHAR)(crc%256),(UCHAR)(crc/256)));
			 else UL_INFO((LOGID,"wrong crc | recieved [0x%x 0x%x] calculated [0x%x 0x%x]",(UCHAR)sBuf1[bytes+5],(UCHAR)sBuf1[bytes+6],(UCHAR)(crc%256),(UCHAR)(crc/256)));
			 UL_INFO((LOGID,"[com%d] [%d/%d] write value=%d ",DeviceRU[DR[i].device].com,i,tag_num,DR[i].value));
			 DR[i].status=1;
			}
		 else 
			{
			 if (dwBytesRead==5) UL_INFO((LOGID,"no bytes recieved [0x%x] [error counter %d]",sBuf1[2],DeviceRU[DR[i].device].connect));
			 else UL_INFO((LOGID,"no bytes recieved [error counter %d]",sBuf1[2],DeviceRU[DR[i].device].connect));
			}

	  }
       VariantClear(&v);
	   if (S_OK != hr) 
			{
			 *master = S_FALSE;
			 error[ii] = hr;
			 UL_TRACE((LOGID, "Write failed"));
			}
	   UL_TRACE((LOGID, "Write success"));
       taglist[ii].tpTi = clean; 
  }
 LeaveCriticalSection(&lk_values); 

 return loDW_TOCACHE; 
}
//-------------------------------------------------------------------
VOID activation_monitor(const loCaller *ca, INT count, loTagPair *til){}
//-------------------------------------------------------------------
UINT InitDriver()
{
 UL_INFO((LOGID, "InitDriver () start"));
 loDriver ld;		// structure of driver description
 LONG ecode;		// error code 
 tTotal = TAGS_NUM_MAX;		// total tag quantity
 if (my_service) {
      UL_ERROR((LOGID, "Driver already initialized!"));
      return 0;
  }
 memset(&ld, 0, sizeof(ld));   
 ld.ldRefreshRate =5000;		// polling time 
 ld.ldRefreshRate_min = 4000;	// minimum polling time
 ld.ldWriteTags = WriteTags;	// pointer to function write tag
 ld.ldReadTags = ReadTags;		// pointer to function read tag
 ld.ldSubscribe = activation_monitor;	// callback of tag activity
 ld.ldFlags = loDF_IGNCASE;				// ignore case
 ld.ldBranchSep = '.';					// hierarchial branch separator
 ecode = loServiceCreate(&my_service, &ld, tTotal);		//	creating loService 
 UL_TRACE((LOGID, "%!e loServiceCreate()=",ecode));	// write to log returning code
 if (ecode) return 1;									// error to create service	
 
 InitializeCriticalSection(&lk_values);
 UL_TRACE((LOGID, "open config file %s",CFG_FILE));
 TiXmlDocument doc(CFG_FILE);
 BOOL ok = doc.LoadFile();
 if (!ok)
	{
	 UL_TRACE((LOGID, "Failed to load config file %s", CFG_FILE));	
	 return 1;
	}
 UL_TRACE((LOGID, "load file success"));
 CHAR	name[100];
 EnumerateSerialPorts();
 
 TiXmlHandle hDoc(&doc);
 TiXmlElement* pElem;
 TiXmlHandle hRoot(0);
 //pElem=hDoc.FirstChildElement().Element();
 //UL_TRACE((LOGID, "pElem=%d",pElem));
 // should always have a valid root but handle gracefully if it does
 //if (!pElem) return 1;
 //strcpy (name,pElem->Value());
 //UL_TRACE((LOGID, "name=%s",name)); // comports
 //hRoot=TiXmlHandle(pElem);
 TiXmlElement* pWindowNode;
 TiXmlElement* root;
 //TiXmlElement* root = pElem=hDoc.FirstChildElement().Element();
/*
 if ( root )
	{
	 TiXmlElement* pWindowNode = root->FirstChildElement( "comports" );
	 // UL_TRACE((LOGID, "pWindowNode=%s",pWindowNode->Value())); // comports
	 for( pWindowNode; pWindowNode; pWindowNode=pWindowNode->NextSiblingElement())
		{
		 const char *pName=pWindowNode->Attribute("name");
		 if (pName) strcpy (DeviceRU[devNum].name,pName);
		 pName=pWindowNode->Attribute("com");
		 if (pName) DeviceRU[devNum].com=atoi(pName);
		 pName=pWindowNode->Attribute("adr");
		 if (pName) DeviceRU[devNum].adr=atoi(pName);
		 DeviceRU[devNum].tags_num=sizeof(ECLTag)/sizeof(ECLTag[0]);
		 UL_INFO((LOGID, "[%d] found device %s [com %d: adr %d] tag_num[%d]",devNum,DeviceRU[devNum].name,DeviceRU[devNum].com,DeviceRU[devNum].adr,DeviceRU[devNum].tags_num));
		 devNum++; tag_num+=DeviceRU[devNum].tags_num;
		}
	}*/
 // COM -------------------------------------------------------------------------------------------------
 COMMTIMEOUTS timeouts;
 timeouts.ReadIntervalTimeout = 3;
 timeouts.ReadTotalTimeoutMultiplier = 0;	//0
 timeouts.ReadTotalTimeoutConstant = 80;	// !!! (180)
 timeouts.WriteTotalTimeoutMultiplier = 0;	//0
 timeouts.WriteTotalTimeoutConstant = 25;	//25 

 pElem=hDoc.FirstChildElement("eclOpcCfg").Element(); 
 if (!pElem) return 1;
 hRoot=TiXmlHandle(pElem);
 pElem=hRoot.FirstChildElement("comports").Element();
 if (!pElem) return 1;
 hRoot=TiXmlHandle(pElem);
 strcpy (name,pElem->Value());
// UL_TRACE((LOGID, "name=%s",name)); // comports

 pWindowNode=hRoot.FirstChild("comport").Element();
 for( pWindowNode; pWindowNode; pWindowNode=pWindowNode->NextSiblingElement())
		{
		 const char *pName2;
		 const char *pName=pWindowNode->Attribute("com");
		 if (pName) Com[com_num].num=atoi(pName);
		 pName=pWindowNode->Attribute("baudrate");
		 if (pName) Com[com_num].speed=atoi(pName);
		 Com[com_num].nm=com_num;

		 TiXmlElement* pWindowNode2 =pWindowNode->FirstChild("comfort")->ToElement();
		 //UL_TRACE((LOGID, "pWindowNode2=%d",pWindowNode2)); // comports

		 for( pWindowNode2; pWindowNode2; pWindowNode2=pWindowNode2->NextSiblingElement())
			{
	 		 pName=pWindowNode2->Attribute("name");
			 if (pName) strcpy (DeviceRU[devNum].name,pName);
			 //UL_TRACE((LOGID, "pName=%d [%s]",pName,DeviceRU[devNum].name)); // comports
			 pName2=pWindowNode2->Attribute("addr");
			 if (pName2) DeviceRU[devNum].adr=atoi(pName2);
			 
			 DeviceRU[devNum].com=Com[com_num].num;
			 DeviceRU[devNum].nm=com_num;

			 //UL_TRACE((LOGID, "pName2=%d [%d]",pName2,DeviceRU[devNum].adr)); // comports
			 DeviceRU[devNum].tags_num=sizeof(ECLTag)/sizeof(ECLTag[0]);			 
			 UL_INFO((LOGID, "Device [%s] | adr [%d] | Com %d | Speed %d (com/dev) [%d/%d]",DeviceRU[devNum].name,DeviceRU[devNum].adr,Com[com_num].num,Com[com_num].speed,com_num,devNum));
			 devNum++; tag_num+=DeviceRU[devNum].tags_num;
			}

		 INT res=sio_checkalive (Com[com_num].num, 3000);
		 switch (res)
			{
			 case D_ALIVE_CLOSE: UL_INFO((LOGID, "This port is alive and is not used by any program")); break;
			 case D_ALIVE_OPEN: UL_INFO((LOGID, "This port is alive and is used by some program")); break;
			 case D_NOT_ALIVE: UL_INFO((LOGID, "This port is not alive")); break;
			 case D_NOT_NPORT_COM: UL_INFO((LOGID, "The COM port number is not belonged to any configured NPort Server on the host")); break;
			 case D_SOCK_INIT_FAIL:	UL_INFO((LOGID, "Initialize the WinSock fail")); break;
			}
		 UL_INFO((LOGID, "Opening port COM%d on speed %d with parity 1 and databits 8",Com[com_num].num,Com[com_num].speed));
		 //if (!port[com_num].Open(Com[com_num].num,Com[com_num].speed, SerialPort::NoParity, 8, SerialPort::OneStopBit, SerialPort::NoFlowControl, FALSE)) { UL_ERROR((LOGID, "Error open COM-port")); continue;}
		// if (!port[com_num].Open(Com[com_num].num,Com[com_num].speed, SerialPort::EvenParity, 8, SerialPort::OneStopBit, SerialPort::NoFlowControl, FALSE)) { UL_ERROR((LOGID, "Error open COM-port")); continue; }
		CHAR sPort[100];
		sprintf (sPort,"\\\\.\\COM%d", Com[com_num].num);
		UL_INFO((LOGID, "%s",sPort));
		/*HANDLE m_hComm;       //Handle to the comms port
		m_hComm = CreateFile((LPCWSTR)_T("\\\\.\\COM16"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FALSE ? FILE_FLAG_OVERLAPPED : 0, NULL);
		if (m_hComm == INVALID_HANDLE_VALUE) UL_INFO((LOGID, "error1"));
		sprintf (sPort,"COM%d:", Com[com_num].num);
		UL_INFO((LOGID, "GetLastError()=%x %ld",GetLastError(),GetLastError()));	
		m_hComm = CreateFile((LPCWSTR)sPort, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (m_hComm == INVALID_HANDLE_VALUE) UL_INFO((LOGID, "error2"));
		UL_INFO((LOGID, "GetLastError()=%x %ld",GetLastError(),GetLastError()));	
		sprintf (sPort,"COM%d", Com[com_num].num);
		m_hComm = CreateFile((LPCWSTR)sPort, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FALSE ? FILE_FLAG_OVERLAPPED : 0, NULL);
		if (m_hComm == INVALID_HANDLE_VALUE) UL_INFO((LOGID, "error3"));*/
//		HANDLE m_hComm;       //Handle to the comms port
//		m_hComm = CreateFile((LPCWSTR)_T("\\\\.\\COM4"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FALSE ? FILE_FLAG_OVERLAPPED : 0, NULL);
//		if (m_hComm == INVALID_HANDLE_VALUE) UL_INFO((LOGID, "error1"));
		if (!port[com_num].Open(Com[com_num].num,Com[com_num].speed, SerialPort::EvenParity, 8, SerialPort::OneStopBit, SerialPort::NoFlowControl, FALSE)) { UL_ERROR((LOGID, "Error open COM-port"));  continue; }
		UL_INFO((LOGID, "GetLastError()=%x %ld",GetLastError(),GetLastError()));		
		port[com_num].SetTimeouts(timeouts);
		UL_INFO((LOGID, "Set COM-port timeouts %d:%d:%d:%d:%d",timeouts.ReadIntervalTimeout,timeouts.ReadTotalTimeoutMultiplier,timeouts.ReadTotalTimeoutConstant,timeouts.WriteTotalTimeoutMultiplier,timeouts.WriteTotalTimeoutConstant));
		com_num++;
		}
 init_tags(1);
 if (ScanBus()) { UL_INFO((LOGID, "total %d devices found on %d com-ports",devNum,com_num)); return 0; }
 else			{ UL_ERROR((LOGID, "no devices found")); 	  return 0; } 
}
//----------------------------------------------------------------------------------------
UINT ScanBus()
{
 UINT ecode=0;
 UL_INFO((LOGID, "scanBus started (%d)",devNum));
 DWORD start=GetTickCount();
 return devNum;
}
//----------------------------------------------------------------------------------------------
VOID poll_device()
{
 UL_DEBUG((LOGID, "poll_device ()"));
 FILETIME ft; INT ecode=0; DWORD start=0;
	for (UINT i=0; i<com_num; i++)
	{
	 hThrd[i+1] = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) PollDeviceCOM, (LPVOID)Com[i].num, 0, &dwThrdID);
	 ReadEndFlag[i+1]=false;
	 Sleep (1000);
	}
 ecode=com_num; start=GetTickCount();
 while (1)
	{
	 for (UINT i=1; i<=com_num; i++)
		 if (ReadEndFlag[i]) 
			{
			 ecode--;
			 UL_DEBUG((LOGID, "[ReadEndFlag]=0x%x | i=%d | ecode=%d",ReadEndFlag,i,ecode));
			 ReadEndFlag[i]=false;
			}
	 if (!my_CF.in_use())
		{
		 UL_DEBUG((LOGID, "my_CF.in_use(). server not use"));
		 for (UINT i=0; i<com_num; i++)
			{
			 hReadEndEvents[i]=(HANDLE)0x10;	// exit signal
			 Sleep(3000);
			 return;
			}
		}
	 if (!ecode) 
		{
		 UL_DEBUG((LOGID, "All threads are complete. Total %d seconds.",(GetTickCount()-start)/1000));
		 Sleep(3000);
		 break;
		}
	 Sleep(2000);
	}
 Sleep (200);
 GetSystemTimeAsFileTime(&ft);
 EnterCriticalSection(&lk_values);
 UL_DEBUG((LOGID, "Data to tag (%d)",tag_num));
 for (UINT ci=0;ci<tag_num; ci++)
	{
	 UL_DEBUG((LOGID, "[%d] v = %f [%d]",ci,DR[ci].value,DR[ci].status));
	 VARTYPE tvVt = tv[ci].tvValue.vt;
	 VariantClear(&tv[ci].tvValue);	  
	 //CHAR   *stopstring;	 
	 switch (tvVt)
		{
		 case VT_I2: V_I2(&tv[ci].tvValue) = (UINT) DR[ci].value; break;
		 case VT_R4: V_R4(&tv[ci].tvValue) = (FLOAT) DR[ci].value; break;
		}

	 V_VT(&tv[ci].tvValue) = tvVt;
	 if (DR[ci].status) tv[ci].tvState.tsQuality = OPC_QUALITY_GOOD;
	 else tv[ci].tvState.tsQuality = OPC_QUALITY_UNCERTAIN;
	 if (DR[ci].status==4 || DR[ci].status==5) tv[ci].tvState.tsQuality = OPC_QUALITY_CONFIG_ERROR;
	 tv[ci].tvState.tsTime = ft;
	}
 loCacheUpdate(my_service, tag_num, tv, 0);
 LeaveCriticalSection(&lk_values);
 Sleep(100);
 UL_DEBUG((LOGID, "loCacheUpdate complete (%d)",tag_num));
 Sleep(3000);
}
//-----------------------------------------------------------------------------------
VOID PollDeviceCOM (LPVOID lpParam)
{
 UL_INFO((LOGID,"PollDeviceCOM (%d)",(UINT) lpParam)); 
 CHAR sBuf2[1000]={0}; CHAR sBuf1[5000]; CHAR Out[500];
 UINT ccom=(UINT)lpParam; UINT DeviceErr=0; SHORT bytes=0;
 UL_INFO((LOGID,"Read COM %d | device=%d | com=%d",ccom,devNum,DeviceRU[0].com));
 UINT device=0;
 for (device=0;device<devNum;device++)
 if (DeviceRU[device].com==(INT)ccom)
	{
	 DeviceErr=0;
	 UL_DEBUG((LOGID, "[com=%d/ecl=%d] device=%s",DeviceRU[device].com,DeviceRU[device].adr,DeviceRU[device].name));
	 DWORD dwStoredFlags = EV_RXCHAR | EV_TXEMPTY | EV_RXFLAG;
	 port[DeviceRU[device].com].SetMask (dwStoredFlags);

	 for (UINT tag=0; tag<tag_num; tag++)
	 if (DR[tag].device==device)		 
		{
		 UL_DEBUG((LOGID, "Tag [%d/%d] %s | adr=[0x%x]",tag,tag_num,DR[tag].name,DR[tag].adr));
		 if (!my_CF.in_use()) { UL_INFO((LOGID,"exit signal")); return;}
		 if (DR[tag].adr==0) 
			{
			 if (DeviceRU[device].connect==0) DR[tag].value=0;
			 else DR[tag].value=1;
			 DR[tag].status=1;
			 UL_DEBUG((LOGID, "connect [%d] %f",DR[tag].status,DR[tag].value));
			 continue;
			}

		 DWORD ctime=GetTickCount();
		 DWORD dwBytesRead=0;
		 DWORD enddt=0;
		 DWORD dwbr1=dwBytesRead;
		 COMSTAT stat;	 
		 DWORD dwErrors=CE_FRAME|CE_IOE|CE_TXFULL|CE_RXPARITY|CE_RXOVER|CE_OVERRUN|CE_MODE|CE_BREAK;	
		 port[DeviceRU[device].nm].ClearWriteBuffer();
		 port[DeviceRU[device].nm].Purge(PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
		 port[DeviceRU[device].nm].ClearError(dwErrors);
		 port[DeviceRU[device].nm].Read(sBuf1, 100);

		 // adr=2; nFunc=3; nAdr=11203;
		 //sprintf (buf,"%c%c%c%c%c",adr,0x41,nFunc,(UCHAR)(nAdr/256),(UCHAR)(nAdr%256));
		 Out[0]=(UCHAR)DeviceRU[device].adr; Out[1]=(UCHAR)DR[tag].func; //Out[2]=0x7; Out[3]=0xd2; Out[4]=0x0; Out[5]=0x1;
		 UINT addr=DR[tag].adr-1;
		 //Out[2]=(UCHAR)(DR[tag].adr/256); Out[3]=(UCHAR)(DR[tag].adr%256);
		 Out[2]=(UCHAR)(addr/256); Out[3]=(UCHAR)(addr%256);
		 Out[4]=0x0; Out[5]=0x1;
		 UL_INFO ((LOGID,"nAdr=(%d)[%d] | nFunc=(%d)",DR[tag].adr,DeviceRU[device].adr,3));
		 // buf[5]=(CHAR)0x2b; buf[6]=(CHAR)0xc4;
		 // buf[7]=(CHAR)0x2f; buf[8]=(CHAR)0xad;
		 unsigned short crc=CRC16((UCHAR *)Out, 6); 
		 Out[6]=crc%256; Out[7]=crc/256;
		 UL_INFO ((LOGID,"send to ecl-300 [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",(UCHAR)Out[0],(UCHAR)Out[1],(UCHAR)Out[2],(UCHAR)Out[3],(UCHAR)Out[4],(UCHAR)Out[5],(UCHAR)Out[6],(UCHAR)Out[7]));

		 dwbr1=0; dwBytesRead=0, enddt=0;
		 //02 41 03   2B C3  2B C4  2F AD  32 F2
		 //adr func data crc
		 port[DeviceRU[device].nm].Write (Out, 8);
		 Sleep (500);			 // WAS 1000
		 BOOL AS=FALSE; 
		 port[DeviceRU[device].nm].GetStatus(stat);
		 if (stat.cbInQue) dwBytesRead = port[DeviceRU[device].nm].Read(sBuf1, 70);
		 UL_INFO((LOGID,"device=%d | nm=%d | dwBytesRead=%d",device,DeviceRU[device].nm,dwBytesRead));
		 if (dwBytesRead>30) dwBytesRead=30;
		 UL_INFO((LOGID,"dwBytesRead=%d",dwBytesRead));
		 for (UINT po=0;po<dwBytesRead;po++) 
		 if (po<30)
			 UL_INFO((LOGID,"I[%d/%d] = 0x%x",po,dwBytesRead,(UCHAR)sBuf1[po]));
		 bytes=sBuf1[2];
		 // 02 41 0A FF 63 C9 54
		 if (dwBytesRead>5)
			{
			 if (bytes>0) UL_INFO((LOGID,"recieved %d bytes",bytes));
			 else UL_INFO((LOGID,"wrong pack %d byte recieved",bytes));

			 unsigned short crc= CRC16((UCHAR *)sBuf1, (unsigned short)dwBytesRead-2);			 
			 if ((UCHAR)sBuf1[bytes+3]==(UCHAR)(crc%256) && (UCHAR)sBuf1[bytes+4]==(UCHAR)(crc/256)) UL_INFO((LOGID,"crc | recieved [0x%x 0x%x] calculated [0x%x 0x%x]",(UCHAR)sBuf1[bytes+3],(UCHAR)sBuf1[bytes+4],(UCHAR)(crc%256),(UCHAR)(crc/256)));
			 else UL_INFO((LOGID,"wrong crc | recieved [0x%x 0x%x] calculated [0x%x 0x%x]",(UCHAR)sBuf1[bytes+3],(UCHAR)sBuf1[bytes+4],(UCHAR)(crc%256),(UCHAR)(crc/256)));
			 INT val=0;
			 //sBuf1[3]=DR[tag].adr/256; sBuf1[4]=DR[tag].adr%256;
			 //UL_INFO((LOGID,"data [0x%x 0x%x] %f %f",sBuf1[3],sBuf1[4],(DOUBLE)((UCHAR)sBuf1[3]*256+(UCHAR)sBuf1[4]), (DOUBLE)((DOUBLE)((UCHAR)sBuf1[3]*256)+(UCHAR)sBuf1[4]*DR[tag].mn)));
			 if ((UCHAR)sBuf1[3]>=0x80) DR[tag].value=(65536-((UCHAR)sBuf1[3]*256+(UCHAR)sBuf1[4]))*DR[tag].mn;
			 else { DR[tag].value=(DOUBLE)((UCHAR)sBuf1[3]*256+(UCHAR)sBuf1[4])*DR[tag].mn; }
			 UL_INFO((LOGID,"[com%d] [%d/%d] value=%f [0x%x 0x%x] [%f]",DeviceRU[device].com,tag,tag_num,DR[tag].value,(UCHAR)sBuf1[3],(UCHAR)sBuf1[4],DR[tag].mn));
			 DR[tag].status=1;
			 DeviceRU[device].connect=0;
			}
		 else 
			{
			 DeviceRU[device].connect++;
			 DR[tag].status=0;
			 if (dwBytesRead==5) UL_INFO((LOGID,"no bytes recieved [0x%x] [error counter %d]",sBuf1[2],DeviceRU[device].connect));
			 else UL_INFO((LOGID,"no bytes recieved [error counter %d]",sBuf1[2],DeviceRU[device].connect));
			}
		}
	}
 Sleep (2000);
 for (device=0;device<devNum;device++)
 if (DeviceRU[device].com==(INT)ccom)
		ReadEndFlag[DeviceRU[device].nm+1]=true;
 UL_INFO((LOGID,"PollDeviceCOM (%d) complete (%d)",(UINT) lpParam,DeviceRU[device].nm));
}
//-----------------------------------------------------------------------------------
UINT DestroyDriver()
{
  if (my_service)		
    {
      INT ecode = loServiceDestroy(my_service);
      UL_INFO((LOGID, "%!e loServiceDestroy(%p) = ", ecode));	// destroy derver
      DeleteCriticalSection(&lk_values);						// destroy CS
      my_service = 0;		
    }
 return	1;
}
//------------------------------------------------------------------------------------
INT init_tags(UINT nSock)
{
  UL_INFO((LOGID, "init_tags(%d | %d)",devNum,tag_num));
  FILETIME ft;		// 64-bit value representing the number of 100-ns intervals since January 1,1601
  UINT rights=0;	// tag type (read/write)
  UINT ecode;
  WCHAR buf[DATALEN_MAX];
  LCID lcid;
  GetSystemTimeAsFileTime(&ft);	// retrieves the current system date and time
  EnterCriticalSection(&lk_values);
  //if (!tag_num) return 0;
  UINT tag_add=0;
  for (UINT i=0; i<devNum; i++)
	{
	 UL_ERROR((LOGID, "(%d/%d) %s | tags=%d",i,devNum,DeviceRU[i].name,DeviceRU[i].tags_num));
	 UINT max = (UINT)DeviceRU[i].tags_num;
	 for (UINT r=0; r<max; r++)
		{
		 //UL_ERROR((LOGID, "device=%s (%d/%d)",DeviceRU[i].name,r,DeviceRU[i].tags_num));
		 tn[tag_add] = new char[DATALEN_MAX];	// reserve memory for massive
		 sprintf(tn[tag_add],"COM%d_%s_Danfoss.Comfort300.%s",DeviceRU[i].com,DeviceRU[i].name, ECLTag[r].tag);
		 if (ECLTag[r].mode==1) rights=OPC_READABLE;
		 if (ECLTag[r].mode==2) rights=OPC_WRITEABLE;
		 if (ECLTag[r].mode==3) rights=OPC_READABLE | OPC_WRITEABLE;
		 //rights=ECLTag[r].mode;
		 VariantInit(&tv[tag_add].tvValue);
		 lcid = MAKELCID(0x0409, SORT_DEFAULT); // This macro creates a locale identifier from a language identifier. Specifies how dates, times, and currencies are formatted
 		 MultiByteToWideChar(CP_ACP, 0,tn[tag_add], strlen(tn[tag_add])+1,	buf, sizeof(buf)/sizeof(buf[0])); // function maps a character string to a wide-character (Unicode) string

		 if (ECLTag[r].type==0)
			{
			 V_I2(&tv[tag_add].tvValue) = 0;
			 V_VT(&tv[tag_add].tvValue) = VT_I2;
			}
		 if (ECLTag[r].type==1)
			{
			 V_R4(&tv[tag_add].tvValue) = 0.0;
			 V_VT(&tv[tag_add].tvValue) = VT_R4;
			}
		 ecode = loAddRealTag_aW(my_service, &ti[tag_add], (loRealTag)(tag_add), buf, 0, rights, &tv[tag_add].tvValue, 0, 0); 
		 tv[tag_add].tvTi = ti[tag_add];
		 DR[tag_add].tn=ti[tag_add];

		 DR[tag_add].value=(DOUBLE)0.0;
		 DR[tag_add].device=i;
		 DR[tag_add].func=ECLTag[r].func;
		 DR[tag_add].adr=ECLTag[r].adr;
		 DR[tag_add].type=ECLTag[r].type;
		 DR[tag_add].mn=(FLOAT)ECLTag[r].mn;
		 strcpy (DR[tag_add].name,tn[tag_add]);
		 tv[tag_add].tvState.tsTime = ft;
		 tv[tag_add].tvState.tsError = S_OK;
		 tv[tag_add].tvState.tsQuality = OPC_QUALITY_NOT_CONNECTED;
		 UL_DEBUG((LOGID, "Tag [%d/%d] dev=%d | adr=[0x%x] val=%f [0x%x %f]",r,max,DR[tag_add].device,DR[tag_add].adr,DR[tag_add].value,ECLTag[r].adr,DR[tag_add].value));
		 UL_TRACE((LOGID, "%!e [%s] loAddRealTag(%s) = %u [%d] - %s", ecode, DeviceRU[i].name, tn[tag_add], ti[tag_add], tag_add, ECLTag[r].name));
		 if (ti[tag_add]>=tag_add) tag_add++;
		}
	}
  tag_num=tag_add;
  LeaveCriticalSection(&lk_values);  
  if(ecode) 
	{
	 UL_ERROR((LOGID, "%!e driver_init()=", ecode));
     return -1;
    }
  return 0;
}
//---------------------------------------------------------------------------------------------------
// 02 41 03   2B C3  2B C4  2F AD  32 F2
// adr func data crc
//---------------------------------------------------------------------------------------------------
CHAR* FormSequence (UCHAR adr, UCHAR nFunc, UINT nAdr)
{
 CHAR buf[250]={0}; CHAR *pbuf=buf;
// adr=2; nFunc=3; nAdr=11203;
 //sprintf (buf,"%c%c%c%c%c",adr,0x41,nFunc,(UCHAR)(nAdr/256),(UCHAR)(nAdr%256));
 //sprintf (buf,"%c%c%c%c",adr,nFunc,(UCHAR)(nAdr/256),(UCHAR)(nAdr%256));
 sprintf (buf,"%c%c%c%c%c%c",2,3,7,0xd2,0,1);
 buf[0]=0x2; buf[1]=0x3; buf[2]=0x7; buf[3]=(UCHAR)0xd2; buf[4]=0x0; buf[5]=0x1;
 UL_INFO ((LOGID,"nAdr=(%d)[%d] | nFunc=(%d)",nAdr,adr,nFunc));
// buf[5]=(CHAR)0x2b; buf[6]=(CHAR)0xc4;
// buf[7]=(CHAR)0x2f; buf[8]=(CHAR)0xad;
 unsigned short crc=CRC16((UCHAR *)buf, 6); 
 buf[6]=crc%256; buf[7]=crc/256;

// UL_INFO ((LOGID,"send to ecl-300 [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",(UCHAR)buf[0],(UCHAR)buf[1],(UCHAR)buf[2],(UCHAR)buf[3],(UCHAR)buf[4],(UCHAR)buf[5],(UCHAR)buf[6],(UCHAR)buf[7],(UCHAR)buf[8],(UCHAR)buf[9],(UCHAR)buf[10]));
  UL_INFO ((LOGID,"send to ecl-300 [0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x]",(UCHAR)buf[0],(UCHAR)buf[1],(UCHAR)buf[2],(UCHAR)buf[3],(UCHAR)buf[4],(UCHAR)buf[5],(UCHAR)buf[6],(UCHAR)buf[7]));
 return pbuf;
}
//-----------------------------------------------------------------------------------------------------------------
UINT ParseInput (CHAR* buf, UINT nSock, UINT type)
{
 //UL_INFO((LOGID,"[%d] [%d] %s",prm_num,PR[nSock][prm_num].prm,PR[nSock][prm_num].name));
 //if (value[0]) strcpy(DeviceRU[nSock].dv,value[0]);
 return 0;
}
//-----------------------------------------------------------------------------------------------------------------
/* Table of CRC values for high–order byte */
const static unsigned char auchCRCHi[] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40
} ;

/* Table of CRC values for low–order byte */
const static unsigned char auchCRCLo[] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
0x40
};
//-----------------------------------------------------------------------------------------------------------------
unsigned short CRC16 (UCHAR* puchMsg, unsigned short usDataLen )
{
unsigned char uchCRCHi = 0xFF ; /* high byte of CRC initialized */
unsigned char uchCRCLo = 0xFF ; /* low byte of CRC initialized */
unsigned uIndex ; /* will index into CRC lookup table */
while (usDataLen--) /* pass through message buffer */
{
uIndex = uchCRCLo ^ *puchMsg++ ; /* calculate the CRC */
uchCRCLo = uchCRCHi ^ auchCRCHi[uIndex];
uchCRCHi = auchCRCLo[uIndex] ;
}
return (uchCRCHi << 8 | uchCRCLo);
}


      void EnumerateSerialPorts()
        {
          // В какой системе работаем?
          OSVERSIONINFO osvi;
          osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
          BOOL bGetVer = GetVersionEx(&osvi);
       
          // В NT используем API QueryDosDevice
          if(bGetVer && (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT))
            {
              // Используем QueryDosDevice для просмотра всех устройств похожих на COMx.
             // Это наилучшее решение, так как порты не требуется открывать
              TCHAR szDevices[65535];
              DWORD dwChars = QueryDosDevice(NULL, szDevices, 65535);
              if(dwChars)
                {
                  int i=0;
       
                    for (;;)
                      {
                        // Получаем текущее имя устройства
                        TCHAR* pszCurrentDevice = &szDevices[i];
       
                        // Если похоже на "COMX" выводим на экран
                        int nLen = _tcslen(pszCurrentDevice);
                          if(nLen > 3 && _tcsnicmp(pszCurrentDevice, _T("COM"), 3) == 0)
                            {
							  UL_INFO ((LOGID,"%ws",pszCurrentDevice));
                           }
       
                        // Переходим к следующему символу терминатору
                        while(szDevices[i] != _T('\0'))
                          i++;
       
                        // Перескакиваем на следующую строку
                          i++;
       
                        // Список завершается двойным симмволом терминатором, так что если символ
                        // NULL, мы дошли до конца
                        if(szDevices[i] == _T('\0'))
                          break;
                      } // for (;;)
                  } // if(dwChars)
            } // if(bGetVer && (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT))
          else
            {
              // В 95/98 открываем каждый порт для определения его существования       
              // Поддерживается до 255 COM портов, так что мы проходим пл всему списку
              // Если мы не можем открыть порт, или происходит ошибка при открытии,
              // получаем access denied или общую ошибку все эти случаи указывают на
              // то, что под таким номером есть порт.
              for (UINT i=1; i<256; i++)
                {
                  // Формируем сырое имя устройства
                  char sPort[10];
                  sprintf(sPort,"\\\\.\\COM%d", i);
                  // Пытаемся открыть каждый порт
                  BOOL bSuccess = FALSE;
                  HANDLE hPort = ::CreateFile((LPCWSTR)sPort, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
                  if(hPort == INVALID_HANDLE_VALUE)
                    {
                      DWORD dwError = GetLastError();       
                      // Смотрим что получилось при открытии
                      if(dwError == ERROR_ACCESS_DENIED || dwError == ERROR_GEN_FAILURE)
					  bSuccess = TRUE;
                    }
                  else
                    {
                      // Порт открыт успешно
                      bSuccess = TRUE;       
                      // Не забываем закрывать каждый открытый порт,
                      // так как мы не собираемся с ним работать...
                      CloseHandle(hPort);
                    } // if(hPort == INVALID_HANDLE_VALUE)
                  // Выводим на экран название порта
                  if(bSuccess)
                    {
                      printf(sPort);
                      printf("\n");
                    }
                } 
            } 
        }
