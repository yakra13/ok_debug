#include "../../common/beacon.h"

//IsElevated
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$OpenProcessToken(HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle);
DECLSPEC_IMPORT BOOL WINAPI ADVAPI32$GetTokenInformation(HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength);
DECLSPEC_IMPORT BOOL WINAPI KERNEL32$CloseHandle(HANDLE hObject);
DECLSPEC_IMPORT HANDLE WINAPI KERNEL32$GetCurrentProcess(void);

//CreateScheduledTask
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
DECLSPEC_IMPORT void WINAPI OLE32$CoUninitialize(void);
DECLSPEC_IMPORT HRESULT WINAPI OLE32$CoCreateInstance (REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
DECLSPEC_IMPORT void WINAPI OLEAUT32$VariantInit(VARIANTARG *pvarg);
DECLSPEC_IMPORT void WINAPI OLEAUT32$VariantClear(VARIANTARG *pvarg);
WINBASEAPI BSTR WINAPI OLEAUT32$SysAllocString(const OLECHAR *);
WINBASEAPI void WINAPI OLEAUT32$SysFreeString(BSTR);
WINBASEAPI int __cdecl MSVCRT$printf(const char * _Format,...);
WINBASEAPI int __cdecl MSVCRT$strcmp(const char *str1, const char *str2);
WINBASEAPI int __cdecl MSVCRT$wcscmp(const wchar_t* str1, const wchar_t* str2);


#define HR_CHECK(x) HR_CHECK_GOTO(x, cleanup)
#define HR_CHECK_GOTO(x, lbl) \
    do { hr = (x); if (FAILED(hr)) { goto lbl; } } while (0);

#define SAFE_SYSFREE_STRING(x) \
    do { if (x) { OLEAUT32$SysFreeString(x); x = NULL; } } while (0);

#define SAFE_INTERFACE_RELEASE(pI) \
    do { if (pI) { pI->lpVtbl->Release(pI); } } while (0);

typedef struct
{
    TASK_TRIGGER_TYPE2 triggerType;
    TASK_SESSION_STATE_CHANGE_TYPE changeType;
    PWCHAR wszTaskName;
    PWCHAR wszHostName;
    PWCHAR wszProgramPath;
    PWCHAR wszProgramArgs;
    PWCHAR wszStartTime;
    PWCHAR wszExpireTime;
    INT daysInterval;
    PWCHAR wszDelay;
    PWCHAR wszUserId;
    PWCHAR wszRepeatTask;
} TaskConfig;

#define IID_TASK_SCHEDULER {0x0f87369f, 0xa4e5, 0x4cfc, {0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd}}
#define IID_IEXEC_ACTION   {0x4c3d624d, 0xfd6b, 0x49a3, {0xb9, 0xb7, 0x09, 0xcb, 0x3c, 0xd3, 0xf0, 0x47}}
#define IID_ITASK_SERVICE  {0x2faba4c7, 0x4da9, 0x4013, {0x96, 0x97, 0x20, 0xcc, 0x3f, 0xd4, 0x0f, 0x85}}

const IID task_lookup[] =
{
    {0xD45B0167, 0x9653, 0x4EEF, {0xB9, 0x4F, 0x07, 0x32, 0xCA, 0x7A, 0xF2, 0x51}}, // TASK_TRIGGER_EVENT
    {0xb45747e0, 0xeba7, 0x4276, {0x9f, 0x29, 0x85, 0xc5, 0xbb, 0x30, 0x00, 0x06}}, // TASK_TRIGGER_TIME
    {0x126c5cd8, 0xb288, 0x41d5, {0x8d, 0xbf, 0xe4, 0x91, 0x44, 0x6a, 0xdc, 0x5c}}, // TASK_TRIGGER_DAILY
    {0x5038fc98, 0x82ff, 0x436d, {0x87, 0x28, 0xa5, 0x12, 0xa5, 0x7c, 0x9d, 0xc1}}, // TASK_TRIGGER_WEEKLY
    {0x97c45ef1, 0x6b02, 0x4a1a, {0x9c, 0x0e, 0x1e, 0xbf, 0xba, 0x15, 0x00, 0xac}}, // TASK_TRIGGER_MONTHLY
    {0x77d025a3, 0x90fa, 0x43aa, {0xb5, 0x2e, 0xcd, 0xa5, 0x49, 0x9b, 0x94, 0x6a}}, // TASK_TRIGGER_MONTHLYDOW
    {0xd537d2b0, 0x9fb3, 0x4d34, {0x97, 0x39, 0x1f, 0xf5, 0xce, 0x7b, 0x1e, 0xf3}}, // TASK_TRIGGER_IDLE
    {0x4c8fec3a, 0xc218, 0x4e0c, {0xb2, 0x3d, 0x62, 0x90, 0x24, 0xdb, 0x91, 0xa2}}, // TASK_TRIGGER_REGISTRATION
    {0x2a9c35da, 0xd357, 0x41f4, {0xbb, 0xc1, 0x20, 0x7a, 0xc1, 0xb1, 0xf3, 0xcb}}, // TASK_TRIGGER_BOOT
    {0x72dade38, 0xfae4, 0x4b3e, {0xba, 0xf4, 0x5d, 0x00, 0x9a, 0xf0, 0x2b, 0x1c}}, // TASK_TRIGGER_LOGON
    {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}, // Empty, there is not a task at value 10
    {0x754da71b, 0x4385, 0x4475, {0x9d, 0xd9, 0x59, 0x82, 0x94, 0xfa, 0x36, 0x41}}  // TASK_TRIGGER_SESSION_STATE_CHANGE
};


