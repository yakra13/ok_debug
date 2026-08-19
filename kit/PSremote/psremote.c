#include <windows.h>
#include <stdio.h>
#include <wtsapi32.h>
#include "beacon.h"
#include "psremote.h"


//START TrustedSec BOF print code: https://github.com/trustedsec/CS-Situational-Awareness-BOF/blob/master/src/common/base.c
#ifndef bufsize
#define bufsize 8192
#endif
char *output = 0;  
WORD currentoutsize = 0;
HANDLE trash = NULL; 
int bofstart();
void internal_printf(const char* format, ...);
void printoutput(BOOL done);

int bofstart() {   
    output = (char*)MSVCRT$calloc(bufsize, 1);
    currentoutsize = 0;
    return 1;
}

void internal_printf(const char* format, ...){
    int buffersize = 0;
    int transfersize = 0;
    char * curloc = NULL;
    char* intBuffer = NULL;
    va_list args;
    va_start(args, format);
    buffersize = MSVCRT$vsnprintf(NULL, 0, format, args); 
    va_end(args);
    
    if (buffersize == -1) return;
    
    char* transferBuffer = (char*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, bufsize);
	intBuffer = (char*)KERNEL32$HeapAlloc(KERNEL32$GetProcessHeap(), HEAP_ZERO_MEMORY, buffersize);
    va_start(args, format);
    MSVCRT$vsnprintf(intBuffer, buffersize, format, args); 
    va_end(args);
    if(buffersize + currentoutsize < bufsize) 
    {
        MSVCRT$memcpy(output+currentoutsize, intBuffer, buffersize);
        currentoutsize += buffersize;
    } else {
        curloc = intBuffer;
        while(buffersize > 0)
        {
            transfersize = bufsize - currentoutsize;
            if(buffersize < transfersize) 
            {
                transfersize = buffersize;
            }
            MSVCRT$memcpy(output+currentoutsize, curloc, transfersize);
            currentoutsize += transfersize;
            if(currentoutsize == bufsize)
            {
                printoutput(FALSE); 
            }
            MSVCRT$memset(transferBuffer, 0, transfersize); 
            curloc += transfersize; 
            buffersize -= transfersize;
        }
    }
	KERNEL32$HeapFree(KERNEL32$GetProcessHeap(), 0, intBuffer);
	KERNEL32$HeapFree(KERNEL32$GetProcessHeap(), 0, transferBuffer);
}

void printoutput(BOOL done) {
    char * msg = NULL;
    BeaconOutput(CALLBACK_OUTPUT, output, currentoutsize);
    currentoutsize = 0;
    MSVCRT$memset(output, 0, bufsize);
    if(done) {MSVCRT$free(output); output=NULL;}
}
//END TrustedSec BOF print code.


BOOL ListProcesses(HANDLE handleTargetHost)
{
	WTS_PROCESS_INFOA* processes;
	DWORD pi_count = 0;

	if (!WTSAPI32$WTSEnumerateProcessesA(handleTargetHost, 0, 1, &processes, &pi_count))
    {
		BeaconPrintf(CALLBACK_ERROR, "Failed to get a valid handle to the specified host!\n");
		return FALSE;
	}

    internal_printf("  remote_process_list:\n");

    for (int i = 0; i < pi_count; i++)
    {
        internal_printf("    - { pid: %5d,", processes[i].ProcessId);
        internal_printf(" session: %3d,", processes[i].SessionId);
        internal_printf(" name: %-30s,", processes[i].pProcessName[0] != '\0' ? processes[i].pProcessName : "null");

        LPSTR sidString = NULL;

        if (ADVAPI32$ConvertSidToStringSidA(processes[i].pUserSid, &sidString))
        {
            internal_printf(" sid: %s", sidString);
            KERNEL32$LocalFree(sidString);
        }
        else
        {
            internal_printf(" sid: null");
        }
        
        internal_printf(" }\n");
	}
    
    WTSAPI32$WTSFreeMemory(processes);
	
    WTSAPI32$WTSCloseServer(handleTargetHost);
	
    return TRUE;
}


int go(char *args, int len)
{
	CHAR *hostName = "";
	datap parser;
	DWORD argSize = NULL;
	HANDLE handleTargetHost = NULL;
	BOOL res = NULL;

	BeaconDataParse(&parser, args, len);
	hostName = BeaconDataExtract(&parser, &argSize);
	if(!bofstart()) return;


	handleTargetHost = WTSAPI32$WTSOpenServerA(hostName);

    internal_printf("%s:\n", hostName);

	res = ListProcesses(handleTargetHost);

	if (!res)
    {
		BeaconPrintf(CALLBACK_ERROR, "Couldn't list remote processes. Do you have enough privileges on the target host?\n");
	}
	else
    {
		printoutput(TRUE);
		// BeaconPrintf(CALLBACK_OUTPUT, "[+] Finished enumerating.");
	}
	
    return 0;
}

