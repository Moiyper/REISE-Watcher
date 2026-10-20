#pragma once
// ============================================================
//  REISE Watcher - telemetry collector (CPU %, RAM, GPU list)
//  Used by the GUI worker thread; safe to call from one thread.
// ============================================================
#include "pch.h"
#include <string>
#include <vector>

namespace reise {

	struct Snapshot
	{
		double cpuPercent = 0.0;      // 0..100 system-wide
		ULONGLONG cpuFreqMHz = 0;     // current EPC / max freq estimate
		int cpuCores = 0;
		std::wstring cpuName;

		ULONGLONG ramTotalMB = 0;
		ULONGLONG ramUsedMB = 0;

		std::vector<std::wstring> gpus;

		ULONGLONG diskReadBps = 0;    // bytes/sec since last sample
		ULONGLONG diskWriteBps = 0;

		std::wstring computerName;
	};

	// Stateful sampler: call Sample() repeatedly (e.g. every second).
	class Telemetry
	{
	public:
		void Sample(Snapshot& out);

	private:
		// CPU time deltas
		FILETIME m_prevIdle{}, m_prevKernel{}, m_prevUser{};
		bool m_hasPrev = false;
		// Disk deltas
		ULONGLONG m_prevRead = 0, m_prevWrite = 0, m_prevTick = 0;
	};

	// Static helpers reused from console code (widened)
	std::wstring CpuNameW();
	std::wstring ComputerNameW();
	std::vector<std::wstring> EnumerateGpusW();

} // namespace reise
