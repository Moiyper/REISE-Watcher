#include "Widgets.h"
#include <GdiPlus.h>
#include <sstream>
#include <iomanip>
#pragma comment(lib, "GdiPlus.lib")

namespace reise {

	using namespace theme;

	static std::wstring FmtPct(double v)
	{
		std::wostringstream oss;
		oss << std::fixed << std::setprecision(0) << v << L"%";
		return oss.str();
	}

	// ==================== Gauge (arc speedometer card) ====================
	bool Gauge::Tick()
	{
		double d = value - shown;
		if (std::abs(d) < 0.15) { shown = value; return false; }
		shown += d * 0.22; // ease-out
		return true;
	}

	void Gauge::Paint(HDC hdc, HFONT fTitle, HFONT fValue, HFONT fSmall)
	{
		using namespace Gdiplus;
		FillRoundRect(hdc, rc, CardRadius, BgPanelTop, BgPanel, BorderSoft, true);

		Graphics g(hdc);
		g.SetSmoothingMode(SmoothingModeAntiAlias);

		const int pad = 16;
		RECT rTitle = { rc.left + pad, rc.top + 10, rc.right - pad, rc.top + 32 };
		DrawTextInRect(hdc, title.c_str(), rTitle, fTitle, TextPrimary, DT_LEFT);
		if (!sub.empty())
		{
			RECT rSub = { rc.left + pad, rc.top + 32, rc.right - pad, rc.top + 50 };
			DrawTextInRect(hdc, sub.c_str(), rSub, fSmall, TextMuted, DT_LEFT);
		}

		// Arc geometry: bottom area of the card
		int arcSize = std::min(rc.right - rc.left - 2 * pad, rc.bottom - rc.top - 78);
		if (arcSize < 40) arcSize = 40;
		REAL cx = (REAL)(rc.left + (rc.right - rc.left) / 2);
		REAL cy = (REAL)(rc.bottom - arcSize / 2 - 14);
		RectF arcRect(cx - arcSize / 2.f, cy - arcSize / 2.f, (REAL)arcSize, (REAL)arcSize);

		// Track (background arc, 240 degrees starting at 150)
		Pen track(Color(70, GetRValue(BorderSoft), GetGValue(BorderSoft), GetBValue(BorderSoft)),
			(REAL)std::max(6, arcSize / 14));
		track.SetStartCap(LineCapRound);
		track.SetEndCap(LineCapRound);
		g.DrawArc(&track, arcRect, 150.f, 240.f);

		// Value arc with accent color depending on load
		double frac = std::clamp(shown / 100.0, 0.0, 1.0);
		COLORREF col = (accent == Accent) ? LoadColor(frac) : accent;
		LinearGradientBrush gb(arcRect,
			Color(255, GetRValue(col) / 2, GetGValue(col) / 2, GetBValue(col) / 2),
			Color(255, GetRValue(col), GetGValue(col), GetBValue(col)),
			LinearGradientModeHorizontal);
		Pen vp(&gb, (REAL)std::max(6, arcSize / 14));
		vp.SetStartCap(LineCapRound);
		vp.SetEndCap(LineCapRound);
		if (frac > 0.005)
			g.DrawArc(&vp, arcRect, 150.f, (REAL)(240.f * frac));

		// Big centered percentage
		std::wstring txt = FmtPct(shown);
		RECT rVal = { rc.left, (int)(cy - arcSize / 4), rc.right, (int)(cy + arcSize / 8) };
		DrawTextInRect(hdc, txt.c_str(), rVal, fValue, TextPrimary, DT_CENTER);

		// Tick labels 0 / 100
		RECT r0 = { (int)(cx - arcSize / 2) - 6, (int)(cy + arcSize / 2 - 26), (int)(cx - arcSize / 2) + 22, (int)(cy + arcSize / 2 - 8) };
		RECT r100 = { (int)(cx + arcSize / 2) - 22, (int)(cy + arcSize / 2 - 26), (int)(cx + arcSize / 2) + 8, (int)(cy + arcSize / 2 - 8) };
		DrawTextInRect(hdc, L"0", r0, fSmall, TextFaint, DT_CENTER);
		DrawTextInRect(hdc, L"100", r100, fSmall, TextFaint, DT_CENTER);
	}

	// ==================== ProgressBar ====================
	bool ProgressBar::Tick()
	{
		double d = value - shown;
		if (std::abs(d) < 0.002) { shown = value; return false; }
		shown += d * 0.2;
		return true;
	}

	void ProgressBar::Paint(HDC hdc, HFONT fSmall, HFONT fValue)
	{
		int h = rc.bottom - rc.top;
		RECT rLbl = { rc.left, rc.top, rc.right - 90, rc.top + 18 };
		DrawTextInRect(hdc, label.c_str(), rLbl, fSmall, TextMuted, DT_LEFT);

		std::wostringstream pct;
		pct << std::fixed << std::setprecision(0) << shown * 100.0 << L"%";
		RECT rPct = { rc.right - 90, rc.top - 2, rc.right, rc.top + 20 };
		DrawTextInRect(hdc, pct.str().c_str(), rPct, fValue, TextPrimary, DT_RIGHT);

		if (!rightText.empty())
		{
			RECT rRight = { rc.left, rc.top + 18, rc.right - 90, rc.top + h };
			DrawTextInRect(hdc, rightText.c_str(), rRight, fSmall, TextFaint, DT_LEFT);
		}

		// Bar track at bottom
		int barH = 8;
		RECT track = { rc.left, rc.bottom - barH, rc.right, rc.bottom };
		FillRoundRect(hdc, track, barH / 2 + 2, BgField, BgField, BorderSoft, true);

		int w = track.right - track.left;
		int fw = (int)(w * std::clamp(shown, 0.0, 1.0));
		if (fw > barH)
		{
			RECT fill = { track.left, track.top, track.left + fw, track.bottom };
			COLORREF c2 = Mix(accent, Bad, 0.0);
			FillRoundRect(hdc, fill, barH / 2 + 2, accent, c2, RGB(0, 0, 0), false);
		}
	}

	// ==================== Sparkline ====================
	void Sparkline::Paint(HDC hdc, HFONT fSmall)
	{
		using namespace Gdiplus;
		FillRoundRect(hdc, rc, CardRadius, BgPanelTop, BgPanel, BorderSoft, true);

		const int pad = 14;
		RECT rCap = { rc.left + pad, rc.top + 8, rc.right - pad, rc.top + 26 };
		DrawTextInRect(hdc, caption.c_str(), rCap, fSmall, TextMuted, DT_LEFT);

		if (data.size() < 2) return;

		Graphics g(hdc);
		g.SetSmoothingMode(SmoothingModeAntiAlias);

		RectF plot((REAL)(rc.left + pad), (REAL)(rc.top + 30),
			(REAL)(rc.right - rc.left - 2 * pad), (REAL)(rc.bottom - rc.top - 44));

		int n = (int)data.size();
		std::vector<PointF> pts(n);
		for (int i = 0; i < n; i++)
		{
			REAL x = plot.X + plot.Width * i / (REAL)(n - 1);
			REAL y = plot.GetBottom() - plot.Height * (REAL)std::clamp(data[i], 0.0, 1.0);
			pts[i] = PointF(x, y);
		}

		// Gradient fill under curve
		GraphicsPath area;
		area.AddLine(plot.X, plot.GetBottom(), pts[0].X, pts[0].Y);
		area.AddCurve(pts.data(), n);
		area.AddLine(pts[n - 1].X, plot.GetBottom(), plot.X, plot.GetBottom());
		area.CloseFigure();
		PathGradientBrush fb(&area, Color(90, GetRValue(line), GetGValue(line), GetBValue(line)),
			Color(0, GetRValue(line), GetGValue(line), GetBValue(line)),
			LinearGradientModeVertical);
		g.FillPath(&fb, &area);

		Pen p(Color(255, GetRValue(line), GetGValue(line), GetBValue(line)), 2.f);
		p.SetLineJoin(LineJoinRound);
		g.DrawCurve(&p, pts.data(), n);

		// Last point glow dot
		const PointF& last = pts[n - 1];
		g.FillEllipse(&SolidBrush(Color(60, GetRValue(line), GetGValue(line), GetBValue(line))),
			last.X - 6, last.Y - 6, 12, 12);
		g.FillEllipse(&SolidBrush(Color(255, GetRValue(line), GetGValue(line), GetBValue(line))),
			last.X - 2.5f, last.Y - 2.5f, 5, 5);
	}

	// ==================== NavRail ====================
	int NavRail::HitTest(POINT pt) const
	{
		if (!PtInRect(&rc, pt)) return -1;
		int top = rc.top + 96; // below brand block
		int idx = (pt.y - top) / itemH;
		if (idx >= 0 && idx < (int)items.size()) return idx;
		return -1;
	}

	void NavRail::Paint(HDC hdc, HFONT fItem, HFONT fBrand, HFONT fSmall)
	{
		// Rail background panel
		FillRoundRect(hdc, rc, CardRadius, BgHeader, BgPanel, BorderSoft, true);

		const int pad = 16;
		// Brand block
		RECT rLogo = { rc.left + pad, rc.top + 16, rc.left + pad + 34, rc.top + 50 };
		FillRoundRect(hdc, rLogo, 9, Accent, Mix(Accent, Violet, 0.6), RGB(0, 0, 0), false);
		DrawTextInRect(hdc, L"R", rLogo, fBrand, BgDeep, DT_CENTER);

		RECT rName = { rc.left + pad + 44, rc.top + 18, rc.right - pad, rc.top + 40 };
		DrawTextInRect(hdc, L"REISE", rName, fBrand, TextPrimary, DT_LEFT);
		RECT rTag = { rc.left + pad + 44, rc.top + 40, rc.right - pad, rc.top + 56 };
		DrawTextInRect(hdc, L"Watcher v2", rTag, fSmall, TextMuted, DT_LEFT);

		// Separator
		HPEN pen = CreatePen(PS_SOLID, 1, BorderSoft);
		HGDIOBJ oldPen = SelectObject(hdc, pen);
		MoveToEx(hdc, rc.left + pad, rc.top + 76, nullptr);
		LineTo(hdc, rc.right - pad, rc.top + 76);
		SelectObject(hdc, oldPen);
		DeleteObject(pen);

		// Items
		int top = rc.top + 96;
		for (int i = 0; i < (int)items.size(); i++)
		{
			RECT r = { rc.left + 10, top + i * itemH, rc.right - 10, top + (i + 1) * itemH };
			if (i == sel)
			{
				FillRoundRect(hdc, r, 9, Mix(Accent, BgPanel, 0.78), Mix(Accent, BgPanel, 0.88), Accent, true);
				// Active indicator bar
				RECT ind = { rc.left + 3, r.top + 10, rc.left + 6, r.bottom - 10 };
				FillRoundRect(hdc, ind, 2, Accent, Accent, RGB(0, 0, 0), false);
			}
			else if (i == hover)
			{
				FillRoundRect(hdc, r, 9, Mix(BgPanel, TextPrimary, 0.06), Mix(BgPanel, TextPrimary, 0.04), RGB(0, 0, 0), false);
			}
			RECT rIcon = { r.left + 10, r.top, r.left + 34, r.bottom };
			DrawTextInRect(hdc, icons[i].c_str(), rIcon, fItem,
				i == sel ? Accent : TextMuted, DT_CENTER);
			RECT rTxt = { r.left + 40, r.top, r.right - 8, r.bottom };
			DrawTextInRect(hdc, items[i].c_str(), rTxt, fItem,
				i == sel ? TextPrimary : TextMuted, DT_LEFT | DT_VCENTER);
		}

		// Footer hint
		RECT rFoot = { rc.left + pad, rc.bottom - 60, rc.right - pad, rc.bottom - 16 };
		DrawTextInRect(hdc, L"F5 - refresh\nEsc - quit", rFoot, fSmall, TextFaint,
			DT_LEFT | DT_WORDBREAK);
	}

	// ==================== TitleButtons ====================
	void TitleButtons::Layout(RECT client)
	{
		const int bw = 44, bh = 30, gap = 4, right = 10;
		int y = (TitleBarHeight - bh) / 2;
		rcClose = { client.right - right - bw, y, client.right - right, y + bh };
		rcMax   = { rcClose.left - gap - bw, y, rcClose.left - gap, y + bh };
		rcMin   = { rcMax.left - gap - bw, y, rcMax.left - gap, y + bh };
	}

	int TitleButtons::HitTest(POINT pt) const
	{
		if (PtInRect(&rcMin, pt)) return 0;
		if (PtInRect(&rcMax, pt)) return 1;
		if (PtInRect(&rcClose, pt)) return 2;
		return -1;
	}

	void TitleButtons::Paint(HDC hdc, HFONT fSegoe)
	{
		struct { RECT* rc; const wchar_t* glyph; COLORREF hoverBg; COLORREF hoverFg; } btns[3] = {
			{ &rcMin,   L"\uE921", Mix(BgPanel, TextPrimary, 0.10), TextPrimary }, // minimize
			{ &rcMax,   L"\uE922", Mix(BgPanel, TextPrimary, 0.10), TextPrimary }, // maximize
			{ &rcClose, L"\uE8BB", Bad,                           RGB(255, 255, 255) }, // close
		};
		for (int i = 0; i < 3; i++)
		{
			bool hot = (hover == i);
			if (hot) FillRoundRect(hdc, *btns[i].rc, 7, btns[i].hoverBg, btns[i].hoverBg, RGB(0, 0, 0), false);
			DrawTextInRect(hdc, btns[i].glyph, *btns[i].rc, fSegoe,
				hot ? btns[i].hoverFg : TextMuted, DT_CENTER);
		}
	}

	// ==================== DeviceList ====================
	int DeviceList::HitTest(POINT pt) const
	{
		if (!PtInRect(&rc, pt)) return -1;
		int idx = (pt.y - rc.top + scroll) / rowH;
		if (idx >= 0 && idx < (int)rows.size()) return idx;
		return -1;
	}

	void DeviceList::Paint(HDC hdc, HFONT fName, HFONT fCls, HFONT fSmall)
	{
		FillRoundRect(hdc, rc, CardRadius, BgPanelTop, BgPanel, BorderSoft, true);

		const int pad = 14;
		// Header row with count + filter echo
		std::wostringstream hdr;
		hdr << rows.size() << L" devices";
		if (!filter.empty()) { std::wostringstream fw; fw << filter.c_str(); hdr << L"  \u2022  filter: \"" << fw.str() << L"\""; }
		RECT rHdr = { rc.left + pad, rc.top + 8, rc.right - pad, rc.top + 28 };
		DrawTextInRect(hdc, hdr.str().c_str(), rHdr, fSmall, TextMuted, DT_LEFT);

		// Clip region for rows
		RECT body = { rc.left + 1, rc.top + 32, rc.right - 1, rc.bottom - 1 };
		HRGN clip = CreateRectRgn(body.left, body.top, body.right, body.bottom);
		SelectClipRgn(hdc, clip);

		int first = scroll / rowH;
		int last = first + body.bottom / rowH + VisibleCount() + 2;
		for (int i = first; i < (int)rows.size() && i < last; i++)
		{
			int y = rc.top + 34 + i * rowH - scroll;
			if (y + rowH < body.top || y > body.bottom) continue;
			RECT r = { body.left + 6, y, body.right - 6, y + rowH - 4 };

			if (i == hoverIdx)
				FillRoundRect(hdc, r, 8, Mix(BgPanel, Accent, 0.10), Mix(BgPanel, Accent, 0.06), RGB(0, 0, 0), false);

			// Status dot
			RECT dot = { r.left + 8, r.top + rowH / 2 - 8, r.left + 14, r.top + rowH / 2 - 2 };
			FillRoundRect(hdc, dot, 3, Good, Good, RGB(0, 0, 0), false);

			RECT rn = { r.left + 20, r.top, r.right - 150, r.bottom };
			DrawTextInRect(hdc, rows[i].name.c_str(), rn, fName, TextPrimary,
				DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS);
			RECT rc2 = { r.right - 140, r.top, r.right - 10, r.bottom };
			DrawTextInRect(hdc, rows[i].cls.c_str(), rc2, fCls, TextFaint,
				DT_RIGHT | DT_VCENTER | DT_END_ELLIPSIS);
		}

		SelectClipRgn(hdc, nullptr);
		DeleteObject(clip);

		if (rows.empty())
		{
			RECT rEmpty = body;
			DrawTextInRect(hdc, L"No devices match. Press F5 to scan hardware.",
				rEmpty, fCls, TextFaint, DT_CENTER | DT_VCENTER);
		}

		// Scrollbar thumb
		int viewH = body.bottom - body.top;
		int totalH = (int)rows.size() * rowH;
		if (totalH > viewH)
		{
			int trackX = rc.right - 8;
			int th = std::max(30, viewH * viewH / totalH);
			int ty = body.top + (viewH - th) * scroll / std::max(1, totalH - viewH);
			RECT thumb = { trackX, ty, trackX + 4, ty + th };
			FillRoundRect(hdc, thumb, 2, BorderSoft, BorderSoft, RGB(0, 0, 0), false);
		}
	}

} // namespace reise
