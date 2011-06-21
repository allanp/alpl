#include "net/tcpserver.h"
#include "net/tcpclient.h"

#include "containers/BlockingQueue.h"
#include "threading/threading.h"
#include "util/stopwatch.h"

using namespace alpl::net;

HANDLE g_hDone;
int counter;

void onDataReceived(TcpConnectionPtr sender, void* data, size_t length){

	/*
	printf("(T%d): ", GetCurrentThreadId());
    printf("data received from %s\n", sender->remote_endpoint());
    printf("sender socket: %d\n", (sender->socket()));
    printf("data length: %d\n", length);
    printf("data string: %s\n", data);
	*/

	if(counter > 100){
		return;
	}

	sender->Send(data, length);

    //// do echo
    //
    // sender->sendData(data, length);

    // printf("Thread %d: Send %d bytes data\n", GetCurrentThreadId(), length);
	
}

void onDataReceiveClient(TcpConnectionPtr sender, void* data, size_t length){
	
	if(++counter > 100){
		SetEvent(g_hDone);
		return;
	}

	sender->Send(data, length);
}

void onDisc(TcpConnectionPtr sender, void* data, size_t length){
	if(!sender)
		return;
	printf("(T%d): ", GetCurrentThreadId());
	printf("disconnected from socket_addr %s\n", sender->remote_endpoint());
}

void onConn(TcpConnectionPtr sender, void* data, size_t length){
	if(!sender)
		return;
	printf("(T%d): ", GetCurrentThreadId());
	printf("connected from socket_addr %s\n", sender->remote_endpoint());
}

void onSendDone(TcpConnectionPtr dest, void* data, size_t length){
	/*
	if(!dest)
		return;
	printf("(T%d): ", GetCurrentThreadId());
	printf("sent to socket_addr %s done\n", dest->remote_endpoint());
	*/
}


HANDLE g_serverStarted;

DWORD WINAPI ServerSession(LPVOID SessionContext){

	TcpServer* server = (TcpServer*)SessionContext;

	server->onDataReceived = &onDataReceived;
	server->onConnected = &onConn;
	server->onDisconnected = &onDisc;
	server->onSentCompleted = NULL; //&onSendDone;

	server->Start();

	printf("(T%d)Hello, I started: %d\n", GetCurrentThreadId(), 123);
	
	SetEvent(g_serverStarted);

	getchar();

	server->Stop();

	return 0;
}

DWORD WINAPI ClientSession(LPVOID SessionContext){

	TcpClient* client = (TcpClient*)SessionContext;

	client->onDataReceived = &onDataReceiveClient;
	client->onConnected = &onConn;
	client->onDisconnected = &onDisc;
	client->onSentCompleted = NULL; //&onSendDone;

	client->Start();

	size_t len = 10 * 1024;
	char* data = new char[len];
	memset(data, 0, len);

	counter = 0;

	int i = 1000;
	{
		alpl::util::Stopwatch sw;
		sw.begin(); //= alpl::util::Stopwatch::start_new();
		//do{
		client->Connection().Send(data, len);
		//}while(0);
		WaitForSingleObject(g_hDone, -1);
		sw.stop();

		printf("Send 1000 * (%d bytes) cost: %f milliseconds.\n", len, sw.elapsedmilliseconds());
	}

	getchar();

	client->Stop();

	return 0;
}


int main(){

	g_serverStarted = CreateEvent(0, TRUE, FALSE, NULL);
	g_hDone = CreateEvent(0,TRUE, FALSE, NULL);

	TcpServer server("8888");
	TcpClient client("localhost", "8888");

	DWORD serverThreadId = 0;
	DWORD clientThreadId = 0;

	HANDLE serverThread = CreateThread(0, 0, &ServerSession, &server, 0, &serverThreadId);

	WaitForSingleObject(g_serverStarted, -1);

	// HANDLE clientThread = CreateThread(0, 0, &ClientSession, &client, 0, &clientThreadId);

	printf("serverThread: %d\n", serverThreadId);
	//printf("clientThread: %d\n", clientThreadId);
	
	WaitForSingleObject(serverThread, -1);
	
	printf("Finished.\n");
	getchar();
	
	return 0;
}

