#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#define _T TEXT
#define BUFSIZE 1024
#define RET_AND_WSCLEAN WSACleanup(); \
						return 1;

// link with Ws2_32.lib  to use WinSock2
#pragma comment(lib, "Ws2_32.lib")

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
static wchar_t* hostname = _T("localhost");
static wchar_t* port = _T("9889");

int wmain() {
	int ret;
	WSADATA wsaData;	// information about the Windows Sockets implementation
	ADDRINFOW* result = NULL;
	ADDRINFOW hints;

	SOCKET clientSock = INVALID_SOCKET;

	// Initialize Winsock
	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0) {
		printError(_T("WSAStartup failed"), ret);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	//hints.ai_family = AF_UNSPEC;	// unspecified, getaddrinfo's job (localhost -> ipv6)
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	ret = GetAddrInfoW(hostname, port, &hints, &result);
	if (ret != 0) {
		printError(_T("GetAddrInfoW failed"), ret);
		RET_AND_WSCLEAN
	}

	// Create a SOCKET for connecting to server
	clientSock = socket(result->ai_family, result->ai_socktype,
		result->ai_protocol);

	if (clientSock == INVALID_SOCKET) {
		printError(_T("socket failed"), WSAGetLastError());
		FreeAddrInfoW(result);
		WSACleanup();
		return 1;
	}

	// Connect to server.
	ret = connect(clientSock, result->ai_addr, (int)result->ai_addrlen);
	if (ret == SOCKET_ERROR) {
		printError(_T("connect failed"), WSAGetLastError());
		closesocket(clientSock);
		clientSock = INVALID_SOCKET;
	}
	FreeAddrInfoW(result);

	if (clientSock == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	wprintf(L"Connected to server (%s, %s)\n", hostname, port);
	
	int recvbuflen = BUFSIZE;
	char recvbuf[BUFSIZE];
	int iResult;
	const char *sendbuf = "this is a test";

	// Send an initial buffer
	iResult = send(clientSock, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		printError(L"send failed", WSAGetLastError());
		closesocket(clientSock);
		WSACleanup();
		return 1;
	}

	printf("Bytes Sent: %ld\n", iResult);
	printf("Sent: '%s'\n", sendbuf);

	// shutdown the connection for sending since no more data will be sent
	// the client can still use the ConnectSocket for receiving data
	iResult = shutdown(clientSock, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printError(L"shutdown failed", WSAGetLastError());
		closesocket(clientSock);
		WSACleanup();
		return 1;
	}

	// Receive data until the server closes the connection
	do {
		iResult = recv(clientSock, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			recvbuf[iResult] = '\x00';
			printf("received: '%s'\n\n", recvbuf);
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printError(L"recv failed", WSAGetLastError());
	} while (iResult > 0);

	// cleanup
	closesocket(clientSock);
	WSACleanup();
	return 0;
}