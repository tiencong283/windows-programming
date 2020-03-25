#include "pch.h"
#include "Client.h"
#include "ChatRoomClientDlg.h"
#include "Resource.h"
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#define BUFSIZE 1024
#define RET_AND_WSCLEAN WSACleanup(); \
						return 1;

// link with Ws2_32.lib  to use WinSock2
#pragma comment(lib, "Ws2_32.lib")

// handle to main dialog
CChatRoomClientDlg* hmDlg = NULL;

// print error msg
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

int sendWideChars(SOCKET s, wchar_t const* buf, int len) {
	int nSent = 0;
	nSent = send(s, (char const*)buf, len * 2, 0);
	return nSent / 2;
}

// receive wide str and return number of characters received (not bytes)
// assumption: nRecv multiple of 2
int recvWideChars(SOCKET s, wchar_t* buf, int len) {
	int nRecv = 0;
	nRecv = recv(s, (char*)buf, len * 2, 0);
	buf[nRecv / 2] = L'\x00';

	return nRecv / 2;
}

const wchar_t* AUTH_REQUEST = L"AUTH USERNAME\n";
const wchar_t* AUTH_REPLY_FMT = L"USERNAME=%s";
const wchar_t* AUTH_SUCESS = L"AUTH OK\n";
const wchar_t* AUTH_FAIL = L"AUTH FAIL\n";

BOOL doAuthentication(SOCKET sock) {
	CUsernameDlg usernameDlg;
	wchar_t buf[BUFSIZE+1];
	int count = 0;
	
	// auth request
	count = recvWideChars(sock, buf, BUFSIZE);
	if (count == SOCKET_ERROR || wcscmp(buf, AUTH_REQUEST) != 0) {
		return FALSE;
	}

	CString authReply;
	while (TRUE) {
		if (usernameDlg.DoModal() == IDOK) {	// show the dialog to get username
			
			// auth reply
			authReply.Format(AUTH_REPLY_FMT, usernameDlg.username);
			count = sendWideChars(sock, authReply.GetBuffer(), authReply.GetLength());
			if (count == SOCKET_ERROR) {
				return FALSE;
			}

			// auth response
			count = recvWideChars(sock, buf, BUFSIZE);

			if (wcscmp(buf, AUTH_SUCESS) == 0) {
				break;
			}
			else if (wcscmp(buf, AUTH_FAIL) == 0) {
				return FALSE;
			}
			hmDlg->MessageBox(L"Username already exists or network errors. Try again", L"ERROR", MB_ICONERROR);
		}
		else {
			return FALSE;
		}
	}

	hmDlg->prepareSession(sock, usernameDlg.username);
	return TRUE;
}

DWORD WINAPI startConnection(LPVOID lpParameter) {
	int ret;
	WSADATA wsaData;	// information about the Windows Sockets implementation
	ADDRINFOW* result = NULL;
	ADDRINFOW hints;
	CString hostname;
	CString port;
	SOCKET sock = INVALID_SOCKET;


	hmDlg = (CChatRoomClientDlg*)lpParameter;

	// Initialize Winsock
	ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (ret != 0) {
		printError(_T("WSAStartup failed"), ret);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	hmDlg->serverIPEdit.GetWindowText(hostname);
	hmDlg->serverPortEdit.GetWindowText(port);
	ret = GetAddrInfoW(hostname, port, &hints, &result);
	if (ret != 0) {
		printError(_T("GetAddrInfoW failed"), ret);
		RET_AND_WSCLEAN
	}

	// Create a SOCKET for connecting to server
	sock = socket(result->ai_family, result->ai_socktype,
		result->ai_protocol);

	if (sock == INVALID_SOCKET) {
		printError(_T("socket failed"), WSAGetLastError());
		FreeAddrInfoW(result);
		WSACleanup();
		return 1;
	}

	// Connect to server.
	ret = connect(sock, result->ai_addr, (int)result->ai_addrlen);
	if (ret == SOCKET_ERROR) {
		printError(_T("connect failed"), WSAGetLastError());
		closesocket(sock);
		sock = INVALID_SOCKET;
	}
	FreeAddrInfoW(result);

	if (sock == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	
	if (!doAuthentication(sock)) {
		closesocket(sock);
		WSACleanup();
	}

	int nRecv = 0;
	wchar_t buf[BUFSIZE + 1];
	do {
		nRecv = recvWideChars(sock, buf, BUFSIZE);
		if (nRecv > 0) {
			hmDlg->appendMsg(buf);
		}
	} while (nRecv > 0);

	// cleanup
	closesocket(sock);
	WSACleanup();
	return 0;
}