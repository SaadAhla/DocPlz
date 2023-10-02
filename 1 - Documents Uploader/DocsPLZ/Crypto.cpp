#include "commun.h"


BOOL computeSHA256(const char* filePath, BYTE* hashValue) {
    BOOL result = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE buffer[4096];
    DWORD bytesRead;
    DWORD hashLength = 32; // SHA-256 hash size in bytes

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return FALSE;
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        goto done;
    }

    hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        goto done;
    }

    while (ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, NULL)) {
        if (bytesRead == 0) break;
        if (!CryptHashData(hHash, buffer, bytesRead, 0)) {
            goto done;
        }
    }

    
    if (!CryptGetHashParam(hHash, HP_HASHVAL, hashValue, &hashLength, 0)) {
        goto done;
    }

    result = TRUE;

done:
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    if (hHash) CryptDestroyHash(hHash);
    if (hProv) CryptReleaseContext(hProv, 0);
    return result;
}