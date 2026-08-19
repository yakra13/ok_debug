#include <taskschd.h>
#include "..\..\common\bof_output.h"

DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
DECLSPEC_IMPORT VOID    WINAPI OLE32$CoUninitialize(void);

DECLSPEC_IMPORT VOID    WINAPI OLEAUT32$VariantInit(VARIANTARG *pvarg);
DECLSPEC_IMPORT VOID    WINAPI OLEAUT32$VariantClear(VARIANTARG *pvarg);

WINBASEAPI BOOL WINAPI KERNEL32$GetComputerNameA(LPSTR lpBuffer,LPDWORD nSize);

WINBASEAPI BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
WINBASEAPI VOID WINAPI OLEAUT32$SysFreeString(BSTR);

//
// Macros
//

#define HR_CHECK(x) HR_CHECK_GOTO(x, cleanup)
#define HR_CHECK_GOTO(x, lbl) \
    do { hr = (x); if (FAILED(hr)) { goto lbl; } } while (0);

#define SAFE_INTERFACE_RELEASE(pI) \
    do { if (pI) { pI->lpVtbl->Release(pI); pI = NULL; } } while (0);

#define SAFE_SYSFREE_STRING(x) \
    do { if (x) { OLEAUT32$SysFreeString(x); x = NULL; } } while (0);

//
// COM Identifiers
//

#define IID_TASK_SCHEDULER {0x0f87369f, 0xa4e5, 0x4cfc, {0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd}}
#define IID_IEXEC_ACTION   {0x4c3d624d, 0xfd6b, 0x49a3, {0xb9, 0xb7, 0x09, 0xcb, 0x3c, 0xd3, 0xf0, 0x47}}
#define IID_ITASK_SERVICE  {0x2faba4c7, 0x4da9, 0x4013, {0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85}}

PWCHAR TRIGGER_TYPE_NAMES_LOOKUP[] = {
	L"EVENT",              //L"On an event",
	L"TIME",               //L"On a schedule",
	L"DAILY",              //L"Daily",
	L"WEEKLY",             //L"Weekly",
	L"MONTHLY",            //L"Monthly",
	L"MONTHLYDOW",         //L"MonthlyDOW",
	L"IDLE",               //L"On idle",
	L"REGISTRATION",       //L"At task creation/modification",
	L"BOOT",               //L"At startup",
	L"LOGON",              //L"At log on",
	L"INVALID",
	L"SESSION_STATE_CHANGE"//L"SessionStateChange (lock/unlock/connection)"
};
