// bridges/GdiRenderer.h
// Direct2D-backed GDI renderer for the few shims that actually need to draw
// (rare — most games use D3D). Each gdi32 drawing call that hits a real HDC
// is routed here, where we lazily create an ID2D1DCRenderTarget per HDC and
// translate the GDI op into the equivalent D2D call.
//
// Brush / text color is tracked per-HDC in a side table so SetTextColor /
// SetBkColor round-trip correctly through FillRect / TextOut.

#pragma once
#include <Windows.h>

#include <cstdint>
#include <unordered_map>

namespace xwr {

class GdiRenderer {
public:
    static GdiRenderer& Instance();

    // Ensure a D2D DC render target exists for the given HDC.
    // Returns the ID2D1DCRenderTarget* (cast to void* for header isolation).
    void* GetRenderTarget(HDC hdc);

    // Drawing ops called by gdi32 shims:
    void FillRect(HDC hdc, const RECT* rc, HBRUSH brush);
    void Rectangle(HDC hdc, int l, int t, int r, int b);
    void LineTo(HDC hdc, int x, int y);
    void MoveToEx(HDC hdc, int x, int y, LPPOINT prev);
    void TextOutW(HDC hdc, int x, int y, LPCWSTR str, int len);
    void BitBlt(HDC hdc, int x, int y, int w, int h, HDC src, int sx, int sy, DWORD rop);
    void SetTextColor(HDC hdc, COLORREF c);
    void SetBkColor(HDC hdc, COLORREF c);

private:
    GdiRenderer();
    GdiRenderer(const GdiRenderer&) = delete;
    GdiRenderer& operator=(const GdiRenderer&) = delete;

    void EnsureD2D();

    // Per-HDC state: current pen position + foreground/background colors.
    struct DcState {
        void*    renderTarget = nullptr;  // ID2D1DCRenderTarget*
        int      penX = 0;
        int      penY = 0;
        COLORREF textColor = 0x00000000;   // black
        COLORREF bkColor   = 0x00FFFFFF;   // white
    };

    void* m_d2dFactory = nullptr;     // ID2D1Factory*
    void* m_dwriteFactory = nullptr;  // IDWriteFactory*

    std::unordered_map<HDC, DcState> m_dcStates;
};

}  // namespace xwr
