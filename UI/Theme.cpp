#include "Theme.h"
#include <GdiPlus.h>
#pragma comment(lib, "GdiPlus.lib")

namespace reise {
namespace theme {

	// GDI+ startup token (initialized by App on startup)
	static ULONG_PTR g_gdiplusToken = 0;

	void InitGdiPlus()
	{
		Gdiplus::GdiplusStartupInput inp;
		Gdiplus::GdiplusStartup(&g_gdiplusToken, &inp, nullptr);
	}

	void ShutdownGdiPlus()
	{
		if (g_gdiplusToken) Gdiplus::GdiplusShutdown(g_gdiplusToken);
		g_gdiplusToken = 0;
	}

	// Filled rounded rectangle with vertical gradient and soft border, drawn via GDI+
	void FillRoundRect(HDC hdc, RECT rc, int rad, COLORREF top, COLORREF bottom,
		COLORREF border, bool drawBorder)
	{
		using namespace Gdiplus;
		Graphics g(hdc);
		g.SetSmoothingMode(SmoothingModeAntiAlias);

		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;
		if (w <= 2 || h <= 2) return;
		if (rad * 2 > w) rad = w / 2;
		if (rad * 2 > h) rad = h / 2;

		RectF r((REAL)rc.left + 1.f, (REAL)rc.top + 1.f, (REAL)w - 2.f, (REAL)h - 2.f);

		GraphicsPath path;
		INT d = rad * 2;
		path.AddArc(r.X, r.Y, d, d, 180, 90);
		path.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
		path.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
		path.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
		path.CloseFigure();

		LinearGradientBrush grad(r,
			Color(255, GetRValue(top), GetGValue(top), GetBValue(top)),
			Color(255, GetRValue(bottom), GetGValue(bottom), GetBValue(bottom)),
		 LinearGradientModeVertical);
		g.FillPath(&grad, &path);

		if (drawBorder)
		{
			Pen p(Color(140, GetRValue(border), GetGValue(border), GetBValue(border)), 1.f);
			g.DrawPath(&p, &path);
		}
	}

	void DrawTextInRect(HDC hdc, const wchar_t* text, RECT rc, HFONT font,
		COLORREF color, UINT fmt)
	{
		HFONT old = (HFONT)SelectObject(hdc, font);
		COLORREF oldColor = SetTextColor(hdc, color);
		int oldBk = SetBkMode(hdc, TRANSPARENT);
		DrawTextW(hdc, text, -1, &rc, fmt | DT_NOPREFIX | DT_SINGLELINE);
		SetBkMode(hdc, oldBk);
		SetTextColor(hdc, oldColor);
		SelectObject(hdc, old);
	}

} // namespace theme
} // namespace reise
