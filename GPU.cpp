#include "GPU.h"
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
void GPU() {
	DISPLAY_DEVICEA displayDevice;
	displayDevice.cb = sizeof(DISPLAY_DEVICEA);
	for (int i = 0; EnumDisplayDevicesA(NULL, i, &displayDevice, NULL); i++) {
		cout << "GPU " << i << ":" << displayDevice.DeviceString << endl;
	}
}