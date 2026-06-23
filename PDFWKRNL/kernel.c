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
    HKEY  hKey;
    DWORD val = 0, sz = sizeof(val);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\DeviceGuard",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "EnableVirtualizationBasedSecurity", NULL, NULL, (PBYTE)&val, &sz);
        RegCloseKey(hKey);
        if (val) return TRUE;
    }

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SYSTEM\\CurrentControlSet\\Control\\DeviceGuard\\Scenarios\\HypervisorEnforcedCodeIntegrity",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExA(hKey, "Enabled", NULL, NULL, (PBYTE)&val, &sz);
        RegCloseKey(hKey);
        if (val) return TRUE;
    }

    return FALSE;
}

ULONG_PTR getPTEForVA(ULONG_PTR pteBase, ULONG_PTR address) {
    address  = address >> 9;
    address &= 0x7FFFFFFFF8;
    address += pteBase;
    return address;
}
