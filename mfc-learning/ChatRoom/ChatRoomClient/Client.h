#pragma once
#include <Windows.h>

DWORD WINAPI startConnection(LPVOID lpParameter);

// send wide str and return number of characters sent (not bytes)
int sendWideChars(SOCKET s, wchar_t const* buf, int len);