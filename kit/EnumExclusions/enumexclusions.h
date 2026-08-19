#pragma once
#include "base/helpers.h"

#include <Windows.h>
//#include <winternl.h>
#include <stdio.h>

#include <wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

extern "C" {
#ifndef _DEBUG
    DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
    DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
    DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeSecurity(PSECURITY_DESCRIPTOR, LONG, SOLE_AUTHENTICATION_SERVICE*, void*, DWORD, DWORD, void*, DWORD, void*);
    DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoSetProxyBlanket(IUnknown*, DWORD, DWORD, OLECHAR*, DWORD, DWORD, RPC_AUTH_IDENTITY_HANDLE, DWORD);
    DECLSPEC_IMPORT void    WINAPI OLE32$CoUninitialize(void);
    
    DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$SafeArrayAccessData(SAFEARRAY*, void**);
    DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$SafeArrayGetLBound(SAFEARRAY*, unsigned int, long*);
    DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$SafeArrayGetUBound(SAFEARRAY*, unsigned int, long*);
    DECLSPEC_IMPORT HRESULT WINAPI OLEAUT32$SafeArrayUnaccessData(SAFEARRAY* psa);
    WINBASEAPI      BSTR    WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
    WINBASEAPI      void    WINAPI OLEAUT32$SysFreeString(BSTR);
    DECLSPEC_IMPORT void    WINAPI OLEAUT32$VariantClear(VARIANTARG *pvarg);
    
    WINBASEAPI void* __cdecl MSVCRT$calloc(size_t number, size_t size);
    WINBASEAPI void  __cdecl MSVCRT$free(void *memblock);
    WINBASEAPI void  __cdecl MSVCRT$memset(void *dest, int c, size_t count);
    WINBASEAPI void* WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
    WINBASEAPI int   __cdecl MSVCRT$printf(const char * _Format,...);
    WINBASEAPI int   WINAPI  MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);
    WINBASEAPI int   WINAPI  MSVCRT$wcscmp(const wchar_t* str1, const wchar_t* str2);
    
    WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
#endif
}
#ifndef _DEBUG
#define CoCreateInstance      OLE32$CoCreateInstance
#define CoInitializeEx        OLE32$CoInitializeEx
#define CoInitializeSecurity  OLE32$CoInitializeSecurity
#define CoSetProxyBlanket     OLE32$CoSetProxyBlanket
#define CoUninitialize        OLE32$CoUninitialize

#define SafeArrayAccessData   OLEAUT32$SafeArrayAccessData
#define SafeArrayGetLBound    OLEAUT32$SafeArrayGetLBound
#define SafeArrayGetUBound    OLEAUT32$SafeArrayGetUBound
#define SafeArrayUnaccessData OLEAUT32$SafeArrayUnaccessData
#define SysAllocString        OLEAUT32$SysAllocString
#define SysFreeString         OLEAUT32$SysFreeString
#define VariantClear          OLEAUT32$VariantClear

#define calloc                MSVCRT$calloc
#define free                  MSVCRT$free
#define memset                MSVCRT$memset
#define memcpy                MSVCRT$memcpy
#define printf                MSVCRT$printf
#define vsnprintf             MSVCRT$vsnprintf
#define wcscmp                MSVCRT$wcscmp

#define GetProcessHeap        KERNEL32$GetProcessHeap
#define HeapAlloc             KERNEL32$HeapAlloc
#define HeapFree              KERNEL32$HeapFree
#endif