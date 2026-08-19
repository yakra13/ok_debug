#include "enumtaskscheduler.h"

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
	
	OLEAUT32$VariantInit(&vHost);
	OLEAUT32$VariantInit(&vNULL);
    
	vHost.vt = VT_BSTR;
    vHost.bstrVal = OLEAUT32$SysAllocString(host);

	hr = OLE32$CoInitializeEx(NULL, COINIT_MULTITHREADED);

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

    HR_CHECK(OLE32$CoCreateInstance(&CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IIDITaskService, (VOID**)&pTaskService));
    
    HR_CHECK(pTaskService->lpVtbl->Connect(pTaskService, vHost, vNULL, vNULL, vNULL)); 
	
    HR_CHECK(pTaskService->lpVtbl->GetFolder(pTaskService, L"\\", &pRootFolder));
    
    HR_CHECK(pRootFolder->lpVtbl->GetTasks(pRootFolder, 0, &pTaskCollection));
    
    hr = pTaskCollection->lpVtbl->get_Count(pTaskCollection, &numTasks);

	for (LONG i = 1; i <= numTasks; i++)
	{ 
		VARIANT index;
		BSTR taskName = NULL;
		ITaskDefinition* pTaskDef = NULL;
	
		index.vt = VT_I4;
		index.lVal = i;

		hr = pTaskCollection->lpVtbl->get_Item(pTaskCollection, index, &pRegisteredTask);
		if (FAILED(hr))
		{
			continue;
		}
		
		hr = pRegisteredTask->lpVtbl->get_Name(pRegisteredTask, &taskName);
		if (SUCCEEDED(hr))
		{
			// BofPrintf(buffer, "    %ls:\n", taskName);
			BofPrintf(buffer, "    %ls: { ", taskName);
			SAFE_SYSFREE_STRING(taskName);
		}
		
		hr = pRegisteredTask->lpVtbl->get_Definition(pRegisteredTask, &pTaskDef);
		if (SUCCEEDED(hr))
		{
			// Fetching the Principal information and print the user account
			IPrincipal* pPrincipal = NULL;

			hr = pTaskDef->lpVtbl->get_Principal(pTaskDef, &pPrincipal);
			if (SUCCEEDED(hr))
			{
				BSTR userId = NULL;
				
				hr = pPrincipal->lpVtbl->get_UserId(pPrincipal, &userId);
				if (SUCCEEDED(hr))
				{
					// BofPrintf(buffer, "      context: %ls\n", userId);
					BofPrintf(buffer, "context: %ls, ", userId);
					SAFE_SYSFREE_STRING(userId);
				}

				SAFE_INTERFACE_RELEASE(pPrincipal);
			}

			// Fetching Action Information

			hr = pRegisteredTask->lpVtbl->get_Definition(pRegisteredTask, &pTaskDef);
			if (SUCCEEDED(hr))
			{
				IActionCollection* pActionColl = NULL;
			
				hr = pTaskDef->lpVtbl->get_Actions(pTaskDef, &pActionColl);
				if (SUCCEEDED(hr))
				{
					LONG actionCount = 0;

					hr = pActionColl->lpVtbl->get_Count(pActionColl, &actionCount);
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