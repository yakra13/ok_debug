#include <Windows.h>
#include "base\helpers.h"
#include "systeminfo.h"
#include "bofoutput.h"
#include "obfuscate_string.h"

#ifdef _DEBUG
	#undef DECLSPEC_IMPORT
	#define DECLSPEC_IMPORT
	#include "base\mock.h"
#endif

extern "C" {
#include "beacon.h"
#include "sleepmask.h"

BOF_Buffer buffer = { 0 };

HRESULT WmiGetProperty(IWbemClassObject* pObject, LPCWSTR propName, VARIANT* vt)
{
    HRESULT hr;

    if (!pObject || !propName || !vt)
    {
        return E_INVALIDARG;
    }

    VariantClear(vt);

    return pObject->Get(propName, 0, vt, NULL, NULL);
}

void FreeSystemInfo(SystemInfo* info)
{
    if (!info)
        return;

    HANDLE hHeap = GetProcessHeap();

    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.computer_name);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.computer_user_name);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.dns_hostname);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.domain);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.domain_role);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.hypervisor_present);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.manufacturer);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.model);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.part_of_domain);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.pc_system_type);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.system_family);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.system_sku);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->computer_info.system_type);

    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.build_number);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.last_boot_time);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.locale);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.organization);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.os_configuration);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.os_name);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.registered_user);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.system_drive);
    // HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.user_sessions);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.version);
    HeapFree(hHeap, HEAP_ZERO_MEMORY, info->os_info.windows_directory);

    if (info->hotfixes.ids)
    {
        for (size_t i = 0; i < info->hotfixes.count; i++)
        {
            HeapFree(hHeap, HEAP_ZERO_MEMORY, info->hotfixes.ids[i]);
        }

        HeapFree(hHeap, HEAP_ZERO_MEMORY, info->hotfixes.ids);
    }

    HeapFree(hHeap, HEAP_ZERO_MEMORY, info);
}

void go(char* args, int len)
{
    HRESULT hr = S_OK;
    ULONG uReturn = 0;

    BOOL bComInitialized = FALSE;

    IWbemLocator *pLoc                = NULL;
    IWbemServices *pSvc               = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject *pclsObj         = NULL;

    IID CLSIDWbemLocator = CLSID_WBEM_LOCATOR;
    IID IIDIWbemLocator  = IIDI_WBEM_LOCATOR;
    
    BSTR strNetworkResource = SysAllocString(OBF(L"ROOT\\CIMV2").get());
    BSTR strQueryLanguage   = SysAllocString(OBF(L"WQL").get());
    BSTR strOSQuery         = SysAllocString(OBF(L"SELECT * FROM Win32_OperatingSystem").get());
    BSTR strComputerQuery   = SysAllocString(OBF(L"SELECT * FROM Win32_ComputerSystem").get());
    BSTR strQFEngQuery      = SysAllocString(OBF(L"SELECT * FROM Win32_QuickFixEngineering").get());

    VARIANT vtProp;

    SystemInfo* systemInfo;
    systemInfo = (SystemInfo*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SystemInfo));

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

    VariantInit(&vtProp);
    
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    switch (hr)
    {
        case S_OK:
        case S_FALSE:
            bComInitialized = TRUE;
            break;
        case RPC_E_CHANGED_MODE:
            break;
        default:
            BeaconPrintf(CALLBACK_ERROR, OBF("Failed to CoInitialize COM object with error: 0x%lx.\n").get(), hr);
            goto cleanup;
    }

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);

    if (FAILED(hr) && hr != RPC_E_TOO_LATE)
    {
        BeaconPrintf(CALLBACK_ERROR, OBF("CoInitializeSecurity failed with error: 0x%lx\n").get(), hr);
        goto cleanup;
    }
    
    HR_CHECK(hr, CoCreateInstance(CLSIDWbemLocator, NULL, CLSCTX_INPROC_SERVER, IIDIWbemLocator, (void**)&pLoc));
    
    HR_CHECK(hr, pLoc->ConnectServer(strNetworkResource, NULL, NULL, NULL, 0, NULL, NULL, &pSvc));

    HR_CHECK(hr,
        CoSetProxyBlanket((IUnknown*)pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE)
    );

    // =====================================================================================
    // QUERY 1: Win32_OperatingSystem
    // =====================================================================================
    
    hr = pSvc->ExecQuery(
        strQueryLanguage,
        strOSQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    
    if (SUCCEEDED(hr))
    {
        hr = CoSetProxyBlanket(
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
                hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (FAILED(hr) || uReturn == 0)
                {
                    break;
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"Caption").get(), &vtProp)))
                {
                    if (vtProp.vt == VT_BSTR)
                    {
                        systemInfo->os_info.os_name = BSTRToWString(vtProp.bstrVal);
                    }
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"Version").get(), &vtProp)))
                {
                    systemInfo->os_info.version = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"BuildNumber").get(), &vtProp)))
                {
                    systemInfo->os_info.build_number = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"ProductType").get(), &vtProp)))
                {
                    if (vtProp.uiVal == 1)
                        systemInfo->os_info.os_configuration = WStringToHeap(OBF(L"Standalone Workstation").get());
                    else if (vtProp.uiVal == 2)
                        systemInfo->os_info.os_configuration = WStringToHeap(OBF(L"Domain Controller").get());
                    else if (vtProp.uiVal == 3)
                        systemInfo->os_info.os_configuration = WStringToHeap(OBF(L"Server").get());
                    else
                        systemInfo->os_info.os_configuration = WStringToHeap(OBF(L"Unknown").get());
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"RegisteredUser").get(), &vtProp)))
                {
                    systemInfo->os_info.registered_user = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"NumberOfUsers").get(), &vtProp)))
                {
                    systemInfo->os_info.user_sessions = vtProp.uintVal; 
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"WindowsDirectory").get(), &vtProp)))
                {
                    systemInfo->os_info.windows_directory = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"SystemDrive").get(), &vtProp)))
                {
                    systemInfo->os_info.system_drive = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"Organization").get(), &vtProp)))
                {
                    systemInfo->os_info.organization = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"LastBootUpTime").get(), &vtProp)))
                {
                    systemInfo->os_info.last_boot_time = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"Locale").get(), &vtProp)))
                {
                    LCID _lcid = wcstoul(vtProp.bstrVal, NULL, 16);
                    
                    WCHAR _localeName[LOCALE_NAME_MAX_LENGTH];

                    if (LCIDToLocaleName(_lcid, _localeName, LOCALE_NAME_MAX_LENGTH, 0))
                    {
                        systemInfo->os_info.locale = WStringToHeap(_localeName);
                    }
                    else
                    {
                        systemInfo->os_info.locale = BSTRToWString(vtProp.bstrVal);
                    }
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
    
    hr = pSvc->ExecQuery(
        strQueryLanguage,
        strComputerQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    
    if (SUCCEEDED(hr))
    {
        //Applying Proxy Blanket to the second enumerator
        hr = CoSetProxyBlanket(
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
                hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (FAILED(hr) || uReturn == 0)
                {
                    break;
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"Model").get(), &vtProp)))
                {
                    systemInfo->computer_info.model = BSTRToWString(vtProp.bstrVal);
                }
                
                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"SystemType").get(), &vtProp)))
                {
                    systemInfo->computer_info.system_type = BSTRToWString(vtProp.bstrVal);
                }
                
                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"Domain").get(), &vtProp)))
                {
                    systemInfo->computer_info.domain = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"Manufacturer").get(), &vtProp)))
                {
                    systemInfo->computer_info.manufacturer = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"DNSHostName").get(), &vtProp)))
                {
                    systemInfo->computer_info.dns_hostname = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"Name").get(), &vtProp)))
                {
                    systemInfo->computer_info.computer_name = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"SystemFamily").get(), &vtProp)))
                {
                    systemInfo->computer_info.system_family = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"SystemSKUNumber").get(), &vtProp)))
                {
                    systemInfo->computer_info.system_sku = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"UserName").get(), &vtProp)))
                {
                    systemInfo->computer_info.computer_user_name = BSTRToWString(vtProp.bstrVal);
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"PartOfDomain").get(), &vtProp)))
                {
                    systemInfo->computer_info.part_of_domain = vtProp.boolVal ? WStringToHeap(L"true") : WStringToHeap(L"false");
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"HypervisorPresent").get(), &vtProp)))
                {
                    systemInfo->computer_info.hypervisor_present = vtProp.boolVal ? WStringToHeap(L"true") : WStringToHeap(L"false");
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"DomainRole").get(), &vtProp)))
                {
                    if (vtProp.uiVal == 0)
                        systemInfo->computer_info.domain_role = WStringToHeap(OBF(L"Standalone Workstation").get());
                    else if (vtProp.uiVal == 1)
                        systemInfo->computer_info.domain_role = WStringToHeap(OBF(L"Member Workstation").get());
                    else if (vtProp.uiVal == 2)
                        systemInfo->computer_info.domain_role = WStringToHeap(OBF(L"Standalone Server").get());
                    else if (vtProp.uiVal == 3)
                        systemInfo->computer_info.domain_role = WStringToHeap(OBF(L"Member Server").get());
                    else if (vtProp.uiVal == 4)
                        systemInfo->computer_info.domain_role = WStringToHeap(OBF(L"Backup Domain Controller").get());
                    else if (vtProp.uiVal == 5)
                        systemInfo->computer_info.domain_role = WStringToHeap(OBF(L"Primary Domain Controller").get());
                    else
                        systemInfo->computer_info.domain_role = WStringToHeap(OBF(L"Unknown").get());
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"PCSystemType").get(), &vtProp)))
                {
                    if (vtProp.uiVal == 0)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Unspecified").get());
                    else if (vtProp.uiVal == 1)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Desktop").get());
                    else if (vtProp.uiVal == 2)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Mobile").get());
                    else if (vtProp.uiVal == 3)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Workstation").get());
                    else if (vtProp.uiVal == 4)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Enterprise Server").get());
                    else if (vtProp.uiVal == 5)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"SOHO Server").get());
                    else if (vtProp.uiVal == 6)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Appliance PC").get());
                    else if (vtProp.uiVal == 7)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Performance Server").get());
                    else if (vtProp.uiVal == 8)
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Slate").get());
                    else
                        systemInfo->computer_info.pc_system_type = WStringToHeap(OBF(L"Unspecified").get());
                }

                SAFE_INTERFACE_RELEASE(pclsObj);
            }

            SAFE_INTERFACE_RELEASE(pclsObj);
        }
    }

    SAFE_INTERFACE_RELEASE(pEnumerator);

    // =====================================================================================
    // QUERY 3: Win32_QuickFixEngineering
    // =====================================================================================
    
    hr = pSvc->ExecQuery(
        strQueryLanguage,
        strQFEngQuery,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator
    );
    
    if (SUCCEEDED(hr))
    {
        // Applying Proxy Blanket to the third enumerator
        hr = CoSetProxyBlanket(
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
            HANDLE hHeap = GetProcessHeap();
            
            while (pEnumerator)
            {
                hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (FAILED(hr) || uReturn == 0)
                {
                    break;
                }

                if (SUCCEEDED(WmiGetProperty(pclsObj, OBF(L"HotFixID").get(), &vtProp)) && vtProp.vt == VT_BSTR)
                {
                   PWCHAR id = BSTRToWString(vtProp.bstrVal);
                   PWCHAR* newIds;
                   
                   if (!id)
                   {
                        SAFE_INTERFACE_RELEASE(pclsObj);        
                        continue;
                   }

                   if (systemInfo->hotfixes.ids == NULL)
                   {
                        newIds = (PWCHAR*)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(PWCHAR));
                   }
                   else
                   {
                        newIds = (PWCHAR*)HeapReAlloc(
                            hHeap,
                            HEAP_ZERO_MEMORY,
                            systemInfo->hotfixes.ids,
                            (systemInfo->hotfixes.count + 1) * sizeof(PWCHAR)
                        );
                   }

                   if (newIds)
                   {
                        systemInfo->hotfixes.ids = newIds;

                        systemInfo->hotfixes.ids[systemInfo->hotfixes.count] = id;

                        systemInfo->hotfixes.count++;
                   }
                   else
                   {
                        HeapFree(hHeap, 0, id);
                   }
                }
            
                SAFE_INTERFACE_RELEASE(pclsObj);
            }

            SAFE_INTERFACE_RELEASE(pclsObj);
        }
    }

    // Print output
    BofPrintf(&buffer, OBF("system_info:\n").get());
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("os_name:").get(),           YAML_STR(systemInfo->os_info.os_name));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("version:").get(),           YAML_STR(systemInfo->os_info.version));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("build_number:").get(),      YAML_STR(systemInfo->os_info.build_number));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("os_configuration:").get(),  YAML_STR(systemInfo->os_info.os_configuration));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("registered_user:").get(),   YAML_STR(systemInfo->os_info.registered_user));
    BofPrintf(&buffer, "  %-20s%u\n",  OBF("user_sessions:").get(),     systemInfo->os_info.user_sessions);
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("windows_directory:").get(), YAML_STR(systemInfo->os_info.windows_directory));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("system_drive:").get(),      YAML_STR(systemInfo->os_info.system_drive));

    BofPrintf(&buffer, "  %-20s%ls\n", OBF("computer_name:").get(),      YAML_STR(systemInfo->computer_info.computer_name));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("computer_user_name:").get(), YAML_STR(systemInfo->computer_info.computer_user_name));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("hypervisor_present:").get(), YAML_STR(systemInfo->computer_info.hypervisor_present));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("dns_hostname:").get(),       YAML_STR(systemInfo->computer_info.dns_hostname));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("part_of_domain:").get(),     YAML_STR(systemInfo->computer_info.part_of_domain));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("domain:").get(),             YAML_STR(systemInfo->computer_info.domain));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("domain_role:").get(),        YAML_STR(systemInfo->computer_info.domain_role));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("manufacturer:").get(),       YAML_STR(systemInfo->computer_info.manufacturer));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("model:").get(),              YAML_STR(systemInfo->computer_info.model));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("pc_system_type:").get(),     YAML_STR(systemInfo->computer_info.pc_system_type));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("system_family:").get(),      YAML_STR(systemInfo->computer_info.system_family));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("system_sku:").get(),         YAML_STR(systemInfo->computer_info.system_sku));
    BofPrintf(&buffer, "  %-20s%ls\n", OBF("system_type:").get(),        YAML_STR(systemInfo->computer_info.system_type));

    BofPrintf(&buffer, OBF("  hotfix_ids:\n").get());
    for (size_t i = 0; i < systemInfo->hotfixes.count; i++)
    {
        BofPrintf(&buffer, "    - %ls\n", systemInfo->hotfixes.ids[i]);
    }

cleanup:
    VariantClear(&vtProp);

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
        CoUninitialize();
    }

    FreeSystemInfo(systemInfo);
    
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

/*
-existing-
version
os_configuration
registered_user
windows_directory
last_boot_time
locale
model
system_type
domain
hotfix_ids


--Win32_OperatingSystem--
BuildNumber	            Exact Windows build
OSArchitecture	        64-bit / 32-bit
InstallDate	            OS installation date
SerialNumber	        Windows installation/product identifier
SystemDirectory	        e.g. C:\Windows\System32
SystemDrive	            e.g. C:
Manufacturer	        Usually Microsoft Corporation
Organization	        Registered organization
ProductType	            Workstation / Domain Controller / Server
OperatingSystemSKU	    More specific Windows edition/SKU
OSLanguage	            Numeric Windows language ID
MUILanguages	        Installed UI languages
ServicePackMajorVersion	Service pack major
ServicePackMinorVersion	Service pack minor
NumberOfProcesses	    Current process count
NumberOfUsers	        Current logged-on/user count
TotalVisibleMemorySize	Total usable physical memory
FreePhysicalMemory	    Available physical memory
TotalVirtualMemorySize	Total virtual memory
FreeVirtualMemory	    Free virtual memory

build_number
os_architecture
install_date
system_drive
system_directory
product_type
operating_system_sku
total_visible_memory

--Win32_ComputerSystem--
Manufacturer	            Dell, HP, Microsoft, VMware, etc.
DNSHostName	                Computer hostname
Name	                    Computer name
SystemFamily	            Hardware family
SystemSKUNumber	            System SKU
TotalPhysicalMemory	        Installed RAM
NumberOfProcessors	        Physical processors
NumberOfLogicalProcessors	Logical CPUs
PartOfDomain	            Domain membership
DomainRole	                Domain role
PCSystemType	            Desktop, mobile, workstation, server
HypervisorPresent	        Whether Windows reports a hypervisor
UserName	                User currently associated with the computer

computer_name
hostname
manufacturer
model
system_family
system_sku
ram
cpu_count
logical_cpu_count
domain
domain_role
part_of_domain
hypervisor_present

--Win32_Processor--
Name
Manufacturer
NumberOfCores
NumberOfLogicalProcessors
MaxClockSpeed
CurrentClockSpeed
ProcessorId
Architecture
Family
Model
Stepping
L2CacheSize
L3CacheSize
VirtualizationFirmwareEnabled
VMMonitorModeExtensions
SecondLevelAddressTranslationExtensions

cpu_name
cpu_manufacturer
cpu_cores
cpu_logical_processors
cpu_max_clock
cpu_current_clock
cpu_id

virtualization_firmware_enabled
vm_monitor_mode_extensions
slat_supported

-------
-OS-
os_name
os_version
os_build
os_architecture
os_configuration
registered_user
windows_directory
system_directory
system_drive
install_date
last_boot_time
locale

-Computer-
computer_name
hostname
manufacturer
model
system_family
system_type
system_sku
domain
part_of_domain
domain_role

-Hardware-
cpu_name
cpu_manufacturer
cpu_cores
cpu_logical_processors
cpu_max_clock
total_memory

-Security / virtualization-
hypervisor_present
virtualization_firmware_enabled

-Updates-
hotfix_ids
*/