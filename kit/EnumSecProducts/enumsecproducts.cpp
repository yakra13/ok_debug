#include <Windows.h>
#include "base\helpers.h"
#include "enumsecproducts.h"
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

void go(char *args, int len)
{
	PCHAR hostName = NULL;
	HANDLE handleHost = NULL;
    datap parser;
	INT argSize = 0;
	WTS_PROCESS_INFOA* proc_info;
	DWORD pi_count = 0;
	LPSTR procName; 
	bool foundSecProduct = false;

	SoftwareData* softwareList = NULL;
	size_t numSoftware = 130; //130
	
	if (!BofBufferInit(&buffer))
	{
		goto cleanup;
	}

    BeaconDataParse(&parser, args, len);
    hostName = BeaconDataExtract(&parser, &argSize);
	
	//allocate memory for list
    softwareList = (SoftwareData*)VirtualAlloc(
		NULL,
		numSoftware * sizeof(SoftwareData),
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE
	);
    
	if (softwareList == NULL)
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to allocate memory for softwareList.\n");
    }

    //Start security product list
	#include "softwarelist.cpp"
	
	//get handle to specified host
	handleHost = WTSOpenServerA(hostName);

	//get list of running processes 
	if (!WTSEnumerateProcessesA(handleHost, 0, 1, &proc_info, &pi_count))
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to get a valid handle to the specified host.\n");
		goto cleanup;
	}
	
	if (pi_count == 0)
	{
		BeaconPrintf(
			CALLBACK_ERROR,
			"Couldn't list remote processes. Do you have enough privileges on the remote host?\n"
		);
		goto cleanup;
	}

	//compare list with running processes
	BofPrintf(&buffer, "security_products:\n");

	if (hostName && hostName[0] != '\0')
	{
		BofPrintf(&buffer, "  target: %s\n", hostName);
	}
	else
	{
		BofPrintf(&buffer, "  target: LOCAL_HOST\n");
	}

	for (int i = 0 ; i < pi_count ; i++ )
	{
		procName = proc_info[i].pProcessName;
		
		for (size_t i = 0; procName[i]; i++)
		{
            procName[i] = tolower(procName[i]); 
        }
		
		for (size_t i = 0; i < numSoftware; i++)
		{
			if (strcmp(procName, softwareList[i].filename) == 0)
			{
				BofPrintf(&buffer, "  - { category: %ls, description: %ls }\n", softwareList[i].category, softwareList[i].description);
				foundSecProduct = true;
                break;
            }
		}

		procName = NULL;
	}

cleanup:

	if (foundSecProduct == FALSE)
	{
		BofPrintf(&buffer, "  - NONE_FOUND\n");
    }
	
	if (handleHost) { WTSCloseServer(handleHost); }
	if (softwareList) { VirtualFree(softwareList, 0, MEM_RELEASE); }

	BofBufferFree(&buffer);
}
} // end extern "C"

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
	int reqArgCount = 1;

	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");

	if (argc == 2)
	{
		bof::runMocked<char*&>(go, argv[1]);
	}
	else
	{
		bof::runMocked<const char*>(go, "");
	}

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