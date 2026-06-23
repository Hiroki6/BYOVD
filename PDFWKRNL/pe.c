#include "pe.h"
#include <string.h>

PVOID MapFileIntoMemory(const char* path) {
    HANDLE fileHandle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return NULL;

    HANDLE fileMapping = CreateFileMapping(fileHandle, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL);
    if (fileMapping == NULL) {
        CloseHandle(fileHandle);
        return NULL;
    }

    void* fileMap = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
    if (fileMap == NULL) {
        CloseHandle(fileMapping);
        CloseHandle(fileHandle);
    }

    return fileMap;
}

PVOID SearchSignature(char* base, char* inSig, int length, int maxHuntLength) {
    for (int i = 0; i < maxHuntLength; i++) {
        if (base[i] == inSig[0]) {
            if (memcmp(base + i, inSig, length) == 0)
                return base + i;
        }
    }
    return NULL;
}

ULONG_PTR SearchSignatureInSection(char* section, char* base, char* inSig, int length) {
    IMAGE_DOS_HEADER*    dosHeader    = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS64*  ntHeaders    = (IMAGE_NT_HEADERS64*)((char*)base + dosHeader->e_lfanew);
    IMAGE_SECTION_HEADER* sectionHeaders = (IMAGE_SECTION_HEADER*)((char*)ntHeaders + sizeof(IMAGE_NT_HEADERS64));
    IMAGE_SECTION_HEADER* textSection = NULL;

    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        if (memcmp(sectionHeaders[i].Name, section, strlen(section)) == 0) {
            textSection = &sectionHeaders[i];
            break;
        }
    }

    if (textSection == NULL)
        return 0;

    return (ULONG_PTR)SearchSignature(
        (char*)base + textSection->VirtualAddress,
        inSig,
        length,
        textSection->SizeOfRawData
    );
}
