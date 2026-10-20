#include "Telemetry.h"
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

namespace reise {

	static ULONGLONG FT2U64(const FILETIME& ft)
	{
		ULARGE_INTEGER u;
		u.LowPart = ft.dwLowDateTime;
		u.HighPart = ft.dwHighDateTime;
		return u.QuadPart;
	}

	std::wstring CpuNameW()
	{
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
			L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			WCHAR buffer[256];
			DWORD bufferSize = sizeof(buffer);
			DWORD type = 0;
			if (RegQueryValueExW(hKey, L"ProcessorNameString", nullptr, &type,
				(LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS)
			{
				RegCloseKey(hKey);
				return std::wstring(buffer);
			}
			RegCloseKey(hKey);
		}
		return L"Unknown CPU";
	}

	std::wstring ComputerNameW()
	{
		WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD size = sizeof(buffer) / sizeof(WCHAR);
		if (GetComputerNameW(buffer, &size)) return std::wstring(buffer, size);
		return L"Unknown";
	}

	std::vector<std::wstring> EnumerateGpusW()
	{
		std::vector<std::wstring> gpus;
		DISPLAY_DEVICEW dd{};
		dd.cb = sizeof(dd);
		for (int i = 0; EnumDisplayDevicesW(nullptr, i, &dd, 0); ++i)
			gpus.push_back(dd.DeviceString);
		return gpus;
	}

	void Telemetry::Sample(Snapshot& out)
	{
		// ---- CPU usage via GetSystemTimes deltas ----
		FILETIME idle, kernel, user;
		if (GetSystemTimes(&idle, &kernel, &user))
		{
			if (m_hasPrev)
			{
				ULONGLONG idl = FT2U64(idle) - FT2U64(m_prevIdle);
				ULONGLONG krn = FT2U64(kernel) - FT2U64(m_prevKernel);
				ULONGLONG usr = FT2U64(user) - FT2U64(m_prevUser);
				ULONGLONG total = krn + usr; // kernel includes idle time
				if (total > 0)
					out.cpuPercent = 100.0 * (double)(total - idl) / (double)total;
			}
			m_prevIdle = idle; m_prevKernel = kernel; m_prevUser = user;
			m_hasPrev = true;
		}

		SYSTEM_INFO si{};
		GetNativeSystemInfo(&si);
		out.cpuCores = (int)si.dwNumberOfProcessors;
		if (out.cpuName.empty()) out.cpuName = CpuNameW();

		// ---- RAM ----
		MEMORYSTATUSEX mem{};
		mem.dwLength = sizeof(mem);
		if (GlobalMemoryStatusEx(&mem))
		{
			out.ramTotalMB = mem.ullTotalPhys / (1024ULL * 1024ULL);
			out.ramUsedMB = (mem.ullTotalPhys - mem.ullAvailPhys) / (1024ULL * 1024ULL);
		}

		// ---- GPU names (cached by caller if needed) ----
		if (out.gpus.empty()) out.gpus = EnumerateGpusW();

		// ---- Disk activity ----
		IO_COUNTERS io{};
		if (GetProcessIoCounters(GetCurrentProcess(), &io))
		{
			// process-wide IO as a lightweight proxy; system-wide would need perf counters
			ULONGLONG tick = GetTickCount64();
			if (m_prevTick != 0 && tick > m_prevTick)
			{
				double dt = (tick - m_prevTick) / 1000.0;
				out.diskReadBps = (ULONGLONG)((io.ReadOperationCount > m_prevRead ?
					(double)(io.ReadTransferCount - m_prevRead) : 0.0) / dt);
				out.diskWriteBps = (ULONGLONG)((io.WriteOperationCount > m_prevWrite ?
					(double)(io.WriteTransferCount - m_prevWrite) : 0.0) / dt);
			}
			m_prevRead = io.ReadTransferCount;
			m_prevWrite = io.WriteTransferCount;
			m_prevTick = tick;
		}

		if (out.computerName.empty()) out.computerName = ComputerNameW();
	}

} // namespace reise
