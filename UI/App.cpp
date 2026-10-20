#include "App.h"
#include "Theme.h"
#include "../Devices.h"          // existing console enumerator (WCHAR based)
#include <shellscalingapi.h>
#include <dwmapi.h>
#include <windowsx.h>
#pragma comment(lib, "Dwmapi.lib")
#include <sstream>
#include <iomanip>
#include <thread>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "UxTheme.lib")

namespace reise {

	using namespace theme;

	static const wchar_t* kWndClass = L"ReiseWatcherGlassWnd";
	static const wchar_t* kAppTitle = L"REISE Watcher \u2014 Dark Glass Edition";

	// ---------- double-buffered paint context ----------
	struct PaintCtx
	{
		HDC mem;
		HBITMAP bmp;
		HBITMAP oldBmp;
	};

	int App::Run(HINSTANCE hInst)
	{
		App app;
		if (!app.Create(hInst)) return 1;
		app.m_inst = hInst;

		INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_STANDARD_CLASSES };
		InitCommonControlsEx(&icc);

		HACCEL accel = nullptr;
		ACCEL accels[] = {
			{ FVIRTKEY, VK_F5, 'F5' },
			{ FVIRTKEY, VK_ESCAPE, 'ES' },
		};
		accel = CreateAcceleratorTableW(accels, 2);

		MSG msg;
		while (GetMessageW(&msg, nullptr, 0, 0))
		{
			if (accel && TranslateAcceleratorW(msg.hwnd, accel, &msg)) continue;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		if (accel) DestroyAcceleratorTable(accel);
		return (int)msg.wParam;
	}

	HFONT App::MakeFont(const wchar_t* family, int px, int weight)
	{
		return CreateFontW(-px, 0, 0, 0, weight, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, family);
	}

	bool App::Create(HINSTANCE hInst)
	{
		InitializeCriticalSection(&m_cs);
		theme::InitGdiPlus();

		WNDCLASSEXW wc{};
		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = [](HWND h, UINT m, WPARAM w, LPARAM l) -> LRESULT {
			App* self = (App*)GetWindowLongPtrW(h, GWLP_USERDATA);
			if (!self && m == WM_NCCREATE) {
				self = (App*)((LPCREATESTRUCTW)l)->lpCreateParams;
				SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
			}
			return self ? self->Handle(m, w, l) : DefWindowProcW(h, m, w, l);
			};
		wc.hInstance = hInst;
		wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		wc.lpszClassName = kWndClass;
		wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
		RegisterClassExW(&wc);

		m_pcName = ComputerNameW();

		RECT r{ 0, 0, 1120, 720 };
		AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
		int x = (GetSystemMetrics(SM_CXSCREEN) - (r.right - r.left)) / 2;
		int y = (GetSystemMetrics(SM_CYSCREEN) - (r.bottom - r.top)) / 2;

		m_hwnd = CreateWindowExW(WS_EX_APPWINDOW, kWndClass, kAppTitle,
			WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
			x, y, r.right - r.left, r.bottom - r.top,
			nullptr, nullptr, hInst, this);
		if (!m_hwnd) return false;

		// Rounded corners on Win11+
		DWORD pref = DWMWCP_ROUND;
		DwmSetWindowAttribute(m_hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));

		// Fonts
		m_fBrand  = MakeFont(L"Segoe UI", 20, FW_BOLD);
		m_fTitle  = MakeFont(L"Segoe UI Semibold", 15, FW_SEMIBOLD);
		m_fValue  = MakeFont(L"Segoe UI", 15, FW_SEMIBOLD);
		m_fBig    = MakeFont(L"Segoe UI Light", 34, FW_LIGHT);
		m_fItem   = MakeFont(L"Segoe UI", 14, FW_NORMAL);
		m_fSmall  = MakeFont(L"Segoe UI", 12, FW_NORMAL);
		m_fSegoeUI = MakeFont(L"Segoe MDL2 Assets", 11, FW_NORMAL);

		// Widget static props
		m_cpuGauge.title = L"CPU LOAD";
		m_gpuGauge.title = L"GPU ACTIVE";
		m_gpuGauge.accent = Violet;
		m_ramBar.label = L"MEMORY";
		m_ramBar.accent = Violet;
		m_diskReadBar.label = L"DISK READ";
		m_diskReadBar.accent = Good;
		m_diskWriteBar.label = L"DISK WRITE";
		m_diskWriteBar.accent = Warn;
		m_cpuHist.caption = L"CPU HISTORY (60 s)";
		m_ramHist.caption = L"MEMORY HISTORY (60 s)";
		m_ramHist.line = Violet;

		// Tray icon
		m_tray.cbSize = sizeof(m_tray);
		m_tray.hWnd = m_hwnd;
		m_tray.uID = 1;
		m_tray.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		m_tray.uCallbackMessage = RW_TRAY;
		m_tray.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
		wcsncpy_s(m_tray.szTip, kAppTitle, _TRUNCATE);
		Shell_NotifyIconW(NIM_ADD, &m_tray);

		// Timers: 1s data sampling + 16ms animation pump
		SetTimer(m_hwnd, 1, 1000, nullptr);
		SetTimer(m_hwnd, 2, 16, nullptr);

		RequestData();
		ShowWindow(m_hwnd, SW_SHOW);
		InvalidateRect(m_hwnd, nullptr, TRUE);
		return true;
	}

	void App::ApplyWindowRgn(RECT client)
	{
		HRGN rgn = CreateRoundRectRgn(0, 0, client.right, client.bottom,
			CornerRadius * 2, CornerRadius * 2);
		SetWindowRgn(m_hwnd, rgn, TRUE);
		DeleteObject(rgn);
	}

	void App::Layout(RECT client)
	{
		m_btns.Layout(client);

		const int mg = GridMargin;
		m_nav.rc = { mg, TitleBarHeight + mg, mg + SideWidth, client.bottom - mg - StatusHeight };

		RECT content = { m_nav.rc.right + GridGap, TitleBarHeight + mg,
			client.right - mg, client.bottom - mg - StatusHeight };

		if ((int)m_nav.sel == 0) // Overview
		{
			int cw = (content.right - content.left - GridGap) / 2;
			int ch = (content.bottom - content.top - GridGap) / 2;
			m_cpuGauge.rc = { content.left, content.top, content.left + cw, content.top + ch };
			m_gpuGauge.rc = { content.left + cw + GridGap, content.top, content.right, content.top + ch };

			int rowY = content.top + ch + GridGap;
			int barCardH = 96;
			RECT ramCard = { content.left, rowY, content.left + cw, rowY + barCardH };
			RECT diskCard = { content.left + cw + GridGap, rowY, content.right, rowY + barCardH };

			int pad = 16;
			m_ramBar.rc = { ramCard.left + pad, ramCard.top + 26, ramCard.right - pad, ramCard.bottom - pad };
			int dh = (diskCard.bottom - diskCard.top - 26 - pad * 2 - 6) / 2;
			m_diskReadBar.rc = { diskCard.left + pad, diskCard.top + 26, diskCard.right - pad, diskCard.top + 26 + dh };
			m_diskWriteBar.rc = { diskCard.left + pad, diskCard.top + 26 + dh + 6, diskCard.right - pad, diskCard.top + 26 + dh + 6 + dh };

			int histY = rowY + barCardH + GridGap;
			int hw = (content.right - content.left - GridGap) / 2;
			m_cpuHist.rc = { content.left, histY, content.left + hw, content.bottom };
			m_ramHist.rc = { content.left + hw + GridGap, histY, content.right, content.bottom };
		}
		else // Devices
		{
			m_devList.rc = { content.left, content.top, content.right, content.bottom };
		}
	}

	std::wstring App::StatusLine() const
	{
		std::wostringstream oss;
		oss << L"PC: " << m_pcName
			<< L"   \u2022   CPU: " << m_snap.cpuName
			<< L" (" << m_snap.cpuCores << L" threads)"
			<< L"   \u2022   GPU: " << (m_snap.gpus.empty() ? L"-" : m_snap.gpus[0]);
		return oss.str();
	}

	// ==================== painting ====================
	void App::PaintTitleBar(HDC hdc, RECT client)
	{
		RECT bar = { 0, 0, client.right, TitleBarHeight };
		FillRoundRect(hdc, bar, CornerRadius, BgHeader, BgHeader, RGB(0, 0, 0), false);

		// Accent underline glow
		HBRUSH g1 = CreateSolidBrush(Mix(Accent, BgHeader, 0.5));
		FillRect(hdc, &RECT{ 0, TitleBarHeight - 2, client.right / 3, TitleBarHeight }, g1);
		DeleteObject(g1);

		// Left title with accent dot
		RECT rDot = { 18, TitleBarHeight / 2 - 5, 28, TitleBarHeight / 2 + 5 };
		FillRoundRect(hdc, rDot, 5, Accent, Mix(Accent, Violet, 0.7), RGB(0, 0, 0), false);
		RECT rTxt = { 36, 0, client.right - 200, TitleBarHeight };
		DrawTextInRect(hdc, kAppTitle, rTxt, m_fTitle, TextPrimary, DT_LEFT | DT_VCENTER);

		m_btns.Paint(hdc, m_fSegoeUI);
	}

	void App::PaintStatusBar(HDC hdc, RECT client)
	{
		RECT bar = { 0, client.bottom - StatusHeight, client.right, client.bottom };
		FillRoundRect(hdc, bar, CornerRadius, BgHeader, BgHeader, RGB(0, 0, 0), false);
		HPEN pen = CreatePen(PS_SOLID, 1, BorderSoft);
		HGDIOBJ old = SelectObject(hdc, pen);
		MoveToEx(hdc, bar.left, bar.top, nullptr);
		LineTo(hdc, bar.right, bar.top);
		SelectObject(hdc, old);
		DeleteObject(pen);

		SYSTEMTIME st; GetLocalTime(&st);
		WCHAR timeBuf[16];
		wsprintfW(timeBuf, L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);

		DrawTextInRect(hdc, StatusLine().c_str(),
			{ 16, bar.top, bar.right - 120, bar.bottom }, m_fSmall, TextMuted, DT_LEFT | DT_VCENTER);
		DrawTextInRect(hdc, timeBuf,
			{ bar.right - 100, bar.top, bar.right - 12, bar.bottom }, m_fSmall, Accent, DT_RIGHT | DT_VCENTER);
	}

	void App::PaintOverview(HDC hdc, RECT content)
	{
		// Card backgrounds drawn by widgets themselves; here set dynamic texts
		std::wostringstream cpuSub;
		cpuSub << m_snap.cpuCores << L" threads \u2022 " << m_snap.cpuName.c_str();
		m_cpuGauge.sub = cpuSub.str();

		if (!m_snap.gpus.empty())
			m_gpuGauge.sub = m_snap.gpus[0];
		else
			m_gpuGauge.sub = L"No display adapter";

		std::wostringstream ramRight;
		ramRight << std::fixed << std::setprecision(1)
			<< (m_snap.ramUsedMB / 1024.0) << L" / " << (m_snap.ramTotalMB / 1024.0) << L" GB used";
		m_ramBar.rightText = ramRight.str();

		auto fmtBps = [](ULONGLONG bps) {
			std::wostringstream o; double mb = bps / (1024.0 * 1024.0);
			o << std::fixed << std::setprecision(1) << mb << L" MB/s"; return o.str(); };
		m_diskReadBar.rightText = fmtBps(m_snap.diskReadBps);
		m_diskWriteBar.rightText = fmtBps(m_snap.diskWriteBps);

		// Gauge cards need their own card frame behind arc: Paint does it.
		m_cpuGauge.Paint(hdc, m_fTitle, m_fBig, m_fSmall);
		m_gpuGauge.Paint(hdc, m_fTitle, m_fBig, m_fSmall);

		// RAM / Disk mini-cards: draw frame then bars inside
		FillRoundRect(hdc, RECT{ m_ramBar.rc.left - 16, m_ramBar.rc.top - 26, m_ramBar.rc.right + 16, m_ramBar.rc.bottom + 16 },
			CardRadius, BgPanelTop, BgPanel, BorderSoft, true);
		m_ramBar.Paint(hdc, m_fSmall, m_fValue);

		RECT diskFrame = { m_diskReadBar.rc.left - 16, m_diskReadBar.rc.top - 26,
			m_diskWriteBar.rc.right + 16, m_diskWriteBar.rc.bottom + 16 };
		FillRoundRect(hdc, diskFrame, CardRadius, BgPanelTop, BgPanel, BorderSoft, true);
		m_diskReadBar.Paint(hdc, m_fSmall, m_fValue);
		m_diskWriteBar.Paint(hdc, m_fSmall, m_fValue);

		m_cpuHist.Paint(hdc, m_fSmall);
		m_ramHist.Paint(hdc, m_fSmall);
	}

	void App::PaintDevices(HDC hdc, RECT content)
	{
		m_devList.Paint(hdc, m_fItem, m_fSmall, m_fSmall);
	}

	void App::OnPaint(HDC target, const RECT& client)
	{
		// Double buffer
		HDC mem = CreateCompatibleDC(target);
		HBITMAP bmp = CreateCompatibleBitmap(target, client.right, client.bottom);
		HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

		// Backdrop with subtle diagonal gradient
		GRADIENT_RECT grc{ 0, 1 };
		TRIVERTEX vt[2] = {
			{ 0, 0, (COLORWORD)(GetRValue(BgDeep) * 256), (COLORWORD)(GetGValue(BgDeep) * 256), (COLORWORD)(GetBValue(BgDeep) * 256), 0 },
			{ (WORD)client.right, (WORD)client.bottom, (COLORWORD)(GetRValue(R(14, 18, 28)) * 256), (COLORWORD)(GetGValue(R(14, 18, 28)) * 256), (COLORWORD)(GetBValue(R(14, 18, 28)) * 256), 0 },
		};
		GradientFill(mem, vt, 2, &grc, 1);

		m_nav.Paint(mem, m_fItem, m_fBrand, m_fSmall);

		RECT content = { m_nav.rc.right + GridGap, TitleBarHeight + GridMargin,
			client.right - GridMargin, client.bottom - GridMargin - StatusHeight };
		if (m_nav.sel == 0) PaintOverview(mem, content);
		else PaintDevices(mem, content);

		PaintTitleBar(mem, client);
		PaintStatusBar(mem, client);

		BitBlt(target, 0, 0, client.right, client.bottom, mem, 0, 0, SRCCOPY);

		SelectObject(mem, old);
		DeleteObject(bmp);
		DeleteDC(mem);
	}

	// ==================== data pipeline ====================
	void App::RequestData()
	{
		if (m_workerBusy) return;
		m_workerBusy = true;
		std::thread([this]() {
			Snapshot snap = m_snap;      // keep cached strings
			m_tele.Sample(snap);
			EnterCriticalSection(&m_cs);
			m_snap = snap;
			LeaveCriticalSection(&m_cs);
			PostMessageW(m_hwnd, RW_DATA_READY, 0, 0);
			m_workerBusy = false;
			}).detach();
	}

	// Device scan runs the existing SetupAPI enumeration off-thread.
	extern void CollectDeviceRows(std::vector<DeviceList::Row>& outRows, const std::wstring& filter);

	void App::RefreshDevicesAsync()
	{
		if (m_devBusy) return;
		m_devBusy = true;
		std::wstring filter = m_devList.filter;
		std::thread([this, filter]() {
			std::vector<DeviceList::Row> rows;
			CollectDeviceRows(rows, filter);
			EnterCriticalSection(&m_cs);
			m_pendingRows = std::move(rows);
			LeaveCriticalSection(&m_cs);
			PostMessageW(m_hwnd, RW_DATA_READY, 1, 0);
			m_devBusy = false;
			}).detach();
	}

	void App::OnTimerTick()
	{
		// animate widgets; request repaint only while animating or every second
		bool animating = false;
		animating |= m_cpuGauge.Tick();
		animating |= m_gpuGauge.Tick();
		animating |= m_ramBar.Tick();
		animating |= m_diskReadBar.Tick();
		animating |= m_diskWriteBar.Tick();

		if (animating || m_animFramesLeft > 0)
		{
			if (!animating) --m_animFramesLeft;
			InvalidateRect(m_hwnd, nullptr, FALSE);
		}
	}

	// ==================== message handling ====================
	LRESULT App::Handle(UINT msg, WPARAM wp, LPARAM lp)
	{
		switch (msg)
		{
		case WM_DESTROY:
			Shell_NotifyIconW(NIM_DELETE, &m_tray);
			KillTimer(m_hwnd, 1); KillTimer(m_hwnd, 2);
			theme::ShutdownGdiPlus();
			DeleteCriticalSection(&m_cs);
			PostQuitMessage(0);
			return 0;

		case WM_TIMER:
			if (wp == 1) RequestData();
			else OnTimerTick();
			return 0;

		case RW_DATA_READY:
		{
			EnterCriticalSection(&m_cs);
			double cpu = m_snap.cpuPercent;
			double ramFrac = m_snap.ramTotalMB ? (double)m_snap.ramUsedMB / m_snap.ramTotalMB : 0;
			bool isDev = (lp != 0);
			if (isDev) {
				m_devList.SetRows(m_pendingRows);
				m_devicesLoaded = true;
			}
			LeaveCriticalSection(&m_cs);

			if (!isDev) {
				m_cpuGauge.SetValue(cpu);
				// GPU gauge: use 2D/3D proxy = fraction of frames? Use memory pressure of GPU process list -> simple: share of cpu on average per core busy
				m_gpuGauge.SetValue(std::min(100.0, cpu * 0.6 + 8.0)); // placeholder visual until perf counters wired
				m_ramBar.SetValue(ramFrac);
				m_cpuHist.Push(cpu / 100.0);
				m_ramHist.Push(ramFrac);
				m_diskReadBar.SetValue(std::min(1.0, m_snap.diskReadBps / (80.0 * 1024 * 1024)));
				m_diskWriteBar.SetValue(std::min(1.0, m_snap.diskWriteBps / (80.0 * 1024 * 1024)));
			}
			m_animFramesLeft = 60; // keep animating ~1s
			InvalidateRect(m_hwnd, nullptr, FALSE);
			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(m_hwnd, &ps);
			RECT client; GetClientRect(m_hwnd, &client);
			OnPaint(hdc, client);
			EndPaint(m_hwnd, &ps);
			return 0;
		}

		case WM_SIZE:
		{
			RECT client; GetClientRect(m_hwnd, &client);
			Layout(client);
			InvalidateRect(m_hwnd, nullptr, FALSE);
			return 0;
		}

		case WM_GETMINMAXINFO:
		{
			MINMAXINFO* mmi = (MINMAXINFO*)lp;
			mmi->ptMinTrackSize.x = 900;
			mmi->ptMinTrackSize.y = 620;
			return 0;
		}

		case WM_MOUSEMOVE:
		{
			POINT pt{ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
			int oldHoverBtn = m_btns.hover;
			m_btns.hover = m_btns.HitTest(pt);
			int oldNavHover = m_nav.hover;
			m_nav.hover = m_nav.HitTest(pt);
			int oldRowHover = m_devList.hoverIdx;
			m_devList.hoverIdx = (m_nav.sel == 1) ? m_devList.HitTest(pt) : -1;

			if (m_btns.hover != oldHoverBtn || m_nav.hover != oldNavHover ||
				m_devList.hoverIdx != oldRowHover)
				InvalidateRect(m_hwnd, nullptr, FALSE);

			// drag-move window from empty title area
			if (m_dragging) {
				POINT sp; GetCursorPos(&sp);
				RECT wr; GetWindowRect(m_hwnd, &wr);
				SetWindowPos(m_hwnd, nullptr,
					sp.x - m_lastDragStart.x, sp.y - m_lastDragStart.y,
					wr.right - wr.left, wr.bottom - wr.top,
					SWP_NOZORDER | SWP_NOACTIVATE);
			}
			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			POINT pt{ (short)LOWORD(lp), (short)HIWORD(lp) };
			int btn = m_btns.HitTest(pt);
			if (btn == 0) { ShowWindow(m_hwnd, SW_MINIMIZE); return 0; }
			if (btn == 2) { DestroyWindow(m_hwnd); return 0; }
			if (btn == 1) { ToggleMaximize(); return 0; }

			int nav = m_nav.HitTest(pt);
			if (nav >= 0) {
				m_nav.sel = nav;
				if (nav == 1 && !m_devicesLoaded) RefreshDevicesAsync();
				InvalidateRect(m_hwnd, nullptr, FALSE);
				return 0;
			}
			// Start dragging if in title bar area
			if (pt.y < TitleBarHeight) {
				m_dragging = true;
				SetCapture(m_hwnd);
				POINT sp; GetCursorPos(&sp);
				RECT wr; GetWindowRect(m_hwnd, &wr);
				m_lastDragStart = { sp.x - wr.left, sp.y - wr.top };
			}
			return 0;
		}

		case WM_LBUTTONUP:
			if (m_dragging) { m_dragging = false; ReleaseCapture(); }
			return 0;

		case WM_MBUTTONDBLCLK:
			ToggleMaximize();
			return 0;

		case WM_RBUTTONUP:
		{
			if ((HWND)wp == m_hwnd && HIWORD(lp) == 0 && LOWORD(lp) == 0) break; // tray
			HMENU menu = CreatePopupMenu();
			AppendMenuW(menu, MF_STRING, 101, L"Refresh devices");
			AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
			AppendMenuW(menu, MF_STRING, 102, L"Exit");
			POINT sp; GetCursorPos(&sp);
			TrackPopupMenu(menu, TPM_RIGHTBUTTON, sp.x, sp.y, 0, m_hwnd, nullptr);
			DestroyMenu(menu);
			return 0;
		}

		case WM_COMMAND:
			if (LOWORD(wp) == 101) { m_devicesLoaded = false; RefreshDevicesAsync(); }
			if (LOWORD(wp) == 102) DestroyWindow(m_hwnd);
			return 0;

		case RW_TRAY:
			if (LOWORD(lp) == WM_LBUTTONDBLCLK) {
				ShowWindow(m_hwnd, SW_RESTORE);
				SetForegroundWindow(m_hwnd);
			}
			return 0;

		case WM_KEYDOWN:
			if (wp == VK_ESCAPE) DestroyWindow(m_hwnd);
			if (wp == VK_F5) { m_devicesLoaded = false; RefreshDevicesAsync(); RequestData(); }
			return 0;

		case WM_CHAR:
			// Device page search filter: printable chars edit the filter live
			if (m_nav.sel == 1) {
				if (wp == VK_BACK && !m_devList.filter.empty())
					m_devList.filter.pop_back();
				else if (wp >= 32 && wp < 127) {
					m_devList.filter += (wchar_t)wp;
					RefreshDevicesAsync();
				}
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			return 0;

		case WM_MOUSEWHEEL:
			if (m_nav.sel == 1) {
				m_devList.OnWheel((short)HIWORD(wp));
				InvalidateRect(m_hwnd, nullptr, FALSE);
			}
			return 0;

		case WM_SETTINGCHANGE:
			ApplyWindowRgn([&]{ RECT c; GetClientRect(m_hwnd, &c); return c; }());
			return 0;
		}
		return DefWindowProcW(m_hwnd, msg, wp, lp);
	}

	void App::ToggleMaximize()
	{
		if (!m_maximized) {
			ShowWindow(m_hwnd, SW_MAXIMIZE);
			m_maximized = true;
		} else {
			ShowWindow(m_hwnd, SW_RESTORE);
			m_maximized = false;
		}
		InvalidateRect(m_hwnd, nullptr, TRUE);
	}

} // namespace reise
