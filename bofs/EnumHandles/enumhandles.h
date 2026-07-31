#pragma once
#include "base/helpers.h"

#include <Windows.h>
//#include <winternl.h>
#include <stdio.h>

//#ifdef _DEBUG
    #include <Shlwapi.h>
    #include <Psapi.h>
    #include <stdio.h>

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
    //DFR(KERNEL32, CloseHandle);
    //DFR(KERNEL32, GetComputerNameA);
    //DFR(KERNEL32, GetCurrentProcess);
    //DFR(KERNEL32, GetCurrentProcessId);
    //DFR(KERNEL32, GetModuleHandleA);
    //DFR(KERNEL32, GetModuleHandleW);
    //DFR(KERNEL32, GetProcAddress);
    //DFR(KERNEL32, GetProcessHeap);
    //DFR(KERNEL32, GetProcessId);
    //DFR(KERNEL32, GetProcessIdOfThread);
    //DFR(KERNEL32, HeapAlloc);
    //DFR(KERNEL32, HeapFree);
    //DFR(KERNEL32, K32GetProcessImageFileNameA);
    ////DFR(KERNEL32, QueryFullProcessImageNameA); // use this instead maybe
    //DFR(KERNEL32, OpenProcess);
    ////DFR(KERNEL32, StrStrIW);

    //DFR(MSVCRT, calloc);
    //DFR(MSVCRT, free);
    //DFR(MSVCRT, malloc);
    //DFR(MSVCRT, memcpy);
    //DFR(MSVCRT, memset);
    //DFR(MSVCRT, printf);
    //DFR(MSVCRT, realloc);
    //DFR(MSVCRT, strcmp);
    //DFR(MSVCRT, vsnprintf);
    //DFR(NTDLL, NtDuplicateObject);
    //DFR(NTDLL, NtQueryObject);
    //DFR(NTDLL, NtQuerySystemInformation);

    //DFR(SHLWAPI, PathFindFileNameA);

    ////DFR(MSVCRT, sprintf_s);
    ////DFR(MSVCRT, swprintf_s);
#ifndef _DEBUG
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
    DECLSPEC_IMPORT DWORD WINAPI KERNEL32$K32GetProcessImageFileNameA(HANDLE hProcess, LPSTR lpImageFileName, DWORD nSize);
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetCurrentProcess();
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetCurrentProcessId();
    DECLSPEC_IMPORT BOOL WINAPI KERNEL32$CloseHandle(HANDLE hObject);
    DECLSPEC_IMPORT DWORD WINAPI KERNEL32$GetProcessId(HANDLE Process);
    DECLSPEC_IMPORT DWORD WINAPI KERNEL32$GetProcessIdOfThread(HANDLE Thread);

    WINBASEAPI BOOL WINAPI KERNEL32$GetComputerNameA(LPSTR lpBuffer, LPDWORD nSize);
    WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    WINBASEAPI BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
    
    WINBASEAPI void* __cdecl MSVCRT$malloc(size_t _Size);
    WINBASEAPI void* __cdecl MSVCRT$realloc(void* _Memory, size_t _NewSize);
    WINBASEAPI int __cdecl MSVCRT$sprintf_s(char* _Dst, size_t _SizeInBytes, const char* _Format, ...);
    WINBASEAPI int __cdecl MSVCRT$swprintf_s(wchar_t* _Dst, size_t _SizeInWords, const wchar_t* _Format, ...);
    WINBASEAPI int __cdecl MSVCRT$printf(const char* _Format, ...);
    WINBASEAPI int __cdecl MSVCRT$strcmp(const char* str1, const char* str2);
    WINBASEAPI void* __cdecl MSVCRT$calloc(size_t number, size_t size);
    WINBASEAPI int WINAPI MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);
    WINBASEAPI void __cdecl MSVCRT$memset(void* dest, int c, size_t count);
    WINBASEAPI void* WINAPI MSVCRT$memcpy(void* dest, const void* src, size_t count);
    WINBASEAPI void __cdecl MSVCRT$free(void* memblock);
    
    DECLSPEC_IMPORT LPCSTR WINAPI SHLWAPI$PathFindFileNameA(LPCSTR pszPath);
    DECLSPEC_IMPORT PCWSTR WINAPI SHLWAPI$StrStrIW(PCWSTR pszFirst, PCWSTR pszSrch);

    DECLSPEC_IMPORT NTSTATUS NTAPI NTDLL$NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
    DECLSPEC_IMPORT NTSTATUS NTAPI NTDLL$NtDuplicateObject(HANDLE SourceProcessHandle, HANDLE SourceHandle, HANDLE TargetProcessHandle, PHANDLE TargetHandle, ACCESS_MASK DesiredAccess, ULONG HandleAttributes, ULONG Options);
    DECLSPEC_IMPORT NTSTATUS NTAPI NTDLL$NtQueryObject(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);

    //bofstart + internal_printf + printoutput
#endif
    //DECLSPEC_IMPORT int __cdecl MSVCRT$sprintf_s(char* _Dst, size_t _SizeInBytes, const char* _Format, ...);
    //DECLSPEC_IMPORT int __cdecl MSVCRT$swprintf_s(wchar_t* _Dst, size_t _SizeInWords, const wchar_t* _Format, ...);

    //DFR(SHLWAPI, trStrIW);
}
#ifdef _DEBUG
    #define KERNEL32$CloseHandle                 CloseHandle               
    #define KERNEL32$GetComputerNameA            GetComputerNameA          
    #define KERNEL32$GetCurrentProcess           GetCurrentProcess         
    #define KERNEL32$GetCurrentProcessId         GetCurrentProcessId       
    #define KERNEL32$GetModuleHandleA            GetModuleHandleA          
    #define KERNEL32$GetModuleHandleW            GetModuleHandleW          
    #define KERNEL32$GetProcAddress              GetProcAddress            
    #define KERNEL32$GetProcessHeap              GetProcessHeap            
    #define KERNEL32$GetProcessId                GetProcessId              
    #define KERNEL32$GetProcessIdOfThread        GetProcessIdOfThread      
    #define KERNEL32$HeapAlloc                   HeapAlloc                 
    #define KERNEL32$HeapFree                    HeapFree                  
    #define KERNEL32$K32GetProcessImageFileNameA K32GetProcessImageFileNameA
    #define KERNEL32$OpenProcess                 OpenProcess

    #define MSVCRT$calloc                        calloc
    #define MSVCRT$free                          free
    #define MSVCRT$malloc                        malloc
    #define MSVCRT$memcpy                        memcpy
    #define MSVCRT$memset                        memset
    #define MSVCRT$printf                        printf
    #define MSVCRT$realloc                       realloc
    #define MSVCRT$strcmp                        strcmp
    #define MSVCRT$sprintf_s                     sprintf_s
    #define MSVCRT$swprintf_s                    swprintf_s
    #define MSVCRT$vsnprintf                     vsnprintf

    #define NTDLL$NtDuplicateObject              NtDuplicateObject           
    #define NTDLL$NtQueryObject                  NtQueryObject               
    #define NTDLL$NtQuerySystemInformation       NtQuerySystemInformation    

    #define SHLWAPI$PathFindFileNameA            PathFindFileNameA
    #define SHLWAPI$StrStrIW                     StrStrIW
#endif



