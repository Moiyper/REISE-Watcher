#include "Processor.h"
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <math.h>
#include <SetupAPI.h>
#include <io.h>
#include <fcntl.h>
using namespace std;
string ProcessorName() {
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		char buffer[256];
		DWORD bufferSize = sizeof(buffer);
		if (RegQueryValueExA(hKey, "ProcessorNameString", nullptr, nullptr, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return string(buffer);
		}
		RegCloseKey(hKey);
	}
	return "Unknown CPU. ";
}
void Processor() {
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	cout << "Processors count: " << sysInfo.dwNumberOfProcessors << endl;
	cout << "Processor name: " << ProcessorName() << endl;
}