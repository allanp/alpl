#ifndef _ALPL_NET_IMPL_TCPSERVER_IPP_
#define _ALPL_NET_IMPL_TCPSERVER_IPP_

#include "../IocpHandler.h"

namespace alpl{
namespace net{

//// TcpServerImpl
//
TcpServer::TcpServer(): exiting_(false) {}

TcpServer::TcpServer(char* port) : iocph_(NULL), port_(port), exiting_(false) {
	
	int rc = WSAStartup(MAKEWORD(2,2), &wsaData);

	if (rc != 0){
		print_err("WSAStartup failed with error: %d\n", rc);
	}
}

TcpServer::~TcpServer(){
	WSACleanup();
}

int TcpServer::Start() {

	//// init critical section and iocp_handler
	//
	ALPL_ASSERT_INIT(init(), "");

	//// init iocph_->iocp_handle_ and iocph_->workthreads_
	//
	ALPL_ASSERT_INIT( iocph_->start(), "iocph_");

	ALPL_ASSERT_INIT( create_listener(default_backlog), "listener");

	ALPL_ASSERT_INIT(init_accept(), "accept");

	connected_ = true;

	return COMM_SUCCESS;
}

int TcpServer::Stop(){
	
	this->exiting_ = true;
	
	this->iocph_->exiting();
	WaitForSingleObject(iocph_->exitHandle_, -1);

	return COMM_SUCCESS;
}

int TcpServer::init(){

	__try {
		InitializeCriticalSection(&locker_);
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		print_err("InitializeCriticalSection raised an exception.\n");
		return COMM_INIT_FAILED;
	}

	ALPL_ASSERT_INIT(SetIocpHandler(), "iocp handl");

	return COMM_SUCCESS;
}

int TcpServer::SetIocpHandler(int max_io_threads){

	if(iocph_ != NULL){
		FREE(iocph_);
	}

	iocph_ = new IocpHandler(TcpConnections, &locker_, max_io_threads);

	if(iocph_ == NULL){
		print_err("Init iocp handle failed.\n");
		return COMM_INIT_FAILED;
	}

	using namespace std::tr1::placeholders;
	if(this->onDataReceived != NULL)
		iocph_->onDataReceived = std::tr1::bind(this->onDataReceived, _1, _2, _3);
	if(this->onConnected != NULL)
		iocph_->onConnected = std::tr1::bind(this->onConnected, _1, _2, _3);
	if(this->onDisconnected != NULL)
		iocph_->onDisconnected = std::tr1::bind(this->onDisconnected, _1, _2, _3);
	if(this->onSentCompleted != NULL)
		iocph_->onSentCompleted = std::tr1::bind(this->onSentCompleted, _1, _2, _3);

	return COMM_SUCCESS;
}


int TcpServer::create_listener(int backlog){
    int nRet = 0;
    LINGER lingerStruct;
    struct addrinfo hints = {0};
    struct addrinfo *addrlocal = NULL;

    lingerStruct.l_onoff = 1;
    lingerStruct.l_linger = 0;    // Don't linger, abortive disconnect

    hints.ai_flags  = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_IP;

	ALPL_ASSERT(getaddrinfo(NULL, port_, &hints, &addrlocal), 0, COMM_INIT_FAILED, "getaddrinfo()");

    if( addrlocal == NULL ) {
        print_err("getaddrinfo() failed to resolve/convert the interface\n");
        return (COMM_INIT_FAILED);
    }

	ls_ = SocketFactory::create_tcp_socket();
    
    if( ls_ == INVALID_SOCKET) {
        freeaddrinfo(addrlocal);
        return (COMM_INIT_FAILED);
    }

    nRet = bind(ls_, addrlocal->ai_addr, (int) addrlocal->ai_addrlen);

    if( nRet == SOCKET_ERROR) {
        print_err("bind() failed: %d\n", WSAGetLastError());
        freeaddrinfo(addrlocal);
        return (COMM_INIT_FAILED);
    }
    
    // The maximum length of the pending connections queue. 
    nRet = listen(ls_, backlog);

    if( nRet == SOCKET_ERROR ) {
        print_err("listen() failed: %d\n", WSAGetLastError());
        freeaddrinfo(addrlocal);
        return (COMM_INIT_FAILED);
    }
    
    freeaddrinfo(addrlocal);

    return COMM_SUCCESS;
}


int TcpServer::init_accept(){

    GUID acceptex_guid = WSAID_ACCEPTEX;

    //The context for listening socket uses the SockAccept member to store the
    //socket for client connection. 
    //
	iocph_->the_socket_context_ = iocph_->updateCompletionPort(ls_, IOAccept, FALSE);

    if( iocph_->the_socket_context_ == NULL ) {
        print_err("failed to update listen socket to IOCP\n");
        return COMM_INIT_FAILED;
    }

	DWORD bytes = 0;
    // Load the AcceptEx extension function from the provider for this socket
	//
    if( WSAIoctl( ls_, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid, sizeof(acceptex_guid),
				&iocph_->the_socket_context_->async_fn, sizeof((LPFN_ACCEPTEX)iocph_->the_socket_context_->async_fn), 
				&bytes, NULL, NULL ) == SOCKET_ERROR) {
        print_err("failed to load AcceptEx: %d\n", WSAGetLastError());
        return COMM_INIT_FAILED;
    }

	ALPL_ASSERT_INIT(iocph_->start_accept(), "Start accept");

    return COMM_SUCCESS;
}

};
};
#endif // _ALPL_NET_IMPL_TCPSERVER_IPP_