#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <winternl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winhttp.h>
#include <lmcons.h> // For UNLEN
#include <sys/stat.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <process.h>


#include "Findem.h"
#include "ComunicationC2.h"
#include "Persistence.h"
#include "Crypto.h"
#include "AntiStuff.h"
#include "genMetadata.h"
#include "ProcStuff.h"

#pragma comment (lib, "crypt32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp")


#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

