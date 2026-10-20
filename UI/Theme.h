#pragma once
// ============================================================
//  REISE Watcher - Dark Glass Edition : Theme / design tokens
// ============================================================
#include "pch.h"
#include <algorithm>

namespace reise {
namespace theme {

	// ---------- Palette (RGB) ----------
	inline COLORREF R(int r, int g, int b) { return RGB(r, g, b); }

	// Backgrounds
	const COLORREF BgDeep      = R(9, 12, 18);     // window backdrop
	const COLORREF BgPanel     = R(17, 22, 31);    // card surface
	const COLORREF BgPanelTop  = R(24, 30, 42);    // card gradient top
	const COLORREF BgHeader    = R(13, 17, 25);    // title bar
	const COLORREF BgField     = R(12, 16, 23);    // list rows / inputs

	// Borders & separators
	const COLORREF BorderSoft  = R(42, 52, 70);
	const COLORREF BorderGlow  = R(0, 229, 255);   // neon cyan accent

	// Accents
	const COLORREF Accent      = R(0, 229, 255);   // neon cyan
	const COLORREF AccentDim   = R(0, 130, 150);
	const COLORREF Violet      = R(124, 92, 255);  // secondary accent
	const COLORREF Good        = R(0, 230, 118);   // mint green
	const COLORREF Warn        = R(255, 176, 32);  // amber
	const COLORREF Bad         = R(255, 82, 82);   // coral red

	// Text
	const COLORREF TextPrimary = R(232, 238, 246);
	const COLORREF TextMuted   = R(128, 143, 166);
	const COLORREF TextFaint   = R(74, 86, 104);

	// ---------- Layout ----------
	const int CornerRadius   = 14;   // window corner radius
	const int CardRadius     = 12;   // card corner radius
	const int TitleBarHeight = 46;
	const int SideWidth      = 210;  // navigation rail width
	const int StatusHeight   = 30;
	const int GridMargin     = 18;
	const int GridGap        = 14;

	// ---------- Helpers ----------
	inline COLORREF Mix(COLORREF a, COLORREF b, double t)
	{
		t = std::max(0.0, std::min(1.0, t));
		int r = (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t);
		int g = (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t);
		int b2 = (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
		return RGB(r, g, b2);
	}

	// Color for a load value 0..1 (green -> amber -> red)
	inline COLORREF LoadColor(double v)
	{
		if (v < 0.6) return Mix(Good, Warn, v / 0.6);
		return Mix(Warn, Bad, (v - 0.6) / 0.4);
	}

	void InitGdiPlus();
	void ShutdownGdiPlus();

	// Draw filled rounded rectangle with optional vertical gradient
	void FillRoundRect(HDC hdc, RECT rc, int rad, COLORREF top, COLORREF bottom,
		COLORREF border = RGB(0, 0, 0), bool drawBorder = false);

	// Draw text inside rect with font/color/alignment
	void DrawTextInRect(HDC hdc, const wchar_t* text, RECT rc, HFONT font,
		COLORREF color, UINT fmt);

} // namespace theme
} // namespace reise
