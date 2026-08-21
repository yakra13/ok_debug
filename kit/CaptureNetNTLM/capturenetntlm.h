#pragma once
#include "base/helpers.h"
#include <Windows.h>
#include <tchar.h>

#define SECURITY_WIN32
#include <Security.h>
#include <Wincrypt.h>

#pragma comment(lib,"Secur32")

#define MSV1_0_CHALLENGE_LENGTH 8
// 
extern "C" {
#ifndef _DEBUG
    //GetNTLMChallengeAndResponse
    //DECLSPEC_IMPORT void* __cdecl MSVCRT$memcpy(void* _Dst, const void* _Src, size_t _Size);
    //DECLSPEC_IMPORT SECURITY_STATUS WINAPI SECUR32$AcquireCredentialsHandleW(SEC_WCHAR* pszPrincipal, SEC_WCHAR* pszPackage, ULONG fCredentialUse, PLUID pvLogonID, PVOID pAuthData, SEC_GET_KEY_FN pGetKeyFn, PVOID pvGetKeyArgument, PCredHandle phCredential, PTimeStamp ptsExpiry);
    //DECLSPEC_IMPORT SECURITY_STATUS WINAPI SECUR32$InitializeSecurityContextW(PCredHandle phCredential, PCtxtHandle phContext, SEC_WCHAR* pszTargetName, ULONG fContextReq, ULONG Reserved1, ULONG TargetDataRep, PSecBufferDesc pInput, ULONG Reserved2, PCtxtHandle phNewContext, PSecBufferDesc pOutput, PULONG pfContextAttr, PTimeStamp ptsExpiry);
    DECLSPEC_IMPORT SECURITY_STATUS WINAPI SECUR32$AcceptSecurityContext(PCredHandle phCredential, PCtxtHandle phContext, PSecBufferDesc pInput, ULONG fContextReq, ULONG TargetDataRep, PCtxtHandle phNewContext, PSecBufferDesc pOutput, PULONG pfContextAttr, PTimeStamp ptsExpiry);
    DECLSPEC_IMPORT SECURITY_STATUS WINAPI SECUR32$AcquireCredentialsHandleA(LPCTSTR, LPCTSTR, ULONG, PLUID, PVOID, SEC_GET_KEY_FN, PVOID, PCredHandle, PTimeStamp);
    DECLSPEC_IMPORT SECURITY_STATUS WINAPI SECUR32$InitializeSecurityContextA(PCredHandle, PCtxtHandle, SEC_CHAR *, ULONG, ULONG, ULONG, PSecBufferDesc, ULONG, PCtxtHandle, PSecBufferDesc, PULONG, PTimeStamp);
    
    WINBASEAPI void* __cdecl MSVCRT$calloc(size_t number, size_t size);
    WINBASEAPI void  __cdecl MSVCRT$free(void *memblock);
    WINBASEAPI void* WINAPI  MSVCRT$memcpy(void* dest, const void* src, size_t count);
    WINBASEAPI void  __cdecl MSVCRT$memset(void *dest, int c, size_t count);
    WINBASEAPI int   __cdecl MSVCRT$printf(const char * _Format,...);
    WINBASEAPI int   WINAPI  MSVCRT$vsnprintf(char* buffer, size_t count, const char* format, va_list arg);

    WINBASEAPI HANDLE WINAPI KERNEL32$GetProcessHeap();
    WINBASEAPI LPVOID WINAPI KERNEL32$HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    WINBASEAPI BOOL   WINAPI KERNEL32$HeapFree(HANDLE, DWORD, PVOID);
#endif
}
#ifndef _DEBUG
    #define AcceptSecurityContext      SECUR32$AcceptSecurityContext
    #define AcquireCredentialsHandleA  SECUR32$AcquireCredentialsHandleA
    #define InitializeSecurityContextA SECUR32$InitializeSecurityContextA
    
    #define calloc                     MSVCRT$calloc
    #define free                       MSVCRT$free
    #define memcpy                     MSVCRT$memcpy
    #define memset                     MSVCRT$memset
    #define printf                     MSVCRT$printf
    #define vsnprintf                  MSVCRT$vsnprintf

    #define GetProcessHeap             KERNEL32$GetProcessHeap
    #define HeapAlloc                  KERNEL32$HeapAlloc
    #define HeapFree                   KERNEL32$HeapFree
#endif


//
//Most of the code originates from: https://github.com/leechristensen/GetNTLMChallenge/tree/master
//

typedef enum {
	NtLmNegotiate = 1,
	NtLmChallenge,
	NtLmAuthenticate,
	NtLmUnknown
} NTLM_MESSAGE_TYPE;

typedef struct _STRING32 {
	USHORT Length;
	USHORT MaximumLength;
	DWORD  Offset;
} STRING32, *PSTRING32;

// Valid values of NegotiateFlags
#define NTLMSSP_NEGOTIATE_UNICODE               0x00000001
#define NTLMSSP_NEGOTIATE_OEM                   0x00000002  
#define NTLMSSP_REQUEST_TARGET                  0x00000004  
#define NTLMSSP_NEGOTIATE_SIGN                  0x00000010  
#define NTLMSSP_NEGOTIATE_SEAL                  0x00000020  
#define NTLMSSP_NEGOTIATE_DATAGRAM              0x00000040  
#define NTLMSSP_NEGOTIATE_NTLM                  0x00000200  
#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED       0x1000 
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED  0x2000  
#define NTLMSSP_NEGOTIATE_LOCAL_CALL            0x00004000  
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN           0x00008000  

// Valid target types returned by the server in Negotiate Flags
#define NTLMSSP_TARGET_TYPE_DOMAIN              0x00010000 
#define NTLMSSP_TARGET_TYPE_SERVER              0x00020000  
#define NTLMSSP_TARGET_TYPE_SHARE               0x00040000  
#define NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY   0x00080000  
#define NTLMSSP_NEGOTIATE_IDENTIFY              0x00100000  

// Valid requests for additional output buffers
#define NTLMSSP_REQUEST_ACCEPT_RESPONSE         0x00200000 
#define NTLMSSP_REQUEST_NON_NT_SESSION_KEY      0x00400000  
#define NTLMSSP_NEGOTIATE_TARGET_INFO           0x00800000 
#define NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT      0x01000000 
#define NTLMSSP_NEGOTIATE_VERSION               0x02000000  
#define NTLMSSP_NEGOTIATE_128                   0x20000000 
#define NTLMSSP_NEGOTIATE_KEY_EXCH              0x40000000 
#define NTLMSSP_NEGOTIATE_56                    0x80000000

// flags used in client space to control sign and seal; never appear on the wire
#define NTLMSSP_APP_SEQ           0x0040  

#define MsvAvEOL                  0x0000
#define MsvAvNbComputerName       0x0001
#define MsvAvNbDomainName         0x0002
#define MsvAvNbDnsComputerName    0x0003
#define MsvAvNbDnsDomainName      0x0004
#define MsvAvNbDnsTreeName        0x0005
#define MsvAvFlags                0x0006
#define MsvAvTimestamp            0x0007
#define MsvAvRestrictions         0x0008
#define MsvAvTargetName           0x0009
#define MsvAvChannelBindings      0x000A

typedef struct _NTLM_VERSION {
	BYTE ProductMajorVersion;
	BYTE ProductMinorVersion;
	USHORT ProductBuild;
	BYTE reserved[3];
	BYTE NTLMRevisionCurrent;
} NTLM_VERSION, *PNTLM_VERSION;

typedef struct _NTLMv2_CLIENT_CHALLENGE {
	BYTE RespType;
	BYTE HiRespType;
	USHORT Reserved1;
	DWORD Reserved2;
	ULONGLONG TimeStamp;
	BYTE ChallengeFromClient[8];
	DWORD Reserved3;
	BYTE AvPair[4];
} NTLMv2_CLIENT_CHALLENGE, *PNTLMv2_CLIENT_CHALLENGE;

typedef struct _NTLMv2_RESPONSE {
	BYTE Response[16];
	NTLMv2_CLIENT_CHALLENGE Challenge;
} NTLMv2_RESPONSE, *PNTLMv2_RESPONSE;

typedef struct _NEGOTIATE_MESSAGE {
	UCHAR Signature[8];
	DWORD MessageType;
	DWORD NegotiateFlags;
	STRING32 OemDomainName;
	STRING32 OemWorkstationName;
} NEGOTIATE_MESSAGE, *PNEGOTIATE_MESSAGE;

typedef struct _NEGOTIATE_MESSAGE_WITH_VERSION {
	UCHAR Signature[8];
	DWORD MessageType;
	DWORD NegotiateFlags;
	STRING32 OemDomainName;
	STRING32 OemWorkstationName;
	NTLM_VERSION Version;
} NEGOTIATE_MESSAGE_WITH_VERSION, *PNEGOTIATE_MESSAGE_WITH_VERSION;

typedef struct _CHALLENGE_MESSAGE {
	UCHAR Signature[8];
	DWORD MessageType;
	STRING32 TargetName;
	DWORD NegotiateFlags;
	UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
	ULONG64 ServerContextHandle;
	STRING32 TargetInfo;
} CHALLENGE_MESSAGE, *PCHALLENGE_MESSAGE;

typedef struct _CHALLENGE_MESSAGE_WITH_VERSION {
	UCHAR Signature[8];
	DWORD MessageType;
	STRING32 TargetName;
	DWORD NegotiateFlags;
	UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
	ULONG64 ServerContextHandle;
	STRING32 TargetInfo;
	NTLM_VERSION Version;
} CHALLENGE_MESSAGE_WITH_VERSION, *PCHALLENGE_MESSAGE_WITH_VERSION;

typedef struct _AUTHENTICATE_MESSAGE {
	UCHAR Signature[8];
	DWORD MessageType;
	STRING32 LmChallengeResponse;
	STRING32 NtChallengeResponse;
	STRING32 DomainName;
	STRING32 UserName;
	STRING32 Workstation;
	STRING32 SessionKey;
	DWORD NegotiateFlags;
} AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

typedef struct _RESTRICTIONS_ENCODING {
	DWORD dwSize;
	DWORD dwReserved;
	DWORD dwIntegrityLevel;
	DWORD dwSubjectIntegrityLevel;
	BYTE MachineId[32];
} RESTRICTIONS_ENCODING, *PRESTRICTIONS_ENCODING;

typedef LONG NTSTATUS;
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _KEY_BLOB {
	BYTE   bType;
	BYTE   bVersion;
	WORD   reserved;
	ALG_ID aiKeyAlg;
	ULONG keysize;
	BYTE Data[16];
} KEY_BLOB;

NTSTATUS WINAPI SystemFunction007(PUNICODE_STRING string, LPBYTE hash);