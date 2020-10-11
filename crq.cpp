//-----------------------------------------------------------------------------
#include "windows.h"
#include "unilog.h"
#include "stdio.h"
#include "resource.h"
#include "logika.h"
//-----------------------------------------------------------------------------
#define LOGID logg,0			// id log
#define COMM_BUFFER_SIZE	500
#define SMALL_BUFFER_SIZE 30
//-----------------------------------------------------------------------------
#define HTTP_PORT	80				// http-port
#define IDENT		"ГРП №1"		// идентификатор
//-----------------------------------------------------------------------------
extern "C" HWND		hDialog;
extern "C" BOOL		workit[20];
extern "C" BOOL		workhttp;
extern "C" unilog	*logg;	

SHORT ver_major;
SHORT ver_minor;
CHAR  ver_time[50];
CHAR  ver_desc[100];
CHAR  sernum[50];
CHAR  softver[50],wsoftver[50];
SHORT maxmodule=2,kA=0,wkA=0;
SYSTEMTIME time1,time2,time3,st,fn;
FILETIME fst,ffn;
//-----------------------------------------------------------------------------
UINT  tTotal=0;						// total quantity of tags
UCHAR pck2[1000];
//-----------------------------------------------------------------------------
UCHAR sBuf1[7000],sBuf2[400];
CHAR Out[150];
//-----------------------------------------------------------------------------
struct sockaddr_in srv_address;
SOCKET server_socket=SOCKET_ERROR;			// Сокет для обмена
SOCKET http_cln_socket=SOCKET_ERROR;		// Сокет клиента для http
PHOSTENT phe;
WSADATA WSAData;

VOID ParseInput (CHAR* buf);
CHAR wwwroot[MAX_PATH];
CHAR hostname[MAX_PATH];
CRITICAL_SECTION output_criticalsection;
SHORT empt=0;
BOOL WorkEnable=TRUE;
//-----------------------------------------------------------------------------
VOID HandleHTTPRequest (VOID *data);
BOOL hConnection (CHAR tP, INT _s_port, SOCKET &sck, SOCKET &cli);
BOOL ConnectToServer (SOCKET server_socket);
//----------------------------------------------------
struct HTTPRequestHeader
	{
	 CHAR method[SMALL_BUFFER_SIZE];
	 CHAR url[MAX_PATH];
	 CHAR filepathname[MAX_PATH];
	 CHAR httpversion[SMALL_BUFFER_SIZE];
     IN_ADDR client_ip;
	};
typedef struct _DataL DataL;		// channels info
struct _DataL {
  SHORT device;		// порядковый номер устройства.
  CHAR  module[20];	// название модуля устройства (для Каскада, например A1-01, DI-02)	
  SHORT kanal;		// порядковый номер канала в  модуле.
  SHORT nmodule;	// порядковый номер модуля в приборе / контроллере
  SHORT status;		// текущий статус, (0-нет связи, 1-нормально, 2-:).
  CHAR  value[500];	// текущее значение.
  SHORT type;		// тип измеряемой величины (0-float, 1-int, 2-text..)
};
DataL DataRE;
DataL DRL[TAGS_NUM_MAX];
//-----------------------------------------------------------------------------
VOID StartHttpSrv(LPVOID lpParam)
{
 BOOL hC=FALSE;
 HANDLE hThrd;
 DWORD dwThrdID;
 UL_INFO((LOGID, "[https] winconn communicator v0.51.11 started"));
 UL_INFO((LOGID, "[https] working on protocol CRQ 5.56 (january 2006)")); 
 while (workhttp)
	{	 
	 while (!hC)
		{
		 hC=hConnection (1, HTTP_PORT, server_socket, http_cln_socket);
		 Sleep (3000);
		}	 
	 UL_INFO((LOGID, "[https] CreateThread HandleHTTPRequest [%d]",server_socket));
	 hThrd = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) HandleHTTPRequest, 0, 0, &dwThrdID);
	 WorkEnable=TRUE;
	 while (workhttp && WorkEnable) Sleep (4000);
	 hC=FALSE;
	}
 WSACleanup();
 UL_INFO((LOGID, "[Http finished]"));
}
//--------------------------------------------------------------
SOCKET StartWebServer();
VOID HandleHTTPRequest(VOID *data );
BOOL ParseHTTPHeader(CHAR *receivebuffer, HTTPRequestHeader &requestheader);
INT  SocketRead(SOCKET client_socket,CHAR *receivebuffer,INT buffersize);
VOID DetermineHost(CHAR *hostname);
//---------------------------------------------------------------
BOOL hConnection (CHAR tP, INT _s_port, SOCKET &sck, SOCKET &cli)
{
 // get name of webserver machine. needed for redirects
 // gethostname does not return a fully qualified host name
 // printf ("DetermineHost(hostname)\n");
 // DetermineHost(hostname);
 // create a critical section for mutual-exclusion synchronization on cout
 InitializeCriticalSection (&output_criticalsection); 
 // init the webserver
 UL_INFO((LOGID, "[https] start web server\n"));
 sck = StartWebServer();
 if (sck)
	{
	 UL_INFO((LOGID, "[https] ConnectToServer()"));
     ConnectToServer (sck);
	 UL_INFO((LOGID, "[https] ConnectToServer success [%d]",sck));
	 //closesocket(sck);
	}
 else
	{
	 UL_INFO((LOGID, "[https] Error in StartWebServer()"));
	 return FALSE;
	}
 // delete and release resources for critical section
 DeleteCriticalSection (&output_criticalsection); 
 return TRUE; 
}
//--------------------------------------------------------------
//	Creates server sock and binds to ip address and port
//--------------------------------------------------------------
SOCKET StartWebServer()
{
 SOCKET s;
 INT rc = WSAStartup(MAKEWORD(2, 2), &WSAData);
 
 UL_INFO ((LOGID, "WSAStartup....."));
 if(rc != 0)
	{
	 UL_INFO((LOGID, "WSAStartup failed. Error: %x",WSAGetLastError ()));
	 Sleep (10000);
	 return(0);
	}
 else UL_INFO((LOGID,"[success]"));
 UL_INFO((LOGID, "create socket....."));
 s = socket(AF_INET,SOCK_STREAM,0);
 if (s==INVALID_SOCKET)
	{
	 Sleep (10000);
	 return(0);
	}
 else UL_INFO((LOGID,"[success]"));
 SOCKADDR_IN si;
 si.sin_family = AF_INET;
 si.sin_port = htons(80);		// port
 si.sin_addr.s_addr = htonl(INADDR_ANY); 
 UL_INFO((LOGID,"bind socket"));
 if (bind(s,(struct sockaddr *) &si,sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
	 UL_INFO((LOGID,"Error in bind()"));
	 closesocket(s);
	 Sleep (10000);
	 return(0);
	}
 return(s);
}
//--------------------------------------------------------------
// WaitForClientConnections()
//		Loops forever waiting for client connections. On connection
//		starts a thread to handling the http transaction
//--------------------------------------------------------------
BOOL ConnectToServer (SOCKET server_sock)
{
 CHAR strC[50];
 GetDlgItemText (hDialog,IDC_EDIT19,strC,49); 
 phe = gethostbyname(strC);
 UL_INFO((LOGID, "[https] ConnectToServer(strC), [phe=%d]",phe));
 if(phe != NULL) // Копируем адрес узла
	{
	 memcpy((CHAR FAR *)&(srv_address.sin_addr), phe->h_addr, phe->h_length);
	 srv_address.sin_family = AF_INET;
	 srv_address.sin_port = htons(80);		// port
	 UL_INFO((LOGID, "[https] IP address is:=%s",inet_ntoa(srv_address.sin_addr)));
	 //ULOGW ("hostbyname \"%s\" is found ", strC);
	}
 else srv_address.sin_addr.s_addr = inet_addr(strC);
 UL_INFO((LOGID, "[https] Establish a connection to the server socket [%d | %d | %ul]",srv_address.sin_family,srv_address.sin_port,srv_address.sin_addr.s_addr));

 if(connect(server_sock, (PSOCKADDR)&srv_address, sizeof(srv_address)) < 0)
	{
	 closesocket(server_sock);
	 UL_INFO((LOGID, "[https] Establish a connection to %s ... connect Error | reason is %d",strC,WSAGetLastError ()));
	 return FALSE;
	}
 else 
	 UL_INFO((LOGID, "Establish a connection to %s ... connect success [%d]",strC,server_socket));
 return TRUE;
}
//--------------------------------------------------------------
// Executed in its own thread to handling http transaction
//--------------------------------------------------------------
VOID HandleHTTPRequest (VOID *data)
{ 
 INT size,err;
 CHAR receivebuffer[COMM_BUFFER_SIZE];
 CHAR sendbuffer[COMM_BUFFER_SIZE];
// sprintf (sendbuffer," GET ");
// strncat (sendbuffer,"/crq?req=version ",MAX_PATH);
// strncat (sendbuffer,"HTTP/1.0\r\n\r\n",SMALL_BUFFER_SIZE);
// err=send(server_socket,sendbuffer,strlen(sendbuffer),0);
// UL_INFO((LOGID,"send (%s)=%d[%d] [%d]",sendbuffer,err,WSAGetLastError(),server_socket));
// size = SocketRead(server_socket, receivebuffer, COMM_BUFFER_SIZE);
// UL_INFO((LOGID,"size (%d)",size));	 
// if (size == SOCKET_ERROR || size == 0)
//	{		 
//	 UL_INFO((LOGID,"Error calling recieve. closesocket ()"));
//	 closesocket(server_socket);
//	 WorkEnable=FALSE;
//	 return;
//	}
// UL_INFO((LOGID,"recieve (%s)",receivebuffer));
// Sleep (5000);
 _snprintf (sendbuffer,SMALL_BUFFER_SIZE," GET ");
 strncat (sendbuffer,"/crq?req=current&n1=1&n2=15 ",MAX_PATH);
 strncat (sendbuffer,"HTTP/1.0\r\n\r\n",SMALL_BUFFER_SIZE);
 err=send(server_socket,sendbuffer,strlen(sendbuffer),0);
 UL_INFO((LOGID,"send (%s)=%d[%d] [%d]",sendbuffer,err,WSAGetLastError(),server_socket));
 Sleep (2000);
 while (size!=SOCKET_ERROR && size!=0)
	{
	 size = SocketRead(server_socket, receivebuffer, COMM_BUFFER_SIZE);
	 if (size!=SOCKET_ERROR && size!=0) UL_INFO((LOGID,"recieve (%s)",receivebuffer));
	 ParseInput (receivebuffer);
	} 
 if (server_socket) closesocket(server_socket);
 WorkEnable=FALSE;
}
//--------------------------------------------------------------
//	Reads data from the client socket until it gets a valid http
//	header or the client disconnects.
//--------------------------------------------------------------
INT SocketRead(SOCKET client_socket, CHAR *receivebuffer, INT buffersize)
{
 INT size=0,totalsize=0;
 do
	{
	 size = recv(client_socket,receivebuffer+totalsize,buffersize-totalsize,0);
	 if (size!=0 && size!=SOCKET_ERROR)
		{
	 	 totalsize += size;
		 // are we done reading the http header?
		 if (strstr(receivebuffer,"\r\n")) break;
		}
	 else
		{
		 totalsize = size;			// remember error state for return
		 UL_INFO((LOGID,"WSAGetLastError(%d)",WSAGetLastError()));
		}
	} 
 while (size!=0 && size != SOCKET_ERROR);
 UL_INFO((LOGID,"totalsize (%d)",totalsize));
 if (totalsize>0 && totalsize<COMM_BUFFER_SIZE) receivebuffer[totalsize]=NULL;
 return(totalsize);
}
//--------------------------------------------------------------
// Fills a HTTPRequestHeader with method, url, http version
// and file system path information.
//--------------------------------------------------------------
BOOL ParseHTTPHeader(CHAR *receivebuffer, HTTPRequestHeader &requestheader)
{
 CHAR *pos;
 // http request header format method uri httpversion
 EnterCriticalSection (&output_criticalsection);
 LeaveCriticalSection (&output_criticalsection);
 // end debuggine
 pos = strtok(receivebuffer," ");
 if (pos == NULL) return(FALSE);
 strncpy(requestheader.method,pos,SMALL_BUFFER_SIZE);
 pos = strtok(NULL," ");
 if (pos == NULL) return(FALSE);
 strncpy(requestheader.url,pos,MAX_PATH);
 pos = strtok(NULL,"\r");
 if (pos == NULL) return(FALSE);
 strncpy(requestheader.httpversion,pos,SMALL_BUFFER_SIZE);
 UL_INFO((LOGID, "recieve header [%s : %s] ver: %s\n",requestheader.method,requestheader.url,requestheader.httpversion));
 //ULOGW ("[%s : %s] ver: %s",requestheader.method,requestheader.url,requestheader.httpversion);
 UL_INFO((LOGID, "[%s]",requestheader.url));
 // based on the url lets figure out the filename + path
 strncpy(requestheader.filepathname,wwwroot,MAX_PATH);
 strncat(requestheader.filepathname,requestheader.url,MAX_PATH);
 return(TRUE);
}
//--------------------------------------------------------------
// If webserver needs to redirect user from directory to
// default html file the server builds a full url and hence
// needs it's full domain name for http address.
//--------------------------------------------------------------
VOID DetermineHost(CHAR *hostname)
{
 IN_ADDR in;
 hostent *h;
 gethostname(hostname,MAX_PATH);
 h = gethostbyname(hostname);
 memcpy(&in,h->h_addr,4);
 h = gethostbyaddr((CHAR *)&in,4,PF_INET);
 strcpy(hostname,h->h_name);
}
//--------------------------------------------------------------
// ShortChanName,Value,State  V1,-0.0015,0  V2,85.12,0  V3,2888.1,0  V4,0.0,0  V13,1,0  V14,0,0  V15,0,0  V5,600.000000,1  V6,85.070778,0  V7,2888.000000,0  V11,664.012451,0  V9,17906.837000,0  V10,12266.183345,0  V8,49.741214,0  V12,398165.136282,0  V12,398165.136282,0
VOID ParseInput (CHAR* buf)
{
 UINT pos=0,pos2=0;
 CHAR *token;
 CHAR data[25][50];
 CHAR value[4][20]; 
 FLOAT T=0,P=0,Q=0,Qs=0,val=0;

 GetDlgItemText (hDialog, IDC_EDIT1 , data[0],49); T=(FLOAT)atof(data[0]);
 SetDlgItemText (hDialog, IDC_EDIT31, data[0]); SetDlgItemText (hDialog, IDC_EDIT17, data[0]);	// P(W8000)
 GetDlgItemText (hDialog, IDC_EDIT2 , data[0],49); P=(FLOAT)atof(data[0]);
 SetDlgItemText (hDialog, IDC_EDIT30, data[0]); SetDlgItemText (hDialog, IDC_EDIT16, data[0]);	// T(W8000)
 GetDlgItemText (hDialog, IDC_EDIT9 , data[0],49); Q=(FLOAT)atof(data[0]);
 SetDlgItemText (hDialog, IDC_EDIT33, data[0]); SetDlgItemText (hDialog, IDC_EDIT21, data[0]);	// Q(W8000)
 UL_INFO((LOGID,"buf (%s)",buf));

 token = strtok(buf,"V\n");
 while(token != NULL)
	{
	 sprintf(data[pos2],token);
	 UL_INFO((LOGID,"data[%d] (%s)",pos2,data[pos2]));
	 token = strtok(NULL,"V\n");
	 pos2++;
	}
 
 for (UINT i=0; i<pos2; i++)
	{	 
	 pos=0;
	 token = strtok(data[i],",\n");
	 while(token!=NULL)
		{
		 strcpy (value[pos],token);
		 UL_INFO((LOGID,"value[%d]=%s",pos,value[pos]));
		 token = strtok(NULL,",\n");
		 pos++;
		}
	 pos=atoi(value[0]); val=(FLOAT)atof (value[1]); 
	 if (pos==1) val=val*1000;
	 sprintf (value[1],"%5.2f",val);
	 UL_INFO((LOGID,"pos=%d",pos));
	 switch (pos)
		{
		 case 1: SetDlgItemText (hDialog, IDC_EDIT34, value[1]); if (P!=0) sprintf (value[0],"%5.2f",(val-P)*100/P); SetDlgItemText (hDialog, IDC_EDIT38, value[0]); break;	// P(961)
		 case 2: SetDlgItemText (hDialog, IDC_EDIT35, value[1]); if (T!=0) sprintf (value[0],"%5.2f",(val-T)*100/T); SetDlgItemText (hDialog, IDC_EDIT39, value[0]); break;	// T(961)
		 case 3: SetDlgItemText (hDialog, IDC_EDIT37, value[1]); if (Q!=0) sprintf (value[0],"%5.2f",(val-Q)*100/Q); SetDlgItemText (hDialog, IDC_EDIT41, value[0]); break;	// Q(961)
		 case 4: SetDlgItemText (hDialog, IDC_EDIT36, value[1]);  break;	// Qs(961)
		 
		 case 11: SetDlgItemText (hDialog, IDC_EDIT22, value[1]); if (P!=0) sprintf (value[0],"%5.2f",(val-P)*100/P); SetDlgItemText (hDialog, IDC_EDIT26, value[0]); break;	// P(W8000)
		 case 6:  SetDlgItemText (hDialog, IDC_EDIT23, value[1]); if (T!=0) sprintf (value[0],"%5.2f",(val-T)*100/T); SetDlgItemText (hDialog, IDC_EDIT27, value[0]); break;	// T(W8000)
		 case 7:  SetDlgItemText (hDialog, IDC_EDIT25, value[1]); if (Q!=0) sprintf (value[0],"%5.2f",(val-Q)*100/Q); SetDlgItemText (hDialog, IDC_EDIT29, value[0]); break;	// Q(W8000)
		 case 9:  SetDlgItemText (hDialog, IDC_EDIT24, value[1]); break;	// Qs(W8000)
		}
	}
}