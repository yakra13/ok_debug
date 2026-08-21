#pragma once
#include "base/helpers.h"

#include <Windows.h>
#include <stdio.h>

//#ifdef _DEBUG
    #include <Shlwapi.h>
    #include <Psapi.h>

    #pragma comment(lib, "Shlwapi.lib")
    #pragma comment(lib, "Psapi.lib")
    #pragma comment(lib, "ntdll.lib")
//#endif

#include "ntapi.h"

//
// Macros
//
#define NT_SUCCESS(x) ((x) >= 0)
#define STATUS_INFO_LENGTH_MISMATCH 0xc0000004
#define QUERY_PROC		0x08
#define QUERY_THREAD	0x10

//
// Imports
//
extern "C" {
#ifndef _DEBUG
    DECLSPEC_IMPORT BOOL   WINAPI KERNEL32$CloseHandle(HANDLE hObject);
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetCurrentProcess();
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetCurrentProcessId();
    DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$GetProcessId(HANDLE Process);
    DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$GetProcessIdOfThread(HANDLE Thread);
    DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$K32GetProcessImageFileNameA(HANDLE hProcess, LPSTR lpImageFileName, DWORD nSize);
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);

    WINBASEAPI      BOOL   WINAPI KERNEL32$GetComputerNameA(LPSTR lpBuffer, LPDWORD nSize);
    WINBASEAPI      HANDLE WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI      LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    WINBASEAPI      BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
    
    WINBASEAPI void* __cdecl MSVCRT$calloc(size_t number, size_t size);
    WINBASEAPI void  __cdecl MSVCRT$free(void* memblock);
    WINBASEAPI void* __cdecl MSVCRT$realloc(void* _Memory, size_t _NewSize);
    WINBASEAPI void* __cdecl MSVCRT$malloc(size_t _Size);
    WINBASEAPI void* WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
    WINBASEAPI void  __cdecl MSVCRT$memset(void* dest, int c, size_t count);
    
    WINBASEAPI int   __cdecl MSVCRT$printf(const char* _Format, ...);
    WINBASEAPI int   __cdecl MSVCRT$sprintf_s(char* _Dst, size_t _SizeInBytes, const char* _Format, ...);
    WINBASEAPI int   __cdecl MSVCRT$swprintf_s(wchar_t* _Dst, size_t _SizeInWords, const wchar_t* _Format, ...);
    WINBASEAPI int   WINAPI  MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);
    
    WINBASEAPI int   __cdecl MSVCRT$strcmp(const char* str1, const char* str2);
    
    DECLSPEC_IMPORT LPCSTR WINAPI SHLWAPI$PathFindFileNameA(LPCSTR pszPath);
    DECLSPEC_IMPORT PCWSTR WINAPI SHLWAPI$StrStrIW(PCWSTR pszFirst, PCWSTR pszSrch);

    DECLSPEC_IMPORT NTSTATUS NTAPI NTDLL$NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
    DECLSPEC_IMPORT NTSTATUS NTAPI NTDLL$NtDuplicateObject(HANDLE SourceProcessHandle, HANDLE SourceHandle, HANDLE TargetProcessHandle, PHANDLE TargetHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, ULONG Options);
    DECLSPEC_IMPORT NTSTATUS NTAPI NTDLL$NtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);
#endif
}
#ifndef _DEBUG
    #define CloseHandle                 KERNEL32$CloseHandle                
    #define GetComputerNameA            KERNEL32$GetComputerNameA           
    #define GetCurrentProcess           KERNEL32$GetCurrentProcess          
    #define GetCurrentProcessId         KERNEL32$GetCurrentProcessId        
    #define GetModuleHandleA            KERNEL32$GetModuleHandleA           
    #define GetModuleHandleW            KERNEL32$GetModuleHandleW           
    #define GetProcAddress              KERNEL32$GetProcAddress             
    #define GetProcessHeap              KERNEL32$GetProcessHeap             
    #define GetProcessId                KERNEL32$GetProcessId               
    #define GetProcessIdOfThread        KERNEL32$GetProcessIdOfThread       
    #define HeapAlloc                   KERNEL32$HeapAlloc                  
    #define HeapFree                    KERNEL32$HeapFree                   
    #define K32GetProcessImageFileNameA KERNEL32$K32GetProcessImageFileNameA 
    #define OpenProcess                 KERNEL32$OpenProcess 

    #define calloc                      MSVCRT$calloc
    #define free                        MSVCRT$free
    #define malloc                      MSVCRT$malloc
    #define memcpy                      MSVCRT$memcpy
    #define memset                      MSVCRT$memset
    #define printf                      MSVCRT$printf
    #define realloc                     MSVCRT$realloc
    #define strcmp                      MSVCRT$strcmp
    #define sprintf_s                   MSVCRT$sprintf_s
    #define swprintf_s                  MSVCRT$swprintf_s
    #define vsnprintf                   MSVCRT$vsnprintf

    #define NtDuplicateObject           NTDLL$NtDuplicateObject
    #define NtQueryObject               NTDLL$NtQueryObject
    #define NtQuerySystemInformation    NTDLL$NtQuerySystemInformation

    #define PathFindFileNameA           SHLWAPI$PathFindFileNameA
    #define StrStrIW                    SHLWAPI$StrStrIW
#endif



