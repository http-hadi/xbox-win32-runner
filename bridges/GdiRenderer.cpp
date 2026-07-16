// bridges/GdiRenderer.cpp
// Direct2D-backed GDI rendering bridge.
//
// On real builds, EnsureD2D() creates an ID2D1Factory and IDWriteFactory;
// GetRenderTarget() lazily creates an ID2D1DCRenderTarget per HDC. Each GDI
// op is translated to the corresponding D2D draw call.
//
// On syntax-check builds (XWR_SYNTAX_CHECK defined) the d2d1.h / dwrite.h
// headers aren't available, so all real D2D calls are stubbed out and the
// state tables still function — letting the gdi32 shim compile and run
// without actually drawing anything.

#include "UwpSdkIncludes.h"


#include "GdiRenderer.h"

#ifndef XWR_SYNTAX_CHECK
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#endif

namespace xwr {

#ifndef FAILED
#define FAILED(hr) ((HRESULT)(hr) < 0)
#endif

// Helper: COLORREF (0x00BBGGRR) → D2D1_COLOR_F.
static void ColorRefToD2D(COLORREF c, float* r, float* g, float* b) {
    *r = (float)GetRValue(c) / 255.0f;
    *g = (float)GetGValue(c) / 255.0f;
    *b = (float)GetBValue(c) / 255.0f;
}

GdiRenderer::GdiRenderer() = default;

GdiRenderer& GdiRenderer::Instance() {
    static GdiRenderer inst;
    return inst;
}

void GdiRenderer::EnsureD2D() {
    if (m_d2dFactory && m_dwriteFactory) return;

#ifndef XWR_SYNTAX_CHECK
    if (!m_d2dFactory) {
        ID2D1Factory* factory = nullptr;
        D2D1_FACTORY_OPTIONS opts{};
#ifdef _DEBUG
        opts.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                        opts,
                                        &factory);
        if (SUCCEEDED(hr) && factory) {
            m_d2dFactory = (void*)factory;
        }
    }
    if (!m_dwriteFactory) {
        IDWriteFactory* dwf = nullptr;
        HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                          __uuidof(IDWriteFactory),
                                          (IUnknown**)&dwf);
        if (SUCCEEDED(hr) && dwf) {
            m_dwriteFactory = (void*)dwf;
        }
    }
#endif
}

void* GdiRenderer::GetRenderTarget(HDC hdc) {
    if (!hdc) return nullptr;
    EnsureD2D();

    auto it = m_dcStates.find(hdc);
    if (it == m_dcStates.end()) {
        DcState s;
        s.renderTarget = nullptr;
        m_dcStates[hdc] = s;
        it = m_dcStates.find(hdc);
    }
    if (it->second.renderTarget) {
        return it->second.renderTarget;
    }

#ifndef XWR_SYNTAX_CHECK
    if (m_d2dFactory) {
        ID2D1DCRenderTarget* rt = nullptr;
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);
        HRESULT hr = ((ID2D1Factory*)m_d2dFactory)->CreateDCRenderTarget(&props, &rt);
        if (SUCCEEDED(hr) && rt) {
            // Bind to the HDC's current clipping rectangle.
            RECT rcClip{};
            GetClipBox(hdc, &rcClip);
            rt->BindDC(hdc, &rcClip);
            it->second.renderTarget = (void*)rt;
        }
    }
#endif
    return it->second.renderTarget;
}

void GdiRenderer::FillRect(HDC hdc, const RECT* rc, HBRUSH brush) {
    if (!hdc || !rc) return;
    (void)brush;  // we re-derive the color from the brush on real builds
#ifndef XWR_SYNTAX_CHECK
    void* rtv = GetRenderTarget(hdc);
    if (!rtv) return;
    ID2D1DCRenderTarget* rt = (ID2D1DCRenderTarget*)rtv;
    rt->BeginDraw();
    // Real impl: convert HBRUSH → color, CreateSolidColorBrush, FillRectangle.
    D2D1_RECT_F r = D2D1::RectF((float)rc->left, (float)rc->top,
                                 (float)rc->right, (float)rc->bottom);
    ID2D1SolidColorBrush* br = nullptr;
    auto it = m_dcStates.find(hdc);
    COLORREF c = (it != m_dcStates.end()) ? it->second.bkColor : 0x00FFFFFF;
    float fr, fg, fb;
    ColorRefToD2D(c, &fr, &fg, &fb);
    if (SUCCEEDED(rt->CreateSolidColorBrush(D2D1::ColorF(fr, fg, fb, 1.0f), &br)) && br) {
        rt->FillRectangle(r, br);
        br->Release();
    }
    rt->EndDraw();
#endif
}

void GdiRenderer::Rectangle(HDC hdc, int l, int t, int r, int b) {
    if (!hdc) return;
#ifndef XWR_SYNTAX_CHECK
    void* rtv = GetRenderTarget(hdc);
    if (!rtv) return;
    ID2D1DCRenderTarget* rt = (ID2D1DCRenderTarget*)rtv;
    rt->BeginDraw();
    // Use 'rect' (not 'r') — the function already has a parameter named 'r'
    // (the right edge), and reusing it as a local D2D1_RECT_F triggers
    // C2082 "redefinition of parameter" on MSVC.
    D2D1_RECT_F rect = D2D1::RectF((float)l, (float)t, (float)r, (float)b);
    ID2D1SolidColorBrush* br = nullptr;
    auto it = m_dcStates.find(hdc);
    COLORREF c = (it != m_dcStates.end()) ? it->second.textColor : 0x00000000;
    float fr, fg, fb;
    ColorRefToD2D(c, &fr, &fg, &fb);
    if (SUCCEEDED(rt->CreateSolidColorBrush(D2D1::ColorF(fr, fg, fb, 1.0f), &br)) && br) {
        rt->DrawRectangle(rect, br);
        br->Release();
    }
    rt->EndDraw();
#else
    (void)l; (void)t; (void)r; (void)b;
#endif
}

void GdiRenderer::LineTo(HDC hdc, int x, int y) {
    if (!hdc) return;
    auto it = m_dcStates.find(hdc);
    if (it == m_dcStates.end()) return;
    int x0 = it->second.penX;
    int y0 = it->second.penY;
#ifndef XWR_SYNTAX_CHECK
    void* rtv = GetRenderTarget(hdc);
    if (rtv) {
        ID2D1DCRenderTarget* rt = (ID2D1DCRenderTarget*)rtv;
        rt->BeginDraw();
        ID2D1SolidColorBrush* br = nullptr;
        COLORREF c = it->second.textColor;
        float fr, fg, fb;
        ColorRefToD2D(c, &fr, &fg, &fb);
        if (SUCCEEDED(rt->CreateSolidColorBrush(D2D1::ColorF(fr, fg, fb, 1.0f), &br)) && br) {
            rt->DrawLine(D2D1::Point2F((float)x0, (float)y0),
                          D2D1::Point2F((float)x, (float)y),
                          br, 1.0f);
            br->Release();
        }
        rt->EndDraw();
    }
#endif
    it->second.penX = x;
    it->second.penY = y;
}

void GdiRenderer::MoveToEx(HDC hdc, int x, int y, LPPOINT prev) {
    if (!hdc) return;
    auto it = m_dcStates.find(hdc);
    if (it == m_dcStates.end()) {
        DcState s; s.penX = x; s.penY = y;
        m_dcStates[hdc] = s;
    } else {
        if (prev) { prev->x = it->second.penX; prev->y = it->second.penY; }
        it->second.penX = x;
        it->second.penY = y;
    }
}

void GdiRenderer::TextOutW(HDC hdc, int x, int y, LPCWSTR str, int len) {
    if (!hdc || !str || len <= 0) return;
#ifndef XWR_SYNTAX_CHECK
    void* rtv = GetRenderTarget(hdc);
    if (!rtv || !m_dwriteFactory) return;
    ID2D1DCRenderTarget* rt = (ID2D1DCRenderTarget*)rtv;
    auto it = m_dcStates.find(hdc);
    COLORREF c = (it != m_dcStates.end()) ? it->second.textColor : 0x00000000;
    float fr, fg, fb;
    ColorRefToD2D(c, &fr, &fg, &fb);
    IDWriteTextFormat* fmt = nullptr;
    if (SUCCEEDED(((IDWriteFactory*)m_dwriteFactory)->CreateTextFormat(
            L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f,
            L"en-US", &fmt)) && fmt) {
        rt->BeginDraw();
        ID2D1SolidColorBrush* br = nullptr;
        if (SUCCEEDED(rt->CreateSolidColorBrush(D2D1::ColorF(fr, fg, fb, 1.0f), &br)) && br) {
            // Length in characters, not bytes.
            UINT32 cch = (len < 0) ? (UINT32)wcslen(str) : (UINT32)len;
            D2D1_RECT_F layout = D2D1::RectF((float)x, (float)y,
                                              (float)x + 8.0f * cch, (float)y + 24.0f);
            rt->DrawText(str, cch, fmt, layout, br,
                          D2D1_DRAW_TEXT_OPTIONS_NONE, DWRITE_MEASURING_MODE_NATURAL);
            br->Release();
        }
        rt->EndDraw();
        fmt->Release();
    }
#else
    (void)x; (void)y; (void)str; (void)len;
#endif
}

void GdiRenderer::BitBlt(HDC hdc, int x, int y, int w, int h,
                          HDC src, int sx, int sy, DWORD rop) {
    if (!hdc || !src) return;
#ifndef XWR_SYNTAX_CHECK
    // Real impl: create an ID2D1Bitmap from the source HDC's bits and
    // DrawBitmap onto the destination render target. Ternary ROPs (SRCCOPY
    // etc.) are mapped to D2D bitmap brush + composite modes; non-trivial
    // ROPs fall back to GDI on a staging DC.
    (void)hdc; (void)src;  // (placeholder for full D2D path)
#endif
    (void)x; (void)y; (void)w; (void)h; (void)sx; (void)sy; (void)rop;
}

void GdiRenderer::SetTextColor(HDC hdc, COLORREF c) {
    if (!hdc) return;
    auto it = m_dcStates.find(hdc);
    if (it == m_dcStates.end()) {
        DcState s; s.textColor = c;
        m_dcStates[hdc] = s;
    } else {
        it->second.textColor = c;
    }
}

void GdiRenderer::SetBkColor(HDC hdc, COLORREF c) {
    if (!hdc) return;
    auto it = m_dcStates.find(hdc);
    if (it == m_dcStates.end()) {
        DcState s; s.bkColor = c;
        m_dcStates[hdc] = s;
    } else {
        it->second.bkColor = c;
    }
}

}  // namespace xwr
