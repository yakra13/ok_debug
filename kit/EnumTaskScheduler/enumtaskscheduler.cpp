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

void EnumTasksInFolder(ITaskFolder* pFolder)
{
	HRESULT hr = S_OK;
	// BOOL bComInitialized = FALSE;

	LONG numTasks = 0;
    ITaskFolderCollection* pSubFolders = NULL;
    // IRegisteredTaskCollection* pTaskCollection = NULL;

	// IID CTaskScheduler  = IID_TASK_SCHEDULER;
    // IID IIDITaskService = IID_ITASK_SERVICE;
	
	// ITaskService* pTaskService = NULL;
    // ITaskFolder* pRootFolder = NULL;
	IRegisteredTaskCollection* pTaskCollection = NULL;
	IRegisteredTask* pRegisteredTask = NULL;

	// LONG numTasks = 0;

	// VARIANT vHost;
	// VARIANT vNULL;

    // 1. Get and process tasks in the CURRENT folder
    if (SUCCEEDED(pFolder->GetTasks(0, &pTaskCollection)))
	{
        pTaskCollection->get_Count(&numTasks);

        for (LONG i = 1; i <= numTasks; i++)
		{
            // IRegisteredTask* pRegisteredTask = NULL;

            if (SUCCEEDED(pTaskCollection->get_Item(_variant_t(i), &pRegisteredTask)))
			{
                // BSTR taskPath = NULL;
                
				// if (SUCCEEDED(pRegisteredTask->get_Path(&taskPath)))
				// {
                //     wprintf(L"Task: %ls\n", taskPath); // This prints the full path!
                //     SysFreeString(taskPath);
                // }
// ... extract action, triggers, context here ...

				// HR_CHECK(pRootFolder->GetTasks(0, &pTaskCollection));
				
				// hr = pTaskCollection->get_Count(&numTasks);

				// for (LONG i = 1; i <= numTasks; i++)
				// { 
				VARIANT index;
				BSTR taskPath = NULL;
				ITaskDefinition* pTaskDef = NULL;
			
				index.vt = VT_I4;
				index.lVal = i;

				hr = pTaskCollection->get_Item(index, &pRegisteredTask);
				if (FAILED(hr))
				{
					continue;
				}
				
				// hr = pRegisteredTask->get_Name(&taskName);
				hr = pRegisteredTask->get_Path(&taskPath);
				if (SUCCEEDED(hr))
				{
					// BofPrintf(buffer, "    %ls:\n", taskName);
					PWSTR wTaskPath;
					CopyBSTRToWString(taskPath, &wTaskPath);

					BofPrintf(&buffer, "    %ls:\n", wTaskPath);

					SAFE_SYSFREE_STRING(taskPath);

					HeapFree(GetProcessHeap(), 0, wTaskPath);
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
						BSTR groupId = NULL;
						
						hr = pPrincipal->get_UserId(&userId);
						if (SUCCEEDED(hr) && userId != NULL)
						{
							// BofPrintf(buffer, "      context: %ls\n", userId);
							PWSTR wUserId;
							CopyBSTRToWString(userId, &wUserId);

							BofPrintf(&buffer, "      context (User): %ls\n", userId);
							
							HeapFree(GetProcessHeap(), 0, wUserId);
						}
						else
						{
							hr = pPrincipal->get_GroupId(&groupId);
							if (SUCCEEDED(hr) && groupId != NULL)
							{
								BofPrintf(&buffer, "      context (Group): %ls\n", groupId);
							}
							else
							{
								BofPrintf(&buffer, "      context: Unknown\n");
							}
						}

						SAFE_SYSFREE_STRING(userId);
						SAFE_SYSFREE_STRING(groupId);

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
								BofPrintf(&buffer, "      actions:\n");
								for (LONG actionIndex = 1; actionIndex <= actionCount; actionIndex++)
								{
									IAction* pAction = NULL;

									hr = pActionColl->get_Item(actionIndex, &pAction);
									if (SUCCEEDED(hr))
									{
										TASK_ACTION_TYPE actionType;
										const WCHAR* actionTypeName = L"INVALID";
									
										hr = pAction->get_Type(&actionType);
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

											BofPrintf(&buffer, "        - { type: %ls, ", actionTypeName);

											if (actionType == TASK_ACTION_EXEC)
											{
												IExecAction* pExecAction = (IExecAction*) pAction;
												BSTR execPath;
												BSTR args;
											
												hr = pExecAction->get_Path(&execPath);
												pExecAction->get_Arguments(&args);

												if (SUCCEEDED(hr))
												{
													BofPrintf(&buffer, "cmd: %ls %ls }\n", execPath, args ? args : L"");
												}

												SAFE_SYSFREE_STRING(execPath);
												SAFE_SYSFREE_STRING(args);
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

					hr = pTaskDef->get_Triggers(&pTriggerColl);
					if (SUCCEEDED(hr))
					{
						LONG triggerCount = 0;

						hr = pTriggerColl->get_Count(&triggerCount);
						if (SUCCEEDED(hr))
						{
							BofPrintf(&buffer, "      triggers: [ ");

							for (LONG triggerIndex = 1; triggerIndex <= triggerCount; triggerIndex++)
							{
								ITrigger* pTrigger = NULL;

								hr = pTriggerColl->get_Item(triggerIndex, &pTrigger);
								if (SUCCEEDED(hr))
								{
									TASK_TRIGGER_TYPE2 triggerType;
									const WCHAR* triggerTypeName = L"UNKNOWN";
								
									hr = pTrigger->get_Type(&triggerType);
									if (SUCCEEDED(hr))
									{
										if (triggerType >= 0 && ARRAYSIZE(TRIGGER_TYPE_NAMES_LOOKUP))
										{
											triggerTypeName = TRIGGER_TYPE_NAMES_LOOKUP[triggerType];
										}

										// BofPrintf(buffer, "        - %ls\n", triggerTypeName);
										BofPrintf(&buffer, "%ls", triggerTypeName);

										if (triggerIndex < triggerCount)
										{
											BofPrintf(&buffer, ", ");
										}
									}

									SAFE_INTERFACE_RELEASE(pTrigger);
								}
							}

							BofPrintf(&buffer, " ]\n");
						}

						SAFE_INTERFACE_RELEASE(pTriggerColl);
					}

					SAFE_INTERFACE_RELEASE(pTaskDef);
				}
				// }












				//------------------------------------------------------------
                pRegisteredTask->Release();
            }
        }

        pTaskCollection->Release();
    }

    // 2. RECURSIVELY look into all subfolders
    if (SUCCEEDED(pFolder->GetFolders(0, &pSubFolders)))
	{
        LONG numFolders = 0;
        pSubFolders->get_Count(&numFolders);
        
		for (LONG i = 1; i <= numFolders; i++)
		{
            ITaskFolder* pSubFolder = NULL;

            if (SUCCEEDED(pSubFolders->get_Item(_variant_t(i), &pSubFolder)))
			{
                // Recursive call to dive deeper
                EnumTasksInFolder(pSubFolder);
                pSubFolder->Release();
            }
        }

        pSubFolders->Release();
    }
}

HRESULT EnumScheduledTasks(PWCHAR host)
{
    HRESULT hr = S_OK;
	BOOL bComInitialized = FALSE;

	IID CTaskScheduler  = IID_TASK_SCHEDULER;
    IID IIDITaskService = IID_ITASK_SERVICE;
	
	ITaskService* pTaskService = NULL;
    ITaskFolder* pRootFolder = NULL;
	// IRegisteredTaskCollection* pTaskCollection = NULL;
	// IRegisteredTask* pRegisteredTask = NULL;

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

	// Recursively enumerate tasks
	EnumTasksInFolder(pRootFolder);
    
    // HR_CHECK(pRootFolder->GetTasks(0, &pTaskCollection));
    
    // hr = pTaskCollection->get_Count(&numTasks);

	// for (LONG i = 1; i <= numTasks; i++)
	// { 
	// 	VARIANT index;
	// 	BSTR taskName = NULL;
	// 	ITaskDefinition* pTaskDef = NULL;
	
	// 	index.vt = VT_I4;
	// 	index.lVal = i;

	// 	hr = pTaskCollection->get_Item(index, &pRegisteredTask);
	// 	if (FAILED(hr))
	// 	{
	// 		continue;
	// 	}
		
	// 	// hr = pRegisteredTask->get_Name(&taskName);
	// 	hr = pRegisteredTask->get_Path(&taskName);
	// 	if (SUCCEEDED(hr))
	// 	{
	// 		// BofPrintf(buffer, "    %ls:\n", taskName);
	// 		BofPrintf(&buffer, "    %ls:\n", taskName);
	// 		SAFE_SYSFREE_STRING(taskName);
	// 	}
		
	// 	hr = pRegisteredTask->get_Definition(&pTaskDef);
	// 	if (SUCCEEDED(hr))
	// 	{
	// 		// Fetching the Principal information and print the user account
	// 		IPrincipal* pPrincipal = NULL;

	// 		hr = pTaskDef->get_Principal(&pPrincipal);
	// 		if (SUCCEEDED(hr))
	// 		{
	// 			BSTR userId = NULL;
				
	// 			hr = pPrincipal->get_UserId(&userId);
	// 			if (SUCCEEDED(hr))
	// 			{
	// 				// BofPrintf(buffer, "      context: %ls\n", userId);
	// 				BofPrintf(&buffer, "      context: %ls\n", userId);
	// 				SAFE_SYSFREE_STRING(userId);
	// 			}

	// 			SAFE_INTERFACE_RELEASE(pPrincipal);
	// 		}

	// 		// Fetching Action Information

	// 		hr = pRegisteredTask->get_Definition(&pTaskDef);
	// 		if (SUCCEEDED(hr))
	// 		{
	// 			IActionCollection* pActionColl = NULL;
			
	// 			hr = pTaskDef->get_Actions(&pActionColl);
	// 			if (SUCCEEDED(hr))
	// 			{
	// 				LONG actionCount = 0;

	// 				hr = pActionColl->get_Count(&actionCount);
	// 				if (SUCCEEDED(hr))
	// 				{
	// 					for (LONG actionIndex = 1; actionIndex <= actionCount; actionIndex++)
	// 					{
	// 						IAction* pAction = NULL;

	// 						hr = pActionColl->get_Item(actionIndex, &pAction);
	// 						if (SUCCEEDED(hr))
	// 						{
	// 							TASK_ACTION_TYPE actionType;
	// 							const WCHAR* actionTypeName = L"INVALID";
							
	// 							hr = pAction->get_Type(&actionType);
	// 							if (SUCCEEDED(hr))
	// 							{
	// 								switch (actionType)
	// 								{
	// 									case TASK_ACTION_EXEC:
	// 										actionTypeName = L"Start a program";
	// 										break;
	// 									case TASK_ACTION_COM_HANDLER:
	// 										actionTypeName = L"COM Handler";
	// 										break;
	// 									case TASK_ACTION_SEND_EMAIL:
	// 										actionTypeName = L"Send an e-mail (Deprecated)";
	// 										break;
	// 									case TASK_ACTION_SHOW_MESSAGE:
	// 										actionTypeName = L"Display a message (Deprecated)";
	// 										break;
	// 								}

	// 								BofPrintf(&buffer, "      action: %ls\n", actionTypeName);

	// 								if (actionType == TASK_ACTION_EXEC)
	// 								{
	// 									IExecAction* pExecAction = (IExecAction*) pAction;
	// 									BSTR execPath;
									
	// 									hr = pExecAction->get_Path(&execPath);
	// 									if (SUCCEEDED(hr))
	// 									{
	// 										BofPrintf(&buffer, "      exe_path: %ls\n", execPath);
	// 										SAFE_SYSFREE_STRING(execPath);
	// 									}
	// 								}
	// 							}

	// 							SAFE_INTERFACE_RELEASE(pAction);
	// 						}
	// 					}
	// 				}
	// 			}
	// 		}
			
	// 		// Fetching Trigger Information
	// 		ITriggerCollection* pTriggerColl = NULL;

	// 		hr = pTaskDef->get_Triggers(&pTriggerColl);
	// 		if (SUCCEEDED(hr))
	// 		{
	// 			LONG triggerCount = 0;

	// 			hr = pTriggerColl->get_Count(&triggerCount);
	// 			if (SUCCEEDED(hr))
	// 			{
	// 				BofPrintf(&buffer, "      triggers: [ ");

	// 				for (LONG triggerIndex = 1; triggerIndex <= triggerCount; triggerIndex++)
	// 				{
	// 					ITrigger* pTrigger = NULL;

	// 					hr = pTriggerColl->get_Item(triggerIndex, &pTrigger);
	// 					if (SUCCEEDED(hr))
	// 					{
	// 						TASK_TRIGGER_TYPE2 triggerType;
	// 						const WCHAR* triggerTypeName = L"UNKNOWN";
						
	// 						hr = pTrigger->get_Type(&triggerType);
	// 						if (SUCCEEDED(hr))
	// 						{
	// 							if (triggerType >= 0 && ARRAYSIZE(TRIGGER_TYPE_NAMES_LOOKUP))
	// 							{
	// 								triggerTypeName = TRIGGER_TYPE_NAMES_LOOKUP[triggerType];
	// 							}

	// 							// BofPrintf(buffer, "        - %ls\n", triggerTypeName);
	// 							BofPrintf(&buffer, "%ls", triggerTypeName);

	// 							if (triggerIndex < triggerCount)
	// 							{
	// 								BofPrintf(&buffer, ", ");
	// 							}
	// 						}

	// 						SAFE_INTERFACE_RELEASE(pTrigger);
	// 					}
	// 				}

	// 				BofPrintf(&buffer, " ]\n");
	// 			}

	// 			SAFE_INTERFACE_RELEASE(pTriggerColl);
	// 		}

	// 		SAFE_INTERFACE_RELEASE(pTaskDef);
	// 	}
	// }
	
cleanup:
	// SAFE_INTERFACE_RELEASE(pRegisteredTask);
    // SAFE_INTERFACE_RELEASE(pTaskCollection);
    SAFE_INTERFACE_RELEASE(pRootFolder);
    SAFE_INTERFACE_RELEASE(pTaskService);

    VariantClear(&vHost);
	VariantClear(&vNULL);

	if (bComInitialized)
	{
    	CoUninitialize();
	}

    return hr;
}

void go(char *args, int len)
{
	HRESULT hr = -1;
		
	// BOF_Buffer buffer = {0};
	datap parser;
	PCHAR hostName = NULL; 
	int required_chars = 0;
	PWCHAR wHostName = NULL;

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

	BeaconDataParse(&parser, args, len);
	hostName = BeaconDataExtract(&parser, NULL);
	
	if (hostName != NULL && hostName[0] != L'\0')
	{
		BofPrintf(&buffer, "%s:\n", hostName);
	}
	else
	{
		CHAR computerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    	DWORD size = sizeof(computerName);

		if (GetComputerNameA(computerName, &size))
		{
			BofPrintf(&buffer, "%s:\n", computerName);
		}
		else
		{
			BofPrintf(&buffer, "UNKNOWN:\n");
		}
	}
	
	
	required_chars = MultiByteToWideChar(CP_UTF8, 0, hostName, -1, NULL, 0);
    if (required_chars == 0)
	{
		BeaconPrintf(CALLBACK_ERROR, "Error calculating size.\n");
		goto cleanup;
    }
	
	wHostName = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, required_chars * sizeof(WCHAR));
    if (wHostName == NULL)
	{
		BeaconPrintf(CALLBACK_ERROR, "Memory allocation failed.\n");
        goto cleanup;
    }

	MultiByteToWideChar(CP_UTF8, 0, hostName, -1, wHostName, required_chars);
	
	BofPrintf(&buffer, "  scheduled_tasks:\n");

	hr = EnumScheduledTasks(wHostName);

cleanup:
	if(FAILED(hr))
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to enumerate scheduled tasks; hostname: %s\n", hostName);
	}

	if (wHostName)
	{
		HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, wHostName);
	}

	BofBufferFree(&buffer);
}
} // End extern "C"

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
	int reqArgCount = 1;

	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");

	if (argc != reqArgCount)
	{
		printf("Usage ... need the args");
		return 1;
	}

	if (argc > 1)
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