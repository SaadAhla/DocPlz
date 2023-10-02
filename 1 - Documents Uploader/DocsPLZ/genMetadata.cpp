#include "commun.h"


#define SERVER_URL L"api.ipify.org"



char* get_computer_name() {
    char* computerName = (char*)malloc(MAX_COMPUTERNAME_LENGTH + 1);
    if (computerName != NULL) {
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        if (!GetComputerNameA(computerName, &size)) {
            free(computerName);
            return NULL; // Handle the error as appropriate for your application
        }
    }
    return computerName;
}


char* get_user_name() {
    char* username = (char*)malloc(UNLEN + 1);
    if (username != NULL) {
        DWORD size = UNLEN + 1;
        if (!GetUserNameA(username, &size)) {
            free(username);
            return NULL; // Handle the error as appropriate for your application
        }
    }
    return username;
}




void get_external_ip(char* ip) {

    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    DWORD dwSize = 0, dwDownloaded = 0;

    // Use WinHttpOpen to obtain a session handle.
    hSession = WinHttpOpen(L"User Agent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession) {
        // Specify an HTTP server.
        hConnect = WinHttpConnect(hSession, SERVER_URL, INTERNET_DEFAULT_HTTPS_PORT, 0);
    }

    if (hConnect) {
        // Create an HTTP request handle.
        hRequest = WinHttpOpenRequest(hConnect, L"GET", NULL, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    }

    // Send a request.
    if (hRequest) {
        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            // Wait for the response.
            if (WinHttpReceiveResponse(hRequest, NULL)) {
                // Read the data.
                do {
                    dwSize = 0;
                    if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                        break;
                    }

                    if (!WinHttpReadData(hRequest, (LPVOID)ip, dwSize, &dwDownloaded)) {
                        break;
                    }
                } while (dwSize > 0);
            }
        }
    }

    // Close the handles.
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

}

