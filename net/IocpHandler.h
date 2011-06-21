#ifndef _ALPL_NET_IOCP_HANDLER_H_
#define _ALPL_NET_IOCP_HANDLER_H_

// #include "net.h"

#include "../threading/threading.h"

namespace alpl{
namespace net{

class IocpHandler {
	private:
		bool exiting_;
	    
		LPCRITICAL_SECTION locker_;
		// WSAEVENT g_hCleanupEvent;

		// connections related

		socket_context_ptr the_socket_context_;
		SocketContextPtrList socket_contexts_;

		TcpConnectionMap connections_;
	    
		// worker threads related
		HANDLE iocp_handle_;
		HANDLE exitHandle_;

		DWORD WorkerThread(LPVOID WorkThreadContext);

		HANDLE *worker_threads_;
		int num_worker_threads;

		bool isrunning_;

		comm_handler_function onDataReceived, onSentCompleted, onConnected, onDisconnected;

		friend class TcpConnection;	
		
		friend class TcpServer;
		friend class TcpClient;

		socket_context_ptr create_socket_context(SOCKET sd, last_io_operation last_op);
		socket_context_ptr updateCompletionPort(SOCKET sd, last_io_operation last_op, BOOL add_to_list);

		int init_handler();
		int init_worker_threads(DWORD max_io_number = max_worker_threads, bool force = false);

		int start_accept();
		int start_connect(const char* ip, const char* port);

		typedef int (workerthread_callback)(socket_context_ptr a_socket_context, iocp_param& arg);

		workerthread_callback handle_accept, handle_connect, handle_receive, handle_send, start_receive;

		workerthread_callback handle_new_connection;

		int exiting();
		//int handle_accept(socket_context_ptr a_socket_context, iocp_param& arg);
		//int handle_connect(socket_context_ptr a_socket_context, iocp_param& arg);
		//int handle_new_connection(socket_context_ptr a_socket_context, iocp_param& arg);
		//int handle_receive(socket_context_ptr a_socket_context, iocp_param& arg);
		//int handle_send(socket_context_ptr a_socket_context, iocp_param& arg);
		//int start_receive(socket_context_ptr a_socket_context, iocp_param& arg);

	public:
		IocpHandler(TcpConnectionMap& connections, LPCRITICAL_SECTION locker, int max_worker_threads);
		IocpHandler(TcpConnectionPtr connection, LPCRITICAL_SECTION locker, int max_worker_threads);

		~IocpHandler();

		int start();
		int disconnect(socket_context_ptr a_socket_context, BOOL isGraceful);
	};
};
};

#endif // 

#ifdef _ALPL_NET_HEADER_ONLY_
# include "impl/IocpHandler.hpp"
#endif // #ifdef _ALPL_NET_HEADER_ONLY_