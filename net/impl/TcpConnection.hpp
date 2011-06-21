#ifndef _ALPL_NET_IMPL_CONNECTION_IPP_
#define _ALPL_NET_IMPL_CONNECTION_IPP_

namespace alpl{
namespace net{

//// invoked in TcpServer
//
TcpConnection::TcpConnection(SOCKET &s, LPWSAOVERLAPPED overlapped, HANDLE iocp_handle): 
		socket_(s), overlapped_(overlapped), iocp_handle_(iocp_handle), connected_(false), local_addr(NULL), remote_addr(NULL) {}

//// invoked in TcpClient
//
TcpConnection::TcpConnection(LPWSAOVERLAPPED overlapped, HANDLE iocp_handle): 
		socket_(INVALID_SOCKET), overlapped_(overlapped), iocp_handle_(iocp_handle), connected_(false), local_addr(NULL), remote_addr(NULL) {}

TcpConnection::~TcpConnection(){}

void TcpConnection::init_addr(){

	if( !this->connected_)
		return;

	if( local_addr == NULL ){
		struct sockaddr_in local_sockaddr;
		int addrlen = sizeof(struct sockaddr_storage);

		getsockname(socket_, (struct sockaddr*)&local_sockaddr, &addrlen);

		DWORD len = addrlen;
		//local_addr = inet_ntoa(local_sockaddr.sin_addr);
		local_addr = new char[22];
		if( WSAAddressToStringA((LPSOCKADDR)&local_sockaddr, addrlen, NULL, local_addr, &len) != 0){
			printf("WSAAddressToString() failed with error code %ld\n", WSAGetLastError());
		}
	}

	if( remote_addr == NULL ){
		struct sockaddr_in remote_sockaddr;
		int addrlen = sizeof(struct sockaddr_storage);

		getpeername(socket_, (struct sockaddr*)&remote_sockaddr, &addrlen);

		DWORD len = addrlen;
		//remote_addr =  inet_ntoa(remote_sockaddr.sin_addr);
		remote_addr = new char[22];
		if( WSAAddressToStringA((LPSOCKADDR)&remote_sockaddr, addrlen, NULL, remote_addr, &len) != 0){
			printf("WSAAddressToString() failed with error code %ld\n", WSAGetLastError());
		}
	}
}

void TcpConnection::on_data_received(TcpConnectionPtr sender, void* data, size_t length){

	if(onDataReceived != NULL){
		onDataReceived(sender, data, length);
	}
	else{
		print_err("No onDataReceived handler!!!\n");
	}
}

void TcpConnection::on_sentcompleted(TcpConnectionPtr dest, void* data, size_t length){

	if(onSentCompleted != NULL){
		onSentCompleted(dest, data, length);
	}
	else if (isVerbose){
		print_err("No onSentCompleted handler!!!\n");
	}
}

void TcpConnection::on_connected(TcpConnectionPtr sender, void* data, size_t length){

	if(onConnected != NULL){
		onConnected(sender, data, length);
	}
	else if (isVerbose){
		print_err("No onConnected handler!!!\n");
	}
}

void TcpConnection::on_disconnected(TcpConnectionPtr sender, void* data, size_t length){

	if(onDisconnected != NULL){
		onDisconnected(sender, data, length);
	}
	else if (isVerbose){
		print_err("No onDisconnected handler!!!\n");
	}
}

int TcpConnection::Send(void* data, size_t length){
	
	if( !this->connected_){
		printf("Not connected.\n");
		return COMM_FAILED;
	}

	send(socket_, (const char*)data, length, 0);

	on_sentcompleted(this, NULL, 0);

	return COMM_SUCCESS;
}
}; // net namespace
}; // alpl namespace

#endif // _ALPL_NET_IMPL_CONNECTION_IPP_