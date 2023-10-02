#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <http.h>
#include <stdint.h>
#include <direct.h> // For _mkdir


#include "Comunication.h"

#pragma comment(lib, "httpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment (lib, "crypt32.lib")

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

