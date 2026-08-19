#pragma once
#include <wbemidl.h>
#include "../../common/bof_output.h"

DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
DECLSPEC_IMPORT VOID    WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoSetProxyBlanket(IUnknown *pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc, OLECHAR *pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities);

DECLSPEC_IMPORT VOID    WINAPI OLEAUT32$VariantClear(VARIANTARG *pvarg);
DECLSPEC_IMPORT VOID    WINAPI OLEAUT32$VariantInit(VARIANTARG *pvarg);

WINBASEAPI BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
WINBASEAPI VOID WINAPI OLEAUT32$SysFreeString(BSTR);

WINBASEAPI SIZE_T __cdecl MSVCRT$mbstowcs(wchar_t *wcstr, const char *mbstr, size_t count);
WINBASEAPI INT    __cdecl MSVCRT$_snwprintf(wchar_t *buffer, size_t count, const wchar_t *format, ...);

//
// Macros
//

#define HR_CHECK(hr, x) HR_CHECK_GOTO(hr, x, cleanup)
#define HR_CHECK_GOTO(hr, x, lbl) \
    do { hr = (x); if (FAILED(hr)) { goto lbl; } } while (0);

#define SAFE_INTERFACE_RELEASE(pI) \
    do { if (pI) { pI->lpVtbl->Release(pI); pI = NULL; } } while (0);

#define SAFE_SYSFREE_STRING(x) \
    do { if (x) { OLEAUT32$SysFreeString(x); x = NULL; } } while (0);

//
// COM Identifiers
//

#define CLSID_WBEM_LOCATOR {0x4590f811, 0x1d3a, 0x11d0, {0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}}
#define IIDI_WBEM_LOCATOR  {0xdc12a687, 0x737f, 0x11cf, {0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}}


//
// Data structures
//

typedef struct
{
    DWORD pid;
    DWORD ppid;
    DWORD session;

    PWCHAR name;
    PWCHAR path;
    PWCHAR command;
    PWCHAR creation_time;

    // PWCHAR owner;
    // PWCHAR owner_domain;
    // PWCHAR owner_sid;

    // DWORD thread_count;
    // DWORD handle_count;
    // SIZE_T working_set;
} ProcessInfo;