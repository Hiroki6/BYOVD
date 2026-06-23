#include "kernel.h"
#include <stdio.h>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

DWORD64 FindKernelModuleAddressByName(const char* name) {
    LPVOID drivers[1024] = { 0 };
    DWORD  cbNeeded = 0;
    char   szDriver[MAX_PATH] = { 0 };

    if (!EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded)) return 0;

    DWORD cDrivers = cbNeeded / sizeof(drivers[0]);
    for (DWORD i = 0; i < cDrivers; i++) {
        if (drivers[i] && GetDeviceDriverBaseNameA(drivers[i], szDriver, sizeof(szDriver))) {
            if (_stricmp(szDriver, name) == 0)
                return (DWORD64)drivers[i];
        }
    }
    printf("[!] Could not resolve %s kernel module's address\n", name);
    return 0;
}

DWORD64 GetKernelBaseAddress(void) {
    return FindKernelModuleAddressByName("ntoskrnl.exe");
}

DWORD64 GetCIBaseAddress(void) {
    return FindKernelModuleAddressByName("CI.dll");
}

BOOL IsHVCIEnabled(void) {
    typedef LONG (WINAPI* PNtQuerySystemInformation)(
        ULONG SystemInformationClass,
        PVOID SystemInformation,
        ULONG SystemInformationLength,
        PULONG ReturnLength
    );
    typedef struct { ULONG Length; ULONG CodeIntegrityOptions; } SYS_CI_INFO;

#define SystemCodeIntegrityInformation   103
#define CODEINTEGRITY_HVCI_KMCI_ENABLED  0x400

    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return FALSE;

    PNtQuerySystemInformation NtQSI =
        (PNtQuerySystemInformation)GetProcAddress(ntdll, "NtQuerySystemInformation");
    if (!NtQSI) return FALSE;

    SYS_CI_INFO ci = { sizeof(ci) };
    if (NtQSI(SystemCodeIntegrityInformation, &ci, sizeof(ci), NULL) != 0)
        return FALSE;

    return (ci.CodeIntegrityOptions & CODEINTEGRITY_HVCI_KMCI_ENABLED) != 0;
}

ULONG_PTR getPTEForVA(ULONG_PTR pteBase, ULONG_PTR address) {
    address  = address >> 9;
    address &= 0x7FFFFFFFF8;
    address += pteBase;
    return address;
}
