#pragma once
#include "base/helpers.h"
#include <Windows.h>

#include <psapi.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <winternl.h>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Shlwapi.lib")

extern "C" {
typedef NTSTATUS (NTAPI * NtGetNextProcess_t)(
    HANDLE ProcessHandle,
    ACCESS_MASK DesiredAccess,
    ULONG HandleAttributes,
    ULONG Flags,
    PHANDLE NewProcessHandle
);

typedef NTSTATUS (NTAPI * NtOpenSection_t)(
    PHANDLE SectionHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
);

#ifndef _DEBUG
    DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$CloseHandle (HANDLE hObject);
    DECLSPEC_IMPORT HMODULE WINAPI KERNEL32$GetModuleHandleA(LPCSTR lpModuleName);
    DECLSPEC_IMPORT DWORD   WINAPI KERNEL32$GetProcessId(HANDLE Process);
    DECLSPEC_IMPORT DWORD   WINAPI KERNEL32$K32GetProcessImageFileNameA(HANDLE hProcess, LPSTR lpImageFileName, DWORD nSize);
    DECLSPEC_IMPORT int     WINAPI KERNEL32$MultiByteToWideChar(UINT CodePage, DWORD dwFlags, _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
    
    WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);

    WINBASEAPI void*  __cdecl MSVCRT$calloc(size_t number, size_t size);
    WINBASEAPI void   __cdecl MSVCRT$free(void *memblock);
    WINBASEAPI void*  WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
    WINBASEAPI void   __cdecl MSVCRT$memset(void *dest, int c, size_t count);
    WINBASEAPI int    __cdecl MSVCRT$printf(const char * _Format,...);
    WINBASEAPI int    WINAPI  MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);
    WINBASEAPI size_t __cdecl MSVCRT$wcslen(const wchar_t *_Str);

    DECLSPEC_IMPORT LPWSTR WINAPI KERNEL32$lstrcatW (LPWSTR lpString1, LPCWSTR lpString2);

    DECLSPEC_IMPORT int WINAPI USER32$wsprintfW(LPWSTR unnamedParam1, LPCWSTR unnamedParam2, ...);

    DECLSPEC_IMPORT LPCSTR WINAPI SHLWAPI$PathFindFileNameA(LPCSTR pszPath);
#endif
}
#ifndef _DEBUG
    #define CloseHandle                 KERNEL32$CloseHandle
    #define GetModuleHandleA            KERNEL32$GetModuleHandleA
    #define GetProcessId                KERNEL32$GetProcessId
    #define K32GetProcessImageFileNameA KERNEL32$K32GetProcessImageFileNameA
    #define MultiByteToWideChar         KERNEL32$MultiByteToWideChar
    #define GetProcessHeap              KERNEL32$GetProcessHeap
    #define HeapAlloc                   KERNEL32$HeapAlloc
    #define HeapFree                    KERNEL32$HeapFree
    #define calloc                      MSVCRT$calloc
    #define free                        MSVCRT$free
    #define memcpy                      MSVCRT$memcpy
    #define memset                      MSVCRT$memset
    #define printf                      MSVCRT$printf
    #define vsnprintf                   MSVCRT$vsnprintf
    #define wcslen                      MSVCRT$wcslen
    #define lstrcatW                    KERNEL32$lstrcatW
    #define wsprintfW                   USER32$wsprintfW
    #define PathFindFileNameA           SHLWAPI$PathFindFileNameA
#endif