#ifndef _ALPL_NET_TCP_SERVER_H_
#define _ALPL_NET_TCP_SERVER_H_

#include "net.h"

namespace alpl{
namespace net{

	class TcpServer{
	private:
		char* port_;

		bool connected_;
		
		WSADATA wsaData;

		IocpHandler *iocph_;

		SOCKET ls_;

		CRITICAL_SECTION locker_;

		bool exiting_;

		int init();

		int create_listener(int backlog);
		int init_accept();

		int startListen(int backlog);

		int disconnect(TcpConnection* conn_p);

		TcpServer();

	public:
		TcpServer(char* port);
		~TcpServer();

		TcpConnectionMap TcpConnections;

		comm_handler_function onDataReceived, onSentCompleted, onConnected, onDisconnected;

		int Start();
		int Stop();

		int SetIocpHandler(int max_io_threads = max_worker_threads);
	};
};
};
#endif // _ALPL_NET_TCP_SERVER_H_


#ifdef _ALPL_NET_HEADER_ONLY_
# include "impl/TcpServer.hpp"
#endif // #ifdef _ALPL_NET_HEADER_ONLY_