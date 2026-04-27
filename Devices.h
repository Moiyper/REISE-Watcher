#pragma once
#include <string>
#include <Windows.h>
std::string WCHAR_TO_STRING(const WCHAR* wstr);
void ListDevices(const WCHAR* filterClass = nullptr);