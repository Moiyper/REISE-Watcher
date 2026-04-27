#include "Processor.h"
#include "ram.h"
#include "GPU.h"
#include "Devices.h"
#include "Header.h"
#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <math.h>
#include <SetupAPI.h>
#include <io.h>
#include <fcntl.h>
#pragma comment(lib, "SetupAPI.lib")
using namespace std;
int main() {
		cout << "========================= REISE Watcher =========================\n";
		cout << "=========== COMPUTER NAME ===========" << endl;
		cout << GetComputerNameSTRING() << endl;
		cout << "=========== CHARACTERISTICS ===========" << endl;
		Processor();
		GPU();
		RAM();
		cout << "\n:::F5 for listDevices:::" << endl;
		while (true) {
			if (GetAsyncKeyState(VK_F5) & 0x8000) {
				ListDevices();
				break;
			}
		}
}