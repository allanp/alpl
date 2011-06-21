#ifndef _ALPL_NET_IMPL_IOCP_HANDLER_IPP_
#define _ALPL_NET_IMPL_IOCP_HANDLER_IPP_

#include "../TcpConnection.h"

namespace alpl{
namespace net{

//// IocphanderImpl::
//
IocpHandler::IocpHandler(TcpConnectionMap& connections, LPCRITICAL_SECTION locker, int max_worker_threads)
        : isrunning_(false), exiting_(false), locker_(locker), num_worker_threads(max_worker_threads),
			connections_(connections), iocp_handle_(NULL) { 
	
	onDataReceived = NULL;
	onSentCompleted = NULL;
	onConnected = NULL;
	onDisconnected = NULL;
}

IocpHandler::IocpHandler(TcpConnectionPtr connection, LPCRITICAL_SECTION locker, int max_worker_threads)
		: isrunning_(false), exiting_(false), locker_(locker), num_worker_threads(max_worker_threads), iocp_handle_(NULL){
	
	onDataReceived = NULL;
	onSentCompleted = NULL;
	onConnected = NULL;
	onDisconnected = NULL;

	connections_.insert(std::make_pair(connection->socket_, connection));
}

IocpHandler::~IocpHandler(){
	
	if(worker_threads_!=NULL){delete[] worker_threads_; worker_threads_ = NULL; }
}

int IocpHandler::start(){

	// initialize iocp_handle_
	// 
	ALPL_ASSERT_INIT( init_handler(), "handler");

	// init. threads
	//
	ALPL_ASSERT_INIT( init_worker_threads(num_worker_threads), "work threads");

	isrunning_ = true;

    return COMM_SUCCESS;
}

int IocpHandler::init_handler(){

    iocp_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    if( iocp_handle_ == NULL ) {
        print_err("CreateIoCompletionPort() failed to create I/O completion port: %d\n", GetLastError());
        return COMM_INIT_FAILED;
    }

	exitHandle_ = CreateEvent(0, TRUE, FALSE, NULL);

	if( exitHandle_ == NULL){
        printf("CreateEvent failed (%d)\n", GetLastError());
        return COMM_INIT_FAILED;
    }

    return COMM_SUCCESS;
}


int IocpHandler::disconnect(socket_context_ptr a_socket_context, BOOL isGraceful){
        
    __try
    {
        EnterCriticalSection(locker_);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        print_err("EnterCriticalSection raised an exception.\n");
        return COMM_FAILED;
    }

	if(!a_socket_context){
		print_err("CloseClient: a_connected_acceptor is NULL\n");
		LeaveCriticalSection(locker_);
        return COMM_FAILED;
	}

	if( a_socket_context->socket == the_socket_context_->socket){
		print_err("Trying to close the listening socket.\n");
		int error = WSAGetLastError();
		DWORD threadId = GetCurrentThreadId();
		print_err("Thread %d: Last error: %d.\n", threadId, error);
	}
	else{
		if( isVerbose )
			print_err("CloseClient: Socket(%d) connection closing (graceful = %s)\n",
				a_socket_context->socket, (isGraceful ? "TRUE" : "FALSE"));

		if( !isGraceful ) {
			LINGER  lingerStruct;
			lingerStruct.l_onoff = 1;    // turn on linger
			lingerStruct.l_linger = 0;   // timeout value
			setsockopt(a_socket_context->socket, SOL_SOCKET, SO_LINGER,
				(char *)&lingerStruct, sizeof(lingerStruct) );
		}

		if( a_socket_context->io_context->SocketAccept != INVALID_SOCKET ) {
			closesocket(a_socket_context->io_context->SocketAccept);
			a_socket_context->io_context->SocketAccept = INVALID_SOCKET;
		}

		onDisconnected(connections_[a_socket_context->socket], NULL, 0);

		connections_.erase(a_socket_context->socket);

		closesocket(a_socket_context->socket);
		a_socket_context->socket = INVALID_SOCKET;

		socket_contexts_.remove(a_socket_context);
		FREE(a_socket_context);
	}
	
	LeaveCriticalSection(locker_);
    
    return COMM_SUCCESS;
}

int IocpHandler::exiting(){
	// for the threads terminate
	this->exiting_ = true;

	//// post an exiting command to all worker threads
	//
	if(iocp_handle_){
		for(int i = 0; i<num_worker_threads;i++){
			PostQueuedCompletionStatus(iocp_handle_, 0, 0, NULL);
		}
	}

	if(WAIT_OBJECT_0 != WaitForMultipleObjects(num_worker_threads, worker_threads_, TRUE, 1000)){
		DWORD error = GetLastError();
		print_err("Wait for thread exit failed: %d: %s\n", error, ErrorHandler::getErrorString(error));
	}
	else{
		for(int i = 0; i<num_worker_threads;i++){
			if( worker_threads_[i] != INVALID_HANDLE_VALUE)
				CloseHandle(worker_threads_[i]);
			worker_threads_[i] = INVALID_HANDLE_VALUE;
		}
	}

	if(this->the_socket_context_->socket != INVALID_SOCKET){
		closesocket(the_socket_context_->socket);
		the_socket_context_->socket = INVALID_SOCKET;
	}

	// WaitForMultipleObjects(num_worker_threads, worker_threads_, TRUE, 1000);

	if (! SetEvent(	exitHandle_) ) {
        printf("SetEvent failed (%d)\n", GetLastError());
        return COMM_EXIT_FAILED;
    }

	return COMM_SUCCESS;
}

socket_context_ptr IocpHandler::create_socket_context(SOCKET sd, last_io_operation last_op){

		socket_context_ptr a_socket_context = NULL;

        __try{
            EnterCriticalSection(this->locker_);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            print_err("EnterCriticalSection raised an exception.\n");
            return NULL;
        }

		a_socket_context = new socket_context();

        if( !a_socket_context ) {
            print_err("Create tcp_acceptor_ptr failed: %d\n", GetLastError());
			LeaveCriticalSection(locker_);
            return (NULL);
        }

        a_socket_context->io_context = new io_context();

        if( !a_socket_context->io_context ) {
            print_err("Create io_conetx failed: %d\n", GetLastError());
            FREE(a_socket_context);
			LeaveCriticalSection(locker_);
            return (NULL);
        }
        
        a_socket_context->socket = sd;

        a_socket_context->io_context->Overlapped.Internal = 0;
        a_socket_context->io_context->Overlapped.InternalHigh = 0;
        a_socket_context->io_context->Overlapped.Offset = 0;
        a_socket_context->io_context->Overlapped.OffsetHigh = 0;
        a_socket_context->io_context->Overlapped.hEvent = NULL;

        a_socket_context->io_context->LastOperation = last_op;

        a_socket_context->io_context->TotalBytes = 0;
        a_socket_context->io_context->SentBytes  = 0;
        a_socket_context->io_context->wsabuf.buf  = a_socket_context->io_context->Buffer;
        a_socket_context->io_context->wsabuf.len  = sizeof(a_socket_context->io_context->Buffer);
        a_socket_context->io_context->SocketAccept = INVALID_SOCKET;
                
        memset(a_socket_context->io_context->wsabuf.buf, 0, a_socket_context->io_context->wsabuf.len);

        LeaveCriticalSection(locker_);

        return (a_socket_context);
    }

socket_context_ptr IocpHandler::updateCompletionPort(SOCKET sd, last_io_operation last_op, BOOL add_to_list){

    //// create a new tcp_acceptor and initialize it 
	//
	socket_context_ptr a_socket_context = create_socket_context(sd, last_op);

    if( a_socket_context == NULL )
        return (NULL);

    iocp_handle_ = CreateIoCompletionPort((HANDLE)sd, iocp_handle_, (DWORD_PTR)a_socket_context, 0);

    if(iocp_handle_ == NULL){
        print_err("CreateIoCompletionPort() failed: %d\n", GetLastError());
        if( a_socket_context->io_context )
            FREE(a_socket_context->io_context);// xfree(an_acceptor->io_context);
        FREE(a_socket_context);// xfree(an_acceptor);                    
        return (NULL);
    }

    //The listening socket context (isClient is FALSE) is not added to the list.
    //All other socket contexts are added to the list.
    //
    if( add_to_list )
        socket_contexts_.push_front(a_socket_context);

    if( isVerbose )
        print_err("(T%d): UpdateCompletionPort: Socket(%d) added to IOCP\n", 
			GetCurrentThreadId(), a_socket_context->socket);

    return (a_socket_context);
}

int IocpHandler::init_worker_threads(DWORD max_io_number, bool force){
        
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);

        // Create two worker threads per CPU to service the overlapped I/O requests.
        //
        DWORD number_of_processors = systemInfo.dwNumberOfProcessors * 2;
		
		num_worker_threads = force ? max_io_number : MIN(number_of_processors, max_io_number);

		worker_threads_ = new HANDLE[num_worker_threads];

		DWORD count = (DWORD)num_worker_threads;
		
		using namespace alpl::threading;

        for( DWORD tid = 0; tid < count; tid++ ) {
            
            DWORD ThreadId;

			thread_t thread_handle = thread(createThreadFunction<IocpHandler>, 
											createThreadInfo<IocpHandler>(this, &IocpHandler::WorkerThread, iocp_handle_), 
											&ThreadId);

            if( thread_handle == NULL ) {
                print_err("CreateThread() failed to create worker thread: %d\n", GetLastError());
                return COMM_INIT_FAILED;
            }

            worker_threads_[tid] = thread_handle;
            thread_handle = INVALID_HANDLE_VALUE;
        }

        return COMM_SUCCESS;
    }

int IocpHandler::start_accept(){

	the_socket_context_->io_context->SocketAccept = SocketFactory::create_tcp_socket();

    if( the_socket_context_->io_context->SocketAccept == INVALID_SOCKET) {
        print_err("failed to create new accept socket\n");
        return (COMM_INIT_FAILED);
    }

    int ret = 0;
    DWORD RecvNumBytes = 0;
    DWORD bytes = 0;

	/*
	BOOL PASCAL FAR AcceptEx (
		__in SOCKET sListenSocket,
		__in SOCKET sAcceptSocket,
		__out_bcount_part(dwReceiveDataLength+dwLocalAddressLength+dwRemoteAddressLength,
			*lpdwBytesReceived) PVOID lpOutputBuffer,
		__in DWORD dwReceiveDataLength,
		__in DWORD dwLocalAddressLength,
		__in DWORD dwRemoteAddressLength,
		__out LPDWORD lpdwBytesReceived,
		__inout LPOVERLAPPED lpOverlapped
		);
	*/
    //// start accept async-ly, to be accepted in one of the worker threads
    //
    ret = ((LPFN_ACCEPTEX)the_socket_context_->async_fn)(
					the_socket_context_->socket,						// listening socket
					the_socket_context_->io_context->SocketAccept,		// accepted socket
                    (LPVOID)(the_socket_context_->io_context->Buffer),	// buffer
                    max_buff_size - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
                    sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
                    &RecvNumBytes,
					(LPOVERLAPPED)&(the_socket_context_->io_context->Overlapped));

	if(ret == SOCKET_ERROR) {
		
		DWORD err = WSAGetLastError();
		if (err != ERROR_IO_PENDING ) {
			print_err("AcceptEx() failed: %d\n", errno);
			return (COMM_INIT_FAILED);
		}
	}

    return COMM_SUCCESS;
}

int IocpHandler::start_connect(const char* ip, const char* port){

	// the_socket_context_->socket = SocketFactory::create_tcp_socket();

    if( the_socket_context_->socket == INVALID_SOCKET) {
        print_err("failed to create new accept socket\n");
        return (COMM_INIT_FAILED);
    }

    int ret = 0;
    DWORD SentNumBytes = 0;
    DWORD bytes = 0;

	/* 
	BOOL PASCAL ConnectEx(
	  __in      SOCKET s,
	  __in      const struct sockaddr *name,
	  __in      int namelen,
	  __in_opt  PVOID lpSendBuffer,
	  __in      DWORD dwSendDataLength,
	  __out     LPDWORD lpdwBytesSent,
	  __in      LPOVERLAPPED lpOverlapped
	);

	typedef void (*LPFN_CONNECTEX)();
	*/
	struct addrinfo hints = {0};
	struct addrinfo *addr = NULL;
	
	hints.ai_flags  = 0; hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM; hints.ai_protocol = IPPROTO_TCP;

	int rc = 0;

	if( getaddrinfo(ip, port, &hints, &addr) != 0 ) {
		int err = WSAGetLastError();
		print_err("getaddrinfo() failed with error %d: %s\n", err, ErrorHandler::getErrorString(err));
		return COMM_INIT_FAILED;
	}

    //// start connect async-ly, to be connected in one of the worker threads
    //
	if( ((LPFN_CONNECTEX)the_socket_context_->async_fn)(
					the_socket_context_->socket,	// connecting socket
					addr->ai_addr,					// sock_addr
					(int)addr->ai_addrlen,			// sock_addrlen
                    (PVOID)(the_socket_context_->io_context->Buffer),	// buffer
                    (DWORD)(max_buff_size - (2 * (sizeof(SOCKADDR_STORAGE) + 16))),
                    &SentNumBytes, 
					(LPOVERLAPPED)&(the_socket_context_->io_context->Overlapped))
		== SOCKET_ERROR) {
		DWORD error = WSAGetLastError();
		if (error != ERROR_IO_PENDING ) {
			print_err("ConnectEx() failed: %d: %s\n", error, ErrorHandler::getErrorString(error));
			return (COMM_INIT_FAILED);
		}
	}

	return COMM_SUCCESS;
}

int IocpHandler::handle_accept(socket_context_ptr an_socket_context, iocp_param& arg){
    
    if( setsockopt( an_socket_context->io_context->SocketAccept, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, 
			(char *)&the_socket_context_->socket, sizeof(the_socket_context_->socket)) == SOCKET_ERROR ) {

        print_err("setsockopt(SO_UPDATE_ACCEPT_CONTEXT) failed to update accept socket\n");
        // WSASetEvent(g_hCleanupEvent[0]);
        arg.error = WSAGetLastError();
        return COMM_ACCEPT_FAILED;
    }

	//// update the new client's socket, io_context, etc
    //
    socket_context_ptr new_socket_context = updateCompletionPort(
		an_socket_context->io_context->SocketAccept, IOConnect, TRUE);

    if( new_socket_context == NULL ) {
        print_err("failed to update connect socket to IOCP\n");
        arg.error = WSAGetLastError();
        return COMM_CONNECT_FAILED;
    }

	handle_new_connection(an_socket_context, arg);

	return COMM_SUCCESS;
}

int IocpHandler::handle_connect(socket_context_ptr an_socket_context, iocp_param& arg){

    if( setsockopt( an_socket_context->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, 
			(char *)&the_socket_context_->socket, sizeof(the_socket_context_->socket)) == SOCKET_ERROR ) {

        print_err("setsockopt(SO_UPDATE_CONNECT_CONTEXT) failed to update connect socket\n");
        // WSASetEvent(g_hCleanupEvent[0]);
        arg.error = WSAGetLastError();
        return COMM_CONNECT_FAILED;
    }

	//// update the new client's socket, io_context, etc
    //
    socket_context_ptr new_socket_context = updateCompletionPort(
		an_socket_context->socket, IOConnect, TRUE);

    if( new_socket_context == NULL ) {
        print_err("failed to update connect socket to IOCP\n");
        arg.error = WSAGetLastError();
        return COMM_CONNECT_FAILED;
    }

	handle_new_connection(an_socket_context, arg);
    
	return COMM_SUCCESS;
}

int IocpHandler::handle_new_connection(socket_context_ptr an_socket_context, iocp_param& arg){

	
    //// create a client connector, and add it to the connector list
    //
	TcpConnectionPtr a_conn = new TcpConnection(new_socket_context->socket, 
								(LPWSAOVERLAPPED)(&(new_socket_context->io_context->Overlapped)),
								iocp_handle_);

    a_conn->connected_ = true;

    using namespace std::tr1::placeholders;

	if(this->onDataReceived != NULL)
		a_conn->onDataReceived = std::tr1::bind(this->onDataReceived, _1, _2, _3);
	
	if(this->onSentCompleted != NULL)
		a_conn->onSentCompleted = std::tr1::bind(this->onSentCompleted, _1, _2, _3);
	
	if(this->onConnected != NULL)
		a_conn->onConnected = std::tr1::bind(this->onConnected, _1, _2, _3);
	
	if(this->onDisconnected != NULL)
		a_conn->onDisconnected = std::tr1::bind(this->onDisconnected, _1, _2, _3);
	
	a_conn->init_addr();

	connections_[a_conn->socket_] = a_conn;

    // connections_.insert(std::pair<SOCKET, TcpConnection*>(a_conn->socket_, a_conn));

	//// return back the acceptor to the acceptors_(stack)
	//
    //// process data
    //
	if( arg.IoSize ) { // have some data recevied by the accept completion
        //// invoke a data_received
        //
		if( a_conn->onDataReceived != NULL)
			a_conn->onDataReceived(a_conn, an_socket_context->io_context->Buffer, arg.IoSize);
		// data recevied on the accept completion
		// length of the data
	}
    
    //// start receive incoming data async-ly, by setting the Overlapped struct
    //
    {
        arg.RecvNumBytes = 0;
        arg.Flags = 0;
        arg.buffRecv.buf = new_socket_context->io_context->Buffer,
        arg.buffRecv.len = max_buff_size;
		new_socket_context->io_context->LastOperation = IOReceive;
		
        if( WSARecv(new_socket_context->socket, // start receive data use the new socket
                      &arg.buffRecv, 1,
                      &arg.RecvNumBytes,
                      &arg.Flags,
                      &new_socket_context->io_context->Overlapped, 
					  NULL)  == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()) ) {
            print_err ("WSARecv() failed: %d\n", WSAGetLastError());
			disconnect(new_socket_context, FALSE);
        }
    }

	return COMM_SUCCESS;
}

int IocpHandler::start_receive(socket_context_ptr an_socket_context, iocp_param& arg){

    arg.RecvNumBytes = 0;
    arg.Flags = 0;
    arg.buffRecv.buf = arg.an_io_context->Buffer;
    arg.buffRecv.len = max_buff_size;
	arg.an_io_context->LastOperation = IOReceive;

    int nRet = WSARecv(an_socket_context->socket,
                    &arg.buffRecv, 1, &arg.RecvNumBytes,
                    &arg.Flags, &arg.an_io_context->Overlapped, NULL);

    if( nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()) ) {
        print_err ("WorkerThread %d: Socket(%d) WSARecv() failed: %d\n", 
            GetCurrentThreadId(), an_socket_context->socket, WSAGetLastError());
        disconnect(an_socket_context, false);
	}
    return COMM_SUCCESS;
}

int IocpHandler::handle_receive(socket_context_ptr a_socket_context, iocp_param& arg){

    if( isVerbose ) {
        print_log("WorkerThread %d: Socket(%d) received completed (%d bytes)\n", 
                GetCurrentThreadId(), a_socket_context->socket, arg.IoSize);
    }

    //// process received data in the associated connector
    //
    if( connections_.count(a_socket_context->socket) > 0){
		TcpConnection* a_conn = connections_[a_socket_context->socket];
		if( a_conn->onDataReceived != NULL)
			a_conn->onDataReceived(a_conn, a_socket_context->io_context->Buffer, arg.IoSize);
    }
    else{
        print_err("Socket %d does not exist.\n", a_socket_context->socket);
        print_err("SocketAccept is: %d.\n", a_socket_context->io_context->SocketAccept);
    }
    //// start recevie again
    //
    return start_receive(a_socket_context, arg);
}

int IocpHandler::handle_send(socket_context_ptr a_socket_context, iocp_param& arg){
	//
    // a write operation has completed, determine if all the data intended to be
    // sent actually was sent.
    //
    arg.an_io_context->LastOperation = IOSend;
    arg.an_io_context->SentBytes += arg.IoSize;
    arg.Flags = 0;
	//
    // the previous write operation didn't send all the data,
    // post another send to complete the operation
    //
    if( arg.an_io_context->SentBytes < arg.an_io_context->TotalBytes ) {

        arg.buffSend.buf = arg.an_io_context->Buffer + arg.an_io_context->SentBytes;
        arg.buffSend.len = arg.an_io_context->TotalBytes - arg.an_io_context->SentBytes;

        int nRet = WSASend (a_socket_context->socket,
			&arg.buffSend, 1, &arg.SendNumBytes,
			arg.Flags, &(arg.an_io_context->Overlapped), NULL);

        if( nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError()) ) {
            print_err ("WSASend() failed: %d\n", WSAGetLastError());
            disconnect(a_socket_context, false);
        }
        else if( isVerbose ) {
            print_log("WorkerThread %d: Socket(%d) Send partially completed (%d bytes)\n", 
                    GetCurrentThreadId(), a_socket_context->socket, arg.IoSize);
        }
    }
    else{ // previous send operation completed for this socket, clear the send buffer
        arg.an_io_context->LastOperation = IOReceive;
		arg.an_io_context->SentBytes = 0;
		;
    }

    return COMM_SUCCESS;
}

DWORD IocpHandler::WorkerThread(LPVOID WorkThreadContext){

    HANDLE hIOCP = (HANDLE)WorkThreadContext;
    BOOL succeed = FALSE;
    
    int nRet = 0;

    LPWSAOVERLAPPED lpOverlapped = NULL;

	socket_context_ptr a_socket_context = NULL;

    iocp_param arg;

    arg.error = 0;
    arg.RecvNumBytes = 0; arg.SendNumBytes = 0;
    arg.Flags = 0; arg.IoSize = 0;
    arg.an_io_context = NULL;
    
    for(;;) {

        succeed = GetQueuedCompletionStatus(hIOCP, 
											&arg.IoSize, 
											(PDWORD_PTR)&a_socket_context, 
											(LPOVERLAPPED*)&lpOverlapped, 
											INFINITE );

        if( !succeed )
            print_err("GetQueuedCompletionStatus() failed: %d\n", GetLastError());

        //if( a_socket_context == NULL )
        //    return (0); // do clean up in the main thread

		if( exiting_)
			return 0;

        if( !isrunning_ )
            return(0); // do clean up in the main thread

        arg.an_io_context = (io_context_ptr)lpOverlapped;
        
        // We should never skip the loop and not post another AcceptEx if the current
        // completion packet is for previous AcceptEx
        //
        if( arg.an_io_context->LastOperation != IOAccept ) {
            if( !succeed || (succeed && (0 == arg.IoSize)) ) { // if succeed == 0 or dwIoSize == 0
                // disconnected by the client
                disconnect(a_socket_context, false);
                continue;
            }
        }

        //
        // determine what type of IO packet has completed by checking the io_context 
        // associated with this socket.  This will determine what action to take.
        //
        switch( arg.an_io_context->LastOperation ) {

        case IOAccept:

			ALPL_ASSERT(handle_accept(a_socket_context, arg), COMM_SUCCESS, 0, "Handle accept");

            // handle next incoming connection
            //
			ALPL_ASSERT(start_accept(), COMM_SUCCESS, 0, "Start accept");

            break;
		case IOConnect:

			ALPL_ASSERT(handle_connect(a_socket_context, arg), COMM_SUCCESS, 0, "Handle connect");
			ALPL_ASSERT(start_receive(a_socket_context, arg), COMM_SUCCESS, 0, "Start receive");

            break;

        case IOReceive:

            //
            // a read operation has completed, post a write operation to echo the
            // data back to the client using the same data buffer.
            //
            ALPL_ASSERT(handle_receive(a_socket_context, arg), COMM_SUCCESS, 0, "Handle receive");
            break;

        case IOSend:

			ALPL_ASSERT(handle_send(a_socket_context, arg), COMM_SUCCESS, 0, "Handle send");
            break;

        default:
            break;
        } //switch
    } //while

    return COMM_SUCCESS;
}

};
};

#endif // ifndef _ALPL_NET_IMPL_IOCP_HANDLER_IPP_