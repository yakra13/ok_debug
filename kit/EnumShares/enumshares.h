#pragma once
#include "base/helpers.h"

#include <Windows.h>
#include <stdio.h>
#include <Lm.h>

#pragma comment(lib, "Netapi32.lib")
//
// Imports
//
extern "C" {
#ifndef _DEBUG

DECLSPEC_IMPORT NET_API_STATUS NET_API_FUNCTION NETAPI32$NetShareEnum(LMSTR servername, DWORD level, LPBYTE *bufptr, DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries, LPDWORD resume_handle);
DECLSPEC_IMPORT NET_API_STATUS NET_API_FUNCTION NETAPI32$NetUseAdd(LMSTR uncname, DWORD level, LPBYTE buf, LPDWORD parm_err);
DECLSPEC_IMPORT NET_API_STATUS NET_API_FUNCTION NETAPI32$NetApiBufferFree(LPVOID Buffer);
DECLSPEC_IMPORT NET_API_STATUS NET_API_FUNCTION NETAPI32$NetUseDel(LMSTR uncname, LMSTR use_name, DWORD force_cond);

DECLSPEC_IMPORT int WINAPI KERNEL32$MultiByteToWideChar(UINT CodePage, DWORD dwFlags, _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);

DECLSPEC_IMPORT char* __cdecl MSVCRT$strtok(char* _String, const char* _Delimiters);

WINBASEAPI int __cdecl MSVCRT$printf(const char * _Format,...);
WINBASEAPI int __cdecl MSVCRT$_snwprintf(wchar_t *buffer, size_t count, const wchar_t *format, ...);

WINBASEAPI int WINAPI  KERNEL32$lstrcmpW(LPCWSTR lpString1, LPCWSTR lpString2);

#endif
}
#ifndef _DEBUG
#define NetShareEnum        NETAPI32$NetShareEnum
#define NetUseAdd           NETAPI32$NetUseAdd
#define NetApiBufferFree    NETAPI32$NetApiBufferFree
#define NetUseDel           NETAPI32$NetUseDel
#define MultiByteToWideChar KERNEL32$MultiByteToWideChar
#define strtok              MSVCRT$strtok
#define printf              MSVCRT$printf
#define _snwprintf          MSVCRT$_snwprintf
#define lstrcmpW            KERNEL32$lstrcmpW
#endif

