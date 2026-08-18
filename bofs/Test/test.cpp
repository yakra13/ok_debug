#include <Windows.h>
#include "base\helpers.h"
#include "test.h"
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

BOF_Buffer output_buffer = { 0 };

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
		MSVCRT$swprintf_s(Filter, 50, L"%s", L"Process");
		break;
	default:
		MSVCRT$swprintf_s(Filter, 50, L"%s", L"Thread");
		break;
	}

	handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)MSVCRT$malloc(handleInfoSize);

	if (!handleInfo)
	{
		//BeaconPrintf(CALLBACK_ERROR, "Out of memory");
		goto cleanup;
	}

	while (status = (
		NTDLL$NtQuerySystemInformation(
			(SYSTEM_INFORMATION_CLASS)SystemExtendedHandleInformation,
			handleInfo,
			handleInfoSize,
			&returnLength)
		) == STATUS_INFO_LENGTH_MISMATCH
		)
	{
		handleInfoSize *= 2;
		tmp = MSVCRT$realloc(handleInfo, handleInfoSize);

		if (!tmp)
		{
			//BeaconPrintf(CALLBACK_ERROR, "Out of memory");
			goto cleanup;
		}

		handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)tmp;
	}

	if (!NT_SUCCESS(status))
	{
		/*BeaconPrintf(
			CALLBACK_ERROR,
			"NtQuerySystemInformation failed: 0x%08X",
			status
		);*/
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
		if (objHandle->UniqueProcessId == (ULONG_PTR)KERNEL32$GetCurrentProcessId())
		{
			continue;
		}

		processHandle = KERNEL32$OpenProcess(
			PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION,
			FALSE,
			(DWORD)objHandle->UniqueProcessId
		);

		if (!processHandle)
		{
			goto iter_cleanup;
		}

		status = NTDLL$NtDuplicateObject(
			processHandle,
			(HANDLE)objHandle->HandleValue,
			KERNEL32$GetCurrentProcess(),
			&dupHandle,
			0,
			0,
			DUPLICATE_SAME_ACCESS
		);

		if (!NT_SUCCESS(status))
		{
			BeaconPrintf(
				CALLBACK_ERROR,
				"NtDuplicateObject failed PID=%llu Handle=%p Status=0x%08X",
				objHandle->UniqueProcessId,
				(PVOID)objHandle->HandleValue,
				status
			);

			goto iter_cleanup;
		}

		objectTypeInfo = (POBJECT_TYPE_INFORMATION)MSVCRT$malloc(0x1000);

		if (!objectTypeInfo)
		{
			goto iter_cleanup;
		}

		status = NTDLL$NtQueryObject(
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

		if (!SHLWAPI$StrStrIW(Filter, objectTypeInfo->Name.Buffer))
		{
			goto iter_cleanup;
		}

		status = NTDLL$NtQueryObject(
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

		objectNameInfo = (POBJECT_NAME_INFORMATION)MSVCRT$malloc(returnLength);

		if (!objectNameInfo)
		{
			goto iter_cleanup;
		}

		status = NTDLL$NtQueryObject(
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
			procID = (ULONG_PTR)KERNEL32$GetProcessId(dupHandle);
		}
		else if (flags == QUERY_THREAD)
		{
			procID = (ULONG_PTR)KERNEL32$GetProcessIdOfThread(dupHandle);
		}

		if (procID != 0)
		{
			if (flags == QUERY_THREAD)
			{
				HANDLE pH = KERNEL32$OpenProcess(
					PROCESS_QUERY_INFORMATION,
					FALSE,
					(DWORD)procID
				);

				if (pH)
				{
					if (!KERNEL32$K32GetProcessImageFileNameA(pH, procNameTemp, MAX_PATH))
					{
						MSVCRT$sprintf_s(procNameTemp, MAX_PATH, "%s", "unknown");
					}

					KERNEL32$CloseHandle(pH);
				}
				else
				{
					MSVCRT$sprintf_s(procNameTemp, MAX_PATH, "%s", "non existent?");
				}
			}
			else
			{
				// KERNEL32$K32GetProcessImageFileNameA(dupHandle, procNameTemp, MAX_PATH);
				if (!(KERNEL32$K32GetProcessImageFileNameA(dupHandle, procNameTemp, MAX_PATH)))
				{
					MSVCRT$sprintf_s(procNameTemp, MAX_PATH, "%s", "unknown");
				}
			}
		}

		if (targetPid != 0 && targetPid != procID)
		{
			goto iter_cleanup;
		}

		if (procID != 0 && objHandle->UniqueProcessId != procID)
		{
			if (!(KERNEL32$K32GetProcessImageFileNameA(processHandle, procHostName, MAX_PATH)))
			{
				MSVCRT$sprintf_s(procHostName, MAX_PATH, "%s", "unknown");
			}
			BeaconPrintf(CALLBACK_OUTPUT,
				//BofPrintf(
				"  - { from_proc: %s, from_pid: %llu, to_proc: %s, to_pid: %llu, handle_obj: %#llx, access_rights: %#x }\n",
				procHostName,
				objHandle->UniqueProcessId,//KERNEL32$GetProcessId(processHandle),
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
			MSVCRT$free(objectNameInfo);
		}

		if (objectTypeInfo)
		{
			MSVCRT$free(objectTypeInfo);
		}

		if (dupHandle && dupHandle != INVALID_HANDLE_VALUE)
		{
			KERNEL32$CloseHandle(dupHandle);
		}

		if (processHandle && processHandle != INVALID_HANDLE_VALUE)
		{
			KERNEL32$CloseHandle(processHandle);
		}
	}

cleanup:

	return foundHandles;
}

// Define the Dynamic Function Resolution declaration for the GetLastError function
DFR(KERNEL32, GetLastError);
#define GetLastError KERNEL32$GetLastError 

void go(char* args, int len)
{
	int basePid = 0;
	int targetPid = 0;
	BYTE flags = QUERY_THREAD;
	const char* search = "all";
	const char* query = "thread";
	BOOL res = FALSE;
	datap parser;

	if (!BofBufferInit(&output_buffer))
	{
		goto cleanup;
	}

	BeaconDataParse(&parser, args, len);
	search = BeaconDataExtract(&parser, NULL);
	query = BeaconDataExtract(&parser, NULL);

	res = GetHandlesEx(0, flags, 0);

	if (res)
	{
		BeaconPrintf(CALLBACK_OUTPUT, "Success I guess");
	}
	else
	{
		BeaconPrintf(CALLBACK_ERROR, "Failure I guess");
	}

cleanup:
	if (!res)
	{
		BeaconPrintf(CALLBACK_ERROR, "No handle found for this search query!\n");
	}

	BofBufferFree(&output_buffer);
}

	/*void sleep_mask(PBEACON_INFO info, PFUNCTION_CALL funcCall) {
		// BeaconGateWrapper(info, funcCall);
	}*/
}

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");
	bof::runMocked<>(go);

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