#pragma once
// ============================================================
//  REISE Watcher - Dark Glass Edition : Application shell
//  Borderless rounded window, custom title bar, nav rail,
//  live telemetry cards, device browser page, tray icon.
// ============================================================
#include "pch.h"
#include <string>
#include <vector>
#include "Widgets.h"
#include "Telemetry.h"

namespace reise {

	// Custom messages
	#define RW_TICK        (WM_APP + 1)   // animation frame
	#define RW_DATA_READY  (WM_APP + 2)   // worker pushed new snapshot
	#define RW_TRAY        (WM_APP + 3)   // tray callback

	class App
	{
	public:
		static int Run(HINSTANCE hInst);

	private:
		// --- lifecycle / window ---
		bool Create(HINSTANCE hInst);
		static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
		LRESULT Handle(UINT msg, WPARAM wp, LPARAM lp);

		void OnPaint(HDC target, const RECT& client);
		void PaintOverview(HDC hdc, RECT content);
		void PaintDevices(HDC hdc, RECT content);
		void PaintTitleBar(HDC hdc, RECT client);
		void PaintStatusBar(HDC hdc, RECT client);
		void Layout(RECT client);

		void ToggleMaximize();
		void OnTimerTick();          // animation + repaint scheduling
		void RequestData();          // kick worker sampling
		void RefreshDevicesAsync();  // scan devices on worker thread

		// --- helpers ---
		HFONT MakeFont(const wchar_t* family, int px, int weight);
		void ApplyWindowRgn(RECT client);
		std::wstring StatusLine() const;

	private:
		HWND m_hwnd = nullptr;
		HINSTANCE m_inst = nullptr;
		NOTIFYICONDATAW m_tray{};

		// fonts
		HFONT m_fBrand{}, m_fTitle{}, m_fValue{}, m_fBig{}, m_fItem{}, m_fSmall{}, m_fSegoeUI{};

		// widgets
		NavRail       m_nav;
		TitleButtons  m_btns;
		Gauge         m_cpuGauge, m_gpuGauge;
		ProgressBar   m_ramBar, m_diskReadBar, m_diskWriteBar;
		Sparkline     m_cpuHist, m_ramHist;
		DeviceList    m_devList;

		// state
		Telemetry m_tele;
		Snapshot  m_snap;          // latest completed sample
		CRITICAL_SECTION m_cs;
		volatile bool m_workerBusy = false;
		volatile bool m_devBusy = false;
		std::vector<DeviceList::Row> m_pendingRows;
		bool m_devicesLoaded = false;
		int  m_animFramesLeft = 0;
		POINT m_lastDragStart{};
		bool  m_dragging = false;
		bool  m_maximized = false;

		// cached static info
		std::wstring m_pcName;
	};

} // namespace reise
