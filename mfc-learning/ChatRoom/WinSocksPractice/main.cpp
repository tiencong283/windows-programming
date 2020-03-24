#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define _T TEXT
#define BUFSIZE 1024
#define DEFAULT_MAX_CLIENT_NUM	1024

#define RET_AND_WSCLEAN WSACleanup(); \
						return 1;
// interface
static wchar_t* hostname = _T("0.0.0.0");
static wchar_t* port = _T("9889");

// hold information for a client
typedef struct ClientStruct {
	ClientStruct* next;
	ClientStruct* previous;
	BOOL auth;
	char* username;
	SOCKET csock;
	SOCKADDR_IN caddr;
} ClientStruct;

ClientStruct* clientList = NULL;	// The head of doubly-linked client list 
int clientListCount = 0;
CRITICAL_SECTION cs;	// ensure atomic access to clientList


// add a new client to the list, return handle to Client if success, otherwise NULL
ClientStruct* addClient(SOCKET cSock, SOCKADDR_IN* clientAddr) {
	if (clientListCount >= DEFAULT_MAX_CLIENT_NUM) {
		return NULL;
	}
	ClientStruct* nc = (ClientStruct*)HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(ClientStruct));
	if (nc == NULL) {
		return NULL;
	}
	// default properties
	nc->auth = FALSE;
	nc->username = NULL;
	nc->csock = cSock;	// client socket
	memcpy(&nc->caddr, clientAddr, sizeof(SOCKADDR_IN));	// client addr

	// linking
	EnterCriticalSection(&cs);
	if (clientList == NULL) {
		nc->next = nc;	// point to itself
		nc->previous = nc;
		clientList = nc;
	}
	else {	// add next to head
		nc->next = clientList->next;
		clientList->next->previous = nc;
		
		clientList->next = nc;
		nc->previous = clientList;
	}
	LeaveCriticalSection(&cs);

	InterlockedExchangeAdd((PLONG)&clientListCount, 1);
	return nc;
}

// return TRUE if checking username already exists
BOOL usernameExists(ClientStruct* start, char const* check) {
	EnterCriticalSection(&cs);
	// must guarantee clientList != NULL
	ClientStruct* p = start->next;
	for (; p != start; p=p->next) {
		if (p->username != NULL && strcmp(p->username, check) == 0) {
			LeaveCriticalSection(&cs);
			return TRUE;
		}
	}
	LeaveCriticalSection(&cs);
	return FALSE;
}

// print error msg
void printError(LPWSTR msg, int errCode)
{
	wchar_t* formatted = NULL;

	int nread = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errCode, 0, (LPTSTR)&formatted, 0, NULL);
	if (nread == 0) {
		wprintf(_T("%s: 0x%x\n"), msg, errCode);
		return;
	}
	formatted[nread] = _T('\x00');
	wprintf(_T("%s: %s\n"), msg, formatted);
	LocalFree(formatted);
}

// print host information as "prompt host:port"
void printHostAddr(char const* prompt, SOCKADDR_IN& addr) {
	char hostStr[BUFSIZE];

	memset(hostStr, 0, BUFSIZE);
	inet_ntop(addr.sin_family, &addr.sin_addr, hostStr, BUFSIZE);
	printf("%s %s:%d\n\n", prompt, hostStr, htons(addr.sin_port));
}

// broadcasing a message to all clients except the sending one
BOOL broadcastMsg(ClientStruct* client, char const* msg, int msgCount) {
	SOCKET cSock = client->csock;
	ClientStruct* p = client->next;
	int nSent = 0;
	char formatted[BUFSIZE];

	if (clientListCount < 2) {	
		return TRUE;
	}
	snprintf(formatted, BUFSIZE, "%s: %s", client->username, msg);
	EnterCriticalSection(&cs);
	while (p != client) {
		nSent = send(p->csock, formatted, strlen(formatted), 0);
		if (nSent == SOCKET_ERROR) {
			LeaveCriticalSection(&cs);
			return FALSE;
		}
		p = p->next;
	}
	LeaveCriticalSection(&cs);
	return TRUE;
}

const char* AUTH_REQUEST = "AUTH USERNAME\n";
const char* AUTH_REPLY = "USERNAME=";
const char* AUTH_SUCESS = "AUTH OK\n";
const char* AUTH_FAIL = "AUTH FAIL\n";

// simple auth process with only username and maxTrials
// return TRUE if success, otherwise FALSE
BOOL doAuthentication(ClientStruct* client) {
	size_t maxTrials = 3;
	SOCKET cSock = client->csock;
	char buf[BUFSIZE];
	int bufSize = BUFSIZE;
	int nRecv = 0;
	int nSent = 0;

	char* username = NULL;
	char const* status = NULL;
	
	while (maxTrials > 0) {
		// auth request
		nSent = send(cSock, AUTH_REQUEST, strlen(AUTH_REQUEST), 0);
		if (nSent == SOCKET_ERROR) {
			return FALSE;
		}
		// auth reply
		nRecv = recv(cSock, buf, bufSize - 1, 0);
		if (nRecv > 0) {
			if (strncmp(buf, AUTH_REPLY, strlen(AUTH_REPLY)) == 0) {	// valid response
				username = buf + strlen(AUTH_REPLY);
				username[strcspn(username, " \n")] = '\x00';
				if (!usernameExists(client, username)) {
					client->username = _strdup(username);
					status = AUTH_SUCESS;
					break;
				}
				else {
					status = AUTH_FAIL;
				}
			}
		} else {	// close connection or network errors
			return FALSE;
		}
		maxTrials -= 1;
	}
	// send the status
	nSent = send(cSock, status, strlen(status), 0);
	if (nSent == SOCKET_ERROR) {
		return FALSE;
	}
	if (status == AUTH_SUCESS)
		return TRUE;
	return FALSE;
}

// remove and free client resources
void removeClient(ClientStruct* client) {
	EnterCriticalSection(&cs);

	// unlink
	ClientStruct* p = client->previous;
	ClientStruct* n = client->next;
	p->next = n;
	n->previous = p;
	
	free(client->username);
	closesocket(client->csock);
	HeapFree(GetProcessHeap(), 0, client);

	LeaveCriticalSection(&cs);
	InterlockedExchangeAdd((PLONG)&clientListCount, -1);
}

DWORD WINAPI clientHandler(LPVOID lpParameter) {
	int ret = 0;
	ClientStruct* client = (ClientStruct*)lpParameter;
	SOCKET cSock = client->csock;
	char buf[BUFSIZE];
	int bufSize = BUFSIZE;
	int nRecv = 0;

	if (!doAuthentication(client)) {	// auth first
		removeClient(client);
		return 1;
	}

	// receive until the peer shuts down the connection
	do {
		nRecv = recv(cSock, buf, bufSize - 1, 0);
		if (nRecv > 0) {
			printf("Bytes received: %d\n", nRecv);
			buf[nRecv] = '\x00';
			printf("'%s'\n\n", buf);

			// Echo the buffer back to the sender
			broadcastMsg(client, buf, nRecv);
		}
		else if (nRecv == 0)
			printf("Connection closing...\n");
		else {	// SOCKET_ERROR 
			printError(_T("recv failed"), WSAGetLastError());
			removeClient(client);
			return 1;
		}
	} while (nRecv > 0);

	removeClient(client);
	return 0;
}

int wmain() {
	int ret;
	WSADATA wsaData;	// information about the Windows Sockets implementation
	
	ADDRINFOW *result = NULL;
	ADDRINFOW hints;

	SOCKET serverSock = INVALID_SOCKET;	// server socket
	SOCKET clientSock = INVALID_SOCKET;	// client socket

	// initialize Winsock
	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0) {
		printError(_T("WSAStartup failed"), ret);
		return 1;
	}

	// resolve hostname -> network addr
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	ret = GetAddrInfoW(hostname, port, &hints, &result);
	if (ret != 0) {
		printError(_T("GetAddrInfoW failed"), ret);
		RET_AND_WSCLEAN
	}
	
	// create a SOCKET for the server to listen for client connections
	serverSock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (serverSock == INVALID_SOCKET) {
		printError(_T("socket failed"), WSAGetLastError());
		RET_AND_WSCLEAN
	}

	// to bind a socket to an IP addr and a port
	ret = bind(serverSock, result->ai_addr, (int)result->ai_addrlen);
	if (ret == SOCKET_ERROR) {
		printError(_T("bind failed"), WSAGetLastError());
		FreeAddrInfoW(result);
		closesocket(serverSock);
		RET_AND_WSCLEAN
	}
	FreeAddrInfoW(result);

	// initialize critical section
	if (!InitializeCriticalSectionAndSpinCount(&cs, 0x400)) {
		printError(_T("InitializeCriticalSectionAndSpinCount failed"), WSAGetLastError());
		closesocket(serverSock);
		RET_AND_WSCLEAN
	}

	// listen incomming connections
	if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
		printError(_T("Listen failed with error"), WSAGetLastError());
		closesocket(serverSock);
		RET_AND_WSCLEAN
	}

	SOCKADDR_IN clientAddr = {};
	int clientAddrSize = sizeof(SOCKADDR_IN);
	
	for (;;) {	// listen for connections from multiple clients
		// accept client connection
		clientSock = accept(serverSock, (SOCKADDR*)&clientAddr, &clientAddrSize);
		if (clientSock == INVALID_SOCKET) {
			printError(_T("accept failed"), WSAGetLastError());
			DeleteCriticalSection(&cs);
			closesocket(serverSock);
			RET_AND_WSCLEAN
		}
		printHostAddr("Accepted connection from client:", clientAddr);
		ClientStruct* nc = addClient(clientSock, &clientAddr);
		// todo: verify nc

		HANDLE hThread = CreateThread(
			NULL,
			0,
			clientHandler,
			(LPVOID)nc,
			0,
			NULL);

		if (hThread == NULL) {
			printError(_T("CreateThread failed"), GetLastError());
			DeleteCriticalSection(&cs);
			closesocket(serverSock);
			RET_AND_WSCLEAN
		}
	}
	
	DeleteCriticalSection(&cs);
	// close serverSocket (no longer need serverSock)
	closesocket(serverSock);
	WSACleanup();
	return 0;
}