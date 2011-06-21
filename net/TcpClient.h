#ifndef _ALPL_NET_TCP_CLIENT_H_
#define _ALPL_NET_TCP_CLIENT_H_

namespace alpl{
	namespace net{

	class TcpClient{
	private:
		const char* ip_;
		const char* port_;

		bool connected_;

		WSADATA wsaData;
		HANDLE iocp_handle_;
		IocpHandler *iocph_;
		CRITICAL_SECTION locker_;

		bool exiting_;

		TcpConnection connection_;

		int init();

		int init_connect();
		int init_receive();

		int disconnect();

		void on_connected(	  TcpConnectionPtr sender, void* data, size_t length);
		void on_disconnected( TcpConnectionPtr sender, void* data, size_t length);

		void on_data_received(TcpConnectionPtr sender, void* data, size_t length);
		void on_sentcompleted(TcpConnectionPtr dest, void* data, size_t length);
		
		TcpClient();
		
	public:
		TcpClient(const char* ip, const char* port, int max_io_threads = 1);

		~TcpClient();
		
		(TcpConnection) Connection() const { 
			return (TcpConnection)connection_; 
		}

		int SetIocpHandler(int max_io_threads = 1);

		comm_handler_function onDataReceived, onSentCompleted, onConnected, onDisconnected;

		int Start();
		
		int Stop();
	};
};
};

#endif // _ALPL_NET_TCP_CLIENT_H_

#ifdef _ALPL_NET_HEADER_ONLY_
# include "impl/TcpClient.hpp"
#endif // #ifdef _ALPL_NET_HEADER_ONLY_