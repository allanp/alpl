#ifndef _ALPL_NET_TCP_CONNECTION_H_
#define _ALPL_NET_TCP_CONNECTION_H_

namespace alpl{
namespace net{

class TcpConnection{
	private:
		SOCKET socket_;
		LPWSAOVERLAPPED overlapped_;
		HANDLE iocp_handle_;

		bool connected_;

		char *local_addr;
		char *remote_addr;

		comm_handler_function onDataReceived, onSentCompleted, onConnected, onDisconnected;

		TcpConnection(SOCKET &s, LPWSAOVERLAPPED overlapped, HANDLE iocp_handle);
		TcpConnection(LPWSAOVERLAPPED overlapped, HANDLE iocp_handle);

		void init_addr();

		void on_connected(TcpConnectionPtr sender, void* data, size_t length);
		void on_disconnected(TcpConnectionPtr sender, void* data, size_t length);

		void on_data_received(TcpConnectionPtr sender, void* data, size_t length);
		void on_sentcompleted(TcpConnectionPtr dest, void* data, size_t length);

		friend class IocpHandler;
		
		friend class TcpAcceptor;
		friend class TcpConnector;
		
		friend class TcpServer;
		friend class TcpClient;
		
	public:
		~TcpConnection();

		const SOCKET socket() const {
			return (const SOCKET)socket_;
		}

		const char* local_endpoint() const {
			return (const char*)local_addr;
		}

		const char* remote_endpoint() const {
			return (const char*)remote_addr;
		}

		int Send(void* data, size_t length);
	};
};
};

#endif // _ALPL_NET_TCP_CONNECTION_H_


#ifdef _ALPL_NET_HEADER_ONLY_
# include "impl/TcpConnection.hpp"
#endif // #ifdef _ALPL_NET_HEADER_ONLY_

