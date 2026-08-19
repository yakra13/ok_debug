#pragma once
#include "beacon.h"
#include "base/helpers.h"
#include <Windows.h>
#include <stdio.h>

#include <oleauto.h>

#pragma comment(lib, "OleAut32.lib")

extern "C"
{
    #ifndef _DEBUG
        WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
        WINBASEAPI LPVOID WINAPI KERNEL32$HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
        WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
        WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();

        WINBASEAPI PVOID WINAPI MSVCRT$memcpy(void* dest, const void* src, size_t count);
        WINBASEAPI INT   WINAPI MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);
        
        WINBASEAPI UINT WINAPI OLEAUT32$SysStringLen(BSTR bstr);

        #define HeapAlloc      KERNEL32$HeapAlloc      
        #define HeapReAlloc    KERNEL32$HeapReAlloc    
        #define HeapFree       KERNEL32$HeapFree       
        #define GetProcessHeap KERNEL32$GetProcessHeap 

        #define memcpy         MSVCRT$memcpy           
        #define vsnprintf      MSVCRT$vsnprintf        

        #define SysStringLen   OLEAUT32$SysStringLen
    #endif

    #ifndef BOF_OUTPUT_BUFFER_SIZE
        #define BOF_OUTPUT_BUFFER_SIZE 8192
    #endif

    typedef struct
    {
        PCHAR buffer;
        SIZE_T used;
        SIZE_T capacity;

    } BOF_Buffer;

    static inline BOOL BofBufferInit(BOF_Buffer* bofOut);
    static inline BOOL BofBufferAppend(BOF_Buffer* bofOut, const PCHAR data, SIZE_T length);
    static inline BOOL BofPrintf(BOF_Buffer* bofOut, const CHAR* format, ...);
    static inline BOOL BofBufferFlush(BOF_Buffer* buffer);
    static inline VOID BofBufferFree(BOF_Buffer* buffer);

    static inline BOOL BofBufferInit(BOF_Buffer* bofOut)
    {
        if (bofOut == NULL)
        {
            return FALSE;
        }

        bofOut->buffer = (PCHAR)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            BOF_OUTPUT_BUFFER_SIZE
        );

        if (bofOut->buffer == NULL)
        {
            return FALSE;
        }

        bofOut->used = 0;
        bofOut->capacity = BOF_OUTPUT_BUFFER_SIZE;

        return TRUE;
    }

    static inline VOID BofBufferFree(BOF_Buffer* buffer)
    {
        if (buffer == NULL)
        {
            return;
        }

        // Flush remaining output
        BofBufferFlush(buffer);

        if (buffer->buffer != NULL)
        {
            HeapFree(
                GetProcessHeap(),
                0,
                buffer->buffer
            );

            buffer->buffer = NULL;
        }

        buffer->used = 0;
        buffer->capacity = 0;
    }

    static inline BOOL BofPrintf(BOF_Buffer* bofOut, const CHAR* format, ...)
    {
        BOOL bResult = FALSE;
        INT length;
        PCHAR szTemp = NULL;

        HANDLE hHeap = NULL;

        va_list args;

        if (bofOut == NULL || bofOut->buffer == NULL || format == NULL)
        {
            return FALSE;
        }

        va_start(args, format);
        length = vsnprintf(NULL, 0, format, args);
        va_end(args);

        if (length < 0)
        {
            return FALSE;
        }

        hHeap = GetProcessHeap();

        szTemp = (PCHAR)HeapAlloc(
            hHeap,
            0,
            (SIZE_T)length + 1
        );

        if (szTemp == NULL)
        {
            return FALSE;
        }

        va_start(args, format);
        vsnprintf(szTemp, length + 1, format, args);
        va_end(args);

        bResult = BofBufferAppend(bofOut, szTemp, (SIZE_T)length);

        HeapFree(hHeap, 0, szTemp);

        return bResult;
    }

    static inline BOOL BofBufferAppend(BOF_Buffer* buffer, PCHAR data, SIZE_T length)
    {
        SIZE_T cBytesToCopy = 0;

        if (buffer == NULL || buffer->buffer == NULL || data == NULL)
        {
            return FALSE;
        }

        while (length > 0)
        {
            // Free space remaining
            cBytesToCopy = buffer->capacity - buffer->used;

            // Flush if buffer is full
            if (cBytesToCopy == 0)
            {
                BofBufferFlush(buffer);
                cBytesToCopy = buffer->capacity;
            }

            // Don't copy more than remaining
            if (cBytesToCopy > length)
            {
                cBytesToCopy = length;
            }

            memcpy(
                buffer->buffer + buffer->used,
                data,
                cBytesToCopy
            );

            buffer->used += cBytesToCopy;

            data += cBytesToCopy;
            length -= cBytesToCopy;
        }

        return TRUE;
    }


    static inline BOOL BofBufferFlush(BOF_Buffer* buffer)
    {
        if (buffer == NULL || buffer->buffer == NULL)
        {
            return FALSE;
        }

        if (buffer->used == 0)
        {
            return TRUE;
        }

        BeaconOutput(CALLBACK_OUTPUT, buffer->buffer, (INT)buffer->used);

        buffer->used = 0;

        return TRUE;
    }

    //
    // Utility functions
    //

    /// @brief Copies a BSTR to a PWCHAR. Caller must call HeapFree.
    /// @param src 
    /// @param dst 
    /// @return HRESULT
    HRESULT CopyBSTRToWString(BSTR src, PWCHAR* dst)
    {
        SIZE_T len;

        if (dst == NULL)
        {
            return E_INVALIDARG;
        }

        *dst = NULL;

        if (src == NULL)
        {
            return S_OK;
        }

        len = SysStringLen(src);

        *dst = (PWCHAR)HeapAlloc(
            GetProcessHeap(),
            HEAP_ZERO_MEMORY,
            (len + 1) * sizeof(WCHAR)
        );

        if (*dst == NULL)
        {
            return E_OUTOFMEMORY;
        }

        memcpy(
            *dst,
            src,
            (len + 1) * sizeof(WCHAR));

        (*dst)[len] = L'\0';

        return S_OK;
    }
}