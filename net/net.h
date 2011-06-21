#define _ALPL_HEADER_ONLY_

#ifndef _ALPL_NET_NET_H_
#define _ALPL_NET_NET_H_

#define COMM_SUCCESS 0
#define COMM_FAILED -1
#define COMM_INIT_FAILED -2
#define COMM_ACCEPT_FAILED -3
#define COMM_CONNECT_FAILED -4
#define COMM_EXIT_FAILED -5


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <mswsock.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <list>
#include <map>
#include <algorithm>
#include <functional>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")


#include "../util/errno.h"

#ifdef _ALPL_HEADER_ONLY_
#define _ALPL_NET_HEADER_ONLY_
#endif

namespace alpl{
namespace net{

    const int prefix_length = 4;
    const int max_buff_size = 4*1024;
    const int max_worker_threads = 64;
	const int default_backlog = 10;

	const bool isVerbose = false;

	typedef enum mode{ 
		LISTENNING, CONNECTING 
	} mode;

	typedef enum last_io_operation{ 
		IOAccept = 0x1, IOConnect = 0x2, IOReceive = 0x3, IOSend = 0x4
	} last_io_operation, *last_io_operation_ptr;

	typedef struct io_context{
        WSAOVERLAPPED Overlapped;
        char Buffer[max_buff_size];
        WSABUF wsabuf;
        int TotalBytes;
        int SentBytes;
        last_io_operation LastOperation;
        SOCKET SocketAccept;    /* the new socket created */
    } io_context, *io_context_ptr;

	typedef struct socket_context{
        SOCKET socket;		/* the socket working on */
        LPVOID async_fn;	/* async function, AcceptEx, ConnectEx */
        io_context_ptr io_context;
    } socket_context, *socket_context_ptr;
    
	typedef struct iocp_param{
        WSABUF buffRecv;
        WSABUF buffSend;
        DWORD RecvNumBytes;
        DWORD SendNumBytes;
        DWORD Flags;
        DWORD IoSize;
        HRESULT RetValue;
        int error;
        io_context_ptr an_io_context;
    } iocp_param, *iocp_param_ptr;

	class IocpHandler;

	class TcpConnection;

	class TcpServer;
	class TcpClient;

	typedef IocpHandler* IocpHandlerPtr;

	typedef TcpConnection* TcpConnectionPtr;

	typedef std::list<socket_context_ptr> SocketContextPtrList;
	typedef std::map<SOCKET, TcpConnectionPtr> TcpConnectionMap;

    typedef std::tr1::function<void(TcpConnectionPtr tcpConnectionPtr, void* data, size_t length)> comm_handler_function;

}; // namespace net
}; // namespace alpl

#endif // ifndef _ALPL_NET_

#include "SocketFactory.h"

//#include "IocpHandler.h"
//#include "TcpConnection.h"
//#include "TcpAcceptor.h"
//#include "TcpServer.h"
//#include "TcpClient.h"
