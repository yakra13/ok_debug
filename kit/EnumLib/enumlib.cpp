#include <windows.h>
#include "base\helpers.h"
#include "enumlib.h"
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

BOOL ListModules(int pid, char *targetModName)
{
    HANDLE hProcess;
    MEMORY_BASIC_INFORMATION mbi;
    char * base = NULL;
	BOOL foundModule = FALSE;

    hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	
	if (hProcess == NULL)
		return foundModule;

	while (VirtualQueryEx(hProcess, base, &mbi, sizeof(mbi)) == sizeof(MEMORY_BASIC_INFORMATION))
	{
		char fqModPath[MAX_PATH];
		char modName[MAX_PATH];

		if(targetModName != NULL)
		{
			// only focus on the base address regions
			if ((mbi.AllocationBase == mbi.BaseAddress) && (mbi.AllocationBase != NULL))
			{
				if (K32GetModuleBaseNameA(
					hProcess,
					(HMODULE)mbi.AllocationBase,
					(LPSTR)modName,
					sizeof(modName) / sizeof(TCHAR)
				))
				{
					if(strcmp(targetModName, modName) == 0)
					{
						K32GetModuleFileNameExA(hProcess,
							(HMODULE)mbi.AllocationBase,
							(LPSTR)fqModPath,
							sizeof(fqModPath) / sizeof(TCHAR)
						);
						// internal_printf("\nModulePath:\t%s\nModuleAddr:\t%#llx\n", fqModPath, mbi.AllocationBase);
						BofPrintf(&buffer, "      - { path: %s, base_addr: %#llx }\n", fqModPath, mbi.AllocationBase);
						foundModule = TRUE;
					}
				}
			}
			// check the next region
			base += mbi.RegionSize;
		}
		else
		{
			// only focus on the base address regions
			if ((mbi.AllocationBase == mbi.BaseAddress) && (mbi.AllocationBase != NULL))
			{
				if (K32GetModuleFileNameExA(
					hProcess,
					(HMODULE)mbi.AllocationBase,
					(LPSTR)fqModPath,
					sizeof(fqModPath) / sizeof(TCHAR)
				))
				{
					// internal_printf("ModulePath [%#llx]: %s\n", mbi.AllocationBase, fqModPath);
					BofPrintf(&buffer, "    - { path: %s, base_addr: %#llx }\n", fqModPath, mbi.AllocationBase);
					foundModule = TRUE;
				}
			}
			// check the next region
			base += mbi.RegionSize;
		}
	}

	CloseHandle(hProcess);
	
	return foundModule;
}

BOOL FindProcess(char *targetModName)
{
	int procID = 0;
	HANDLE currentProc = NULL;
	char procPath[MAX_PATH];
	char procName[MAX_PATH];
	BOOL foundProc = FALSE;
	BOOL res = FALSE;
	
	// resolve function address
	NtGetNextProcess_t pNtGetNextProcess = (NtGetNextProcess_t)GetProcAddress(
		GetModuleHandleA("ntdll.dll"),
		"NtGetNextProcess"
	);
	
	// loop through all processes
	while (!pNtGetNextProcess(currentProc, MAXIMUM_ALLOWED, 0, 0, &currentProc))
	{
		procID = GetProcessId(currentProc);
		
		if (procID == 4)
			continue;

		if (procID == GetCurrentProcessId())
			continue;

		if (procID != 0)
			foundProc = ListModules(procID, targetModName);

		if(foundProc)
		{
			K32GetProcessImageFileNameA(currentProc, procPath, MAX_PATH);
			
			strncpy(procName, PathFindFileNameA(procPath), MAX_PATH);
			
			// internal_printf("ProcName:\t%s\nProcID:\t\t%d\nProcPath:\tC:\%s\n", procName, procID, procPath);
			BofPrintf(&buffer, "    - { id: %d, name: %s, path: %s }\n", procID, procName, procPath);
			res = TRUE;
		}
	}

	return res;
}

void go(char *args, int len)
{
	int pid = 0;
	BOOL res = NULL;
	CHAR *option;
	CHAR *targetModName;
	datap parser;

	if (!BofBufferInit(&buffer))
	{
		goto cleanup;
	}
	
	BeaconDataParse(&parser, args, len);
	option = BeaconDataExtract(&parser, NULL);

	
	if (strcmp(option, "list") == 0)
	{
		pid = BeaconDataInt(&parser);
		// targetModName = BeaconDataExtract(&parser, NULL);
	
		BeaconPrintf(CALLBACK_OUTPUT, "[*] Start enumerating loaded modules for PID: %d\n\n", pid);

		BofPrintf(&buffer, "loaded_modules:\n");
		BofPrintf(&buffer, "  - pid: %d\n", pid);

	
		// internal_printf("[+] FOUND MODULES:\n==============================================================\n"); 
	
		res = ListModules(pid, NULL);
	}
	else if (strcmp(option, "search") == 0)
	{
		targetModName = BeaconDataExtract(&parser, NULL);
	
		BeaconPrintf(CALLBACK_OUTPUT, "[*] Start enumerating processes that loaded module: %s\n[!] Can take some time..\n\n", targetModName);
	
		// internal_printf("[+] FOUND PROCESSES:\n==============================================================\n"); 
		BofPrintf(&buffer, "loaded_modules:\n");
		BofPrintf(&buffer, "  - module: %s\n", targetModName);

		res = FindProcess(targetModName);
	}
	else
	{
		BeaconPrintf(CALLBACK_ERROR, "This enumeration option isn't supported. Please specify one of the following enumeration options: search | list\n");
	}

cleanup:

	if(!res)
	{
		BeaconPrintf(CALLBACK_ERROR, "No modules found for this search query!\n\n");
	}

	BofBufferFree(&buffer);
}
} // end extern "C"


// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
	int reqArgCount = 3;

	if (argc != reqArgCount)
	{
		printf("Usage ... need the args");
		return 1;
	}

	if (strcmp(argv[1], "search") == 0)
	{
		bof::runMocked<char*&, char*&>(go, argv[1], argv[2]);
	}
	else if (strcmp(argv[1], "list") == 0)
	{
		int pid = static_cast<int>(std::strtol(argv[2], nullptr, 10));
		printf("pid is %d\n");

		bof::runMocked<char*&, int&>(go, argv[1], pid);
	}
	else
	{
		printf("Invalid option");
		return 1;
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