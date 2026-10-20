#pragma once
// ============================================================
//  REISE Watcher - custom themed widgets (pure Win32 + GDI+)
//  Card / Gauge (arc speedometer) / ProgressBar / Sparkline /
//  NavRail / TitleBarButton / DeviceList
// ============================================================
#include "pch.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <deque>
#include "Theme.h"

namespace reise {

	// ---------- Widget base: everything is owner-drawn on one HDC ----------
	// Widgets are passive: App paints them in WM_PAINT and routes mouse msgs.

	struct Gauge
	{
		RECT rc{};                 // bounding rect (client coords)
		double value = 0.0;        // 0..100 (%)
		double shown = 0.0;        // animated value
		std::wstring title;        // e.g. "CPU"
		std::wstring sub;          // e.g. "Intel Core i5"
		COLORREF accent = theme::Accent;

		void SetValue(double v) { value = v; }
		// ease animation tick, returns true if still animating
		bool Tick();
		void Paint(HDC hdc, HFONT fTitle, HFONT fValue, HFONT fSmall);
		bool HitTest(POINT pt) const { return PtInRect(&rc, pt) != FALSE; }
	};

	struct ProgressBar
	{
		RECT rc{};
		double value = 0.0;        // 0..1
		double shown = 0.0;
		std::wstring label;        // left text
		std::wstring rightText;    // e.g. "8.4 / 16 GB"
		COLORREF accent = theme::Violet;

		void SetValue(double v) { value = v; }
		bool Tick();
		void Paint(HDC hdc, HFONT fSmall, HFONT fValue);
	};

	struct Sparkline
	{
		RECT rc{};
		std::deque<double> data;   // 0..1 samples
		COLORREF line = theme::Accent;
		std::wstring caption;

		void Push(double v)
		{
			data.push_back(v);
			while (data.size() > 120) data.pop_front();
		}
		void Paint(HDC hdc, HFONT fSmall);
	};

	// ---------- Navigation rail ----------
	enum class NavPage { Overview = 0, Devices = 1 };

	struct NavRail
	{
		RECT rc{};
		int sel = 0;
		int hover = -1;
		const int itemH = 44;
		std::vector<std::wstring> items = { L"Overview", L"Devices" };
		std::vector<std::wstring> icons = { L"\u25A9", L"\u2630" }; // ▩ ≣ style glyphs

		int HitTest(POINT pt) const;
		void Paint(HDC hdc, HFONT fItem, HFONT fBrand, HFONT fSmall);
	};

	// ---------- Title bar buttons (minimize / maximize / close) ----------
	struct TitleButtons
	{
		RECT rcMin{}, rcMax{}, rcClose{};
		int hover = -1; // 0 min, 1 max, 2 close
		void Layout(RECT client);
		int HitTest(POINT pt) const; // -1 none
		void Paint(HDC hdc, HFONT fSegoe);
	};

	// ---------- Scrollable device list ----------
	struct DeviceList
	{
		RECT rc{};
		struct Row { std::wstring name; std::wstring cls; };
		std::vector<Row> rows;
		std::string filter;        // current search filter (ASCII)
		int scroll = 0;            // pixel scroll
		int hoverIdx = -1;
		const int rowH = 40;

		void SetRows(std::vector<Row> r) { rows = std::move(r); scroll = 0; }
		int VisibleCount() const { return std::max(1, (rc.bottom - rc.top) / rowH); }
		void OnWheel(int delta)
		{
			int maxScroll = (int)rows.size() * rowH - (rc.bottom - rc.top);
			scroll = std::clamp(scroll - delta / 120 * rowH * 2, 0, std::max(0, maxScroll));
		}
		int HitTest(POINT pt) const;
		void Paint(HDC hdc, HFONT fName, HFONT fCls, HFONT fSmall);
	};

} // namespace reise
