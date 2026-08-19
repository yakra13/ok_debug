#include "systeminfo.h"

HRESULT WmiGetProperty(IWbemClassObject* pObject, LPCWSTR propName, VARIANT* vt)
{
    HRESULT hr;

    if (!pObject || !propName || !vt)
    {
        return E_INVALIDARG;
    }

    OLEAUT32$VariantClear(vt);

    return pObject->lpVtbl->Get(pObject, propName, 0, vt, NULL, NULL);
}

int go()
{
    HRESULT hr = S_OK;
    ULONG uReturn = 0;
 
    BOF_Buffer buffer = {0};

    BOOL bComInitialized = FALSE;

    IWbemLocator *pLoc                = NULL;
    IWbemServices *pSvc               = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject *pclsObj         = NULL;

    IID CLSIDWbemLocator = CLSID_WBEM_LOCATOR;
    IID IIDIWbemLocator  = IIDI_WBEM_LOCATOR;
    
    BSTR strNetworkResource = OLEAUT32$SysAllocString(L"ROOT\\CIMV2");
    BSTR strQueryLanguage   = OLEAUT32$SysAllocString(L"WQL");
    BSTR strOSQuery         = OLEAUT32$SysAllocString(L"SELECT * FROM Win32_OperatingSystem");
    BSTR strComputerQuery   = OLEAUT32$SysAllocString(L"SELECT * FROM Win32_ComputerSystem");
    BSTR strQFEngQuery      = OLEAUT32$SysAllocString(L"SELECT * FROM Win32_QuickFixEngineering");

    VARIANT vtProp;

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

    OLEAUT32$VariantInit(&vtProp);
    
    if (bofstart() == FALSE)
    {
        BeaconPrintf(CALLBACK_ERROR, "Not enough memory. Failed to allocate output buffer.\n");
        goto cleanup;
    }

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

    hr = OLE32$CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL);
    if (FAILED(hr) && hr != RPC_E_TOO_LATE)
    {
        BeaconPrintf(CALLBACK_ERROR, "CoInitializeSecurity failed with error: 0x%lx\n", hr);
        goto cleanup;
    }
    
    HR_CHECK(
        hr,
        OLE32$CoCreateInstance(&CLSIDWbemLocator, NULL, CLSCTX_INPROC_SERVER, &IIDIWbemLocator, (void**)&pLoc)
    );
    
    HR_CHECK(
        hr,
        pLoc->lpVtbl->ConnectServer(pLoc, strNetworkResource, NULL, NULL, NULL, 0, NULL, NULL, &pSvc)
    );

    HR_CHECK(
        hr,
        OLE32$CoSetProxyBlanket(
            (IUnknown*)pSvc,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE)
    );

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

    BofPrintf(&buffer, "  system_info:\n");
    
    // =====================================================================================
    // QUERY 1: Win32_OperatingSystem
    // =====================================================================================
    
    hr = pSvc->lpVtbl->ExecQuery(
        pSvc,
        strQueryLanguage,
        strOSQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    
    if (SUCCEEDED(hr))
    {
        hr = OLE32$CoSetProxyBlanket(
            (IUnknown*)pEnumerator,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE
        );
        
        if (SUCCEEDED(hr)) 
        {
            while (pEnumerator)
            {
                hr = pEnumerator->lpVtbl->Next(pEnumerator, WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (FAILED(hr) || uReturn == 0)
                {
                    break;
                }

                hr = WmiGetProperty(pclsObj, L"Caption", &vtProp);
                BofPrintf(&buffer,
                    "    %-19s%ls\n",
                    "os_name:",
                    SUCCEEDED(hr) && vtProp.vt == VT_BSTR ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                hr = WmiGetProperty(pclsObj, L"Version", &vtProp);
                BofPrintf(&buffer,
                    "    %-19s%ls\n",
                    "version:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                hr = WmiGetProperty(pclsObj, L"ProductType", &vtProp);
                PWCHAR _config = FAIL_GET_PROP_STRING;

                if (SUCCEEDED(hr))
                {
                    switch(vtProp.uintVal)
                    {
                        case 1:  _config = L"Standalone Workstation"; break;
                        case 2:  _config = L"Domain Controller";      break;
                        case 3:  _config = L"Server";                 break;
                        default: _config = L"Unknown";               break;
                    }
                }

                BofPrintf(&buffer, "    %-19s%ls\n", "os_configuration:", _config);

                hr = WmiGetProperty(pclsObj, L"RegisteredUser", &vtProp);
                BofPrintf(&buffer,
                    "    %-19s%ls\n",
                    "registered_user:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );


                hr = WmiGetProperty(pclsObj, L"WindowsDirectory", &vtProp);
                BofPrintf(&buffer,
                    "    %-19s%ls\n",
                    "windows_directory:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                hr = WmiGetProperty(pclsObj, L"LastBootUpTime", &vtProp);
                BofPrintf(&buffer,
                    "    %-19s%ls\n",
                    "last_boot_time:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                // Try to get locale friendly name
                hr = WmiGetProperty(pclsObj, L"Locale", &vtProp);
                LCID _lcid = MSVCRT$wcstoul(vtProp.bstrVal, NULL, 16);
                WCHAR _localeName[LOCALE_NAME_MAX_LENGTH];
                if (KERNEL32$LCIDToLocaleName(_lcid, _localeName, LOCALE_NAME_MAX_LENGTH, 0))
                {
                    BofPrintf(&buffer, "    %-19s%ls\n", "locale:", _localeName);
                }
                else
                {
                    BofPrintf(&buffer, "    %-19s%ls\n", "locale:", vtProp.bstrVal);
                }
                
                SAFE_INTERFACE_RELEASE(pclsObj);
            }

            SAFE_INTERFACE_RELEASE(pclsObj);
        }
    }
    
    // Prevent memory leak by releasing the enumerator before reusing it
    SAFE_INTERFACE_RELEASE(pEnumerator);

    // =====================================================================================
    // QUERY 2: Win32_ComputerSystem
    // =====================================================================================
    
    hr = pSvc->lpVtbl->ExecQuery(
        pSvc,
        strQueryLanguage,
        strComputerQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    
    if (SUCCEEDED(hr))
    {
        //Applying Proxy Blanket to the second enumerator
        hr = OLE32$CoSetProxyBlanket(
            (IUnknown*)pEnumerator,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE
        );
        
        if (SUCCEEDED(hr))
        {
            while (pEnumerator)
            {
                hr = pEnumerator->lpVtbl->Next(pEnumerator, WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (FAILED(hr) || uReturn == 0)
                {
                    break;
                }

                hr = WmiGetProperty(pclsObj, L"Model", &vtProp);
                // internal_printf(
                //     "    %-19s%ls\n",
                //     "model:",
                //     SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                // );
                BofPrintf(&buffer,
                    "    %-19s%ls\n",
                    "model:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );
                
                hr = WmiGetProperty(pclsObj, L"SystemType", &vtProp);
                // internal_printf(
                //     "    %-19s%ls\n",
                //     "system_type:",
                //     SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                // );
                BofPrintf(&buffer,
                    "    %-19s%ls\n",
                    "system_type:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );
                
                hr = WmiGetProperty(pclsObj, L"Domain", &vtProp);
                // internal_printf(
                //     "    %-19s%ls\n",
                //     "domain:",
                //     SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                // );
                BofPrintf(&buffer,
                    "    %-19s%ls\n",
                    "domain:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                SAFE_INTERFACE_RELEASE(pclsObj);
            }

            SAFE_INTERFACE_RELEASE(pclsObj);
        }
    }

    SAFE_INTERFACE_RELEASE(pEnumerator);

    // =====================================================================================
    // QUERY 3: Win32_QuickFixEngineering
    // =====================================================================================
    
    hr = pSvc->lpVtbl->ExecQuery(
        pSvc,
        strQueryLanguage,
        strQFEngQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    
    if (SUCCEEDED(hr))
    {
        // Applying Proxy Blanket to the third enumerator
        hr = OLE32$CoSetProxyBlanket(
            (IUnknown*)pEnumerator,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE
        );
        
        if (SUCCEEDED(hr))
        {
            // internal_printf("    hotfix_ids:\n");
            BofPrintf(&buffer, "    hotfix_ids:\n");
            
            while (pEnumerator)
            {
                hr = pEnumerator->lpVtbl->Next(pEnumerator, WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (FAILED(hr) || uReturn == 0)
                {
                    break;
                }

                hr = WmiGetProperty(pclsObj, L"HotFixID", &vtProp);
                // internal_printf(
                //     "      - %ls\n",
                //     SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                // );
                BofPrintf(&buffer,
                    "      - %ls\n",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );
            
                SAFE_INTERFACE_RELEASE(pclsObj);
            }

            SAFE_INTERFACE_RELEASE(pclsObj);
        }
    }

cleanup:
    OLEAUT32$VariantClear(&vtProp);

    SAFE_INTERFACE_RELEASE(pclsObj);
    SAFE_INTERFACE_RELEASE(pEnumerator);
    SAFE_INTERFACE_RELEASE(pLoc);
    SAFE_INTERFACE_RELEASE(pSvc);
    
    SAFE_SYSFREE_STRING(strNetworkResource);
    SAFE_SYSFREE_STRING(strQueryLanguage);
    SAFE_SYSFREE_STRING(strOSQuery);
    SAFE_SYSFREE_STRING(strComputerQuery);
    SAFE_SYSFREE_STRING(strQFEngQuery);
    
    if (bComInitialized == TRUE)
    {
        OLE32$CoUninitialize();
    }
    
    // printoutput(TRUE);
    BofBufferFree(&buffer);
    
    return 0;
}