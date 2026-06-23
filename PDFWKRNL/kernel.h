#pragma once
#include <windows.h>

DWORD64   FindKernelModuleAddressByName(const char* name);
DWORD64   GetKernelBaseAddress(void);
DWORD64   GetCIBaseAddress(void);
BOOL      IsHVCIEnabled(void);
ULONG_PTR getPTEForVA(ULONG_PTR pteBase, ULONG_PTR address);
