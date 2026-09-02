#pragma once
#include "base/helpers.h"

#include <windows.h>
#include <stdio.h>
#include <tdh.h>
#include <pla.h>
#include <oleauto.h>
#include <tlhelp32.h>
#include <fltuser.h>

#pragma comment(lib, "tdh.lib")
#pragma comment(lib, "Ole32.lib") 
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "FltLib.lib" )

#define HRESULT_FROM_WIN32(x) (x ? ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000)) : 0)
#define MAX_GUID_SIZE 39
#define MAX_DATA_LENGTH 65000
// #define true 1

typedef struct
{
    double min;
    double max;
    const char* name;
} FilterAltitudeRange;

static const FilterAltitudeRange altitudeRanges[] =
{
    { 420000, 429999, "Filter" },
    { 400000, 409999, "FSFilter Top" },
    { 360000, 389999, "FSFilter Activity Monitor" },
    { 340000, 349999, "FSFilter Undelete" },
    { 320000, 329999, "FSFilter Anti-Virus" },
    { 300000, 309999, "FSFilter Replication" },
    { 280000, 289999, "FSFilter Continuous Backup" },
    { 260000, 269999, "FSFilter Content Screener" },
    { 240000, 249999, "FSFilter Quota Management" },
    { 220000, 229999, "FSFilter System Recovery" },
    { 200000, 209999, "FSFilter Cluster File System" },
    { 180000, 189999, "FSFilter HSM" },
    { 170000, 175000, "FSFilter Imaging" },
    { 160000, 169999, "FSFilter Compression" },
    { 140000, 149999, "FSFilter Encryption" },
    { 130000, 139999, "FSFilter Virtualization" },
    { 120000, 129999, "FSFilter Physical Quota Management" },
    { 100000, 109999, "FSFilter Open File" },
    {  80000,  89999, "FSFilter Security Enhancer" },
    {  60000,  69999, "FSFilter Copy Protection" },
    {  40000,  49999, "FSFilter Bottom" },
    {  20000,  29999, "FSFilter System" },
    {      0,  19999, "FSFilter Infrastructure" }
};

//
// Imports
//
extern "C" {
#ifndef _DEBUG
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
DECLSPEC_IMPORT void    WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);

DECLSPEC_IMPORT void    WINAPI OLEAUT32$VariantInit(VARIANTARG *pvarg);
DECLSPEC_IMPORT void    WINAPI OLEAUT32$VariantClear(VARIANTARG *pvarg);

DECLSPEC_IMPORT LONG    WINAPI ADVAPI32$RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
DECLSPEC_IMPORT LSTATUS WINAPI ADVAPI32$RegGetValueA(HKEY hkey, LPCSTR lpSubKey, LPCSTR lpValue, DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData);
DECLSPEC_IMPORT LONG    WINAPI ADVAPI32$RegCloseKey(HKEY hKey);

DECLSPEC_IMPORT int WINAPI KERNEL32$MultiByteToWideChar(UINT CodePage, DWORD dwFlags, _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);

DECLSPEC_IMPORT int __cdecl OLE32$StringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cchMax);

WINBASEAPI TDHSTATUS WINAPI TDH$TdhEnumerateProviders(PPROVIDER_ENUMERATION_INFO pBuffer, ULONG *pBufferSize);

WINBASEAPI void*  __cdecl MSVCRT$realloc(void *ptr, size_t size);
WINBASEAPI size_t __cdecl MSVCRT$strlen(const char *str);
WINBASEAPI int    __cdecl MSVCRT$_wcsicmp(const wchar_t *str1, const wchar_t *str2);
WINBASEAPI void*  __cdecl MSVCRT$malloc(size_t size);
WINBASEAPI int    __cdecl MSVCRT$wprintf(const wchar_t *format, ...);
WINBASEAPI int    __cdecl MSVCRT$printf(const char * _Format,...);
WINBASEAPI int    __cdecl MSVCRT$strcmp(const char *str1, const char *str2);
WINBASEAPI int    __cdecl MSVCRT$getchar(void);
WINBASEAPI double __cdecl MSVCRT$wcstod(const wchar_t* strSource, wchar_t** endptr);
WINBASEAPI size_t __cdecl MSVCRT$wcslen(const wchar_t *string);
WINBASEAPI size_t __cdecl MSVCRT$strlen(const char *_Str);

WINBASEAPI HRESULT WINAPI Fltlib$FilterFindFirst(FILTER_INFORMATION_CLASS dwInformationClass, LPVOID lpBuffer, DWORD dwBufferSize, LPDWORD lpBytesReturned, LPHANDLE lpFilterFind);
WINBASEAPI HRESULT WINAPI Fltlib$FilterFindNext(HANDLE hFilterFind, FILTER_INFORMATION_CLASS dwInformationClass, LPVOID lpBuffer, DWORD dwBufferSize, LPDWORD lpBytesReturned);
wcstod
#endif
}
#ifndef _DEBUG
#define CoInitializeEx        OLE32$CoInitializeEx
#define CoUninitialize        OLE32$CoUninitialize
#define CoCreateInstance      OLE32$CoCreateInstance

#define VariantInit           OLEAUT32$VariantInit
#define VariantClear          OLEAUT32$VariantClear

#define RegOpenKeyExA         ADVAPI32$RegOpenKeyExA
#define RegGetValueA          ADVAPI32$RegGetValueA
#define RegCloseKey           ADVAPI32$RegCloseKey

#define MultiByteToWideChar   KERNEL32$MultiByteToWideChar

#define StringFromGUID2       OLE32$StringFromGUID2

#define TdhEnumerateProviders TDH$TdhEnumerateProviders

#define realloc               MSVCRT$realloc
#define strlen                MSVCRT$strlen
#define _wcsicmp              MSVCRT$_wcsicmp
#define malloc                MSVCRT$malloc
#define wprintf               MSVCRT$wprintf
#define printf                MSVCRT$printf
#define strcmp                MSVCRT$strcmp
#define getchar               MSVCRT$getchar
#define wcstod                MSVCRT$wcstod
#define wcslen                MSVCRT$wcslen
#define strlen                MSVCRT$strlen

#define FilterFindFirst       Fltlib$FilterFindFirst
#define FilterFindNext        Fltlib$FilterFindNext
#endif