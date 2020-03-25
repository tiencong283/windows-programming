#include "stdafx.h"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Server.h"

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define BUFSIZE 1024
#define DEFAULT_MAX_CLIENT_NUM	1024

#define RET_AND_WSCLEAN WSACleanup(); \
						return 1;

// handle to main dialog
CChatRoomServerDlg* hmDlg = NULL;

// hold information for a client
typedef struct ClientStruct {
	ClientStruct* next;
	ClientStruct* previous;
	BOOL auth;
	wchar_t* username;
	SOCKET csock;
	SOCKADDR_IN caddr;
	wchar_t* ip;
	int port;
} ClientStruct;

ClientStruct* clientList = NULL;	// The head of doubly-linked client list 
int clientListCount = 0;
CRITICAL_SECTION cs;	// ensure atomic access to clientList

// log the msg
void log(wchar_t const* msg) {
	hmDlg->hLogDlg->appendMsg(msg);
}

// log host information (connected, disconnected)
void logHost(ClientStruct* client, wchar_t const* prompt) {
	CString buffer;
	
	buffer.Format(L"%s %s:%d\n", prompt, client->ip, client->port);
	log(buffer);
}

// log error msg
void printError(LPWSTR msg, int errCode)
{
	wchar_t* formatted = NULL;
	wchar_t buffer[BUFSIZE];

	int nread = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errCode, 0, (LPTSTR)&formatted, 0, NULL);
	if (nread == 0) {
		swprintf_s(buffer, L"%s: 0x%x", msg, errCode);
		return;
	}
	formatted[nread] = L'\x00';
	swprintf_s(buffer, L"%s: %s", msg, formatted);
	
	hmDlg->MessageBox(buffer, L"ERROR", MB_ICONERROR);
	LocalFree(formatted);
}

// add a new client to the list, return handle to Client if success, otherwise NULL
ClientStruct* addClient(SOCKET cSock, SOCKADDR_IN* addr) {
	wchar_t hostStr[BUFSIZE];

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
	
	memset(hostStr, 0, BUFSIZE);
	InetNtopW(addr->sin_family, &addr->sin_addr, hostStr, BUFSIZE);
	nc->ip = _wcsdup(hostStr);
	nc->port = htons(addr->sin_port);

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
BOOL usernameExists(ClientStruct* start, wchar_t const* check) {
	EnterCriticalSection(&cs);
	// must guarantee clientList != NULL
	ClientStruct* p = start->next;
	for (; p != start; p = p->next) {
		if (p->username != NULL && wcscmp(p->username, check) == 0) {
			LeaveCriticalSection(&cs);
			return TRUE;
		}
	}
	LeaveCriticalSection(&cs);
	return FALSE;
}

// send wide str and return number of characters sent (not bytes)
int sendWideChars(SOCKET s, wchar_t const* buf, int len) {
	int nSent = 0;
	nSent = send(s, (char const*)buf, len*2, 0);
	return nSent / 2;
}

// receive wide str and return number of characters received (not bytes)
// assumption: nRecv multiple of 2
int recvWideChars(SOCKET s, wchar_t* buf, int len) {
	int nRecv = 0;
	nRecv = recv(s, (char*)buf, len*2, 0);
	buf[nRecv / 2] = L'\x00';

	return nRecv / 2;
}

// broadcasing a message to all clients except the sending one
BOOL broadcastMsg(ClientStruct* client, wchar_t const* content, int msgCount) {
	SOCKET cSock = client->csock;
	ClientStruct* p = client->next;
	int count = 0;
	CString msg;

	if (clientListCount < 2) {
		return TRUE;
	}
	msg.Format(L"[%s] %s", client->username, content);
	EnterCriticalSection(&cs);
	while (p != client) {
		count = sendWideChars(p->csock, msg.GetBuffer(), msg.GetLength());
		if (count == SOCKET_ERROR) {
			LeaveCriticalSection(&cs);
			return FALSE;
		}
		p = p->next;
	}
	LeaveCriticalSection(&cs);
	return TRUE;
}

const wchar_t* AUTH_REQUEST = L"AUTH USERNAME\n";
const wchar_t* AUTH_REPLY = L"USERNAME=";
const wchar_t* AUTH_SUCESS = L"AUTH OK\n";
const wchar_t* AUTH_FAIL = L"AUTH FAIL\n";

// simple auth process with only username and maxTrials
// return TRUE if success, otherwise FALSE
BOOL doAuthentication(ClientStruct* client) {
	size_t maxTrials = 3;
	SOCKET sock = client->csock;
	wchar_t buf[BUFSIZE + 1];
	int count = 0;

	CString username;
	wchar_t const* status = NULL;

	while (maxTrials > 0) {

		// auth request
		count = sendWideChars(sock, AUTH_REQUEST, wcslen(AUTH_REQUEST));
		if (count == SOCKET_ERROR) {
			return FALSE;
		}
		// auth reply
		count = recvWideChars(sock, buf, BUFSIZE);

		if (count > 0) {
			if (wmemcmp(buf, AUTH_REPLY, wcslen(AUTH_REPLY)) == 0){	// auth response
				
				username = buf + wcslen(AUTH_REPLY);
				int wsAt = wcscspn(username, L" \t\n");
				if (wsAt < username.GetLength()) {	// trim whitespaces
					username.SetAt(wsAt, L'\x00');
				}

				if (!usernameExists(client, username)) {
					client->username = _wcsdup(username);
					status = AUTH_SUCESS;
					break;
				}
				else {
					status = AUTH_FAIL;
				}
			}
		}
		else {	// close connection or network errors
			return FALSE;
		}
		maxTrials -= 1;
	}
	// send the status
	count = sendWideChars(sock, status, wcslen(status));
	if (count == SOCKET_ERROR) {
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
	logHost(client, L"disconnected:");
	InterlockedExchangeAdd((PLONG)&clientListCount, -1);
}

DWORD WINAPI clientHandler(LPVOID lpParameter) {
	int ret = 0;
	ClientStruct* client = (ClientStruct*)lpParameter;
	SOCKET cSock = client->csock;
	wchar_t buf[BUFSIZE + 1];
	int count = 0;

	if (!doAuthentication(client)) {	// auth first
		removeClient(client);
		return 1;
	}
	// joined
	CString buffer;
	buffer.Format(L"%s joined %s:%d", client->username, client->ip, client->port);
	log(buffer);

	hmDlg->addClient(client->username, client->ip, client->port);

	// receive until the peer shuts down the connection
	do {
		count = recvWideChars(cSock, buf, BUFSIZE);
		if (count > 0) {
			// Echo the buffer back to the sender
			broadcastMsg(client, buf, count);
		}
		else {
			hmDlg->removeClient(client->username);
			removeClient(client);
			
			if (count == SOCKET_ERROR) {
				printError(_T("recv failed"), WSAGetLastError());
				return 1;
			}
		}
	} while (count > 0);
	
	return 0;
}

 DWORD WINAPI startServer(LPVOID lpParameter) {
	int ret;
	WSADATA wsaData;	// information about the Windows Sockets implementation

	ADDRINFOW* result = NULL;
	ADDRINFOW hints;

	CString hostname;
	CString port;

	SOCKET serverSock = INVALID_SOCKET;	// server socket
	SOCKET clientSock = INVALID_SOCKET;	// client socket

	hmDlg = (CChatRoomServerDlg*)lpParameter;

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

	hmDlg->GetDlgItemText(IDC_EDIT_ServerIP, hostname);
	hmDlg->GetDlgItemText(IDC_EDIT_ServerPort, port);

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

	// listen incomming connection
	if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
		printError(_T("Listen failed with error"), WSAGetLastError());
		closesocket(serverSock);
		RET_AND_WSCLEAN
	}
	
	log(L"started the server");
	hmDlg->sock = serverSock;

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
		
		ClientStruct* nc = addClient(clientSock, &clientAddr);
		logHost(nc, L"connected:");

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