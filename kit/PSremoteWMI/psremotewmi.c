#include "psremotewmi.h"

HRESULT GetProcesses(const BSTR ntwk_resc, BOF_Buffer* buffer)
{
    if (!ntwk_resc || !buffer)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    BOOL bComInitialized = FALSE;

    IWbemLocator* locator            = NULL;
    IWbemServices* services          = NULL;
    IEnumWbemClassObject* enumerator = NULL;
    IWbemClassObject* wbem_obj       = NULL;

    IID CLSID_WbemLocator = CLSID_WBEM_LOCATOR;
    IID IIDI_WbemLocator  = IIDI_WBEM_LOCATOR;

    BSTR strQueryLang    = NULL;
    BSTR strQueryProcesses = NULL;

    ULONG ret = 0;
    VARIANT vt;

    OLEAUT32$VariantInit(&vt);

    strQueryLang = OLEAUT32$SysAllocString(L"WQL");
    strQueryProcesses = OLEAUT32$SysAllocString(
        L"SELECT Name, ProcessId, ParentProcessId, SessionId, "
        L"ExecutablePath, CommandLine, CreationDate, "
        L"ThreadCount, HandleCount, WorkingSetSize "
        L"FROM Win32_Process"
    );

    hr = OLE32$CoInitializeEx(NULL, COINIT_MULTITHREADED);
    switch (hr)
    {
        case S_OK:
        case S_FALSE:
            bComInitialized = TRUE;
            break;
        case RPC_E_CHANGED_MODE:
            break;
        default:
            BeaconPrintf(CALLBACK_ERROR, "Failed to CoInitialize COM object with error: 0x%lx.\n", hr);
            goto cleanup;
    }
   
    HR_CHECK(
        hr,
        OLE32$CoCreateInstance(
            &CLSID_WbemLocator,
            NULL,
            CLSCTX_INPROC_SERVER,
            &IIDI_WbemLocator,
            (LPVOID*)&locator)
    );
    
    HR_CHECK(
        hr,
        locator->lpVtbl->ConnectServer(
            locator,
            ntwk_resc,
            NULL,
            NULL,
            NULL,
            0,
            NULL,
            NULL,
            &services)
    );

    HR_CHECK(
        hr,
        OLE32$CoSetProxyBlanket(
            (IUnknown*)services,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL, // RPC_C_AUTHN_LEVEL_PKT_PRIVACY
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE)
    );

    services->lpVtbl->ExecQuery(
        services,
        strQueryLang,
        strQueryProcesses,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &enumerator);

    while (enumerator->lpVtbl->Next(enumerator, WBEM_INFINITE, 1, &wbem_obj, &ret) == WBEM_S_NO_ERROR && ret)
    {
        ProcessInfo proc = {0};

        wbem_obj->lpVtbl->Get(wbem_obj, L"ProcessId", 0, &vt, NULL, NULL);
        proc.pid = vt.uintVal; // VT_UI4
        OLEAUT32$VariantClear(&vt);
        
        wbem_obj->lpVtbl->Get(wbem_obj, L"ParentProcessId", 0, &vt, NULL, NULL);
        proc.ppid = vt.uintVal;
        OLEAUT32$VariantClear(&vt);
        
        wbem_obj->lpVtbl->Get(wbem_obj, L"SessionId", 0, &vt, NULL, NULL);
        proc.session = vt.uintVal;
        OLEAUT32$VariantClear(&vt);

        //
        // Strings
        //
        
        wbem_obj->lpVtbl->Get(wbem_obj, L"Name", 0, &vt, NULL, NULL);
        if (vt.vt == VT_BSTR)
        {
            CopyBSTRToWString(vt.bstrVal, &(proc.name));
        }
        OLEAUT32$VariantClear(&vt);
        
        wbem_obj->lpVtbl->Get(wbem_obj, L"ExecutablePath", 0, &vt, NULL, NULL);
        if (vt.vt == VT_BSTR)
        {
            CopyBSTRToWString(vt.bstrVal, &(proc.path));
        }
        OLEAUT32$VariantClear(&vt);

        wbem_obj->lpVtbl->Get(wbem_obj, L"CommandLine", 0, &vt, NULL, NULL);
        if (vt.vt == VT_BSTR)
        {
            CopyBSTRToWString(vt.bstrVal, &(proc.command));
        }
        OLEAUT32$VariantClear(&vt);

        wbem_obj->lpVtbl->Get(wbem_obj, L"CreationDate", 0, &vt, NULL, NULL);
        if (vt.vt == VT_BSTR)
        {
            CopyBSTRToWString(vt.bstrVal, &proc.creation_time);
        }
        OLEAUT32$VariantClear(&vt);

        BofPrintf(buffer,
            "    - { pid: %4d, ppid: %4d, session: %3d, name: %-30ls, path: %ls, command: \"%ls\", ctime: %ls }\n",
            proc.pid,
            proc.ppid,
            proc.session,
            proc.name,
            proc.path,
            proc.command,
            proc.creation_time
        );

        // TODO: free allocated strings in proc struct
        HANDLE hHeap = KERNEL32$GetProcessHeap();
        KERNEL32$HeapFree(hHeap, 0, proc.name);
        KERNEL32$HeapFree(hHeap, 0, proc.path);
        KERNEL32$HeapFree(hHeap, 0, proc.command);
        KERNEL32$HeapFree(hHeap, 0, proc.creation_time);
       
        SAFE_INTERFACE_RELEASE(wbem_obj);
    }

cleanup:
    OLEAUT32$VariantClear(&vt);

    SAFE_INTERFACE_RELEASE(wbem_obj);

    SAFE_SYSFREE_STRING(strQueryLang);
    SAFE_SYSFREE_STRING(strQueryProcesses);

    if (bComInitialized)
    {
        OLE32$CoUninitialize();
    }

    return hr;
}

int go(char* args, int len)
{
    HRESULT hr;
    PCHAR hostname = NULL;
    WCHAR w_hostname[256];
    WCHAR namespace[256] = {0};
    BSTR resourceLocation = NULL;
	
    datap parser;
    DWORD argSize = NULL;

    BOF_Buffer buffer = {0};

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

    BeaconDataParse(&parser, args, len);
    hostname = BeaconDataExtract(&parser, &argSize);
    
    MSVCRT$mbstowcs(w_hostname, hostname, 256);
    
    MSVCRT$_snwprintf(namespace, 256, L"\\\\%s\\ROOT\\CIMV2", w_hostname);

    resourceLocation = OLEAUT32$SysAllocString(namespace);
    
    BofPrintf(&buffer, "%s:\n", hostname);
    BofPrintf(&buffer, "  process_list:\n", hostname);
    
    hr = GetProcesses(resourceLocation, &buffer);
    if (FAILED(hr))
    {
        BeaconPrintf(CALLBACK_ERROR, "Failed to enumerate processes with error code: 0x%lx", hr);
    }

cleanup:
    if (buffer.buffer == NULL)
    {
        BeaconPrintf(CALLBACK_ERROR, "Failed to allocate output buffer.");        
    }

    BofBufferFree(&buffer);

    return 0;
}