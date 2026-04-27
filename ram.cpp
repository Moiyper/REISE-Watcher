#include "ram.h"
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
void RAM() {
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memInfo)) {
		double total = memInfo.ullTotalPhys / pow(1024.0, 3);
		double avail = memInfo.ullAvailPhys / pow(1024.0, 3);
		double used = total - avail;
		cout << "RAM Total : " << total << " GB" << endl;
		cout << "RAM Used: " << used << " GB" << endl;
		cout << "RAM Free: " << avail << " GB" << endl;
	}
}