#include "commun.h"


wchar_t* ConvertToWChar(const char* str) {
    if (str == NULL) return NULL;

    // Get the required size of the destination buffer.
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);

    // Allocate memory for the wide character string.
    wchar_t* wideStr = new wchar_t[sizeNeeded];

    // Perform the actual conversion.
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wideStr, sizeNeeded);

    return wideStr;
}


void killProcessByName(const char* processName) {

    wchar_t* WprocessName = ConvertToWChar(processName);

    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(pEntry);
    BOOL hRes = Process32First(hSnapShot, &pEntry);

    while (hRes) {


        if (wcscmp(pEntry.szExeFile, WprocessName) == 0) {

            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, (DWORD)pEntry.th32ProcessID);
            if (hProcess != NULL) {
                TerminateProcess(hProcess, 9);
                CloseHandle(hProcess);
            }
        }
        hRes = Process32Next(hSnapShot, &pEntry);
        }
    CloseHandle(hSnapShot);
    }