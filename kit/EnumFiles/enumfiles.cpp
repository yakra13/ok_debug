#include <Windows.h>
#include "base\helpers.h"
#include "enumfiles.h"
#include "bofoutput.h"

#ifdef _DEBUG
#undef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT
#include "base\mock.h"
#endif

#define MAX_PREVIEW_LENGTH 200

extern "C"{
#include "beacon.h"
#include "sleepmask.h"

BOF_Buffer buffer = {0};

bool keywordMatches(const char* content, const char* keyword)
{
    size_t keywordLen = strlen(keyword);
    
    // If keyword is "*example*"
    if (keyword[0] == '*' && keyword[keywordLen - 1] == '*')
    {
        char tempKeyword[MAX_PATH]; 
        strncpy(tempKeyword, keyword + 1, keywordLen - 2);
        tempKeyword[keywordLen - 2] = '\0';
    
        if (strstr(content, tempKeyword))
        {
            return true;
        }
    }
    // If keyword is "example*"
    else if (keyword[keywordLen - 1] == '*')
    {
        char tempKeyword[MAX_PATH];
        strncpy(tempKeyword, keyword, keywordLen - 1);
        tempKeyword[keywordLen - 1] = '\0';
    
        if (strncmp(content, tempKeyword, keywordLen - 1) == 0)
        {
            return true;
        }
    }
    // If keyword is "*example"
    else if (keyword[0] == '*')
    {
        if (strlen(content) >= keywordLen - 1 && 
            strcmp(content + strlen(content) - (keywordLen - 1), keyword + 1) == 0)
        {
            return true;
        }
    }
    // If keyword is "example"
    else if (strstr(content, keyword))
    {
        return true;
    }

    return false;
}

bool SearchFileForKeyword(const char* filePath, const char* keyword)
{
    FILE *file = fopen(filePath, "rb");  

    if (!file)
    {
        BeaconPrintf(CALLBACK_ERROR, "Failed to open file: %s\n", filePath);
        return false;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* fileContents = (char*)malloc(fileSize + 1); 
    
    if(!fileContents)
    {
        BeaconPrintf(CALLBACK_ERROR, "Failed to allocate memory for file: %s\n", filePath);
        fclose(file);
        return false;
    }
    
    fread(fileContents, 1, fileSize, file);
    fileContents[fileSize] = '\0';  
    fclose(file);

    // Convert file contents to lowercase
    for (long i = 0; i < fileSize; i++)
    {
        fileContents[i] = tolower(fileContents[i]);
    }

    // Convert keyword to lowercase
    char* lowerKeyword = _strdup(keyword);
    
    if (!lowerKeyword)
    {
        free(fileContents);
        return false;
    }

    for (int i = 0; lowerKeyword[i]; i++)
    {
        lowerKeyword[i] = tolower(lowerKeyword[i]);
    }
	
	//match line with keyword and return pattern if true
	char* line = strtok(fileContents, "\n");
	bool found = false;
	bool firstPrint = true;
	char preview[MAX_PREVIEW_LENGTH + 1]; 

	while (line)
    {
		if (keywordMatches(line, lowerKeyword))
        {
			found = true;
			int lineLength = strlen(line);
	
			if (lineLength > MAX_PREVIEW_LENGTH)
            {
				strncpy(preview, line, MAX_PREVIEW_LENGTH);
				preview[MAX_PREVIEW_LENGTH] = '\0'; 
			}
            else
            {
				strcpy(preview, line);
			}
			
            if (firstPrint)
            {
                BofPrintf(&buffer, "\n[+] Keyword '%s' found in file: %s\n", keyword, filePath);
				firstPrint = false;
			}

            BofPrintf(&buffer, "\t- Matched on pattern: %s\n", preview);
		}

		line = strtok(NULL, "\n");
	}

    free(fileContents);
    free(lowerKeyword);
	
    return found;
}

void SearchFilesRecursive(const char* lpFolder, const char* lpSearchPattern, const char* keyword)
{
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    char szDir[MAX_PATH];
    DWORD dwError;

    // Build search path for files in the current directory
    strcpy(szDir, lpFolder);
    strcat(szDir, "\\");
    strcat(szDir, lpSearchPattern);
	
	// Search for files
    hFind = FindFirstFileA(szDir, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
			if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
				char fullPath[MAX_PATH];
				sprintf(fullPath, "%s\\%s", lpFolder, findFileData.cFileName);
				
				if (*keyword)
                { 
				    SearchFileForKeyword(fullPath, keyword); 
				}
                else if (!*keyword)
                {
                    BofPrintf(&buffer, "[+] File found: %s\n", fullPath);
				}
			}
		} while (FindNextFileA(hFind, &findFileData) != 0);
		
		dwError = GetLastError();
		if (dwError != ERROR_NO_MORE_FILES)
        {
			BeaconPrintf(CALLBACK_ERROR, "Error searching for next file: %d\n", dwError);
		}

		FindClose(hFind);
	}
	
	//search for subdirectories and recurse into them
    strcpy(szDir, lpFolder);
    strcat(szDir, "\\*");

    hFind = FindFirstFileA(szDir, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                strcmp(findFileData.cFileName, ".") != 0 && 
                strcmp(findFileData.cFileName, "..") != 0)
            {
				
				// Build path for the subdirectory
                char subDir[MAX_PATH];
                strcpy(subDir, lpFolder);
                strcat(subDir, "\\");
                strcat(subDir, findFileData.cFileName);

                SearchFilesRecursive(subDir, lpSearchPattern, keyword);
            }
        } while (FindNextFileA(hFind, &findFileData) != 0);
		
        dwError = GetLastError();
        
        if (dwError != ERROR_NO_MORE_FILES)
        {
            BeaconPrintf(CALLBACK_ERROR, "Error searching for next file: %d\n", dwError);
        }

		FindClose(hFind);
    }
}

void go(char *args, int len)
{
	datap parser;
    CHAR* lpDirectory = NULL;
    CHAR* lpSearchPattern = NULL;
    CHAR* keyword = NULL; // If not empty, SearchFileForKeyword is called to verify if the keyword is in the text file

    if (!BofBufferInit(&buffer))
    {
        goto cleanup;
    }

	BeaconDataParse(&parser, args, len);
	lpDirectory     = BeaconDataExtract(&parser, NULL);
	lpSearchPattern = BeaconDataExtract(&parser, NULL);
	keyword         = BeaconDataExtract(&parser, NULL);

    BofPrintf(&buffer, "====================FILE SEARCH RESULTS====================\n");
	
    SearchFilesRecursive(lpDirectory, lpSearchPattern, keyword);

cleanup:
    BofBufferFree(&buffer);

	BeaconPrintf(CALLBACK_OUTPUT_UTF8, "[+] Finished searching!\n");
}
} // End extern "C"

#if defined(_DEBUG) && !defined(_GTEST)

int main(int argc, char* argv[])
{
    int reqArgCount = 4;

    // directory
    // pattern
    // keyword

    if (argc != reqArgCount)
    {
        printf("Usage ... need the args");
		return 1;
    }

    bof::runMocked<char*&, char*&, char*&>(go, argv[1], argv[2], argv[3]);

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