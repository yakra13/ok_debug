#include <Windows.h>
#include "base\helpers.h"
#include "enumtaskscheduler.h"
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

HRESULT EnumScheduledTasks(PWCHAR host, BOF_Buffer* buffer)
{
    HRESULT hr = S_OK;
	BOOL bComInitialized = FALSE;

	IID CTaskScheduler  = IID_TASK_SCHEDULER;
    IID IIDITaskService = IID_ITASK_SERVICE;
	
	ITaskService *pTaskService = NULL;
    ITaskFolder* pRootFolder = NULL;
	IRegisteredTaskCollection* pTaskCollection = NULL;
	IRegisteredTask* pRegisteredTask = NULL;

	LONG numTasks = 0;

	VARIANT vHost;
	VARIANT vNULL;

	BSTR rootPath = SysAllocString(L"\\");
	
	VariantInit(&vHost);
	VariantInit(&vNULL);
    
	vHost.vt = VT_BSTR;
    vHost.bstrVal = SysAllocString(host);

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	switch (hr)
	{
		case S_OK:
		case S_FALSE:
			bComInitialized = TRUE;
		case RPC_E_CHANGED_MODE:
			break;
		default:
			goto cleanup;
	}

    HR_CHECK(CoCreateInstance(
		CTaskScheduler,
		NULL,
		CLSCTX_INPROC_SERVER,
		IIDITaskService,
		(VOID**)&pTaskService)
	);
    
    HR_CHECK(pTaskService->Connect(vHost, vNULL, vNULL, vNULL)); 
	
    HR_CHECK(pTaskService->GetFolder(rootPath, &pRootFolder));
    
    HR_CHECK(pRootFolder->GetTasks(0, &pTaskCollection));
    
    hr = pTaskCollection->get_Count(&numTasks);

	for (LONG i = 1; i <= numTasks; i++)
	{ 
		VARIANT index;
		BSTR taskName = NULL;
		ITaskDefinition* pTaskDef = NULL;
	
		index.vt = VT_I4;
		index.lVal = i;

		hr = pTaskCollection->get_Item(index, &pRegisteredTask);
		if (FAILED(hr))
		{
			continue;
		}
		
		hr = pRegisteredTask->get_Name(&taskName);
		if (SUCCEEDED(hr))
		{
			// BofPrintf(buffer, "    %ls:\n", taskName);
			BofPrintf(buffer, "    %ls: { ", taskName);
			SAFE_SYSFREE_STRING(taskName);
		}
		
		hr = pRegisteredTask->get_Definition(&pTaskDef);
		if (SUCCEEDED(hr))
		{
			// Fetching the Principal information and print the user account
			IPrincipal* pPrincipal = NULL;

			hr = pTaskDef->get_Principal(&pPrincipal);
			if (SUCCEEDED(hr))
			{
				BSTR userId = NULL;
				
				hr = pPrincipal->get_UserId(&userId);
				if (SUCCEEDED(hr))
				{
					// BofPrintf(buffer, "      context: %ls\n", userId);
					BofPrintf(buffer, "context: %ls, ", userId);
					SAFE_SYSFREE_STRING(userId);
				}

				SAFE_INTERFACE_RELEASE(pPrincipal);
			}

			// Fetching Action Information

			hr = pRegisteredTask->get_Definition(&pTaskDef);
			if (SUCCEEDED(hr))
			{
				IActionCollection* pActionColl = NULL;
			
				hr = pTaskDef->get_Actions(&pActionColl);
				if (SUCCEEDED(hr))
				{
					LONG actionCount = 0;

					hr = pActionColl->get_Count(&actionCount);
					if (SUCCEEDED(hr))
					{
						for (LONG actionIndex = 1; actionIndex <= actionCount; actionIndex++)
						{
							IAction* pAction = NULL;

							hr = pActionColl->lpVtbl->get_Item(pActionColl, actionIndex, &pAction);
							if (SUCCEEDED(hr))
							{
								TASK_ACTION_TYPE actionType;
								PWCHAR actionTypeName = L"INVALID";
							
								hr = pAction->lpVtbl->get_Type(pAction, &actionType);
								if (SUCCEEDED(hr))
								{
									switch (actionType)
									{
										case TASK_ACTION_EXEC:
											actionTypeName = L"Start a program";
											break;
										case TASK_ACTION_COM_HANDLER:
											actionTypeName = L"COM Handler";
											break;
										case TASK_ACTION_SEND_EMAIL:
											actionTypeName = L"Send an e-mail (Deprecated)";
											break;
										case TASK_ACTION_SHOW_MESSAGE:
											actionTypeName = L"Display a message (Deprecated)";
											break;
									}

									// BofPrintf(buffer, "      action: %ls\n", actionTypeName);
									BofPrintf(buffer, "action: %ls, ", actionTypeName);

									if (actionType == TASK_ACTION_EXEC)
									{
										IExecAction* pExecAction = (IExecAction*) pAction;
										BSTR execPath;
									
										hr = pExecAction->lpVtbl->get_Path(pExecAction, &execPath);
										if (SUCCEEDED(hr))
										{
											// BofPrintf(buffer, "      exe_path: %ls\n", execPath);
											BofPrintf(buffer, "exe_path: %ls, ", execPath);
											SAFE_SYSFREE_STRING(execPath);
										}
									}
								}

								SAFE_INTERFACE_RELEASE(pAction);
							}
						}
					}
				}
			}
			
			// Fetching Trigger Information
			ITriggerCollection* pTriggerColl = NULL;

			hr = pTaskDef->lpVtbl->get_Triggers(pTaskDef, &pTriggerColl);
			if (SUCCEEDED(hr))
			{
				LONG triggerCount = 0;

				hr = pTriggerColl->lpVtbl->get_Count(pTriggerColl, &triggerCount);
				if (SUCCEEDED(hr))
				{
					// BofPrintf(buffer, "      triggers:\n");
					BofPrintf(buffer, "triggers: [ ");

					for (LONG triggerIndex = 1; triggerIndex <= triggerCount; triggerIndex++)
					{
						ITrigger* pTrigger = NULL;

						hr = pTriggerColl->lpVtbl->get_Item(pTriggerColl, triggerIndex, &pTrigger);
						if (SUCCEEDED(hr))
						{
							TASK_TRIGGER_TYPE2 triggerType;
							PWCHAR triggerTypeName = L"UNKNOWN";
						
							hr = pTrigger->lpVtbl->get_Type(pTrigger, &triggerType);
							if (SUCCEEDED(hr))
							{
								if (triggerType >= 0 && ARRAYSIZE(TRIGGER_TYPE_NAMES_LOOKUP))
								{
									triggerTypeName = TRIGGER_TYPE_NAMES_LOOKUP[triggerType];
								}

								// BofPrintf(buffer, "        - %ls\n", triggerTypeName);
								BofPrintf(buffer, "%ls", triggerTypeName);

								if (triggerIndex < triggerCount)
								{
									BofPrintf(buffer, ", ");
								}
							}

							SAFE_INTERFACE_RELEASE(pTrigger);
						}
					}

					BofPrintf(buffer, " ]");
				}

				SAFE_INTERFACE_RELEASE(pTriggerColl);
			}

			SAFE_INTERFACE_RELEASE(pTaskDef);
		}

		BofPrintf(buffer, " }\n\n");
	}
	
cleanup:
	SAFE_INTERFACE_RELEASE(pRegisteredTask);
    SAFE_INTERFACE_RELEASE(pTaskCollection);
    SAFE_INTERFACE_RELEASE(pRootFolder);
    SAFE_INTERFACE_RELEASE(pTaskService);

    OLEAUT32$VariantClear(&vHost);
	OLEAUT32$VariantClear(&vNULL);

	if (bComInitialized)
    	OLE32$CoUninitialize();

    return hr;
}

int go(char *args, int len)
{
	HRESULT hr;
		
	BOF_Buffer buffer = {0};

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

	datap parser;
	PWCHAR hostName  = L""; 

	BeaconDataParse(&parser, args, len);
	hostName = BeaconDataExtract(&parser, NULL);
	
	if (hostName != NULL && hostName[0] != L'\0')
	{
		BofPrintf(&buffer, "%s:\n", hostName);
	}
	else
	{
		char computerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    	DWORD size = sizeof(computerName);

		if (KERNEL32$GetComputerNameA(computerName, &size))
		{
			BofPrintf(&buffer, "%s:\n", computerName);
		}
		else
		{
			BofPrintf(&buffer, "UNKNOWN:\n");
		}
	}
	
	BofPrintf(&buffer, "  scheduled_tasks:\n");

	hr = EnumScheduledTasks(hostName, &buffer);

cleanup:
	if(FAILED(hr))
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to enumerate scheduled tasks; hostname: %ls.\n", hostName);
	}

	BofBufferFree(&buffer);

	return 0;
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