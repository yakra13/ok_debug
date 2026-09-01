#include <windows.h>
#include "base\helpers.h"
#include "enumrwx.h"
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

// TODO: Maybe this could be an interesting thing I dunno
// can map all of memory this way (protection is an additional field)
void MapMemory(HANDLE hProcess)
{
    BYTE* addr = 0;
	
    MEMORY_BASIC_INFORMATION mbi;
	mbi.BaseAddress = 0;
	mbi.AllocationBase = 0;
	mbi.AllocationProtect = 0;
	mbi.RegionSize = 0;
	mbi.State = 0;
	mbi.Protect = 0;
	mbi.Type = 0;
	
	while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)) == sizeof(mbi))
    {
		addr = (BYTE*)mbi.BaseAddress + mbi.RegionSize;

        DWORD_PTR start = (DWORD_PTR)mbi.BaseAddress;
        DWORD_PTR end   = start + mbi.RegionSize - 1;

        BofPrintf(&buffer, "    - { addr: %016llX - %016llX, ", start, end);
        if (mbi.Type == MEM_PRIVATE)
        {
            BofPrintf(&buffer, "type: MEM_PRIVATE, ");
        }
        else if (mbi.Type == MEM_IMAGE)
        {
            BofPrintf(&buffer, "type: MEM_IMAGE, ");
        }
        else if (mbi.Type == MEM_MAPPED)
        {
            BofPrintf(&buffer, "type: MEM_MAPPED, ");
        }
        else
        {
            BofPrintf(&buffer, "type: UNKNOWN, ");
        }


        if (mbi.State == MEM_FREE)
        {
            BofPrintf(&buffer, "state: FREE }\n");
        }
        else if (mbi.State == MEM_RESERVE)
        {
            BofPrintf(&buffer, "state: RESERVE }\n");
        }
        else if (mbi.State == MEM_COMMIT)
        {
            BofPrintf(&buffer, "state: COMMIT }\n");
        }
        else
        {
            BofPrintf(&buffer, "state: UNKNOWN }\n");
        }
        
	}
}

void FindRWX(HANDLE hProcess)
{
	BOOL foundRWX = FALSE;
	LPVOID addr = 0;
	MEMORY_BASIC_INFORMATION mbi;
	mbi.BaseAddress = 0;
	mbi.AllocationBase = 0;
	mbi.AllocationProtect = 0;
	mbi.RegionSize = 0;
	mbi.State = 0;
	mbi.Protect = 0;
	mbi.Type = 0;
	
	while (VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi)))
    {
		addr = (LPVOID)((DWORD_PTR)mbi.BaseAddress + mbi.RegionSize);

		if (mbi.Protect == PAGE_EXECUTE_READWRITE &&
            mbi.State == MEM_COMMIT &&
            mbi.Type == MEM_PRIVATE
        )
        {
            BofPrintf(&buffer, "    - { base_addr: %#llx, size: %llu }\n", mbi.BaseAddress, mbi.RegionSize);
			foundRWX = TRUE;
		}
	}

    if (foundRWX == FALSE)
    {
        BofPrintf(&buffer, "    - NONE_FOUND");
    }
}

void go(char *args, int len)
{
	int pID = 0;
	datap parser;
	HANDLE hProcess = NULL;
	BOOL res = NULL;
	
    if (!BofBufferInit(&buffer))
	{
		goto cleanup;
	}

	BeaconDataParse(&parser, args, len);
	pID = BeaconDataInt(&parser);
	
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pID);

	if (hProcess == NULL)
    {
		BeaconPrintf(CALLBACK_ERROR, "Error opening remote process or thread!\n");
		goto cleanup;
	}

    BofPrintf(&buffer, "rwx_memory_allocations:\n");
    BofPrintf(&buffer, "  pid: %d\n", pID);
	
	FindRWX(hProcess);
    // MapMemory(hProcess);

cleanup:
	if (hProcess) { CloseHandle(hProcess); }

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

	// Convert string to integer - Ignoring error checking as this part of the code does not ship
	int pid = static_cast<int>(std::strtol(argv[1], nullptr, 10));

	bof::runMocked<int&>(go, pid);

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