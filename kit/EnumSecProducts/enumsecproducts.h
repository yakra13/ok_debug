#pragma once
#include "base/helpers.h"

#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <wtsapi32.h>

#pragma comment(lib, "Wtsapi32.lib")

typedef struct
{
    const CHAR* filename;
    const WCHAR* description;
    const WCHAR* category;
} SoftwareData;

extern "C" {
#ifndef _DEBUG

//
// Imports
//
DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$GetLastError(void);
DECLSPEC_IMPORT void*  WINAPI KERNEL32$VirtualAlloc (LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
DECLSPEC_IMPORT int    WINAPI KERNEL32$VirtualFree (LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
DECLSPEC_IMPORT HANDLE WINAPI WTSAPI32$WTSCloseServer(HANDLE hServer);
DECLSPEC_IMPORT BOOL   WINAPI WTSAPI32$WTSEnumerateProcessesA(HANDLE hServer, DWORD Reserved, DWORD Version, PWTS_PROCESS_INFOA *ppProcessInfo, DWORD *pCount);
DECLSPEC_IMPORT HANDLE WINAPI WTSAPI32$WTSOpenServerA(LPSTR pServerName);

WINBASEAPI char* __cdecl MSVCRT$strcpy(char* _Dest, const char* _Source);
WINBASEAPI int   __cdecl MSVCRT$tolower(int _C);
WINBASEAPI int   __cdecl MSVCRT$strcmp(const char *str1, const char *str2);
WINBASEAPI int   __cdecl MSVCRT$printf(const char * _Format,...);

#endif
}
#ifndef _DEBUG

#define GetLastError           KERNEL32$GetLastError
#define VirtualAlloc           KERNEL32$VirtualAlloc
#define VirtualFree            KERNEL32$VirtualFree
#define WTSCloseServer         WTSAPI32$WTSCloseServer
#define WTSEnumerateProcessesA WTSAPI32$WTSEnumerateProcessesA
#define WTSOpenServerA         WTSAPI32$WTSOpenServerA

#define strcpy  MSVCRT$strcpy
#define tolower MSVCRT$tolower
#define strcmp  MSVCRT$strcmp
#define printf  MSVCRT$printf

#endif