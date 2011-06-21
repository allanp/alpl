#ifndef _ALPL_NET_IMPL_TCPCLIENT_H_
#define _ALPL_NET_IMPL_TCPCLIENT_H_

namespace alpl{
namespace net{

	TcpClient::TcpClient() : connection_(NULL, iocph_) {}

TcpClient::TcpClient(const char* ip, const char* port, int max_io_threads): 
		iocph_(NULL), connection_(NULL, iocph_), connected_(false), exiting_(false),
		ip_(ip), port_(port) {}

TcpClient::~TcpClient(){
	FREE(iocph_);
	// FREE(iocp_handle_);
}

int TcpClient::Start(){

	ALPL_ASSERT_INIT(init(), "");

	ALPL_ASSERT_INIT(init_connect(), "connect to server");
		
	ALPL_ASSERT_INIT(iocph_->start(), "iocph_" );

	ALPL_ASSERT_INIT(init_receive(), "receive");
	
	return COMM_SUCCESS;
}

int TcpClient::init(){

	ALPL_ASSERT(WSAStartup(MAKEWORD(2,2), &wsaData), 0, COMM_INIT_FAILED, "WSAStartup");

	__try {
		InitializeCriticalSection(&locker_);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		print_err("InitializeCriticalSection raised an exception.\n");
		return COMM_INIT_FAILED;
	}

	return COMM_SUCCESS;
}

int TcpClient::init_connect(){
	
	ALPL_ASSERT(
		SocketFactory::connect_to(connection_.socket_, ip_, port_),
		COMM_SUCCESS, COMM_CONNECT_FAILED, "Connect to server");

	connection_.connected_ = true;
	using namespace std::tr1::placeholders;
	if( this->onDataReceived != NULL)
		connection_.onDataReceived = std::tr1::bind(this->onDataReceived, _1, _2, _3);
	if( this->onConnected != NULL)
		connection_.onConnected = std::tr1::bind(this->onConnected, _1, _2, _3);
	if( this->onDisconnected != NULL)
		connection_.onDisconnected = std::tr1::bind(this->onDisconnected, _1, _2, _3);
	if( this->onSentCompleted != NULL)
		connection_.onSentCompleted = std::tr1::bind(this->onSentCompleted, _1, _2, _3);
	
	connection_.init_addr();

	ALPL_ASSERT(SetIocpHandler(), COMM_SUCCESS, COMM_CONNECT_FAILED, "Set Iocp handler");

	iocph_->connections_[connection_.socket_] = &connection_;

	return COMM_SUCCESS;
}

int TcpClient::init_receive(){

	iocph_->the_socket_context_ = iocph_->updateCompletionPort(connection_.socket_, IOReceive, TRUE);

	DWORD RecvNumBytes = 0;
    DWORD Flags = 0;
	WSABUF buffRecv;
    buffRecv.buf = iocph_->the_socket_context_->io_context->Buffer,
    buffRecv.len = max_buff_size;

	iocph_->the_socket_context_->io_context->LastOperation = IOReceive;
		
    if( WSARecv(iocph_->the_socket_context_->socket, // start receive data use the new socket
                    &buffRecv, 1,
                    &RecvNumBytes,
                    &Flags,
                    &iocph_->the_socket_context_->io_context->Overlapped, 
					NULL)  == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()) ) {
        print_err ("WSARecv() failed: %d\n", WSAGetLastError());
		iocph_->disconnect(iocph_->the_socket_context_, FALSE);
    }
	return COMM_SUCCESS;
}
//int TcpClient::init_connect(){
//
//	GUID connectex_guid = WSAID_CONNECTEX;
//
//    //The context for listening socket uses the SockAccept member to store the
//    //socket for client connection. 
//    //
//	iocph_->the_socket_context_ = iocph_->updateCompletionPort(connection_.socket_, IOConnect, FALSE);
//
//    if( iocph_->the_socket_context_ == NULL ) {
//        print_err("failed to update connect socket to IOCP\n");
//        return COMM_INIT_FAILED;
//    }
//
//	DWORD bytes = 0;
//
//	/*
//	__control_entrypoint(DllExport) WINSOCK_API_LINKAGE
//	int
//	WSAAPI
//	WSAIoctl(
//		__in SOCKET s,
//		__in DWORD dwIoControlCode,
//		__in_bcount_opt(cbInBuffer) LPVOID lpvInBuffer,
//		__in DWORD cbInBuffer,
//		__out_bcount_part_opt(cbOutBuffer, *lpcbBytesReturned) LPVOID lpvOutBuffer,
//		__in DWORD cbOutBuffer,
//		__out LPDWORD lpcbBytesReturned,
//		__inout_opt LPWSAOVERLAPPED lpOverlapped,
//		__in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
//		);
//	*/
//    // Load the ConnectEx extension function from the provider for this socket
//	//
//    if( WSAIoctl( connection_.socket_, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectex_guid, sizeof(connectex_guid),
//				&iocph_->the_socket_context_->async_fn, sizeof((LPFN_CONNECTEX)iocph_->the_socket_context_->async_fn), 
//				&bytes, NULL, NULL ) == SOCKET_ERROR) {
//
//		//WSAIoctl( connection_.socket_, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectex_guid, sizeof(connectex_guid),
//		//		  &iocph_->the_socket_context_->async_fn, sizeof((LPFN_CONNECTEX)iocph_->the_socket_context_->async_fn), 
//		//		  &bytes, NULL, NULL ) == SOCKET_ERROR) {
//		DWORD err = WSAGetLastError();
//		print_err("failed to load ConnectEx: %d: %s\n", err, ErrorHandler::getErrorString(err));
//        return COMM_INIT_FAILED;
//    }
//
//	if( iocph_->start_connect(ip_, port_) != COMM_SUCCESS){
//		print_err("Start accept failed\n");
//		return COMM_INIT_FAILED;
//	}
//
//    return COMM_SUCCESS;
//}

int TcpClient::SetIocpHandler(int max_io_threads){

	iocph_ = new IocpHandler(&connection_, &locker_, max_io_threads);

	if(iocph_ == NULL){
		print_err("Init iocp handle failed.\n");
		return COMM_INIT_FAILED;
	}

	using namespace std::tr1::placeholders;
	
	if(this->onDataReceived != NULL)
		iocph_->onDataReceived = std::tr1::bind(this->onDataReceived, _1, _2, _3);
	
	if(this->onSentCompleted != NULL)
		iocph_->onSentCompleted = std::tr1::bind(this->onSentCompleted, _1, _2, _3);
	
	if(this->onConnected != NULL)
		iocph_->onConnected = std::tr1::bind(this->onConnected, _1, _2, _3);
	
	if(this->onDisconnected != NULL)
		iocph_->onDisconnected = std::tr1::bind(this->onDisconnected, _1, _2, _3);
	

	return COMM_SUCCESS;
}

int TcpClient::disconnect(){
	return COMM_SUCCESS;
}

int TcpClient::Stop(){
	return COMM_SUCCESS;
}

};
};
#endif // _ALPL_NET_TCPCLIENT_