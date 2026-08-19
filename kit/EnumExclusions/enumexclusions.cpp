#include <Windows.h>
#include "base\helpers.h"
#include "enumexclusions.h"
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

int EnumerateDefenderExclusions()
{
    HRESULT hr;

	int result = 0;

	IWbemLocator* pLoc = NULL;
	IWbemServices* pSvc = NULL;
	IEnumWbemClassObject* pEnumerator = NULL;
	ULONG returnedCount = 0;
	IWbemClassObject *pResult = NULL;

	IID CLSIDWbemLocator = {0x4590f811, 0x1d3a, 0x11d0, {0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}};
	IID IIDIWbemLocator = {0xdc12a687, 0x737f, 0x11cf, {0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24}};
    
    hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) goto cleanup;
	
    hr = CoCreateInstance(
		CLSIDWbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IIDIWbemLocator,
		(LPVOID *)&pLoc
	);
    
	if (FAILED(hr)) goto cleanup;
	
    hr = pLoc->ConnectServer(
		SysAllocString(L"ROOT\\Microsoft\\Windows\\Defender"),
		NULL,
		NULL,
		0,
		NULL,
		0,
		0,
		&pSvc
	);
    
	if (FAILED(hr)) goto cleanup;

    hr = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);

    
	hr = pSvc->ExecQuery(
		SysAllocString(L"WQL"),
		SysAllocString(L"SELECT * FROM MSFT_MpPreference"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator
	);
	
	if (FAILED(hr)) goto cleanup;

	BofPrintf(
		&buffer,
		"\nExclusion enumeration results:\n====================================================\n"
	);

	while (pEnumerator)
	{
		hr = pEnumerator->Next(
			WBEM_INFINITE,
			1,
			&pResult,
			&returnedCount
		);

		if (0 == returnedCount) break;
		
		//folder and files
		VARIANT pathName;
		hr = pResult->Get(L"ExclusionPath", 0, &pathName, 0, 0);

		if (SUCCEEDED(hr))
		{
			if (pathName.vt == VT_NULL)
			{
				BofPrintf(&buffer, "[-] No file or folder exclusion configured\n");
				result = 1; 
			}
			else if (pathName.vt == (VT_ARRAY | VT_BSTR))
			{
				SAFEARRAY* sa = pathName.parray;
				BSTR* bstrArray;
				long lBound, uBound;

				SafeArrayGetLBound(sa, 1, &lBound);
				SafeArrayGetUBound(sa, 1, &uBound);
				SafeArrayAccessData(sa, (void**)&bstrArray);

				for (long i = lBound; i <= uBound; i++)
				{
					if (wcscmp(bstrArray[i], L"N/A: Must be an administrator to view exclusions") == 0)
					{
						BeaconPrintf(
							CALLBACK_ERROR,
							"Access Denied! "
							"The current user does not have sufficient permissions to enumerate exclusions.\n"
						);
						goto cleanup;
					}
					else
					{
						BofPrintf(&buffer, "[+] Found folder/file exclusion: %ls\n", bstrArray[i]);
						result = 1; 
					}
				}

				SafeArrayUnaccessData(sa);
			} 
			else
			{
				BeaconPrintf(
					CALLBACK_ERROR,
					"Error occurred! "
					"Couldn't properly parse path data with error code: %d\n",
					pathName.vt
				);
			}

			VariantClear(&pathName);
		}
		
		//extension
		VARIANT extName;
		hr = pResult->Get(L"ExclusionExtension", 0, &extName, 0, 0);
		
		if (SUCCEEDED(hr))
		{
			if (extName.vt == VT_NULL)
			{
				BofPrintf(&buffer, "[-] No extention exclusion configured\n");
				result = 1; 
			}
			else if (extName.vt == (VT_ARRAY | VT_BSTR))
			{
				SAFEARRAY* sa = extName.parray;
				BSTR* bstrArray;
				long lBound, uBound;

				SafeArrayGetLBound(sa, 1, &lBound);
				SafeArrayGetUBound(sa, 1, &uBound);
				SafeArrayAccessData(sa, (void**)&bstrArray);

				for (long i = lBound; i <= uBound; i++)
				{
					BofPrintf(&buffer, "[+] Found extention exclusion: %ls\n", bstrArray[i]);
					result = 1; 
				}

				SafeArrayUnaccessData(sa);
			}
			else
			{
				BeaconPrintf(CALLBACK_ERROR, "Error occurred! Couldn't properly parse extention data with error code: %d\n", extName.vt);
			}
			
			VariantClear(&extName);
		}
		
		//processes
		VARIANT procName;
		hr = pResult->Get(L"ExclusionProcess", 0, &procName, 0, 0);

		if (SUCCEEDED(hr))
		{
			if (procName.vt == VT_NULL)
			{
				BofPrintf(&buffer, "[-] No process exclusion configured\n");
				result = 1; 
			}
			else if (procName.vt == (VT_ARRAY | VT_BSTR))
			{
				SAFEARRAY* sa = procName.parray;
				BSTR* bstrArray;
				long lBound, uBound;

				SafeArrayGetLBound(sa, 1, &lBound);
				SafeArrayGetUBound(sa, 1, &uBound);
				SafeArrayAccessData(sa, (void**)&bstrArray);

				for (long i = lBound; i <= uBound; i++)
				{
					BofPrintf(&buffer, "[+] Found process exclusion: %ls\n", bstrArray[i]);
					result = 1; 
				}

				SafeArrayUnaccessData(sa);
			}
			else
			{
				BeaconPrintf(CALLBACK_ERROR, "Error occurred! Couldn't properly parse process data with error code: %d\n", procName.vt);
			}
			
			VariantClear(&procName);
		}
	}
	
cleanup:
    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();
    if (pEnumerator) pEnumerator->Release();
	if (pResult) pResult->Release();
    
	CoUninitialize();

	return result;
}

void go(char* args, int len)
{
	int result = 0; 
	
	if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }
	
	result = EnumerateDefenderExclusions();

cleanup:
	if (result == FALSE)
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to enumerate exclusions.");
	}

	BofBufferFree(&buffer);
}
} // end extern "C"

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
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