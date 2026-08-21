#pragma once
#include "base/helpers.h"

#include <Windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlwapi.h>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "Shlwapi.lib")

//
// Imports
//
extern "C" {
#ifndef _DEBUG
    DECLSPEC_IMPORT BOOL   WINAPI KERNEL32$CloseHandle (HANDLE hObject);
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetCurrentProcessId();
    DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$GetProcessId(HANDLE Process);
    DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$K32GetModuleBaseNameA(HANDLE hProcess, HMODULE hModule, LPSTR lpBaseName, DWORD nSize);
    DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$K32GetModuleFileNameExA(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
    DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$K32GetProcessImageFileNameA(HANDLE hProcess, LPSTR lpImageFileName, DWORD nSize);
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
    DECLSPEC_IMPORT SIZE_T WINAPI KERNEL32$VirtualQueryEx(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength);

    DECLSPEC_IMPORT LPCSTR WINAPI SHLWAPI$PathFindFileNameA(LPCSTR pszPath);

    WINBASEAPI void* __cdecl MSVCRT$calloc(size_t number, size_t size);
    WINBASEAPI void  __cdecl MSVCRT$free(void *memblock);
    WINBASEAPI void* WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
    WINBASEAPI void  __cdecl MSVCRT$memset(void *dest, int c, size_t count);
    WINBASEAPI int   __cdecl MSVCRT$printf(const char * _Format,...); 
    WINBASEAPI int   __cdecl MSVCRT$strcmp(const char *str1, const char *str2);
    WINBASEAPI char* WINAPI  MSVCRT$strncpy(char* dest, const char* src, size_t n);
    WINBASEAPI int   WINAPI  MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);

    WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);

    DECLSPEC_IMPORT FARPROC WINAPI KERNEL32$GetProcAddress(HMODULE hModule, LPCSTR  lpProcName);
    DECLSPEC_IMPORT HMODULE WINAPI KERNEL32$GetModuleHandleA(LPCSTR lpModuleName);
#endif
}
#ifndef _DEBUG
    #define CloseHandle                 KERNEL32$CloseHandle
    #define GetCurrentProcessId         KERNEL32$GetCurrentProcessId
    #define GetProcessId                KERNEL32$GetProcessId
    #define K32GetModuleBaseNameA       KERNEL32$K32GetModuleBaseNameA
    #define K32GetModuleFileNameExA     KERNEL32$K32GetModuleFileNameExA
    #define K32GetProcessImageFileNameA KERNEL32$K32GetProcessImageFileNameA
    #define OpenProcess                 KERNEL32$OpenProcess
    #define VirtualQueryEx              KERNEL32$VirtualQueryEx

    #define PathFindFileNameA           SHLWAPI$PathFindFileNameA

    #define calloc                      MSVCRT$calloc
    #define free                        MSVCRT$free
    #define memcpy                      MSVCRT$memcpy
    #define memset                      MSVCRT$memset
    #define printf                      MSVCRT$printf
    #define strcmp                      MSVCRT$strcmp
    #define strncpy                     MSVCRT$strncpy
    #define vsnprintf                   MSVCRT$vsnprintf

    #define GetProcessHeap              KERNEL32$GetProcessHeap
    #define HeapAlloc                   KERNEL32$HeapAlloc
    #define HeapFree                    KERNEL32$HeapFree

    #define GetProcAddress   KERNEL32$GetProcAddress
    #define GetModuleHandleA KERNEL32$GetModuleHandleA
#endif

typedef NTSTATUS (NTAPI * NtGetNextProcess_t)(HANDLE ProcessHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, ULONG Flags, PHANDLE NewProcessHandle);