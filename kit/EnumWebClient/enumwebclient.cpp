#include <Windows.h>
#include "base\helpers.h"
#include "enumwebclient.h"
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

void go(char *args, int len)
{
    auto pipeNameTailObf = OBF("\\pipe\\DAV RPC SERVICE");
    
    const CHAR* pipeNameHead = "\\\\";
    const CHAR* pipeNameTail = pipeNameTailObf.get();

    BOOL pipeStatus = 0;
    DWORD pipeError;
    PCHAR hostname;
    size_t nameLength;
	PCHAR nextHostname;
	PCHAR debug;
    int iBytesLen = 0;
    PCHAR hostFileBytes;
    datap parser;

    if (!BofBufferInit(&buffer))
	{
		goto cleanup;
	}

    BeaconDataParse(&parser, args, len);
    hostFileBytes = BeaconDataExtract(&parser, &iBytesLen);
	debug = BeaconDataExtract(&parser, NULL);
	
    if(iBytesLen != 0)
    {
        BofPrintf(&buffer, OBF("running_web_clients:\n").get());
	
        hostname = strtok(hostFileBytes, "\r\n");

        while (hostname != NULL)
        {
            PCHAR fullPipeName;

			nextHostname = strtok(NULL, "\r\n");

            nameLength = strlen(hostname);

            fullPipeName = (PCHAR)malloc(nameLength + strlen(pipeNameHead) + strlen(pipeNameTail) + 1);

            strcpy(fullPipeName, pipeNameHead);
            strcat(fullPipeName, hostname);
            strcat(fullPipeName, pipeNameTail);
		
            pipeStatus = WaitNamedPipeA(fullPipeName, 3000);

            if (pipeStatus == 0)
            {
                pipeError = GetLastError();
            }
            
            BofPrintf(&buffer, "  %s:\n", hostname);

			if (pipeStatus != 0)
            {
                BofPrintf(&buffer, OBF("    available: true\n").get());
                
			}
            else if (strcmp(debug, "debug") == 0)
            {
                BofPrintf(&buffer, OBF("    available: false\n    error_code: %lu\n").get(), pipeError);

                switch (pipeError)
                {
                    case ERROR_FILE_NOT_FOUND:
                        BofPrintf(&buffer, OBF("    error: pipe not found\n").get());
                        break;

                    case ERROR_SEM_TIMEOUT:
                        BofPrintf(&buffer, OBF("    error: timeout\n").get());
                        break;
                    
                    case ERROR_ACCESS_DENIED:
                        BofPrintf(&buffer, OBF("    error: access denied\n").get());
                        break;
                    
                    case ERROR_LOGON_FAILURE:
                        BofPrintf(&buffer, OBF("    error: logon failure\n").get());
                        break;
                    default:
                        BofPrintf(&buffer, OBF("    error: unknown\n").get());
                        break;
                }
            }

            BofPrintf(&buffer, OBF("    pipe_name: %s\n").get(), fullPipeName);

            free(fullPipeName);
            hostname = nextHostname;
        }
    }
    else
    {
        BeaconPrintf(CALLBACK_ERROR, OBF("Couldn't load the host file from disk.\n").get());
    }

cleanup:

    BofBufferFree(&buffer);
}

} // End extern "C"

// Define a main function for the debug build
#if defined(_DEBUG) && !defined(_GTEST)
#include <fstream>
#include <vector>

int main(int argc, char* argv[])
{
	int reqArgCount = 3;

	// Run BOF's entrypoint
	// To pack arguments for the bof use e.g.: bof::runMocked<int, short, const char*>(go, 6502, 42, "foobar");

	if (argc != reqArgCount)
	{
		printf("Usage ... need the args");
		return 1;
	}

     std::ifstream file(argv[1], std::ios::binary | std::ios::ate);

    if (!file)
    {
        printf("Failed to open file\n");
        return 1;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    char* data = new char[size + 1];

    if (!file.read(data, size))
    {
        delete[] data;
        printf("Failed to read file\n");
        return 1;
    }

    data[size] = '\0';
    printf("%s\n", data);

	bof::runMocked<char*&, char*&>(go, data, argv[2]);

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