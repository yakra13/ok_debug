#pragma once
#include "base/helpers.h"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <winsock2.h>
#include <windows.h>

#include <iphlpapi.h>


#ifdef BOF
#include <ws2ipdef.h>
#else
#include <ws2tcpip.h>
#endif

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")

// #include "bofdefs.h"
// #include "beacon.h"
// #ifndef bufsize
// #define bufsize 8192
// #endif


//
// Imports
//
extern "C" {
char * Utf16ToUtf8(const wchar_t* input);

#ifndef _DEBUG


WINBASEAPI int __cdecl MSVCRT$sprintf(char * _DstBuf, const char * _Format, ...);
WINBASEAPI char * __cdecl MSVCRT$strcpy(char *destination, const char *source);
WINBASEAPI size_t __cdecl MSVCRT$wcstombs(char *mbstr, const wchar_t *wcstr, size_t count );
WINBASEAPI PCWSTR WSAAPI WS2_32$InetNtopW(INT Family, const VOID *pAddr, PWSTR pStringBuf, size_t StringBufSize);

WINBASEAPI BOOL WINAPI KERNEL32$FileTimeToLocalFileTime(const FILETIME *lpFileTime, LPFILETIME lpLocalFileTime);
WINBASEAPI BOOL WINAPI KERNEL32$FileTimeToSystemTime(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime);

WINADVAPI LSTATUS APIENTRY ADVAPI32$RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
WINADVAPI LSTATUS APIENTRY ADVAPI32$RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
WINADVAPI LSTATUS APIENTRY ADVAPI32$RegCloseKey(HKEY hKey);

WINBASEAPI char* __cdecl MSVCRT$strtok_s(char *strToken, const char *strDelimit, char **context);
WINBASEAPI int __cdecl MSVCRT$strcmp(const char *string1, const char *string2);

WINBASEAPI ULONG WINAPI IPHLPAPI$GetAdaptersAddresses(ULONG Family, ULONG Flags, PVOID Reserved, PIP_ADAPTER_ADDRESSES AdapterAddresses, PULONG SizePointer);
WINBASEAPI ULONG WINAPI IPHLPAPI$GetNetworkParams(PFIXED_INFO pFixedInfo, PULONG pOutBufLen);

WINBASEAPI int WINAPI KERNEL32$WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar);




#define intAlloc(size) KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, size)
#define intFree(addr) KERNEL32$HeapFree(KERNEL32$GetProcessHeap(), 0, addr)
#else
#define intAlloc(size) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)
#define intFree(addr) HeapFree(GetProcessHeap(), 0, addr)
#endif
}

#ifndef _DEBUG

#define sprintf     MSVCRT$sprintf
#define strcpy      MSVCRT$strcpy
#define wcstombs    MSVCRT$wcstombs
#define InetNtopW   WS2_32$InetNtopW

#define FileTimeToLocalFileTime KERNEL32$FileTimeToLocalFileTime
#define FileTimeToSystemTime    KERNEL32$FileTimeToSystemTime

#define RegOpenKeyExA       ADVAPI32$RegOpenKeyExA
#define RegQueryValueExA    ADVAPI32$RegQueryValueExA
#define RegCloseKey         ADVAPI32$RegCloseKey

#define strtok_s            MSVCRT$strtok_s
#define strcmp              MSVCRT$strcmp

#define GetAdaptersAddresses    IPHLPAPI$GetAdaptersAddresses
#define GetNetworkParams        IPHLPAPI$GetNetworkParams

#define WideCharToMultiByte     KERNEL32$WideCharToMultiByte

#endif

// char * output __attribute__((section (".data"))) = 0;  // this is just done so its we don't go into .bss which isn't handled properly
// WORD currentoutsize __attribute__((section (".data"))) = 0;
// HANDLE trash __attribute__((section (".data"))) = NULL; // Needed for x64 to not give relocation error

// #ifdef BOF
// int bofstart();
// void internal_printf(const char* format, ...);
// void printoutput(BOOL done);
// #endif

// #ifdef BOF
// int bofstart()
// {   
//     output = (char*)MSVCRT$calloc(bufsize, 1);
//     currentoutsize = 0;
//     return 1;
// }

// void internal_printf(const char* format, ...){
//     int buffersize = 0;
//     int transfersize = 0;
//     char * curloc = NULL;
//     char* intBuffer = NULL;
//     va_list args;
//     va_start(args, format);
//     buffersize = MSVCRT$vsnprintf(NULL, 0, format, args); // +1 because vsprintf goes to buffersize-1 , and buffersize won't return with the null
//     va_end(args);
    
//     // vsnprintf will return -1 on encoding failure (ex. non latin characters in Wide string)
//     if (buffersize == -1)
//         return;
    
//     char* transferBuffer = (char*)intAlloc(bufsize);
//     intBuffer = (char*)intAlloc(buffersize);
//     /*Print string to memory buffer*/
//     va_start(args, format);
//     MSVCRT$vsnprintf(intBuffer, buffersize, format, args); // tmpBuffer2 has a null terminated string
//     va_end(args);
//     if(buffersize + currentoutsize < bufsize) // If this print doesn't overflow our output buffer, just buffer it to the end
//     {
//         //BeaconFormatPrintf(&output, intBuffer);
//         memcpy(output+currentoutsize, intBuffer, buffersize);
//         currentoutsize += buffersize;
//     }
//     else // If this print does overflow our output buffer, lets print what we have and clear any thing else as it is likely this is a large print
//     {
//         curloc = intBuffer;
//         while(buffersize > 0)
//         {
//             transfersize = bufsize - currentoutsize; // what is the max we could transfer this request
//             if(buffersize < transfersize) //if I have less then that, lets just transfer what's left
//             {
//                 transfersize = buffersize;
//             }
//             memcpy(output+currentoutsize, curloc, transfersize); // copy data into our transfer buffer
//             currentoutsize += transfersize;
//             //BeaconFormatPrintf(&output, transferBuffer); // copy it to cobalt strikes output buffer
//             if(currentoutsize == bufsize)
//             {
//             printoutput(FALSE); // sets currentoutsize to 0 and prints
//             }
//             memset(transferBuffer, 0, transfersize); // reset our transfer buffer
//             curloc += transfersize; // increment by how much data we just wrote
//             buffersize -= transfersize; // subtract how much we just wrote from how much we are writing overall
//         }
//     }
//     intFree(intBuffer);
//     intFree(transferBuffer);
// }

// void printoutput(BOOL done)
// {

//     char * msg = NULL;
//     BeaconOutput(CALLBACK_OUTPUT, output, currentoutsize);
//     currentoutsize = 0;
//     memset(output, 0, bufsize);
//     if(done) {MSVCRT$free(output); output=NULL;}
// }
// #else
// #define internal_printf printf
// #define printoutput 
// #define bofstart 
// #endif

// Changes to address issue #65.
// We can't use more dynamic resolve functions in this file, which means a call to HeapRealloc is unacceptable.
// To that end if you're going to use this function, declare how many libraries you'll be loading out of, multiple functions out of 1 library count as one
// Normallize your library name to uppercase, yes I could do it, yes I'm also lazy and putting that on the developer.
// Finally I'm going to assume actual string constants are passed in, which is to say don't pass in something to this you plan to free yourself
// If you must then free it after bofstop is called
// #ifdef DYNAMIC_LIB_COUNT


// typedef struct loadedLibrary {
//     HMODULE hMod; // mod handle
//     const char * name; // name normalized to uppercase
// }loadedLibrary, *ploadedLibrary;
// loadedLibrary loadedLibraries[DYNAMIC_LIB_COUNT] __attribute__((section (".data"))) = {0};
// DWORD loadedLibrariesCount __attribute__((section (".data"))) = 0;

// BOOL intstrcmp(LPCSTR szLibrary, LPCSTR sztarget)
// {
//     BOOL bmatch = FALSE;
//     DWORD pos = 0;
//     while(szLibrary[pos] && sztarget[pos])
//     {
//         if(szLibrary[pos] != sztarget[pos])
//         {
//             goto end;
//         }
//         pos++;
//     }
//     if(szLibrary[pos] | sztarget[pos]) // if either of these down't equal null then they can't match
//         {goto end;}
//     bmatch = TRUE;

//     end:
//     return bmatch;
// }

// //GetProcAddress, LoadLibraryA, GetModuleHandle, and FreeLibrary are gimmie functions
// //
// // DynamicLoad
// // Retrieves a function pointer given the BOF library-function name
// // szLibrary           - The library containing the function you want to load
// // szFunction          - The Function that you want to load
// // Returns a FARPROC function pointer if successful, or NULL if lookup fails
// //
// FARPROC DynamicLoad(const char * szLibrary, const char * szFunction)
// {
//     FARPROC fp = NULL;
//     HMODULE hMod = NULL;
//     DWORD i = 0;
//     DWORD liblen = 0;
//     for(i = 0; i < loadedLibrariesCount; i++)
//     {
//         if(intstrcmp(szLibrary, loadedLibraries[i].name))
//         {
//             hMod = loadedLibraries[i].hMod;
//         }
//     }
//     if(!hMod)
//     {
//         hMod = LoadLibraryA(szLibrary);
//         if(!hMod){ 
//             BeaconPrintf(CALLBACK_ERROR, "*** DynamicLoad(%s) FAILED!\nCould not find library to load.", szLibrary);
//             return NULL;
//         }
//         loadedLibraries[loadedLibrariesCount].hMod = hMod;
//         loadedLibraries[loadedLibrariesCount].name = szLibrary; //And this is why this HAS to be a constant or not freed before bofstop
//         loadedLibrariesCount++;
//     }
//     fp = GetProcAddress(hMod, szFunction);

//     if (NULL == fp)
//     {
//         BeaconPrintf(CALLBACK_ERROR, "*** DynamicLoad(%s) FAILED!\n", szFunction);
//     }
//     return fp;
// }
// #endif


// char* Utf16ToUtf8(const wchar_t* input)
// {
//     int ret = Kernel32$WideCharToMultiByte(
//         CP_UTF8,
//         0,
//         input,
//         -1,
//         NULL,
//         0,
//         NULL,
//         NULL
//     );

//     char* newString = (char*)intAlloc(sizeof(char) * ret);

//     ret = Kernel32$WideCharToMultiByte(
//         CP_UTF8,
//         0,
//         input,
//         -1,
//         newString,
//         sizeof(char) * ret,
//         NULL,
//         NULL
//     );

//     if (0 == ret)
//     {
//         goto fail;
//     }

// retloc:
//     return newString;
// /*location to free everything centrally*/
// fail:
//     if (newString){
//         intFree(newString);
//         newString = NULL;
//     };
//     goto retloc;
// }

//release any global functions here
// void bofstop()
// {
// #ifdef DYNAMIC_LIB_COUNT
//     DWORD i;
//     for(i = 0; i < loadedLibrariesCount; i++)
//     {
//         FreeLibrary(loadedLibraries[i].hMod);
//     }
// #endif
// 	return;
// }
