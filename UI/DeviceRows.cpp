#include "Widgets.h"
#include <SetupAPI.h>
#include <cctype>

#pragma comment(lib, "Setupapi.lib")

// Device-row collection for the GUI Devices page.
// Same SetupAPI enumeration strategy as ListDevices() in Devices.cpp,
// but returns rows instead of printing them to the console.
void CollectDeviceRows(std::vector<reise::DeviceList::Row>& outRows, const std::string& filter)
{
HDEVINFO hDevInfo = SetupDiGetClassDevsW(nullptr, nullptr, nullptr,
DIGCF_PRESENT | DIGCF_ALLCLASSES);
if (hDevInfo == INVALID_HANDLE_VALUE) return;

SP_DEVINFO_DATA devInfoData{};
devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

WCHAR namebuf[256];
WCHAR classbuf[128];

std::wstring f;
for (char c : filter) f += (wchar_t)(unsigned char)c;
auto lower = [](std::wstring s) {
for (auto& c : s) c = (wchar_t)std::towlower(c);
return s; };
const std::wstring fl = lower(f);

for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i)
{
std::wstring displayName;
std::wstring className;

if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_CLASS,
nullptr, (PBYTE)classbuf, sizeof(classbuf), nullptr))
className = classbuf;

if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_FRIENDLYNAME,
nullptr, (PBYTE)namebuf, sizeof(namebuf), nullptr))
displayName = namebuf;
else if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DEVICEDESC,
nullptr, (PBYTE)namebuf, sizeof(namebuf), nullptr))
displayName = namebuf;
else
continue;

if (!fl.empty() &&
lower(displayName).find(fl) == std::wstring::npos &&
lower(className).find(fl) == std::wstring::npos)
continue;

outRows.push_back({ std::move(displayName), std::move(className) });
}

SetupDiDestroyDeviceInfoList(hDevInfo);
}
