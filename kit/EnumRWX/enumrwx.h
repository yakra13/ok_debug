#pragma once
#include "base/helpers.h"

#include <Windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <winternl.h>

//
// Imports
//
extern "C" {
#ifndef _DEBUG
//FindRWX
DECLSPEC_IMPORT BOOL   WINAPI KERNEL32$CloseHandle (HANDLE hObject);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
DECLSPEC_IMPORT SIZE_T WINAPI KERNEL32$VirtualQueryEx(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength);

WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);

WINBASEAPI void*  __cdecl MSVCRT$calloc(size_t number, size_t size);
WINBASEAPI void   __cdecl MSVCRT$free(void *memblock);
WINBASEAPI void*  WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
WINBASEAPI void   __cdecl MSVCRT$memset(void *dest, int c, size_t count);
WINBASEAPI int    WINAPI  MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);

#endif
}
#ifndef _DEBUG
#define CloseHandle    KERNEL32$CloseHandle
#define OpenProcess    KERNEL32$OpenProcess
#define VirtualQueryEx KERNEL32$VirtualQueryEx
#define GetProcessHeap KERNEL32$GetProcessHeap
#define HeapAlloc      KERNEL32$HeapAlloc
#define HeapFree       KERNEL32$HeapFree

#define calloc         MSVCRT$calloc
#define free           MSVCRT$free
#define memcpy         MSVCRT$memcpy
#define memset         MSVCRT$memset
#define vsnprintf      MSVCRT$vsnprintf
#endif