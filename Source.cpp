#include <Windows.h>
#include <string>
#include <iostream>
using namespace std;
string GetComputerNameSTRING() {
	wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD size = sizeof(buffer) / sizeof(wchar_t);
	if (GetComputerNameW(buffer, &size)) {
		string result;
		for (DWORD i = 0; i < size; i++) {
			result += static_cast<char>(buffer[i]);
		}
		return result;
	}
	return "Unknown. ";
}