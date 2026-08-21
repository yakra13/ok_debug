#include <Windows.h>
#include "base\helpers.h"
#include "enumdotnet.h"
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

BOOL FindDotNet()
{
	int p = 0;
	int pid = 0;
	char psPath[MAX_PATH];
	
	HANDLE currentProc = NULL;
	UNICODE_STRING sectionName = { 0 };
	WCHAR ProcNumber[30];
	OBJECT_ATTRIBUTES objectAttributes;
	BOOL dotNetFound = FALSE;
	LPCSTR procName;
	//WCHAR WCprocName[256];
	WCHAR objPath[] = L"\\BaseNamedObjects\\Cor_Private_IPCBlock_v4_";
	
	NtGetNextProcess_t pNtGetNextProcess = (NtGetNextProcess_t)GetProcAddress(
		GetModuleHandleA("ntdll.dll"),
		"NtGetNextProcess"
	);
	NtOpenSection_t pNtOpenSection = (NtOpenSection_t)GetProcAddress(
		GetModuleHandleA("ntdll.dll"),
		"NtOpenSection"
	);
	
	if (pNtGetNextProcess == NULL || pNtOpenSection == NULL)
	{
		BeaconPrintf(CALLBACK_ERROR, "Error resolving native API calls!\n");
		return -1;		
	}

	sectionName.Buffer = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 500);

	// BofPrintf(&buffer, "\nProcess name\t\t\t\t\t\tPID\n");
	// BofPrintf(&buffer, "=====================================================================\n");

	BofPrintf(&buffer, "dotnet_processes:\n");

	while (!pNtGetNextProcess(currentProc, MAXIMUM_ALLOWED, 0, 0, &currentProc))
	{
		
		pid = GetProcessId(currentProc);
		if (pid == 0) continue;		

		wsprintfW(ProcNumber, L"%d", pid);

		memset(sectionName.Buffer, 0, 500);
		
		memcpy(sectionName.Buffer, objPath, wcslen(objPath) * 2);   // add section name "prefix"
		
		lstrcatW(sectionName.Buffer, ProcNumber);
		
		sectionName.Length = wcslen(sectionName.Buffer) * 2;		// finally, adjust the string size
		sectionName.MaximumLength = sectionName.Length + 1;		
	
		InitializeObjectAttributes(
			&objectAttributes,
			&sectionName,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL
		);

		HANDLE sectionHandle = NULL;		
		NTSTATUS status = pNtOpenSection(
			&sectionHandle,
			SECTION_QUERY,
			&objectAttributes
		);
		
		if (NT_SUCCESS(status))
		{
			CloseHandle(sectionHandle);
			
			K32GetProcessImageFileNameA(currentProc, psPath, MAX_PATH);
			
			procName = PathFindFileNameA(psPath);
			
			BofPrintf(&buffer, "  - { pid: %d, name: %s }\n", pid, procName);
			
			dotNetFound = TRUE;
		}
	}
	
	return dotNetFound;
}

void go(char* args, int len)
{
	BOOL res = NULL;

	if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }
	
	res = FindDotNet();

cleanup:
	if(!res)
	{
		BeaconPrintf(CALLBACK_ERROR, "No .NET process found!");
	}

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