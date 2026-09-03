#pragma once
#include "base/helpers.h"

#include <Windows.h>
#include <taskschd.h>
#include <comutil.h> // C++ for _variant_t

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comsuppw.lib") // C++ for _variant_t

//
// Macros
//

#define HR_CHECK(x) HR_CHECK_GOTO(x, cleanup)
#define HR_CHECK_GOTO(x, lbl) \
    do { hr = (x); if (FAILED(hr)) { goto lbl; } } while (0);

#define SAFE_INTERFACE_RELEASE(pI) \
    do { if (pI) { pI->Release(); pI = NULL; } } while (0);

#define SAFE_SYSFREE_STRING(x) \
    do { if (x) { SysFreeString(x); x = NULL; } } while (0);

//
// COM Identifiers
//

#define IID_TASK_SCHEDULER {0x0f87369f, 0xa4e5, 0x4cfc, {0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd}}
#define IID_IEXEC_ACTION   {0x4c3d624d, 0xfd6b, 0x49a3, {0xb9, 0xb7, 0x09, 0xcb, 0x3c, 0xd3, 0xf0, 0x47}}
#define IID_ITASK_SERVICE  {0x2faba4c7, 0x4da9, 0x4013, {0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85}}

const WCHAR* TRIGGER_TYPE_NAMES_LOOKUP[] = {
	L"EVENT",              //L"On an event",
	L"TIME",               //L"On a schedule", once
	L"DAILY",              //L"Daily",
	L"WEEKLY",             //L"Weekly",
	L"MONTHLY",            //L"Monthly",
	L"MONTHLYDOW",         //L"MonthlyDOW",
	L"IDLE",               //L"On idle",
	L"REGISTRATION",       //L"At task creation/modification",
	L"BOOT",               //L"At startup",
	L"LOGON",              //L"At log on",
	L"INVALID",
	L"SESSION_STATE_CHANGE",//L"SessionStateChange (lock/unlock/connection)"
	L"CUSTOM_TRIGGER_01"
};

//
// Imports
//
extern "C" {
#ifndef _DEBUG
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
DECLSPEC_IMPORT VOID    WINAPI OLE32$CoUninitialize(void);

DECLSPEC_IMPORT VOID    WINAPI OLEAUT32$VariantInit(VARIANTARG *pvarg);
DECLSPEC_IMPORT VOID    WINAPI OLEAUT32$VariantClear(VARIANTARG *pvarg);

WINBASEAPI BOOL WINAPI KERNEL32$GetComputerNameA(LPSTR lpBuffer,LPDWORD nSize);

WINBASEAPI BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
WINBASEAPI VOID WINAPI OLEAUT32$SysFreeString(BSTR);

DECLSPEC_IMPORT int WINAPI KERNEL32$MultiByteToWideChar(UINT CodePage, DWORD dwFlags, _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);

WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
#endif
}
#ifndef _DEBUG
#define CoCreateInstance 	OLE32$CoCreateInstance
#define CoInitializeEx 		OLE32$CoInitializeEx
#define CoUninitialize 		OLE32$CoUninitialize

#define VariantInit 		OLEAUT32$VariantInit
#define VariantClear 		OLEAUT32$VariantClear

#define GetComputerNameA 	KERNEL32$GetComputerNameA

#define SysAllocString 		OLEAUT32$SysAllocString
#define SysFreeString 		OLEAUT32$SysFreeString

#define MultiByteToWideChar KERNEL32$MultiByteToWideChar

#define GetProcessHeap 		KERNEL32$GetProcessHeap
#define HeapAlloc 			KERNEL32$HeapAlloc
#define HeapFree 			KERNEL32$HeapFree
#endif

