#include <windows.h>
#include "base\helpers.h"
#include "enumdrives.h"
#include "bofoutput.h"

#ifdef _DEBUG
	#undef DECLSPEC_IMPORT
	#define DECLSPEC_IMPORT
	#include "base\mock.h"
#endif

extern "C" {
#include "beacon.h"
#include "sleepmask.h"

BOF_Buffer buffer = {0};

VOID printDriveType(const PCHAR drive)
{
    UINT driveType = GetDriveTypeA(drive);
	
    static const char* driveTypes[] =
    {
        "unknown",           // 0 = DRIVE_UNKNOWN       Unknown drive type
        "invalid_root_path", // 1 = DRIVE_NO_ROOT_DIR   Invalid root path
        "removable",         // 2 = DRIVE_REMOVABLE     Removable drive
        "fixed",             // 3 = DRIVE_FIXED         Fixed drive
        "remote",            // 4 = DRIVE_REMOTE        Network drive
        "cd_rom",            // 5 = DRIVE_CDROM         CD-ROM drive
        "ram_disk"           // 6 = DRIVE_RAMDISK       RAM disk
    };

    const char* type = driveType < ARRAYSIZE(driveTypes) ? driveTypes[driveType] : "unknown";

    BofPrintf(&buffer, "  '%s': %s\n", drive);
    BofPrintf(&buffer, "    type: %s\n", type);
}

void go(char* args, int len)
{
    CHAR driveStrings[256];
    DWORD length = 0;

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

    length = GetLogicalDriveStringsA(sizeof(driveStrings), driveStrings);

    if (length == 0)
    {
        BeaconPrintf(CALLBACK_ERROR, "[-] Failed to get logical drive strings.\n");
        goto cleanup;
    }

    BofPrintf(&buffer, "available_drives:\n");

    // Iterate through the drive strings
    for (PCHAR drive = driveStrings; *drive; drive += strlen(drive) + 1)
    {
        printDriveType(drive);
    }

cleanup:
    BofBufferFree(&buffer);
}
} // end extern "C"

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
	bof::runMocked<>(go);

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
