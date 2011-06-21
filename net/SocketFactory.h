#ifndef _ALPL_NET_SOCKET_FACTORY_H_
#define _ALPL_NET_SOCKET_FACTORY_H_

namespace alpl{
namespace net{
class SocketFactory{
public:
	    
    typedef enum address_family{
        IPv4 = AF_INET, IPv6 = AF_INET6
    } address_family;
    typedef enum socket_type{
        Stream = SOCK_STREAM, Packet = SOCK_DGRAM, RawSocket = SOCK_RAW
    } socket_type;
    typedef enum protocol_type{
        TCP = IPPROTO_TCP, UDP = IPPROTO_UDP, Raw = IPPROTO_RAW
    }protocol_type;
	typedef struct socket_arg{
        address_family addr_family;
        socket_type sock_type;
        protocol_type protocol;
        BOOL enable_overlapped;
        BOOL disable_send_buffer;
        BOOL flag;  // reserved, always set to TRUE
		socket_arg(): addr_family(IPv4), sock_type(Stream), protocol(TCP), 
            enable_overlapped(TRUE), disable_send_buffer(TRUE), flag(TRUE){}
    }socket_arg, *socket_arg_ptr;

    static socket_arg_ptr create_tcp_socket_arg(){
        return new socket_arg();
    }
    static SOCKET create_socket(socket_arg &sa){
        SOCKET s = WSASocket(sa.addr_family, sa.sock_type, sa.protocol, 
                            NULL, 0, sa.enable_overlapped);
        if(s == INVALID_SOCKET){
            print_err("WSASocket() failed: %d\n", WSAGetLastError());
            return s;
        }
        if(sa.disable_send_buffer){
            // disable send_buffer, send immediately
            int nZero = 0;
            int nRet = setsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
            if( nRet == SOCKET_ERROR) {
                print_err("setsockopt(SNDBUF) failed: %d\n", WSAGetLastError());
                return (s);
            }
        }
        return s;
    }

	static int connect_to(SOCKET& socket, const char* ip, const char* port){

		if(socket == INVALID_SOCKET)
			socket = create_tcp_socket();

		struct addrinfo hints = {0};
		struct addrinfo *addr = NULL;
	
		hints.ai_flags  = 0;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		BOOL succeed = TRUE;
		int rc = 0;

		if( getaddrinfo(ip, port, &hints, &addr) != 0 ) {
			int err = WSAGetLastError();
			print_err("getaddrinfo() failed with error %d: %s\n", err, ErrorHandler::getErrorString(err));
			succeed = FALSE;
		}

		if( succeed != FALSE ) {

			rc = connect(socket, addr->ai_addr, (int) addr->ai_addrlen);

			if( rc == SOCKET_ERROR ) {
				int err = WSAGetLastError();
				print_err("connect(thread %d) failed: %d: %s\n", GetCurrentThreadId(), err, ErrorHandler::getErrorString(err));
				succeed = FALSE;
			} 
			else if(isVerbose)
				print_err("connected(thread %d), Socket (%d)\n", GetCurrentThreadId(), socket);

			freeaddrinfo(addr);
		}

		return succeed ? COMM_SUCCESS : COMM_INIT_FAILED;
	}

    static SOCKET create_tcp_socket(bool enable_overlapped = true) {
        // create a new tcp socket(overlapped enabled)
        //
        SOCKET socket = INVALID_SOCKET;
		int zero = 0;
		int nRet = 0;
        if(enable_overlapped)
            socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED); 
        else
            socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, 0);
        if( socket == INVALID_SOCKET ) {
            print_err("WSASocket(socket) failed: %d\n", WSAGetLastError());
            return socket;
        }
        // disable send_buffer, send immediately
        //
        nRet = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero));
        if( nRet == SOCKET_ERROR) {
            print_err("setsockopt(SNDBUF) failed: %d\n", WSAGetLastError());
            return socket;
        }
        return socket;
    }
	static const char* toRemoteAddress(SOCKET& socket){
		struct sockaddr_in remote_sockaddr;
		int addrlen = sizeof(struct sockaddr_storage);
		getpeername(socket, (struct sockaddr*)&remote_sockaddr, &addrlen);
		DWORD len = addrlen;
		char remote_addr[22] = "INVALID";
		/*
		WSAAddressToStringW(
		__in_bcount(dwAddressLength) LPSOCKADDR lpsaAddress,
		__in     DWORD               dwAddressLength,
		__in_opt LPWSAPROTOCOL_INFOW lpProtocolInfo,
		__out_ecount_part(*lpdwAddressStringLength,*lpdwAddressStringLength) LPWSTR lpszAddressString,
		__inout  LPDWORD             lpdwAddressStringLength
		);
		*/
		if( WSAAddressToStringA((LPSOCKADDR)&remote_sockaddr, addrlen, NULL, remote_addr, &len) != 0){
			printf("WSAAddressToString() failed with error code %ld\n", WSAGetLastError());
		}
		return strdup(remote_addr);
	}
	static const char* toLocalAddress(SOCKET& socket){
		struct sockaddr_in local_sockaddr;
		int addrlen = sizeof(struct sockaddr_storage);
		getsockname(socket, (struct sockaddr*)&local_sockaddr, &addrlen);
		DWORD len = addrlen;
		char local_addr[22] = "INVALID";
		if( WSAAddressToStringA((LPSOCKADDR)&local_sockaddr, addrlen, NULL, local_addr, &len) != 0){
			printf("WSAAddressToString() failed with error code %ld\n", WSAGetLastError());
		}
		
		return strdup(local_addr);
	}
	//static const char* toHostPort(SOCKET& socket){
	//	struct sockaddr_in local_sockaddr;
	//	int addrlen = sizeof(struct sockaddr_storage);
	//	getsockname(socket, (struct sockaddr*)&local_sockaddr, &addrlen);
	//	DWORD len = addrlen;
	//	char host[INET_ADDRSTRLEN] = "INVALID";
	//	
	//	uint16_t port = networkToHost16(local_sockaddr.sin_port;);
	//	snprintf(buf, size, "%s:%u", host, port);
	//	char local_addr[22] = "INVALID"; memset(local_addr, 0, 22);
	//	
	//	if( WSAAddressToString((LPSOCKADDR)&local_sockaddr, addrlen, NULL, local_addr, &len) != 0){
	//		printf("WSAAddressToString() failed with error code %ld\n", WSAGetLastError());
	//	}
	//	return strdup(local_addr);
	//}
};
}; // net namespace
}; // alpl namespace
#endif // ifndef _ALPL_NET_FACTORY_
