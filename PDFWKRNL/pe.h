#pragma once
#include <windows.h>

PVOID    MapFileIntoMemory(const char* path);
PVOID    SearchSignature(char* base, char* inSig, int length, int maxHuntLength);
ULONG_PTR SearchSignatureInSection(char* section, char* base, char* inSig, int length);
