#pragma once
#include "base/helpers.h"

#include <Windows.h>
#include <stdio.h>

//
// Imports
//
extern "C" {
#ifndef _DEBUG
    WINBASEAPI UINT   WINAPI  KERNEL32$GetDriveTypeA(LPCSTR lpRootPathName);
    WINBASEAPI DWORD  WINAPI  KERNEL32$GetLogicalDriveStringsA(DWORD nBufferLength, LPSTR lpBuffer);

    WINBASEAPI void*  __cdecl MSVCRT$calloc(size_t number, size_t size);
    WINBASEAPI void   __cdecl MSVCRT$free(void *memblock);
    WINBASEAPI void*  WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
    WINBASEAPI void   __cdecl MSVCRT$memset(void *dest, int c, size_t count);
    WINBASEAPI int    __cdecl MSVCRT$printf(const char * _Format,...);
    WINBASEAPI size_t __cdecl MSVCRT$strlen(const char *str);
    WINBASEAPI int    WINAPI  MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);

    WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
#endif
}
#ifndef _DEBUG
    #define GetDriveTypeA           KERNEL32$GetDriveTypeA
    #define GetLogicalDriveStringsA KERNEL32$GetLogicalDriveStringsA

    #define calloc                  MSVCRT$calloc
    #define free                    MSVCRT$free
    #define memcpy                  MSVCRT$memcpy
    #define memset                  MSVCRT$memset
    #define printf                  MSVCRT$printf
    #define strlen                  MSVCRT$strlen
    #define vsnprintf               MSVCRT$vsnprintf

    #define GetProcessHeap          KERNEL32$GetProcessHeap
    #define HeapAlloc               KERNEL32$HeapAlloc
    #define HeapFree                KERNEL32$HeapFree
#endif
