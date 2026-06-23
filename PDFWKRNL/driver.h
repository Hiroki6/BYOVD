#pragma once
#include <windows.h>

#define DEVICE_NAME L"\\\\.\\PdFwKrnl"
#define IOCTL_MEMCPY 0x80002014

typedef struct PDFW_MEMCPY {
    BYTE  Reserved[16];
    PVOID Destination;
    PVOID Source;
    PVOID Reserved2;
    DWORD Size;
    DWORD Reserved3;
} PDFW_MEMCPY, *PPDFW_MEMCPY;

HANDLE  OpenDevice(void);
BOOL    ReadMemory(HANDLE driver, DWORD64 address, PVOID buffer, DWORD size);
BOOL    WriteMemory(HANDLE driver, DWORD64 address, PVOID buffer, DWORD size);
DWORD64 ReadMemoryDWORD64(HANDLE driver, DWORD64 address);
