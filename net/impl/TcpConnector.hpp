
#ifndef _ALPL_NET_IMPL_CONNECTOR_IPP_
#define _ALPL_NET_IMPL_CONNECTOR_IPP_

//namespace alpl{
//namespace net{
//
//int TcpConnector::connect_to_server(SOCKET& socket, const char* ip, const char* port){
//
//	if(socket == INVALID_SOCKET)
//		socket = SocketFactory::create_tcp_socket();
//
//	struct addrinfo hints = {0};
//	struct addrinfo *addr = NULL;
//	
//	hints.ai_flags  = 0;
//	hints.ai_family = AF_INET;
//	hints.ai_socktype = SOCK_STREAM;
//	hints.ai_protocol = IPPROTO_TCP;
//
//	BOOL succeed = TRUE;
//	int rc = 0;
//
//	if( getaddrinfo(ip, port, &hints, &addr) != 0 ) {
//		int err = WSAGetLastError();
//		print_err("getaddrinfo() failed with error %d: %s\n", err, ErrorHandler::getErrorString(err));
//		succeed = FALSE;
//	}
//
//	if( succeed != FALSE ) {
//
//		rc = connect(socket, addr->ai_addr, (int) addr->ai_addrlen);
//
//		if( rc == SOCKET_ERROR ) {
//			int err = WSAGetLastError();
//			print_err("connect(thread %d) failed: %d: %s\n", GetCurrentThreadId(), err, ErrorHandler::getErrorString(err));
//			succeed = FALSE;
//		} 
//		else if(isVerbose)
//			print_err("connected(thread %d), Socket (%d)\n", GetCurrentThreadId(), socket);
//
//		freeaddrinfo(addr);
//	}
//
//	return succeed ? COMM_SUCCESS : COMM_INIT_FAILED;
//}
//
//};
//
//};

#endif // _ALPL_NET_IMPL_CONNECTOR_IPP_