#include "Devices.h"
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
string WCHAR_TO_STRING(const WCHAR* wstr) {
	if (!wstr) return "";
	int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
	if (len == 0) return "";
	string result(len - 1, '/0');
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr);
	return result;
}
void ListDevices(const WCHAR* filterClass) {
	_setmode(_fileno(stdout), _O_U16TEXT);

	HDEVINFO hDevInfo = SetupDiGetClassDevs(nullptr, nullptr, nullptr,
		DIGCF_ALLCLASSES | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) {
		wcout << L"Error: Cannot get device list." << endl;
		return;
	}
	if (filterClass)
		wcout << L"\n========== DEVICES (Class: " << filterClass << L") ==========\n" << endl;
	else
		wcout << L"\n========== CONNECTED DEVICES ==========\n" << endl;

	SP_DEVINFO_DATA devInfoData;
	devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	WCHAR namebuf[256];
	WCHAR classbuf[128];
	int count = 0;

	for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
		wstring displayName = L"(No name)";
		wstring className = L"";
		if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData,
			SPDRP_CLASS, nullptr, (PBYTE)classbuf, sizeof(classbuf), nullptr)) {
			className = classbuf;
		}
		if (filterClass && className != filterClass) {
			continue;
		}
		if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData,
			SPDRP_FRIENDLYNAME, nullptr, (PBYTE)namebuf, sizeof(namebuf), nullptr)) {
			displayName = namebuf;
		}
		else if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData,
			SPDRP_DEVICEDESC, nullptr, (PBYTE)namebuf, sizeof(namebuf), nullptr)) {
			displayName = namebuf;
		}
		wcout << L"[" << count++ << L"] " << displayName << endl;
		if (!filterClass)
			wcout << L"    Class: " << className << endl;
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	wcout << L"\nTotal devices: " << count << endl;
}