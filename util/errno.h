#ifndef _ALPL_ERRNO_H_
#define _ALPL_ERRNO_H_

#define _CRT_SECURE_NO_WARNINGS

namespace alpl{

#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif

#define FREE(p)			do{ if(p!=NULL){ delete p; p = NULL; } } while(0)
#define FREE_ARRAY(arr) do{ if(arr!=NULL){delete[] arr; arr = NULL; } } while(0)

#include <strsafe.h>
#ifndef StringCbCopyN
#define StringCbCopyN(pszDest,cbDest,pszSrc,cbSrc) \
	std::strncpy(pszDest, pszSrc, cbSrc)
#endif

#define print_err	printf
#define print_log	printf

#ifndef print_err
#define print_err(err) fprintf(stderr, err)
#endif
#ifndef print_log
#define print_log(log) fprintf(stdout, log)
#endif

#define ALPL_ASSERT(function, condition, error, name) \
	do{if( function != condition ){ \
		int err = WSAGetLastError(); \
		print_err("%s failed. error: %d: %s\n", name, err, ErrorHandler::getErrorString(err));\
		return error; \
	}}while(0)

#define ALPL_ASSERT_INIT(function, name) \
	do{ if( function != COMM_SUCCESS ){ \
		int err = WSAGetLastError(); \
		print_err("Init %s failed. error: %d: %s\n", name, err, ErrorHandler::getErrorString(err));\
		return COMM_INIT_FAILED; \
	}}while(0)

class ErrorHandler{
	public:
		static const char* getErrorString(DWORD error_code){
			switch (error_code){
				case 997:   return _strdup("Overlapped operations will complete later");
				case 10014: return _strdup("Bad Address");
				case 10024: return _strdup("Too many open files");
				case 10038: return _strdup("The Descriptor is not a socket");
				case 10039: return _strdup("Destination address required");
				case 10048: return _strdup("Address already in use");
				case 10050: return _strdup("Network is down");
				case 10051: return _strdup("Network is unreachable");
				case 10052: return _strdup("Network dropped connection");
				case 10053: return _strdup("Software caused connection abort");
				case 10054: return _strdup("Connection reset by peer");
				case 10055: return _strdup("No buffer space available");
				case 10060: return _strdup("Connection timed out");
				case 10061: return _strdup("Connection refused");
				case 10065: return _strdup("No route to host");
				case 10091: return _strdup("Network sub-system is unavailable");
				case 10092: return _strdup("Wrong WinSock DLL version");
				case 10093: return _strdup("WSAStartup() not performed");
				case 11001: return _strdup("Host not found");
				case 11002: return _strdup("Non-authoritative host found");
				case 11003: return _strdup("Non-recoverable query error");
				case 11004: return _strdup("Valid name, no data of that type");
				default:
					char number[10]; 
					_itoa_s(error_code, number, 10);
					return _strdup(number);
			}
		}
	};


//int myprintf(const char *lpFormat, ... ) {
//
//    int nLen = 0;
//    int nRet = 0;
//    char cBuffer[512] ;
//    va_list arglist ;
//    HANDLE hOut = NULL;
//    HRESULT hRet;
//
//    ZeroMemory(cBuffer, sizeof(cBuffer));
//
//    va_start(arglist, lpFormat);
//
//    nLen = lstrlen( lpFormat ) ;
//    hRet = StringCchVPrintf(cBuffer,512,lpFormat,arglist);
//    
//    if( nRet >= nLen || GetLastError() == 0 ) {
//        hOut = GetStdHandle(STD_OUTPUT_HANDLE) ;
//        if( hOut != INVALID_HANDLE_VALUE )
//            WriteConsole( hOut, cBuffer, lstrlen(cBuffer), (LPDWORD)&nLen, NULL ) ;
//    }
//
//    return nLen ;
//}

//// Convert number to bytes, and vice versa
//


#ifndef prefix_length
#define prefix_length 4

int ntob(int _number, char* _buffer){
	if(!_buffer) {
		fprintf(stderr, "!Error: nton:: Data is null.\n");
		return -1;
	}
	for(int i = 0; i < prefix_length; i++)
		_buffer[i] = (char)(_number>>(i*8));
	return 0;
}

int bton(char* _buffer){
	if(!_buffer) {
		fprintf(stderr, "!Error: bton:: Data is null.\n");
		return -1;
	}
	
	int result = 0, n; /* prefix_length == 4 for 32-bits or 8 for 64-bits */
	for( n = prefix_length; n>=0; n--)
		result = (result<<8) + _buffer[n];
	return result;
}

#undef prefix_length
#endif

};

#endif // _ALPL_ERRNO_H_