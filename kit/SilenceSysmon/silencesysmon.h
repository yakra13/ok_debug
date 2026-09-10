#pragma once
#include "base/helpers.h"
#include <Windows.h>
#include <stdio.h>
#include <winternl.h>

#pragma comment (lib, "advapi32")
#pragma comment(lib, "mscoree.lib")

#define ENABLE 1
#define DISABLE 0


//
// Imports
//
extern "C" {
#ifndef _DEBUG
DECLSPEC_IMPORT BOOL WINAPI Advapi32$AdjustTokenPrivileges(HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength);
DECLSPEC_IMPORT BOOL WINAPI Advapi32$LookupPrivilegeValueA(LPCSTR lpSystemName, LPCSTR lpName, PLUID lpLuid);
DECLSPEC_IMPORT BOOL WINAPI Advapi32$LookupPrivilegeValueW(LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid);
DECLSPEC_IMPORT BOOL WINAPI Advapi32$OpenProcessToken(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle);

DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$CloseHandle(HANDLE hObject);
DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$FlushInstructionCache(HANDLE hProcess, LPCVOID lpBaseAddress, SIZE_T dwSize);
DECLSPEC_IMPORT HANDLE  WINAPI KERNEL32$GetCurrentProcess();
DECLSPEC_IMPORT DWORD   WINAPI KERNEL32$GetLastError(void);
DECLSPEC_IMPORT HMODULE WINAPI KERNEL32$GetModuleHandleA(LPCSTR lpModuleName);
DECLSPEC_IMPORT HMODULE WINAPI KERNEL32$GetModuleHandleW(LPCWSTR lpModuleName);
DECLSPEC_IMPORT HANDLE  WINAPI KERNEL32$OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
DECLSPEC_IMPORT BOOL    WINAPI KERNEL32$WriteProcessMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T  *lpNumberOfBytesWritten);

#endif
}
#ifndef _DEBUG
#define AdjustTokenPrivileges   Advapi32$AdjustTokenPrivileges
#define LookupPrivilegeValueA   Advapi32$LookupPrivilegeValueA
#define OpenProcessToken        Advapi32$OpenProcessToken

#define CloseHandle             KERNEL32$CloseHandle
#define FlushInstructionCache   KERNEL32$FlushInstructionCache
#define GetCurrentProcess       KERNEL32$GetCurrentProcess
#define GetLastError            KERNEL32$GetLastError
#define OpenProcess             KERNEL32$OpenProcess
#define WriteProcessMemory      KERNEL32$WriteProcessMemory

#endif