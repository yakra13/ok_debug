#pragma once
#include "base/helpers.h"

#include <windows.h>
#include <wincrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Advapi32.lib")


//
// Imports
//
extern "C" {
#ifndef _DEBUG
//printCertProperties
DECLSPEC_IMPORT int    WINAPI KERNEL32$GetDateFormatW(LCID Locale, DWORD dwFlags, const SYSTEMTIME *lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate);
DECLSPEC_IMPORT BOOL   WINAPI KERNEL32$FileTimeToSystemTime(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);
DECLSPEC_IMPORT HLOCAL WINAPI KERNEL32$LocalAlloc(UINT uFlags, SIZE_T uBytes);
DECLSPEC_IMPORT HLOCAL WINAPI KERNEL32$LocalFree(HLOCAL hMem);

DECLSPEC_IMPORT HCERTSTORE WINAPI CRYPT32$CertOpenSystemStoreW(HCRYPTPROV hProv, LPCWSTR szSubsystemProtocol);
DECLSPEC_IMPORT HCERTSTORE WINAPI CRYPT32$CertOpenStore(LPCWSTR lpszStoreProvider, DWORD dwEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags, const void *pvPara);
DECLSPEC_IMPORT HCERTSTORE WINAPI CRYPT32$CertEnumCertificatesInStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pPrevCertContext);
DECLSPEC_IMPORT BOOL       WINAPI CRYPT32$CertGetEnhancedKeyUsage(PCCERT_CONTEXT pCertContext, DWORD dwFlags, PCERT_ENHKEY_USAGE pUsage, DWORD *pcbUsage);
DECLSPEC_IMPORT BOOL       WINAPI CRYPT32$CertFreeCertificateContext(PCCERT_CONTEXT pCertContext);
DECLSPEC_IMPORT BOOL       WINAPI CRYPT32$CertCloseStore(HCERTSTORE hCertStore, DWORD dwFlags);
DECLSPEC_IMPORT BOOL       WINAPI CRYPT32$CertGetCertificateContextProperty(PCCERT_CONTEXT pCertContext, DWORD dwPropId, void *pvData, DWORD *pcbData); //TEST
DECLSPEC_IMPORT DWORD      WINAPI CRYPT32$CertGetNameStringW(PCCERT_CONTEXT pCertContext, DWORD dwType, DWORD dwFlags, void *pvTypePara, LPWSTR pszNameString, DWORD cchNameString);

DECLSPEC_IMPORT PCCRYPT_OID_INFO WINAPI CRYPT32$CryptFindOIDInfo(DWORD dwKeyType, void *pvKey, DWORD dwGroupId);

WINBASEAPI int __cdecl MSVCRT$_snwprintf_s(wchar_t * _DstBuf, size_t _DstSize, size_t _MaxCount, const wchar_t * _Format, ...);
WINBASEAPI int __cdecl MSVCRT$wprintf(const wchar_t * _Format, ...);
WINBASEAPI int WINAPI  MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);

WINBASEAPI void* __cdecl MSVCRT$calloc(size_t number, size_t size);
WINBASEAPI void __cdecl  MSVCRT$free(void *memblock);
WINBASEAPI void* WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
WINBASEAPI void  __cdecl MSVCRT$memset(void *dest, int c, size_t count);

WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);

WINBASEAPI int    WINAPI KERNEL32$MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);

#endif
}
#ifndef _DEBUG
#define GetDateFormatW                    KERNEL32$GetDateFormatW
#define FileTimeToSystemTime              KERNEL32$FileTimeToSystemTime
#define LocalAlloc                        KERNEL32$LocalAlloc
#define LocalFree                         KERNEL32$LocalFree

#define CertOpenSystemStoreW              CRYPT32$CertOpenSystemStoreW
#define CertOpenStore                     CRYPT32$CertOpenStore
#define CertEnumCertificatesInStore       CRYPT32$CertEnumCertificatesInStore
#define CertGetEnhancedKeyUsage           CRYPT32$CertGetEnhancedKeyUsage
#define CertFreeCertificateContext        CRYPT32$CertFreeCertificateContext
#define CertCloseStore                    CRYPT32$CertCloseStore
#define CertGetCertificateContextProperty CRYPT32$CertGetCertificateContextProperty
#define CertGetNameStringW                CRYPT32$CertGetNameStringW
#define CryptFindOIDInfo                  CRYPT32$CryptFindOIDInfo

#define _snwprintf_s                      MSVCRT$_snwprintf_s
#define wprintf                           MSVCRT$wprintf
#define vsnprintf                         MSVCRT$vsnprintf
#define calloc                            MSVCRT$calloc
#define free                              MSVCRT$free
#define memcpy                            MSVCRT$memcpy
#define memset                            MSVCRT$memset

#define GetProcessHeap                    KERNEL32$GetProcessHeap
#define HeapAlloc                         KERNEL32$HeapAlloc
#define HeapFree                          KERNEL32$HeapFree

#define MultiByteToWideChar               KERNEL32$MultiByteToWideChar
#endif
