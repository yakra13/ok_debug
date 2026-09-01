#include <Windows.h>
#include "base\helpers.h"
#include "enumlocalcert.h"
#include "bofoutput.h"

#ifdef _DEBUG
	#undef DECLSPEC_IMPORT
	#define DECLSPEC_IMPORT
	#include "base\mock.h"
#endif

extern "C" {
#include "beacon.h"
#include "sleepmask.h"

// Global output buffer for beacon CALLBACK_OUTPUT
// You can use CALLBACK_OUTPUT_UTF8 to send updates to CS stdout
// Ensure to call BofBufferFree() when done to return any remaining data to CS
BOF_Buffer buffer = { 0 };

void PrintCertProperties(PCCERT_CONTEXT pCertContext)
{
    LPWSTR pszName = NULL;
    DWORD dwSize;

	LPWSTR pszFriendlyName = NULL;

	LPWSTR pszIssuedBy = NULL;

	BYTE thumbprint[20];
	DWORD thumbprintSize = sizeof(thumbprint);
	WCHAR thumbprintStr[41] = { L'\0' };
	
	SYSTEMTIME stExpirationDate;
	WCHAR szExpirationDate[256] = { L'\0' };
	
	PCERT_ENHKEY_USAGE pUsage = NULL;
	DWORD dwUsageSize = 0;

	DWORD dwIssueBySize = 0;
	DWORD dwFriendlyNameSize = 0;

	// LPWSTR pszThumbprint = NULL;
	// LPWSTR pszExpiration = NULL;


    // Get the "Issued By" property
    if (!CertGetNameStringW(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0))
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to list certificates in specified store.\n");
		goto cleanup;
	}

    dwIssueBySize 	   = CertGetNameStringW(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
	dwFriendlyNameSize = CertGetNameStringW(pCertContext, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, NULL, 0);

    pszIssuedBy = (LPWSTR)LocalAlloc(LPTR, dwIssueBySize * sizeof(wchar_t));
    if (pszIssuedBy == NULL)
	{
		BeaconPrintf(CALLBACK_ERROR, "Failed to allocate memory.\n");
		goto cleanup;
	}

	pszFriendlyName = (LPWSTR)LocalAlloc(LPTR, dwFriendlyNameSize * sizeof(wchar_t));
	if (pszFriendlyName == NULL) 
	{ 
		BeaconPrintf(CALLBACK_ERROR, "Failed to allocate memory.\n");
		goto cleanup; 
	}

	// Init "Intended Purposes" property
	if (CertGetEnhancedKeyUsage(pCertContext, 0, NULL, &dwUsageSize))
	{
		pUsage = (PCERT_ENHKEY_USAGE)LocalAlloc(LPTR, dwUsageSize);

		if (pUsage == NULL)
		{
			BeaconPrintf(CALLBACK_ERROR, "Failed to allocate memory.\n");
			goto cleanup;
		}
	}

	// Get the "Issued By" property
    if (!CertGetNameStringW(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, pszIssuedBy, dwIssueBySize))
	{
		pszIssuedBy[0] = L'\0';
	}
	
	// Get the "Thumbprint" property
	if (CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, thumbprint, &thumbprintSize))
	{
		for (DWORD i = 0; i < thumbprintSize; ++i)
		{
			_snwprintf_s(thumbprintStr + (i * 2), 3, 2, L"%02X", thumbprint[i]);
		}

		thumbprintStr[40] = L'\0';
	}

	// Get "Friendly Name" property
	if (!CertGetNameStringW(pCertContext, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, pszFriendlyName, dwFriendlyNameSize))
	{
		pszFriendlyName[0] = L'\0';
	}

	// Get the "Expiration Date" property
	FileTimeToSystemTime(&pCertContext->pCertInfo->NotAfter, &stExpirationDate);

	GetDateFormatW(
		LOCALE_USER_DEFAULT,
		0,
		&stExpirationDate,
		L"yyyy-MM-dd",
		szExpirationDate,
		sizeof(szExpirationDate) / sizeof(WCHAR)
	);

cleanup:
	BofPrintf(
		&buffer,
		"    - friendly_name: %ls\n",
		(dwFriendlyNameSize > 1 || pszFriendlyName[0] != L'\0') ? pszFriendlyName : L"null"
	);

	BofPrintf(
		&buffer,
		"      issued_by: %ls\n",
		(pszIssuedBy && pszIssuedBy[0] != L'\0') ? pszIssuedBy : L"null"
	);

	BofPrintf(&buffer, "      thumbprint: %ls\n", thumbprintStr[0] != L'\0' ? thumbprintStr : L"null");
	
	BofPrintf(&buffer, "      expiration: %ls\n", szExpirationDate[0] != L'\0' ? szExpirationDate : L"null");
	
	BofPrintf(&buffer, "      purpose:\n");

	if (pUsage && CertGetEnhancedKeyUsage(pCertContext, 0, pUsage, &dwUsageSize))
	{
		for (DWORD i = 0; i < pUsage->cUsageIdentifier; ++i)
		{
			LPCSTR pszOID = pUsage->rgpszUsageIdentifier[i];
			PCCRYPT_OID_INFO pInfo = CryptFindOIDInfo(CRYPT_OID_INFO_OID_KEY, (void*)pszOID, 0);

			BofPrintf(
				&buffer,
				"        - { name: %ls, oid: %s }\n",
				pInfo ? pInfo->pwszName : L"UNKNOWN_OID",
				pszOID
			);
		}
	}

	if (pszIssuedBy) { LocalFree(pszIssuedBy); }
	if (pszFriendlyName) { LocalFree(pszFriendlyName); }
	if (pUsage) { LocalFree(pUsage); }
}

void go(char *args, int len)
{
	// BOOL res = NULL;
	PCHAR store; // Options: ROOT, MY, TRUST, CA, USERDS, AuthRoot, Disallowed
	PWCHAR wStore;
	HCERTSTORE hStore = NULL;
	datap parser;
	PCCERT_CONTEXT pCertContext = NULL;

	int req = 0;

	if (!BofBufferInit(&buffer))
	{
		goto cleanup;
	}
	
	BeaconDataParse(&parser, args, len);
	store = (PCHAR)BeaconDataExtract(&parser, NULL);


	req = MultiByteToWideChar(CP_ACP, 0, store, -1, NULL, 0);

	if (req == 0)
	{
		BeaconPrintf(CALLBACK_ERROR, "MultiByteToWideChar failed: %lu", GetLastError());
		goto cleanup;
	}

	wStore = (PWCHAR)LocalAlloc(LPTR, req * sizeof(WCHAR));

	if (MultiByteToWideChar(CP_ACP, 0, store, -1, wStore, req) == 0)
	{
		BeaconPrintf(CALLBACK_ERROR, "MultiByteToWideChar failed: %lu", GetLastError());
		goto cleanup;
	}

	// Open Local Computer store
	hStore = CertOpenStore(
		CERT_STORE_PROV_SYSTEM_W,
		0,
		(HCRYPTPROV)NULL,
		CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG,
		wStore
	); 
	
	if (!hStore)
	{
		DWORD error = GetLastError();
		BeaconPrintf(CALLBACK_ERROR, "Failed to open specified certificate store: error %lu\n", error);
		goto cleanup;
	}

	while (pCertContext = CertEnumCertificatesInStore(hStore, pCertContext))
	{
		PrintCertProperties(pCertContext);
	}
	
cleanup:

	// if(!res)
	// {
	// 	BeaconPrintf(CALLBACK_ERROR, "Failed to list certificates in specified store.\n");
	// }

	if (pCertContext) { CertFreeCertificateContext(pCertContext); }
	
	if (hStore) { CertCloseStore(hStore, 0); }

	if (wStore) { LocalFree(wStore); }

	BofBufferFree(&buffer);
}
} // end extern "C"

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
	int reqArgCount = 2;

	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");

	if (argc != reqArgCount)
	{
		printf("Usage ... need the args");
		return 1;
	}
	printf("store: %s\n", argv[1]);

	bof::runMocked<char*&>(go, argv[1]);

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