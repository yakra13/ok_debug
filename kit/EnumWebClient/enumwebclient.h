#pragma once
#include "base/helpers.h"

#include <Windows.h>

//
// Imports
//
extern "C" {
#ifndef _DEBUG
WINBASEAPI DWORD WINAPI KERNEL32$GetLastError(void);
WINBASEAPI BOOL  WINAPI KERNEL32$WaitNamedPipeA(LPCSTR lpNamedPipeName, DWORD nTimeOut);

WINBASEAPI void*  WINAPI  MSVCRT$malloc(SIZE_T);
WINBASEAPI SIZE_T WINAPI  MSVCRT$strlen(const char* str);
WINBASEAPI void*  WINAPI  MSVCRT$strcpy(const char* dest, const char* source);
WINBASEAPI void*  WINAPI  MSVCRT$strcat(const char* dest, const char* source);
WINBASEAPI int    __cdecl MSVCRT$printf(const char * _Format,...);
WINBASEAPI int    __cdecl MSVCRT$strcmp(const char *str1, const char *str2);
WINBASEAPI void   __cdecl MSVCRT$free(void *memblock);

DECLSPEC_IMPORT int   __cdecl MSVCRT$fclose(FILE* _File);
DECLSPEC_IMPORT char* __cdecl MSVCRT$fgets(char* _Buffer, int _MaxCount, FILE* _File);
DECLSPEC_IMPORT FILE* __cdecl MSVCRT$fopen(const char* _Filename, const char* _Mode);
DECLSPEC_IMPORT char* __cdecl MSVCRT$strtok(char* _String, const char* _Delimiters);

// //bofstart + internal_printf + printoutput
// WINBASEAPI void *__cdecl MSVCRT$calloc(size_t number, size_t size);
// WINBASEAPI int WINAPI MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);
// WINBASEAPI void __cdecl MSVCRT$memset(void *dest, int c, size_t count);
// WINBASEAPI void* WINAPI MSVCRT$memcpy(void* dest, const void* src, size_t count);
// WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
// WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
// WINBASEAPI BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
#endif
}
#ifndef _DEBUG
#define GetLastError   KERNEL32$GetLastError
#define WaitNamedPipeA KERNEL32$WaitNamedPipeA

#define malloc         MSVCRT$malloc
#define strlen         MSVCRT$strlen
#define strcpy         MSVCRT$strcpy
#define strcat         MSVCRT$strcat
#define printf         MSVCRT$printf
#define strcmp         MSVCRT$strcmp
#define free           MSVCRT$free

#define fclose         MSVCRT$fclose
#define fgets          MSVCRT$fgets
#define fopen          MSVCRT$fopen
#define strtok         MSVCRT$strtok
#endif