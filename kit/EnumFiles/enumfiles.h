#pragma once
#include "base/helpers.h"

// #include "../../common/bof_output.h"
#include <Windows.h>
#include <stdio.h>
#include <stdbool.h>
//
extern "C" {
#ifndef _DEBUG
    WINBASEAPI int    __cdecl MSVCRT$fclose(FILE *_File);
    WINBASEAPI FILE*  WINAPI  MSVCRT$fopen(const char* filename, const char* mode);
    WINBASEAPI size_t __cdecl MSVCRT$fread(void * _DstBuf, size_t _ElementSize, size_t _Count, FILE * _File);
    WINBASEAPI int    __cdecl MSVCRT$fseek(FILE *_File, long _Offset, int _Origin);
    WINBASEAPI long   __cdecl MSVCRT$ftell(FILE *_File);
    
    WINBASEAPI void* __cdecl MSVCRT$calloc(size_t number, size_t size);
    WINBASEAPI void  __cdecl MSVCRT$free(void *memblock);
    WINBASEAPI void* __cdecl MSVCRT$malloc(size_t _Size);
    WINBASEAPI void* WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
    WINBASEAPI void  __cdecl MSVCRT$memset(void *dest, int c, size_t count);
    
    WINBASEAPI char*  WINAPI MSVCRT$_strdup(const char* str);
    WINBASEAPI char*  WINAPI MSVCRT$strcat(char* dest, const char* src);
    WINBASEAPI int    WINAPI MSVCRT$strcmp(const char* str1, const char* str2);
    WINBASEAPI char*  WINAPI MSVCRT$strcpy(char* dest, const char* src);
    WINBASEAPI size_t WINAPI MSVCRT$strlen(const char* str);
    WINBASEAPI int    WINAPI MSVCRT$strncmp(const char* str1, const char* str2, size_t n);
    WINBASEAPI char*  WINAPI MSVCRT$strncpy(char* dest, const char* src, size_t n);
    WINBASEAPI char*  WINAPI MSVCRT$strstr(const char* haystack, const char* needle);
    WINBASEAPI int    WINAPI MSVCRT$tolower(int c);
    
    DECLSPEC_IMPORT char* WINAPI MSVCRT$strtok(char* str, const char* delimiters);

    WINBASEAPI int WINAPI MSVCRT$sprintf(char* buffer, const char* format, ...);
    WINBASEAPI int WINAPI MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);

    DECLSPEC_IMPORT BOOL   WINAPI KERNEL32$FindClose(HANDLE hFindFile);
    DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
    DECLSPEC_IMPORT BOOL   WINAPI KERNEL32$FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
    DECLSPEC_IMPORT DWORD  WINAPI KERNEL32$GetLastError(void);
    DECLSPEC_IMPORT int    WINAPI KERNEL32$MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
    
    WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
#endif
}
#ifndef _DEBUG
    #define fclose              MSVCRT$fclose
    #define fopen               MSVCRT$fopen
    #define fread               MSVCRT$fread
    #define fseek               MSVCRT$fseek
    #define ftell               MSVCRT$ftell
    
    #define calloc              MSVCRT$calloc
    #define free                MSVCRT$free
    #define malloc              MSVCRT$malloc
    #define memcpy              MSVCRT$memcpy
    #define memset              MSVCRT$memset
    
    #define _strdup             MSVCRT$_strdup
    #define strcat              MSVCRT$strcat
    #define strcmp              MSVCRT$strcmp
    #define strcpy              MSVCRT$strcpy
    #define strlen              MSVCRT$strlen
    #define strncmp             MSVCRT$strncmp
    #define strncpy             MSVCRT$strncpy
    #define strstr              MSVCRT$strstr
    #define strtok              MSVCRT$strtok
    #define tolower             MSVCRT$tolower
    
    #define sprintf             MSVCRT$sprintf
    #define vsnprintf           MSVCRT$vsnprintf

    #define FindClose           KERNEL32$FindClose
    #define FindFirstFileA      KERNEL32$FindFirstFileA
    #define FindNextFileA       KERNEL32$FindNextFileA
    #define GetLastError        KERNEL32$GetLastError
    #define MultiByteToWideChar KERNEL32$MultiByteToWideChar
    
    #define GetProcessHeap      KERNEL32$GetProcessHeap
    #define HeapAlloc           KERNEL32$HeapAlloc
    #define HeapFree            KERNEL32$HeapFree
#endif