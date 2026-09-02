#include <Windows.h>
#include "base\helpers.h"
#include "enumsysmon.h"
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

//IID: https://gist.githubusercontent.com/stevemk14ebr/af8053c506ef895cd520f8017a81f913/raw/98944bc6ae995229d5231568a8ae73dd287e8b4f/guids
BOOL PrintSysmonPID(wchar_t * guid)
{
	HRESULT hr = S_OK;
	ITraceDataProvider *itdProvider = NULL;
	IID CTraceDataProvider = {0x03837513,0x098b,0x11d8,{0x94,0x14,0x50,0x50,0x54,0x50,0x30,0x30}};
	IID IIDITraceDataProvider = {0x03837512,0x098b,0x11d8,{0x94,0x14,0x50,0x50,0x54,0x50,0x30,0x30}};
	IID IIDIEnumVARIANT = {0x00020404,0x0000,0x0000,{0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
	IID IIDIValueMapItem = {0x03837533,0x098b,0x11d8,{0x94,0x14,0x50,0x50,0x54,0x50,0x30,0x30}};
	BOOL activeSysmon = FALSE;
	
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	
	if(FAILED(hr))
	{
		return FALSE;
	}

	hr = CoCreateInstance(
		CTraceDataProvider,
		0,
		CLSCTX_INPROC_SERVER,
		IIDITraceDataProvider,
		(LPVOID*)&itdProvider
	); 
	
	if(FAILED(hr))
	{
		BeaconPrintf(CALLBACK_ERROR,"Failed to create instance of object: %lX", hr);
	}
	
	hr = itdProvider->Query(guid, NULL);

	if(FAILED(hr))
	{
		BeaconPrintf(CALLBACK_ERROR,"Failed to query the process based on the GUID: %lX\n", hr);
	}

	IValueMap *ivmProcesses = NULL;
	hr = itdProvider->GetRegisteredProcesses(&ivmProcesses);
	
	if (hr == S_OK)
	{
		long count = 0;
		hr = ivmProcesses->get_Count(&count);
		
		if (count > 0)
		{
			IUnknown *pUnk = NULL;
			hr = ivmProcesses->get__NewEnum(&pUnk);

			IEnumVARIANT *pItems = NULL;
			
			hr = pUnk->QueryInterface(IIDIEnumVARIANT, (void **)&pItems);
			pUnk->Release();
			
			VARIANT vItem;
			VARIANT vPID;
			
			VariantInit(&vItem);
			VariantInit(&vPID);
			
			IValueMapItem *pProc = NULL;

			while ((hr = pItems->Next(1, &vItem, NULL)) == S_OK)
			{
				vItem.punkVal->QueryInterface(IIDIValueMapItem, (void **)&pProc);

				pProc->get_Value(&vPID);
				
				if (vPID.ulVal)
				{
					// internal_printf("Sysmon procID:\t\t%d\n", vPID.ulVal);
					BofPrintf(&buffer, "  pid: %d", vPID.ulVal);
					activeSysmon = TRUE;
				}

				VariantClear(&vPID);

				pProc->Release();

				VariantClear(&vItem);
			}
		}
	}

	ivmProcesses->Release();
	itdProvider->Release();

	CoUninitialize();

	return activeSysmon;
}

BOOL FindSysmon()
{
    DWORD status = ERROR_SUCCESS;
    PROVIDER_ENUMERATION_INFO * penum = NULL;    
    PROVIDER_ENUMERATION_INFO * ptemp = NULL;
    DWORD BufferSize = 0;                       
    HRESULT hr = S_OK;                          
    WCHAR StringGuid[MAX_GUID_SIZE];
	
    HKEY hKey;
	DWORD cbLength = MAX_DATA_LENGTH;
	DWORD dwType;
	char* RegData = NULL;
	wchar_t guid[256];	
	BOOL activeSysmon = FALSE;


	if(RegOpenKeyExA(
		HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WINEVT\\Channels\\Microsoft-Windows-Sysmon/Operational",
		0,
		KEY_READ,
		&hKey) == ERROR_SUCCESS
	)
	{
		RegData = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbLength);
		
		if (RegData == NULL)
		{
			return FALSE;
		}

		if(RegGetValueA(
			hKey,
			NULL,
			"OwningPublisher",
			RRF_RT_ANY,
			&dwType,
			(PVOID)RegData,
			&cbLength) != ERROR_SUCCESS
		)
		{
			return FALSE;
		}
		
		if (strlen(RegData) != 0)
		{
			MultiByteToWideChar(CP_UTF8, 0, RegData, -1, guid, 256);
		}
		else
		{
			return FALSE;
		}
	}
	else 
	{
		return FALSE;
	}

	if(RegData)
	{
		HeapFree(GetProcessHeap(), 0, RegData);
	}
	
	RegCloseKey(hKey);
	
    status = TdhEnumerateProviders(penum, &BufferSize);

    while (status == ERROR_INSUFFICIENT_BUFFER)
	{
        ptemp = (PROVIDER_ENUMERATION_INFO*)realloc(penum, BufferSize);

        if (ptemp == NULL)
		{
            return FALSE;
        }

        penum = ptemp;
        ptemp = NULL;

        status = TdhEnumerateProviders(penum, &BufferSize);
    }
	
    if (status != ERROR_SUCCESS) 
	{
		BeaconPrintf(CALLBACK_ERROR,"TdhEnumerateProviders failed.\n");
	}
	else
	{
        for (DWORD i = 0; i < penum->NumberOfProviders; i++)
		{
            hr = StringFromGUID2(
				penum->TraceProviderInfoArray[i].ProviderGuid,
				StringGuid,
				ARRAYSIZE(StringGuid)
			);

            if (FAILED(hr))
			{
				return FALSE;
			}
			
			if (!_wcsicmp(StringGuid, (wchar_t *)guid))
			{ 
				BofPrintf(&buffer, "sysmon:\n");
				// internal_printf("[!] Sysmon service found:\n===============================================================\n");
				activeSysmon = PrintSysmonPID(guid);	

				// if(!activeSysmon)
				// {
				// 	internal_printf("Sysmon service status:\tStopped\n");
				// }
				// else
				// {
				// 	internal_printf("Sysmon service status:\tRunning\n");
				// }
				
				// internal_printf("Sysmon provider name:\t%ls\nSysmon provider GUID:\t%ls\n", (LPWSTR)((PBYTE)(penum)+penum->TraceProviderInfoArray[i].ProviderNameOffset), StringGuid); 

				BofPrintf(&buffer, "  provider: %ls\n", (LPWSTR)((PBYTE)(penum)+penum->TraceProviderInfoArray[i].ProviderNameOffset));
				BofPrintf(&buffer, "  guid: %ls\n", StringGuid);
				BofPrintf(&buffer, "  status: %ls\n", activeSysmon == TRUE ? L"Running" : L"Stopped");

				if (penum)
				{
					free(penum);
					penum = NULL;
				}

				return TRUE;
			}
        }
    }
    
	if (penum)
	{
        free(penum);
        penum = NULL;
    }
	
	return FALSE;
}

const char* GetFilterAltitudeGroup(const wchar_t* altitude)
{
    if (!altitude || !altitude[0])
	{
        return "Unknown";
	}

    double value = wcstod(altitude, NULL);

    for (size_t i = 0;
         i < sizeof(altitudeRanges) / sizeof(altitudeRanges[0]);
         ++i)
    {
        if (value >= altitudeRanges[i].min &&
            value <= altitudeRanges[i].max)
        {
            return altitudeRanges[i].name;
        }
    }

    return "Unknown";
}

void PrintMiniFilterData(FILTER_AGGREGATE_STANDARD_INFORMATION* lpFilterInfo)
{
	FILTER_AGGREGATE_STANDARD_INFORMATION* fltInfo = NULL;
	PWCHAR fltName;
	PWCHAR fltAlt;
	
	fltInfo = (FILTER_AGGREGATE_STANDARD_INFORMATION*)lpFilterInfo;

	int fltName_size = fltInfo->Type.MiniFilter.FilterNameLength;

	LONGLONG src = ((LONGLONG)lpFilterInfo) + fltInfo->Type.MiniFilter.FilterNameBufferOffset;
	
	fltName = (PWCHAR)malloc(fltName_size + sizeof(WCHAR));
	
	memset(fltName, 0, fltName_size + sizeof(WCHAR));
	memcpy(fltName, (void*)src, fltName_size);
	
	int fltAlt_size = fltInfo->Type.MiniFilter.FilterAltitudeLength;
	
	src = ((LONGLONG)lpFilterInfo) + fltInfo->Type.MiniFilter.FilterAltitudeBufferOffset;
	
	fltAlt = (PWCHAR)malloc(fltAlt_size + sizeof(WCHAR));
	
	memset(fltAlt, 0, fltAlt_size + sizeof(WCHAR));
	memcpy(fltAlt, (void*)src, fltAlt_size);	
	
	if (fltInfo->Flags == FLTFL_ASI_IS_MINIFILTER)
	{
		// internal_printf("%-29ls%ls\t%26d\n", fltName, fltAlt, fltInfo->Type.MiniFilter.NumberOfInstances);
		const char* group = GetFilterAltitudeGroup(fltAlt);

		int groupPadding = 34 - (int)strlen(group);
		if (groupPadding < 1) { groupPadding = 1; }

		int namePadding = 14 - (int)wcslen(fltName);
		if (namePadding < 1) { namePadding = 1; }

		int altPadding = 9 - (int)wcslen(fltAlt);
		if (altPadding < 1) { altPadding = 1; }

		BofPrintf(
			&buffer,
			"  - { name: %ls,%*s altitude: %ls,%*s group: %s,%*s instances: %d }\n",
			fltName,
			namePadding, "",
			fltAlt,
			altPadding, "",
			group,
			groupPadding, "",
			fltInfo->Type.MiniFilter.NumberOfInstances
		);
	}

cleanup:
	if (fltName) { free(fltName); }
	if (fltAlt) { free(fltAlt);	}
}


BOOL FindMiniFilters()
{
	HRESULT res;
	DWORD dwBytesReturned;
	HANDLE hFilterFind;
	DWORD dwFilterInfoSize = 1024;
	LPVOID lpFilterInfo = NULL;
	BOOL foundMinifilter = FALSE;
	
	lpFilterInfo = HeapAlloc(GetProcessHeap(), NULL, dwFilterInfoSize);
	if (!lpFilterInfo)
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to allocate memory.\n");
		goto cleanup;
	}

	res = FilterFindFirst(
		FilterAggregateStandardInformation,
		lpFilterInfo,
		dwFilterInfoSize,
		&dwBytesReturned,
		&hFilterFind
	);

	if (res == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
	{
		return foundMinifilter;
	}

	if (res != S_OK)
	{
		return foundMinifilter;
	}
	
	// internal_printf("[+] Found MiniFilter drivers.\n[*] Check if you can identify one that is associated with Sysmon (e.g. SysmonDrv):\n\n");
	// internal_printf("Name Minifilter\t\tPriority altitude\t\tLoaded instances\n=======================================================================\n");
	
	BofPrintf(&buffer, "minifilter_drivers:\n");

	PrintMiniFilterData((FILTER_AGGREGATE_STANDARD_INFORMATION*)lpFilterInfo);
	
	foundMinifilter = TRUE;

	while(true)
	{
		res = FilterFindNext(
			hFilterFind,
			FilterAggregateStandardInformation,
			lpFilterInfo,
			dwFilterInfoSize,
			&dwBytesReturned
		);
		
		if (res == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
		{
			break;
		}

		if (res != S_OK)
		{
			return foundMinifilter;
		}

		PrintMiniFilterData((FILTER_AGGREGATE_STANDARD_INFORMATION*)lpFilterInfo);		
	}

cleanup:
	if (lpFilterInfo)
	{
		HeapFree(GetProcessHeap(), NULL, lpFilterInfo);
	}

    return foundMinifilter;
}

void go(char *args, int len)
{
	BOOL res = NULL;
	PCHAR action;
	datap parser;

	if (!BofBufferInit(&buffer))
	{
		goto cleanup;
	}
	
	BeaconDataParse(&parser, args, len);
	action = BeaconDataExtract(&parser, NULL);

	if (strcmp(action, "reg") == 0)
	{
		if(!FindSysmon())
		{
			BeaconPrintf(CALLBACK_OUTPUT_UTF8, "[+] No Sysmon service found\n");
			goto cleanup;
		}
	}
	else if (strcmp(action, "driver") == 0)
	{
		if(!FindMiniFilters())
		{
			BeaconPrintf(CALLBACK_ERROR,"Couldn't list Minifilter drivers. Running with high enough privileges?\n");
			goto cleanup;
		}
	}
	else
	{
		BeaconPrintf(CALLBACK_ERROR, "Please specify one of the following enumeration options: reg | driver (must be elevated)\n");
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