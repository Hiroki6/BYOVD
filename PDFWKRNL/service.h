#pragma once
#include <windows.h>

BOOL LoadDriver(const char* driverPath, const char* serviceName);
BOOL UnloadDriver(const char* serviceName);
