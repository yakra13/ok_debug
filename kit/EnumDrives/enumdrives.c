// #include <windows.h>
// #include <stdio.h>
#include "enumdrives.h"
// #include "beacon.h"

BOF_Buffer buffer = {0};

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
//END TrustedSec BOF print code.


VOID printDriveType(const PCHAR drive)
{
    UINT driveType = KERNEL32$GetDriveTypeA(drive);
	
    if (driveType == DRIVE_UNKNOWN)
    {
        // internal_printf("%s\t[Unknown drive type]\n", drive);
        BofPrintf(&buffer, "%s\t[Unknown drive type]\n", drive);
    }
    else if (driveType == DRIVE_NO_ROOT_DIR)
    {
        // internal_printf("%s\t[Invalid root path]\n", drive);
        BofPrintf(&buffer, "%s\t[Invalid root path]\n", drive);
    }
    else if (driveType == DRIVE_REMOVABLE)
    {
        // internal_printf("%s\t[Removable drive]\n", drive);
        BofPrintf(&buffer, "%s\t[Removable drive]\n", drive);
    }
    else if (driveType == DRIVE_FIXED)
    {
        // internal_printf("%s\t[Fixed drive]\n", drive);
        BofPrintf(&buffer, "%s\t[Fixed drive]\n", drive);
    }
    else if (driveType == DRIVE_REMOTE)
    {
        // internal_printf("%s\t[Network drive]\n", drive);
        BofPrintf(&buffer, "%s\t[Network drive]\n", drive);
    }
    else if (driveType == DRIVE_CDROM)
    {
        // internal_printf("%s\t[CD-ROM drive]\n", drive);
        BofPrintf(&buffer, "%s\t[CD-ROM drive]\n", drive);
    }
    else if (driveType == DRIVE_RAMDISK)
    {
        // internal_printf("%s\t[RAM disk]\n", drive);
        BofPrintf(&buffer, "%s\t[RAM disk]\n", drive);
    }
    else
    {
        // internal_printf("%s\t[Unknown drive type]\n", drive);
        BofPrintf(&buffer, "%s\t[Unknown drive type]\n", drive);
    }
}

int go()
{
    CHAR driveStrings[256];
    DWORD length = 0;

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

    // if(!bofstart()) return;
	
    // Buffer to store drive strings
    
    length = KERNEL32$GetLogicalDriveStringsA(sizeof(driveStrings), driveStrings);

    if (length == 0)
    {
        BeaconPrintf(CALLBACK_ERROR, "[-] Failed to get logical drive strings.\n");
        goto cleanup;
    }

    // internal_printf("[+] Available drive letters:\n\nDRIVE\tTYPE\n==========================================\n");
    BofPrintf(&buffer, "[+] Available drive letters:\n\nDRIVE\tTYPE\n==========================================\n");

    // Iterate through the drive strings
    for (PCHAR drive = driveStrings; *drive; drive +=  MSVCRT$strlen(drive) + 1)
    {
        printDriveType(drive);
    }

cleanup:
	// printoutput(TRUE);
    BofBufferFree(&buffer);
	// BeaconPrintf(CALLBACK_OUTPUT, "[+] Finished enumerating!\n"); 

    return 0;
}


