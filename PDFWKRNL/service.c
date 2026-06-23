#include "service.h"
#include <stdio.h>

BOOL LoadDriver(const char* driverPath, const char* serviceName) {
    SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCM) {
        fprintf(stderr, "[-] OpenSCManager failed: %lu\n", GetLastError());
        return FALSE;
    }

    SC_HANDLE hService = CreateServiceA(
        hSCM, serviceName, serviceName,
        SERVICE_START | SERVICE_STOP | DELETE,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        driverPath,
        NULL, NULL, NULL, NULL, NULL
    );

    if (!hService) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS) {
            hService = OpenServiceA(hSCM, serviceName, SERVICE_START);
        }
        if (!hService) {
            fprintf(stderr, "[-] CreateService failed: %lu\n", err);
            CloseServiceHandle(hSCM);
            return FALSE;
        }
    }

    if (!StartServiceA(hService, 0, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_ALREADY_RUNNING) {
            fprintf(stderr, "[-] StartService failed: %lu\n", err);
            CloseServiceHandle(hService);
            CloseServiceHandle(hSCM);
            return FALSE;
        }
    }

    printf("[+] Driver service '%s' started.\n", serviceName);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return TRUE;
}

