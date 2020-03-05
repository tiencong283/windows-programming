#pragma once

#include "windows.h"

typedef struct _PROCESS_BASIC_INFORMATION32 {
	__int32  Reserved1;
	__int32  PebBaseAddress;
	__int32  Reserved2[2];
	__int32  UniqueProcessId;
	__int32  Reserved3;
} PROCESS_BASIC_INFORMATION32;


typedef struct _PEB32 {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	__int32 Reserved3[2];
	__int32 Ldr;
	__int32 ProcessParameters;	// + 0x10

	/* unprocessed
		PVOID Reserved4[3];
		PVOID AtlThunkSListPtr;
		PVOID Reserved5;
		ULONG Reserved6;
		PVOID Reserved7;
		ULONG Reserved8;
		ULONG AtlThunkSListPtr32;
		PVOID Reserved9[45];
		BYTE Reserved10[96];
		PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
		BYTE Reserved11[128];
		PVOID Reserved12[1];
		ULONG SessionId;
	*/
} PEB32;


typedef struct _UNICODE_STRING32 {
	USHORT Length;
	USHORT MaximumLength;
	__int32  Buffer;
} UNICODE_STRING32;

typedef struct _RTL_USER_PROCESS_PARAMETERS32 {
	BYTE Reserved1[16];
	__int32 Reserved2[10];
	UNICODE_STRING32 ImagePathName;
	UNICODE_STRING32 CommandLine;
} RTL_USER_PROCESS_PARAMETERS32;

