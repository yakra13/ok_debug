#include <wbemidl.h>
#include "../../common/bof_output.h"

//Go
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoSetProxyBlanket(IUnknown *pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc, OLECHAR *pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel, RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
DECLSPEC_IMPORT void WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeSecurity(PSECURITY_DESCRIPTOR pSecDesc, LONG cAuthSvc, SOLE_AUTHENTICATION_SERVICE *asAuthSvc, void *pReserved1, DWORD dwAuthnLevel, DWORD dwImpLevel, void *pAuthList, DWORD dwCapabilities, void *pReserved3);
DECLSPEC_IMPORT void WINAPI OLEAUT32$VariantClear(VARIANTARG *pvarg);
DECLSPEC_IMPORT void WINAPI OLEAUT32$VariantInit(VARIANTARG *pvarg);
WINBASEAPI BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
WINBASEAPI void WINAPI OLEAUT32$SysFreeString(BSTR);
WINBASEAPI int __cdecl MSVCRT$printf(const char * _Format,...);
WINBASEAPI unsigned long __cdecl MSVCRT$wcstoul(const wchar_t *nptr, wchar_t **endptr, int base);
WINBASEAPI int WINAPI KERNEL32$LCIDToLocaleName( LCID Locale, LPWSTR lpName, int cchName, DWORD dwFlags);

//bofstart + internal_printf + printoutput
WINBASEAPI void *__cdecl MSVCRT$calloc(size_t number, size_t size);
WINBASEAPI int WINAPI MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);
WINBASEAPI void __cdecl MSVCRT$memset(void *dest, int c, size_t count);
WINBASEAPI void* WINAPI MSVCRT$memcpy(void* dest, const void* src, size_t count);
WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
WINBASEAPI void __cdecl MSVCRT$free(void *memblock);
WINBASEAPI BOOL WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);

WINBASEAPI BOOL WINAPI KERNEL32$GetComputerNameA(LPSTR lpBuffer,LPDWORD nSize);

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

#define FAIL_GET_PROP_STRING L"[Failed to get property]"

#define CLSID_WBEM_LOCATOR {0x4590f811, 0x1d3a, 0x11d0, {0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}}
#define IIDI_WBEM_LOCATOR  {0xdc12a687, 0x737f, 0x11cf, {0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}}
