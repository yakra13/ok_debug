
// #define _WIN32_WINNT 0x0601
// #include <winsock2.h>
// #ifdef BOF
// #include <ws2ipdef.h>
// #else
// #include <ws2tcpip.h>
// #endif
// #include <windows.h>
// #include <iphlpapi.h>
// #include "bofdefs.h"
// #include "base.c"

// #define WIN32_LEAN_AND_MEAN
// #include <Windows.h>
#include "base\helpers.h"
#include <WinSock2.h>
#include "bofoutput.h"
#include "ipconfig.h"

#ifdef _DEBUG
	#undef DECLSPEC_IMPORT
	#define DECLSPEC_IMPORT
	#include "base\mock.h"
#endif


/* GAA flags - define if not available in mingw headers */
#ifndef GAA_FLAG_INCLUDE_PREFIX
#define GAA_FLAG_INCLUDE_PREFIX          0x0010
#endif
#ifndef GAA_FLAG_INCLUDE_GATEWAYS
#define GAA_FLAG_INCLUDE_GATEWAYS        0x0080
#endif
#ifndef GAA_FLAG_INCLUDE_ALL_INTERFACES
#define GAA_FLAG_INCLUDE_ALL_INTERFACES  0x0100
#endif

#ifndef AF_INET6
#define AF_INET6 23
#endif

#ifndef IF_TYPE_IEEE80211
#define IF_TYPE_IEEE80211 71
#endif

#ifndef IF_TYPE_TUNNEL
#define IF_TYPE_TUNNEL 131
#endif

#ifndef IP_ADAPTER_DHCP_ENABLED
#define IP_ADAPTER_DHCP_ENABLED 0x00000004
#endif

extern "C" {
#include "beacon.h"
#include "sleepmask.h"


BOF_Buffer buffer = { 0 };

const char *dayNames[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
const char *monthNames[] = {"January","February","March","April","May","June", "July","August","September","October","November","December"};

char* Utf16ToUtf8(const wchar_t* input)
{
    int ret = WideCharToMultiByte(
        CP_UTF8,
        0,
        input,
        -1,
        NULL,
        0,
        NULL,
        NULL
    );

    char* newString = (char*)intAlloc(sizeof(char) * ret);

    ret = WideCharToMultiByte(
        CP_UTF8,
        0,
        input,
        -1,
        newString,
        sizeof(char) * ret,
        NULL,
        NULL
    );

    if (0 == ret)
    {
        goto fail;
    }

retloc:
    return newString;
/*location to free everything centrally*/
fail:
    if (newString){
        intFree(newString);
        newString = NULL;
    };
    goto retloc;
}

const char* get_adapter_type_string(DWORD ifType)
{
    switch (ifType)
    {
        case IF_TYPE_ETHERNET_CSMACD:
            return "ethernet_adapter";
        case IF_TYPE_IEEE80211:
            return "wireless_lan_adapter";
        case IF_TYPE_TUNNEL:
            return "tunnel_adapter";
        case IF_TYPE_PPP:
            return "ppp_adapter";
        default:
            return "unknown_adapter";
    }
}

const char* get_node_type_string(UINT nodeType)
{
    switch (nodeType) {
        case 1: return "Broadcast";
        case 2: return "Peer-Peer";
        case 4: return "Mixed";
        case 8: return "Hybrid";
        default: return "Unknown";
    }
}

void format_mac_address(BYTE *addr, DWORD addrLen, char *outBuf, int outBufSize)
{
    int pos = 0;
    DWORD i;
    
    for (i = 0; i < addrLen && pos < outBufSize - 4; i++)
    {
        if (i == addrLen - 1)
        {
            pos += sprintf(outBuf + pos, "%02X", (int)addr[i]);
        }
        else
        {
            pos += sprintf(outBuf + pos, "%02X-", (int)addr[i]);
        }
    }
}

void format_ipv4_address(struct sockaddr *sa, char *outBuf, int outBufSize)
{
    struct sockaddr_in *sa_in = (struct sockaddr_in *)sa;
    
    BYTE *b = (BYTE *)&sa_in->sin_addr;

    sprintf(outBuf, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
}

void format_ipv6_address(struct sockaddr *sa, char *outBuf, int outBufSize)
{
    /* Use InetNtopW then convert to narrow */
    wchar_t wBuf[64];
    struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
    
    LPCWSTR result = InetNtopW(AF_INET6, &sa6->sin6_addr, wBuf, 64);

    if (result)
    {
        wcstombs(outBuf, wBuf, outBufSize);
    }
    else
    {
        strcpy(outBuf, "::?");
    }
}

void prefix_length_to_subnet_mask(UINT8 prefixLen, char *outBuf, int outBufSize)
{
    ULONG mask = 0;
    if (prefixLen > 0 && prefixLen <= 32) 
    {
        mask = ~0UL << (32 - prefixLen);
    }

    sprintf(
        outBuf,
        "%d.%d.%d.%d",
        (mask >> 24) & 0xFF,
        (mask >> 16) & 0xFF,
        (mask >> 8) & 0xFF,
        mask & 0xFF
    );
}

void format_duid(BYTE *duid, DWORD duidLen, char *outBuf, int outBufSize)
{
    int pos = 0;
    DWORD i;

    for (i = 0; i < duidLen && pos < outBufSize - 4; i++)
    {
        if (i == duidLen - 1)
        {
            pos += sprintf(outBuf + pos, "%02X", (int)duid[i]);
        }
        else
        {
            pos += sprintf(outBuf + pos, "%02X-", (int)duid[i]);
        }
    }
}

void format_unix_time(DWORD unixTime, char *outBuf, int outBufSize)
{
    /* Convert Unix timestamp to local time string matching ipconfig format */
    /* e.g. "Saturday, March 8, 2026 10:15:30 PM" */
    
    
    ULONGLONG ft64;
    FILETIME ftUtc, ftLocal;
    SYSTEMTIME st;

    if (unixTime == 0)
    {
        outBuf[0] = '\0';
        return;
    }

    /* Unix epoch to FILETIME: add 11644473600 seconds, convert to 100-ns */
    ft64 = ((ULONGLONG)unixTime + 11644473600ULL) * 10000000ULL;
    ftUtc.dwLowDateTime = (DWORD)(ft64 & 0xFFFFFFFF);
    ftUtc.dwHighDateTime = (DWORD)(ft64 >> 32);

    FileTimeToLocalFileTime(&ftUtc, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &st);

    {
        int hour12 = st.wHour % 12;
        const char *ampm = (st.wHour >= 12) ? "PM" : "AM";
        
        if (hour12 == 0) hour12 = 12;

        sprintf(
            outBuf,
            "%s, %s %d, %d %d:%02d:%02d %s",
            dayNames[st.wDayOfWeek],
            monthNames[st.wMonth - 1],
            st.wDay, st.wYear,
            hour12, st.wMinute, st.wSecond, ampm
        );
    }
}

void get_dhcp_lease_info(const char *adapterName, DWORD *leaseObtained, DWORD *leaseExpires, char *dhcpServer, int dhcpServerBufSize)
{
    /* Read DHCP lease times and server from registry */
    char *regPath = (char *)intAlloc(512);
    HKEY hKey;
    DWORD size, type;

    *leaseObtained = 0;
    *leaseExpires = 0;
    dhcpServer[0] = '\0';

    if (!regPath) return;

    sprintf(regPath,
        "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s",
        adapterName);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        size = sizeof(DWORD);
        type = 0;
        RegQueryValueExA(hKey, "LeaseObtainedTime", NULL, &type, (LPBYTE)leaseObtained, &size);

        size = sizeof(DWORD);
        type = 0;
        RegQueryValueExA(hKey, "LeaseTerminatesTime", NULL, &type, (LPBYTE)leaseExpires, &size);

        size = (DWORD)dhcpServerBufSize;
        type = 0;
        RegQueryValueExA(hKey, "DhcpServer", NULL, &type, (LPBYTE)dhcpServer, &size);

        RegCloseKey(hKey);
    }

    intFree(regPath);
}

int get_netbios_option(const char *adapterName)
{
    /* Read NetBIOS over TCP/IP setting from registry */
    char *regPath = (char *)intAlloc(512);
    HKEY hKey;
    DWORD value = 0;
    DWORD size = sizeof(DWORD);
    DWORD type = 0;
    int result = 0; /* default = enabled */

    if (!regPath) return 0;

    sprintf(
        regPath,
        "SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\Tcpip_%s",
        adapterName
    );

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(hKey, "NetbiosOptions", NULL, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS)
        {
            result = (int)value;
        }
        
        RegCloseKey(hKey);
    }

    intFree(regPath);
    
    return result; /* 0=default(enabled), 1=enabled, 2=disabled */
}

void print_global_section(PFIXED_INFO pFixedInfo)
{
    // internal_printf("\nWindows IP Configuration\n\n");
    // internal_printf("   Host Name . . . . . . . . . . . . : %s\n", pFixedInfo->HostName);
    // internal_printf("   Primary Dns Suffix  . . . . . . . : %s\n", pFixedInfo->DomainName);
    // internal_printf("   Node Type . . . . . . . . . . . . : %s\n", get_node_type_string(pFixedInfo->NodeType));
    // internal_printf("   IP Routing Enabled. . . . . . . . : %s\n", pFixedInfo->EnableRouting ? "Yes" : "No");
    // internal_printf("   WINS Proxy Enabled. . . . . . . . : %s\n", pFixedInfo->EnableProxy ? "Yes" : "No");

    BofPrintf(
        &buffer,
        "%-27s%s\n", "host_name:", pFixedInfo->HostName);
    BofPrintf(&buffer,
        "%-27s%s\n", "primary_dns_suffix:", pFixedInfo->DomainName);
    BofPrintf(&buffer, "%-27s%s\n", "node_type:", get_node_type_string(pFixedInfo->NodeType));
    BofPrintf(&buffer, "%-27s%s\n", "ip_routing_enabled:", pFixedInfo->EnableRouting ? "Yes" : "No");
    BofPrintf(&buffer, "%-27s%s\n", "WINS_proxy_enabled:", pFixedInfo->EnableProxy ? "Yes" : "No");

    /* DNS Suffix Search List from registry */
    {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
            0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            DWORD size = 0;
            DWORD type = 0;
            /* First try SearchList (comma-separated) */
            if (RegQueryValueExA(hKey, "SearchList", NULL, &type, NULL, &size) == ERROR_SUCCESS && size > 1)
            {
                char *searchList = (char *)intAlloc(size + 1);

                if (searchList)
                {
                    if (RegQueryValueExA(hKey, "SearchList", NULL, &type, (LPBYTE)searchList, &size) == ERROR_SUCCESS && searchList[0])
                    {
                        /* Parse comma-separated list */
                        int first = 1;
                        char *ctx = NULL;
                        char *token = strtok_s(searchList, ",", &ctx);

                        while (token)
                        {
                            /* Skip leading spaces */
                            while (*token == ' ') token++;
                            if (*token)
                            {
                                if (first)
                                {
                                    // internal_printf("   DNS Suffix Search List. . . . . . : %s\n", token);
                                    BofPrintf(&buffer, "dns_suffix_search_list:\n");
                                    BofPrintf(&buffer, "  -%s\n", token);

                                    first = 0;
                                }
                                else
                                {
                                    // internal_printf("                                       %s\n", token);
                                    BofPrintf(&buffer, "  -%s\n", token);
                                }
                            }

                            token = strtok_s(NULL, ",", &ctx);
                        }
                    }
                    intFree(searchList);
                }
            }
            else if (pFixedInfo->DomainName[0])
            {
                /* Fall back to DomainName if no SearchList */
                // internal_printf("   DNS Suffix Search List. . . . . . : %s\n", pFixedInfo->DomainName);
                BofPrintf(&buffer, "dns_suffix_search_list:\n");
                BofPrintf(&buffer, "  - %s\n", pFixedInfo->DomainName);
            }

            RegCloseKey(hKey);
        }
        else if (pFixedInfo->DomainName[0])
        {
            // internal_printf("   DNS Suffix Search List. . . . . . : %s\n", pFixedInfo->DomainName);
            BofPrintf(&buffer, "dns_suffix_search_list:\n");
            BofPrintf(&buffer, "  - %s\n", pFixedInfo->DomainName);
        }
    }
}

void print_adapter_section(PIP_ADAPTER_ADDRESSES pAddr)
{
    char addrBuf[80];
    char maskBuf[20];
    char macBuf[24];
    char *utf8Str = NULL;
    int hasDhcpv6Info = 0;

    /* Skip loopback */
    if (pAddr->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
        return;

    /* Adapter header: "\nType FriendlyName:\n\n" */
    utf8Str = Utf16ToUtf8(pAddr->FriendlyName);
    // internal_printf("\n%s %s:\n\n", get_adapter_type_string(pAddr->IfType), utf8Str ? utf8Str : "");
    BofPrintf(&buffer, "%s:\n", get_adapter_type_string(pAddr->IfType));
    BofPrintf(&buffer, "  %-27s%s\n", "name:", utf8Str ? utf8Str : "null");

    if (utf8Str)
    {
        intFree(utf8Str); utf8Str = NULL;
    }

    /* Media State - only show if not up */
    if (pAddr->OperStatus != IfOperStatusUp)
    {
        // internal_printf("   Media State . . . . . . . . . . . : Media disconnected\n");
        BofPrintf(&buffer, "  %-27s%s\n", "media_State:", "disconnected");
    }
    else
    {
        BofPrintf(&buffer, "  %-27s%s\n", "media_State:", "connected");
    }

    /* Connection-specific DNS Suffix */
    utf8Str = Utf16ToUtf8(pAddr->DnsSuffix);
    // internal_printf("   Connection-specific DNS Suffix  . : %s\n", utf8Str ? utf8Str : "");
    if (utf8Str && utf8Str[0] != '\0')
    {
        BofPrintf(&buffer,  "  %-27s%s?\n", "conn_specific_dns_suffix:", utf8Str);
    }
    else
    {
        BofPrintf(&buffer,  "  %-27s%s\n", "conn_specific_dns_suffix:", "null");
    }


    if (utf8Str)
    {
        intFree(utf8Str); utf8Str = NULL;
    }

    /* Description */
    utf8Str = Utf16ToUtf8(pAddr->Description);
    // internal_printf("   Description . . . . . . . . . . . : %s\n", utf8Str ? utf8Str : "");
    BofPrintf(&buffer,  "  %-27s%s\n", "description:", utf8Str ? utf8Str : "null");

    if (utf8Str)
    {
        intFree(utf8Str); utf8Str = NULL;
    }

    /* Physical Address */
    if (pAddr->PhysicalAddressLength > 0)
    {
        format_mac_address(pAddr->PhysicalAddress, pAddr->PhysicalAddressLength, macBuf, sizeof(macBuf));
        // internal_printf("   Physical Address. . . . . . . . . : %s\n", macBuf);
        BofPrintf(&buffer,  "  %-27s%s\n", "physical_address:", macBuf);
    }

    /* DHCP Enabled */
    // internal_printf("   DHCP Enabled. . . . . . . . . . . : %s\n",
    //     (pAddr->Flags & IP_ADAPTER_DHCP_ENABLED) ? "Yes" : "No");

    BofPrintf(&buffer,  "  %-27s%s\n", "dhcp_enabled:", (pAddr->Flags & IP_ADAPTER_DHCP_ENABLED) ? "true" : "false");

    /* Autoconfiguration Enabled */
    // internal_printf("   Autoconfiguration Enabled . . . . : Yes\n");
    // TODO: how do we know this is true?
    BofPrintf(&buffer,  "  %-27s%s\n", "autoconfig_enabled:", "true");

    /* If adapter is disconnected, stop here */
    if (pAddr->OperStatus != IfOperStatusUp)
    {
        return;
    }

    /* Walk unicast addresses - IPv6 first, then IPv4 (matching ipconfig order) */
    {
        PIP_ADAPTER_UNICAST_ADDRESS pUni;

        /* First pass: IPv6 addresses */
        for (pUni = pAddr->FirstUnicastAddress; pUni; pUni = pUni->Next)
        {
            if (pUni->Address.lpSockaddr->sa_family == AF_INET6)
            {
                struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)pUni->Address.lpSockaddr;
                format_ipv6_address(pUni->Address.lpSockaddr, addrBuf, sizeof(addrBuf));

                /* Check if link-local (fe80::) */
                BYTE *ipv6bytes = (BYTE *)&sa6->sin6_addr;

                if (ipv6bytes[0] == 0xfe && ipv6bytes[1] == 0x80)
                {
                    // internal_printf("   Link-local IPv6 Address . . . . . : %s%%%lu(Preferred) \n",
                        // addrBuf, (unsigned long)sa6->sin6_scope_id);
                    BofPrintf(
                        &buffer,
                         "  %-27s%s%%%lu(Preferred)\n", "link_local_ipv6_address:",
                        addrBuf,
                        (unsigned long)sa6->sin6_scope_id
                    );
                }
                else
                {
                    // internal_printf("   IPv6 Address. . . . . . . . . . . : %s(Preferred) \n", addrBuf);
                    BofPrintf(&buffer, "  %-27s%s(Preferred)\n", "ipv6_address:", addrBuf);
                }
            }
        }

        /* Second pass: IPv4 addresses */
        for (pUni = pAddr->FirstUnicastAddress; pUni; pUni = pUni->Next)
        {
            if (pUni->Address.lpSockaddr->sa_family == AF_INET)
            {
                format_ipv4_address(pUni->Address.lpSockaddr, addrBuf, sizeof(addrBuf));

                // internal_printf("   IPv4 Address. . . . . . . . . . . : %s(Preferred) \n", addrBuf);
                BofPrintf(&buffer, "  %-27s%s(Preferred)\n", "ipv4_address:", addrBuf);

                /* Subnet Mask from prefix length */
                prefix_length_to_subnet_mask(pUni->OnLinkPrefixLength, maskBuf, sizeof(maskBuf));
                // internal_printf("   Subnet Mask . . . . . . . . . . . : %s\n", maskBuf);
                BofPrintf(&buffer, "  %-27s%s\n", "subnet_mask:", maskBuf);
            }
        }
    }

    /* Lease Obtained / Lease Expires (only for DHCP-enabled adapters with IPv4) */
    if (pAddr->Flags & IP_ADAPTER_DHCP_ENABLED)
    {
        DWORD leaseObtained = 0, leaseExpires = 0;
        char dhcpServerStr[64];
        char timeBuf[128];

        get_dhcp_lease_info(pAddr->AdapterName, &leaseObtained, &leaseExpires, dhcpServerStr, sizeof(dhcpServerStr));

        if (leaseObtained)
        {
            format_unix_time(leaseObtained, timeBuf, sizeof(timeBuf));
            // internal_printf("   Lease Obtained. . . . . . . . . . : %s\n", timeBuf);
            BofPrintf(&buffer, "  %-27s%s\n", "lease_obtained:", timeBuf);
        }

        if (leaseExpires)
        {
            format_unix_time(leaseExpires, timeBuf, sizeof(timeBuf));
            // internal_printf("   Lease Expires . . . . . . . . . . : %s\n", timeBuf);
            BofPrintf(&buffer, "  %-27s%s\n", "lease_expires:", timeBuf);
        }
    }

    /* Default Gateway */
    {
        PIP_ADAPTER_GATEWAY_ADDRESS_LH pGw = pAddr->FirstGatewayAddress;
        if (pGw)
        {
            if (pGw->Address.lpSockaddr->sa_family == AF_INET)
            {
                format_ipv4_address(pGw->Address.lpSockaddr, addrBuf, sizeof(addrBuf));
            }
            else if (pGw->Address.lpSockaddr->sa_family == AF_INET6)
            {
                format_ipv6_address(pGw->Address.lpSockaddr, addrBuf, sizeof(addrBuf));
            }
            else
            {
                addrBuf[0] = '\0';
            }
            // internal_printf("   Default Gateway . . . . . . . . . : %s\n", addrBuf);
            BofPrintf(&buffer, "  default_gateway:\n    - %s\n", addrBuf);
            
            pGw = pGw->Next;
            
            while (pGw)
            {
                if (pGw->Address.lpSockaddr->sa_family == AF_INET)
                {
                    format_ipv4_address(pGw->Address.lpSockaddr, addrBuf, sizeof(addrBuf));
                }
                else if (pGw->Address.lpSockaddr->sa_family == AF_INET6)
                {
                    format_ipv6_address(pGw->Address.lpSockaddr, addrBuf, sizeof(addrBuf));
                }
                // internal_printf("                                       %s\n", addrBuf);
                BofPrintf(&buffer, "    - %s\n", addrBuf);
                
                pGw = pGw->Next;
            }
        }
        else
        {
            // internal_printf("   Default Gateway . . . . . . . . . : \n");
            BofPrintf(&buffer, "  default_gateway:\n   - null\n");
        }
    }

    /* DHCP Server (after gateway, only for DHCP-enabled) */
    if (pAddr->Flags & IP_ADAPTER_DHCP_ENABLED)
    {
        /* Try Dhcpv4Server from adapter structure first */
        if (pAddr->Dhcpv4Server.iSockaddrLength > 0 &&
            pAddr->Dhcpv4Server.lpSockaddr &&
            pAddr->Dhcpv4Server.lpSockaddr->sa_family == AF_INET)
        {
            format_ipv4_address(pAddr->Dhcpv4Server.lpSockaddr, addrBuf, sizeof(addrBuf));
            /* Only print if not 0.0.0.0 or 255.255.255.255 */
            if (strcmp(addrBuf, "0.0.0.0") != 0 && strcmp(addrBuf, "255.255.255.255") != 0)
            {
                // internal_printf("   DHCP Server . . . . . . . . . . . : %s\n", addrBuf);
                BofPrintf(&buffer, "  %-27s%s\n", "dhcp_server:", addrBuf);
            }
        }
        else
        {
            /* Fallback: read from registry */
            char dhcpSrv[64];
            DWORD dummy1 = 0, dummy2 = 0;
            get_dhcp_lease_info(pAddr->AdapterName, &dummy1, &dummy2, dhcpSrv, sizeof(dhcpSrv));
        
            if (dhcpSrv[0] && strcmp(dhcpSrv, "255.255.255.255") != 0)
            {
                // internal_printf("   DHCP Server . . . . . . . . . . . : %s\n", dhcpSrv);
                BofPrintf(&buffer, "  %-27s%s\n", "dhcp_server:", dhcpSrv);
            }
        }
    }

    /* DHCPv6 IAID and Client DUID - only show if adapter has IPv6 unicast addresses */
    {
        PIP_ADAPTER_UNICAST_ADDRESS pUni;
        for (pUni = pAddr->FirstUnicastAddress; pUni; pUni = pUni->Next)
        {
            if (pUni->Address.lpSockaddr->sa_family == AF_INET6)
            {
                hasDhcpv6Info = 1;
                break;
            }
        }
    }
    if (hasDhcpv6Info)
    {
        // internal_printf("   DHCPv6 IAID . . . . . . . . . . . : %lu\n", (unsigned long)pAddr->Dhcpv6Iaid);
        BofPrintf(&buffer, "  %-27s%lu\n", "dhcpv6_iaid:", (unsigned long)pAddr->Dhcpv6Iaid);

        if (pAddr->Dhcpv6ClientDuidLength > 0)
        {
            char *duidBuf = (char *)intAlloc(pAddr->Dhcpv6ClientDuidLength * 4);
            
            if (duidBuf)
            {
                format_duid(pAddr->Dhcpv6ClientDuid, pAddr->Dhcpv6ClientDuidLength, duidBuf, pAddr->Dhcpv6ClientDuidLength * 4);
                // internal_printf("   DHCPv6 Client DUID. . . . . . . . : %s\n", duidBuf);
                BofPrintf(&buffer, "  %-27s%s\n", "dhcpv6_client_duid:", duidBuf);
                intFree(duidBuf);
            }
        }
    }

    /* DNS Servers */
    {
        PIP_ADAPTER_DNS_SERVER_ADDRESS pDns = pAddr->FirstDnsServerAddress;
        int first = 1;
        while (pDns)
        {
            addrBuf[0] = '\0';
        
            if (pDns->Address.lpSockaddr->sa_family == AF_INET)
            {
                format_ipv4_address(pDns->Address.lpSockaddr, addrBuf, sizeof(addrBuf));
            }
            else if (pDns->Address.lpSockaddr->sa_family == AF_INET6)
            {
                format_ipv6_address(pDns->Address.lpSockaddr, addrBuf, sizeof(addrBuf));
            }

            if (first)
            {
                // internal_printf("   DNS Servers . . . . . . . . . . . : %s\n", addrBuf);
                BofPrintf(&buffer, "  dns_servers:\n", addrBuf);
                BofPrintf(&buffer, "    - %s\n", addrBuf);
                first = 0;
            }
            else
            {
                // internal_printf("                                       %s\n", addrBuf);
                // BofPrintf(&buffer, "                                       %s\n", addrBuf);
                BofPrintf(&buffer, "    - %s\n", addrBuf);
            }

            pDns = pDns->Next;
        }
    }

    /* NetBIOS over Tcpip */
    {
        int nbOpt = get_netbios_option(pAddr->AdapterName);
        // internal_printf("   NetBIOS over Tcpip. . . . . . . . : %s\n",
        //     (nbOpt == 2) ? "Disabled" : "Enabled");
        BofPrintf(&buffer, "  %-27s%s\n", "netbios_over_tcpip:", (nbOpt == 2) ? "Disabled" : "Enabled");
    }

    /* Connection-specific DNS Suffix Search List */
    {
        PIP_ADAPTER_DNS_SUFFIX pSuffix = pAddr->FirstDnsSuffix;
        int first = 1;
        while (pSuffix)
        {
            if (pSuffix->String[0])
            {
                utf8Str = Utf16ToUtf8(pSuffix->String);
                if (utf8Str)
                {
                    if (first)
                    {
                        // internal_printf("   Connection-specific DNS Suffix Search List :\n");
                        BofPrintf(&buffer, "  connection_specific_dns_suffix_search_list:\n");
                        first = 0;
                    }

                    // internal_printf("                                       %s\n", utf8Str);
                    BofPrintf(&buffer, "    - %s\n", utf8Str);
                    intFree(utf8Str);
                    utf8Str = NULL;
                }
            }
            pSuffix = pSuffix->Next;
        }
    }
}

void getIPInfo(void)
{
    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pCurr = NULL;
    PFIXED_INFO pFixedInfo = NULL;
    ULONG addrBufLen = 0;
    ULONG netBufLen = 0;
    DWORD ret;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;

    BeaconPrintf(CALLBACK_ERROR, "in get ip info");

    /* Get adapter addresses - two-call pattern */
    ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, NULL, &addrBufLen);

    if (ret != ERROR_BUFFER_OVERFLOW)
    {
        BeaconPrintf(CALLBACK_ERROR, "GetAdaptersAddresses failed: %lu", ret);
        goto END;
    }

    pAddresses = (PIP_ADAPTER_ADDRESSES)intAlloc(addrBufLen);
    
    if (!pAddresses)
    {
        BeaconPrintf(CALLBACK_ERROR, "Memory allocation failed for adapter addresses");
        goto END;
    }
    
    ret = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAddresses, &addrBufLen);
    
    if (ret != ERROR_SUCCESS)
    {
        BeaconPrintf(CALLBACK_ERROR, "GetAdaptersAddresses failed: %lu", ret);
        goto END;
    }

    /* Get network params - two-call pattern */
    if (GetNetworkParams(NULL, &netBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        pFixedInfo = (PFIXED_INFO)intAlloc(netBufLen);

        if (!pFixedInfo)
        {
            BeaconPrintf(CALLBACK_ERROR, "Memory allocation failed for network params");
            goto END;
        }

        if (GetNetworkParams(pFixedInfo, &netBufLen) != NO_ERROR)
        {
            BeaconPrintf(CALLBACK_ERROR, "GetNetworkParams failed");
            goto END;
        }

    }
    else
    {
        BeaconPrintf(CALLBACK_ERROR, "GetNetworkParams failed to get buffer size");
        goto END;
    }

    /* Print global section */
    print_global_section(pFixedInfo);

    /* Print per-adapter sections */
    for (pCurr = pAddresses; pCurr; pCurr = pCurr->Next)
    {
        print_adapter_section(pCurr);
    }

END:
    if (pAddresses)
    {
        intFree(pAddresses);
    }
    if (pFixedInfo)
    {
        intFree(pFixedInfo);
    }
}

// #ifdef BOF

void go(char* args, int len)
{
    BeaconPrintf(CALLBACK_ERROR, "We are in go");
    if (!BofBufferInit(&buffer))
	{
		goto cleanup;
	}

    getIPInfo();

cleanup:
    BofBufferFree(&buffer);
}
} // End extern "C"

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