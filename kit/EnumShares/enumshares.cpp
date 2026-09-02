#include <Windows.h>
#include "base\helpers.h"
#include "enumshares.h"
#include "bofoutput.h"

#ifdef _DEBUG
	#undef DECLSPEC_IMPORT
	#define DECLSPEC_IMPORT
	#include "base\mock.h"
#endif

extern "C" {
#include "beacon.h"
#include "sleepmask.h"

BOF_Buffer buffer = { 0 };

PSHARE_INFO_1 listShares(wchar_t* servername)
{
    PSHARE_INFO_1 pShareInfo = NULL;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    DWORD dwResumeHandle = 0;

    NET_API_STATUS nStatus;

    // internal_printf("\n\nListing shares for: %ls\n", servername);
    // internal_printf("=====================================================\n");
	
    do
    {
        nStatus = NetShareEnum(
            servername,
            1,
            (LPBYTE*)&pShareInfo,
            MAX_PREFERRED_LENGTH,
            &dwEntriesRead,
            &dwTotalEntries,
            &dwResumeHandle
        );
		
        if ((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA))
        {
            for (DWORD i = 0; i < dwEntriesRead; i++)
            {
                // internal_printf("Share Name: %-10ls <- ", pShareInfo[i].shi1_netname);
                // BofPrintf(&buffer, "  - host: %s\n", hostname);
                BofPrintf(&buffer, "    - { name: %ls", pShareInfo[i].shi1_netname);
				
				if (lstrcmpW(pShareInfo[i].shi1_netname, L"IPC$") == 0)
                {
                    internal_printf("[!] No file system access\n");
                    BofPrintf(&buffer, " }/n", pShareInfo[i].shi1_netname);
                    continue;
                }
				
                USE_INFO_2 useInfo = { 0 };
                wchar_t fullPath[260];
                
                _snwprintf(
                    fullPath,
                    sizeof(fullPath) / sizeof(wchar_t) - 1,
                    L"\\\\%s\\%s",
                    servername ? servername : L"localhost",
                    pShareInfo[i].shi1_netname
                );
                
                useInfo.ui2_remote = fullPath;
                useInfo.ui2_asg_type = USE_DISKDEV; 
                useInfo.ui2_username = NULL; // Use current user's credentials
                useInfo.ui2_password = L"";
				
                nStatus = NetUseAdd(NULL, 2, (LPBYTE)&useInfo, NULL);

                if (nStatus == NERR_Success)
                {
                    internal_printf("[+] Accessible\n");
                    NetUseDel(NULL, fullPath, USE_LOTS_OF_FORCE);
                }
                else
                {
                    internal_printf("[-] Error access denied\n");
                }
            }
			
            NetApiBufferFree(pShareInfo);
            
            pShareInfo = NULL;
        }
        else
        {
            if (nStatus == ERROR_BAD_NETPATH)
            {
                internal_printf("Connection error: ERROR_BAD_NETPATH\n");
			}
            else if (nStatus == ERROR_ACCESS_DENIED)
            {
                internal_printf("Connection error: ERROR_ACCESS_DENIED\n");
            }
            else
            {
                internal_printf("Connection error code: %d\n", nStatus);
            }

            break;
        }
		
    } while (nStatus == ERROR_MORE_DATA);
	
	return pShareInfo;
}

void go(char *args, int len)
{
	char* hostname;
	// char* nextHostname;
    int iBytesLen = 0;
    CHAR *hostFileBytes;
	WCHAR wHostname[MAX_PATH];
    datap parser;
	
	if (!BofBufferInit(&buffer))
	{
		goto cleanup;
	}

    BeaconDataParse(&parser, args, len);
    hostFileBytes = BeaconDataExtract(&parser, &iBytesLen);

	if(iBytesLen != 0)
    {
        BeaconPrintf(CALLBACK_OUTPUT, "[+] Loaded hostname file in memory with a size of %d bytes\n", iBytesLen); 
		
        hostname = strtok(hostFileBytes, "\r\n");

        BofPrintf(&buffer, "network_shares:\n");
        
        while (hostname != NULL)
        {
            BofPrintf(&buffer, "  - host: %s\n", hostname);

			MultiByteToWideChar(CP_ACP, 0, hostname, -1, wHostname, MAX_PATH);

			PSHARE_INFO_1 pShareInfo = listShares(wHostname);

            if (pShareInfo)
            {
			    NetApiBufferFree(pShareInfo);
            }

            hostname = strtok(NULL, "\r\n");
        }

		// printoutput(TRUE);
		BeaconPrintf(CALLBACK_OUTPUT, "[+] Finished enumerating!\n"); 
    }
    else
    {
        BeaconPrintf(CALLBACK_ERROR, "Couldn't load the host file from disk.\n");
    }

cleanup:

	BofBufferFree(&buffer);
}
} // End extern "C"

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
	int reqArgCount = 2;

	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");

	if (argc != reqArgCount)
	{
		printf("Usage ... need the args");
		return 1;
	}

	bof::runMocked<char*&>(go, argv[1]);

	return 0;
}

// Define unit tests
#elif defined(_GTEST)
#include <gtest\gtest.h>

TEST(BofTest, Test1) {
	std::vector<bof::output::OutputEntry> got =
		bof::runMocked<>(go);
	std::vector<bof::output::OutputEntry> expected = {
		{CALLBACK_OUTPUT, "System Directory: C:\\Windows\\system32"}
	};
	// It is possible to compare the OutputEntry vectors, like directly
	// ASSERT_EQ(expected, got);
	// However, in this case, we want to compare the output, ignoring the case.
	ASSERT_EQ(expected.size(), got.size());
	ASSERT_STRCASEEQ(expected[0].output.c_str(), got[0].output.c_str());
}
#endif



