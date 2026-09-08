#include <Windows.h>
#include "base\helpers.h"
#include "systeminfo.h"
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
    
    BSTR strNetworkResource = SysAllocString(L"ROOT\\CIMV2");
    BSTR strQueryLanguage   = SysAllocString(L"WQL");
    BSTR strOSQuery         = SysAllocString(L"SELECT * FROM Win32_OperatingSystem");
    BSTR strComputerQuery   = SysAllocString(L"SELECT * FROM Win32_ComputerSystem");
    BSTR strQFEngQuery      = SysAllocString(L"SELECT * FROM Win32_QuickFixEngineering");

    VARIANT vtProp;

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

    VariantInit(&vtProp);
    
    // if (bofstart() == FALSE)
    // {
    //     BeaconPrintf(CALLBACK_ERROR, "Not enough memory. Failed to allocate output buffer.\n");
    //     goto cleanup;
    // }

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
            BeaconPrintf(CALLBACK_ERROR, "Failed to CoInitialize COM object with error: 0x%lx.\n", hr);
            goto cleanup;
    }

    hr = CoInitializeSecurity(
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
        CoCreateInstance(
            CLSIDWbemLocator,
            NULL,
            CLSCTX_INPROC_SERVER,
            IIDIWbemLocator,
            (void**)&pLoc
        )
    );
    
    HR_CHECK(
        hr,
        pLoc->ConnectServer(
            strNetworkResource,
            NULL,
            NULL,
            NULL,
            0,
            NULL,
            NULL,
            &pSvc
        )
    );

    HR_CHECK(
        hr,
        CoSetProxyBlanket(
            (IUnknown*)pSvc,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE)
    );

    // char computerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    // DWORD size = sizeof(computerName);

    // if (GetComputerNameA(computerName, &size))
    // {
    //     BofPrintf(&buffer, "%s:\n", computerName);
    // }
    // else
    // {
    //     BofPrintf(&buffer, "UNKNOWN:\n");
    // }

    BofPrintf(&buffer, "system_info:\n");
    
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

                hr = WmiGetProperty(pclsObj, L"Caption", &vtProp);
                BofPrintf(&buffer,
                    "  %-19s%ls\n",
                    "os_name:",
                    SUCCEEDED(hr) && vtProp.vt == VT_BSTR ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                hr = WmiGetProperty(pclsObj, L"Version", &vtProp);
                BofPrintf(&buffer,
                    "  %-19s%ls\n",
                    "version:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                hr = WmiGetProperty(pclsObj, L"ProductType", &vtProp);
                const WCHAR* _config = FAIL_GET_PROP_STRING;

                if (SUCCEEDED(hr))
                {
                    switch(vtProp.uintVal)
                    {
                        case 1:  _config = L"Standalone Workstation"; break;
                        case 2:  _config = L"Domain Controller";      break;
                        case 3:  _config = L"Server";                 break;
                        default: _config = L"Unknown";                break;
                    }
                }

                BofPrintf(&buffer, "  %-19s%ls\n", "os_configuration:", _config);

                hr = WmiGetProperty(pclsObj, L"RegisteredUser", &vtProp);
                BofPrintf(&buffer,
                    "  %-19s%ls\n",
                    "registered_user:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );


                hr = WmiGetProperty(pclsObj, L"WindowsDirectory", &vtProp);
                BofPrintf(&buffer,
                    "  %-19s%ls\n",
                    "windows_directory:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                hr = WmiGetProperty(pclsObj, L"LastBootUpTime", &vtProp);
                BofPrintf(&buffer,
                    "  %-19s%ls\n",
                    "last_boot_time:",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );

                // Try to get locale friendly name
                hr = WmiGetProperty(pclsObj, L"Locale", &vtProp);
                
                LCID _lcid = wcstoul(vtProp.bstrVal, NULL, 16);
                
                WCHAR _localeName[LOCALE_NAME_MAX_LENGTH];

                if (LCIDToLocaleName(_lcid, _localeName, LOCALE_NAME_MAX_LENGTH, 0))
                {
                    BofPrintf(&buffer, "  %-19s%ls\n", "locale:", _localeName);
                }
                else
                {
                    BofPrintf(&buffer, "  %-19s%ls\n", "locale:", vtProp.bstrVal);
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

                hr = WmiGetProperty(pclsObj, L"Model", &vtProp);
                // internal_printf(
                //     "    %-19s%ls\n",
                //     "model:",
                //     SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                // );
                BofPrintf(&buffer,
                    "  %-19s%ls\n",
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
                    "  %-19s%ls\n",
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
                    "  %-19s%ls\n",
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
            BofPrintf(&buffer, "  hotfix_ids:\n");
            
            while (pEnumerator)
            {
                hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
                if (FAILED(hr) || uReturn == 0)
                {
                    break;
                }

                hr = WmiGetProperty(pclsObj, L"HotFixID", &vtProp);
                BofPrintf(
                    &buffer,
                    "    - %ls\n",
                    SUCCEEDED(hr) ? vtProp.bstrVal : FAIL_GET_PROP_STRING
                );
            
                SAFE_INTERFACE_RELEASE(pclsObj);
            }

            SAFE_INTERFACE_RELEASE(pclsObj);
        }
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