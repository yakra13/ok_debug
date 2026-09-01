#include <Windows.h>
#include "base\helpers.h"
#include "enumhandles.h"
#include "bofoutput.h"

/**
 * For the debug build we want:
 *   a) Include the mock-up layer
 *   b) Undefine DECLSPEC_IMPORT since the mocked Beacon API
 *      is linked against the the debug build.
 */
#ifdef _DEBUG
	#undef DECLSPEC_IMPORT
	#define DECLSPEC_IMPORT
	#include "base\mock.h"
#endif

extern "C" {
#include "beacon.h"
#include "sleepmask.h"

// Global output buffer for beacon CALLBACK_OUTPUT
// You can use CALLBACK_OUTPUT_UTF8 to send updates to CS stdout
// Ensure to call BofBufferFree() when done to return any remaining data to CS
BOF_Buffer outputBuffer = { 0 };

BOOL GetHandlesEx(ULONG_PTR basePid, BYTE flags, ULONG_PTR targetPid)
{
	NTSTATUS status;
	BOOL foundHandles = FALSE;

	PSYSTEM_HANDLE_INFORMATION_EX handleInfo = NULL;

	WCHAR Filter[100] = { 0 };

	ULONG handleInfoSize = 0x10000;

	PVOID tmp = NULL;
	ULONG returnLength = 0;

	switch (flags)
	{
	case QUERY_PROC:
		swprintf_s(Filter, 50, L"%s", L"Process");
		break;
	default:
		swprintf_s(Filter, 50, L"%s", L"Thread");
		break;
	}

	// TODO: heap alloc? heap free 
	handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)malloc(handleInfoSize);

	if (!handleInfo)
	{
		BeaconPrintf(CALLBACK_ERROR, "Out of memory");
		goto cleanup;
	}

	while (status = (
		NtQuerySystemInformation(
			(SYSTEM_INFORMATION_CLASS)SystemExtendedHandleInformation,
			handleInfo,
			handleInfoSize,
			&returnLength)
		) == STATUS_INFO_LENGTH_MISMATCH
		)
	{
		handleInfoSize *= 2;
		tmp = realloc(handleInfo, handleInfoSize);

		if (!tmp)
		{
			BeaconPrintf(CALLBACK_ERROR, "Out of memory");
			goto cleanup;
		}

		handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)tmp;
	}

	if (!NT_SUCCESS(status))
	{
		BeaconPrintf(CALLBACK_ERROR, "NtQuerySystemInformation failed: 0x%08X", status);
		goto cleanup;
	}

	for (ULONG_PTR i = 0; i < handleInfo->NumberOfHandles; i++)
	{
		PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX objHandle = &handleInfo->Handles[i];

		HANDLE processHandle = NULL;
		HANDLE dupHandle = NULL;

		POBJECT_NAME_INFORMATION objectNameInfo = NULL;
		POBJECT_TYPE_INFORMATION objectTypeInfo = NULL;

		ULONG returnLength = 0;
		UNICODE_STRING objectName = { 0 };

		ULONG_PTR procID = 0;

		CHAR procHostName[MAX_PATH] = { 0 };
		CHAR procNameTemp[MAX_PATH] = { 0 };

		//
		// Skip System process handles
		//
		if (objHandle->UniqueProcessId == 4)
		{
			continue;
		}

		//
		// If a base PID was provided, only inspect handles owned by it
		//
		if (basePid != 0 && objHandle->UniqueProcessId != (ULONG_PTR)basePid)
		{
			continue;
		}

		//
		// Skip handles owned by ourselves
		//
		if (objHandle->UniqueProcessId == (ULONG_PTR)GetCurrentProcessId())
		{
			continue;
		}

		processHandle = OpenProcess(
			PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION,
			FALSE,
			(DWORD)objHandle->UniqueProcessId
		);

		if (!processHandle)
		{
			goto iter_cleanup;
		}

		status = NtDuplicateObject(
			processHandle,
			(HANDLE)objHandle->HandleValue,
			GetCurrentProcess(),
			&dupHandle,
			0,
			0,
			DUPLICATE_SAME_ACCESS
		);

		if (!NT_SUCCESS(status))
		{
			// BeaconPrintf(
			// 	CALLBACK_ERROR,
			// 	"NtDuplicateObject failed PID=%llu Handle=%p Status=0x%08X",
			// 	objHandle->UniqueProcessId,
			// 	(PVOID)objHandle->HandleValue,
			// 	status
			// );

			goto iter_cleanup;
		}

		objectTypeInfo = (POBJECT_TYPE_INFORMATION)malloc(0x1000);

		if (!objectTypeInfo)
		{
			goto iter_cleanup;
		}

		status = NtQueryObject(
			dupHandle,
			ObjectTypeInformation,
			objectTypeInfo,
			0x1000,
			NULL
		);

		if (!NT_SUCCESS(status))
		{
			goto iter_cleanup;
		}

		if (!StrStrIW(Filter, objectTypeInfo->Name.Buffer))
		{
			goto iter_cleanup;
		}

		status = NtQueryObject(
			dupHandle,
			(OBJECT_INFORMATION_CLASS)ObjectNameInformation,
			NULL,
			0,
			&returnLength
		);

		if (status != STATUS_INFO_LENGTH_MISMATCH && !NT_SUCCESS(status))
		{
			goto iter_cleanup;
		}

		objectNameInfo = (POBJECT_NAME_INFORMATION)malloc(returnLength);

		if (!objectNameInfo)
		{
			goto iter_cleanup;
		}

		status = NtQueryObject(
			dupHandle,
			(OBJECT_INFORMATION_CLASS)ObjectNameInformation,
			objectNameInfo,
			returnLength,
			&returnLength
		);

		if (!NT_SUCCESS(status))
		{
			goto iter_cleanup;
		}

		if (objectNameInfo->Name.Buffer)
		{
			objectName = objectNameInfo->Name;
		}

		if (flags == QUERY_PROC)
		{
			procID = (ULONG_PTR)GetProcessId(dupHandle);
		}
		else if (flags == QUERY_THREAD)
		{
			procID = (ULONG_PTR)GetProcessIdOfThread(dupHandle);
		}

		if (procID != 0)
		{
			if (flags == QUERY_THREAD)
			{
				HANDLE pH = OpenProcess(
					PROCESS_QUERY_INFORMATION,
					FALSE,
					(DWORD)procID
				);

				if (pH)
				{
					if (!K32GetProcessImageFileNameA(pH, procNameTemp, MAX_PATH))
					{
						sprintf_s(procNameTemp, MAX_PATH, "%s", "unknown");
					}

					CloseHandle(pH);
				}
				else
				{
					sprintf_s(procNameTemp, MAX_PATH, "%s", "non existent?");
				}
			}
			else
			{
				// KERNEL32$K32GetProcessImageFileNameA(dupHandle, procNameTemp, MAX_PATH);
				if (!(K32GetProcessImageFileNameA(dupHandle, procNameTemp, MAX_PATH)))
				{
					sprintf_s(procNameTemp, MAX_PATH, "%s", "unknown");
				}
			}
		}

		if (targetPid != 0 && targetPid != procID)
		{
			goto iter_cleanup;
		}

		if (procID != 0 && objHandle->UniqueProcessId != procID)
		{
			if (!(K32GetProcessImageFileNameA(processHandle, procHostName, MAX_PATH)))
			{
				sprintf_s(procHostName, MAX_PATH, "%s", "unknown");
			}
			
			BofPrintf(
				&outputBuffer,
				"  - { from_proc: %s, from_pid: %llu, to_proc: %s, to_pid: %llu, handle_obj: %#llx, access_rights: %#x }\n",
				procHostName,
				objHandle->UniqueProcessId, //KERNEL32$GetProcessId(processHandle),
				procNameTemp,
				procID,
				objHandle->HandleValue,
				objHandle->GrantedAccess
			);

			foundHandles = TRUE;
		}

	iter_cleanup:

		if (objectNameInfo)
		{
			free(objectNameInfo);
		}

		if (objectTypeInfo)
		{
			free(objectTypeInfo);
		}

		if (dupHandle && dupHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(dupHandle);
		}

		if (processHandle && processHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(processHandle);
		}
	}

cleanup:

	if (handleInfo)
	{
		free(handleInfo);
	}

	return foundHandles;
}

void go(char* args, int len)
{
	int basePid = 0;
	int targetPid = 0;
	BYTE flags;
	CHAR* search;
	CHAR* query;
	
	datap parser;
	
	BOOL result = FALSE;

	char computerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD nameSize = sizeof(computerName);

	if (!BofBufferInit(&outputBuffer))
	{
		goto cleanup;
	}

	BeaconDataParse(&parser, args, len);
	search = BeaconDataExtract(&parser, NULL);
	query = BeaconDataExtract(&parser, NULL);

	// if (GetComputerNameA(computerName, &nameSize))
    // {
    //     BofPrintf(&outputBuffer, "%s:\n", computerName);
    // }
    // else
    // {
    //     BofPrintf(&outputBuffer, "UNKNOWN:\n");
    // }


	if (strcmp(query, "proc") == 0)
	{
		flags = QUERY_PROC;
		BofPrintf(&outputBuffer, "  process_handles:\n");
	}
	else if (strcmp(query, "thread") == 0)
	{
		flags = QUERY_THREAD;
		BofPrintf(&outputBuffer, "  thread_handles:\n");
	}
	else
	{
		BeaconPrintf(CALLBACK_ERROR, "Please specify either 'proc' (PROCESS_HANDLE) or 'thread' (THREAD_HANDLE) as handle search options.\n");
		goto cleanup;
	}

	if (strcmp(search, "all") == 0)
	{
		BeaconPrintf(CALLBACK_OUTPUT_UTF8, "[*] Start enumerating all processes with handles to all other processes\n");
		
		result = GetHandlesEx(0, flags, 0);
	}
	else if (strcmp(search, "h2p") == 0)
	{
		targetPid = BeaconDataInt(&parser);

		BeaconPrintf(CALLBACK_OUTPUT_UTF8, "[*] Start enumerating all processes that have a handle to PID: [%d]\n", targetPid);
		
		result = GetHandlesEx(0, flags, targetPid);
	}
	else if (strcmp(search, "p2h") == 0)
	{
		basePid = BeaconDataInt(&parser);
		
		BeaconPrintf(CALLBACK_OUTPUT_UTF8, "[*] Start enumerating handles from PID [%d] to all other processes\n", basePid);
		
		result = GetHandlesEx(basePid, flags, 0);
	}
	else
	{
		BeaconPrintf(CALLBACK_ERROR, "Please specify one of the following process search options: all | h2p | p2h\n");
		
		goto cleanup;
	}


cleanup:
	if (!result)
	{
		BeaconPrintf(CALLBACK_ERROR, "No handle found for this search query!\n");
	}

	BofBufferFree(&outputBuffer);
}
} // End extern "C"

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
	int reqArgCount = 4;

	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");

	if (argc != reqArgCount)
	{
		printf("Usage ... need the args");
		return 1;
	}

	// Convert string to integer - Ignoring error checking as this part of the code does not ship
	int pid = static_cast<int>(std::strtol(argv[3], nullptr, 10));

	bof::runMocked<char*&, char*&, int&>(go, argv[1], argv[2], pid);

	/* To test a sleepmask BOF, the following mockup executors can be used
	// Mock up Beacon and run the sleep mask once
	bof::runMockedSleepMask(sleep_mask);

	// Mock up Beacon with the specific .stage C2 profile
	bof::runMockedSleepMask(sleep_mask,
		{
			.allocator = bof::profile::Allocator::VirtualAlloc,
			.obfuscate = bof::profile::Obfuscate::False,
			.useRWX = bof::profile::UseRWX::True,
			.module = "",
		},
		{
			.sleepTimeMs = 5000,
			.runForever = false,
		}
	);
	*/

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