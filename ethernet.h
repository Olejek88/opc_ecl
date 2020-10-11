//---------------------------------------------------------------------------------
struct sockaddr_in srv_address;
SOCKET server_socket=SOCKET_ERROR;			// Сокет для обмена
SOCKET http_cln_socket=SOCKET_ERROR;		// Сокет клиента для http
PHOSTENT phe;
WSADATA WSAData;
CHAR IP[16];
UINT Socket=80;
//--------------------------------------------------------------
//	Creates server sock and binds to ip address and port
//--------------------------------------------------------------
SOCKET StartWebServer(UINT dv)
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
 si.sin_port = htons(cfg[dv].sock);		// port
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
BOOL ConnectToServer (UINT dv)
{
 //phe = gethostbyname(cfg[dv].adr);
 //UL_INFO((LOGID, "[https] ConnectToServer(%s), [phe=%d]",cfg[dv].adr,phe));
 srv_address.sin_addr.s_addr = inet_addr(cfg[dv].adr);
 srv_address.sin_family = AF_INET;
 srv_address.sin_port = htons(cfg[dv].sock);		// port
 UL_INFO((LOGID, "[https] IP address is:=%s",inet_ntoa(srv_address.sin_addr)));
 UL_INFO((LOGID, "[https] Establish a connection to the server socket [%d | %d | %ul]",srv_address.sin_family,srv_address.sin_port,srv_address.sin_addr.s_addr));
 if(connect(cfg[dv].server_socket, (PSOCKADDR)&srv_address, sizeof(srv_address)) < 0)
	{
	 closesocket(cfg[dv].server_socket);
	 UL_INFO((LOGID, "[https] Establish a connection to %s ... connect Error | reason is %d",cfg[dv].adr,WSAGetLastError ()));
	 return FALSE;
	}
 else 
	 UL_INFO((LOGID, "Establish a connection to %s ... connect success [%d]",cfg[dv].adr,cfg[dv].server_socket));
 return TRUE;
}
