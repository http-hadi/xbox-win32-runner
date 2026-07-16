// shims/gdi32/Gdi32Shim.cpp
// Win32 gdi32 shim layer. Provides an in-process DC/object table so games
// can call GDI functions without crashing. No actual rendering is performed.

#include "UwpSdkIncludes.h"


#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <cstring>

#include "../ShimRegistry.h"

#ifndef GDI_ERROR
#define GDI_ERROR (0xFFFFFFFFL)
#endif

// Constants not in the UWP SDK subset.
#ifndef OBJ_PEN
#define OBJ_PEN 1
#endif
#ifndef OBJ_BRUSH
#define OBJ_BRUSH 2
#endif
#ifndef OBJ_DC
#define OBJ_DC 3
#endif
#ifndef OBJ_METADC
#define OBJ_METADC 4
#endif
#ifndef OBJ_PAL
#define OBJ_PAL 5
#endif
#ifndef OBJ_FONT
#define OBJ_FONT 6
#endif
#ifndef OBJ_BITMAP
#define OBJ_BITMAP 7
#endif
#ifndef OBJ_REGION
#define OBJ_REGION 8
#endif
#ifndef OBJ_MEMDC
#define OBJ_MEMDC 10
#endif
#ifndef OBJ_ENHMETADC
#define OBJ_ENHMETADC 12
#endif
#ifndef OBJ_ENHMETAFILE
#define OBJ_ENHMETAFILE 13
#endif
#ifndef RGN_AND
#define RGN_AND 1
#endif
#ifndef RGN_OR
#define RGN_OR 2
#endif
#ifndef RGN_XOR
#define RGN_XOR 3
#endif
#ifndef RGN_DIFF
#define RGN_DIFF 4
#endif
#ifndef RGN_COPY
#define RGN_COPY 5
#endif
#ifndef NULLREGION
#define NULLREGION 1
#endif
#ifndef SIMPLEREGION
#define SIMPLEREGION 2
#endif
#ifndef COMPLEXREGION
#define COMPLEXREGION 3
#endif

namespace xwr {

// ---------------------------------------------------------------------------
// DC / object table
// ---------------------------------------------------------------------------
struct ObjectState {
    int     type = 0;
    DWORD   color = 0;       // for brush/pen
    int     stockId = -1;    // for stock objects
    LOGFONTW logFont{};
    COLORREF textColor = RGB(0, 0, 0);
    COLORREF bkColor = RGB(255, 255, 255);
    int      bkMode = OPAQUE;
    int      mapMode = MM_TEXT;
    int      rop2 = R2_COPYPEN;
    int      polyFillMode = ALTERNATE;
    int      stretchBltMode = BLACKONWHITE;
    UINT     textAlign = TA_LEFT | TA_TOP;
    HGDIOBJ  curBrush = 0;
    HGDIOBJ  curPen = 0;
    HGDIOBJ  curFont = 0;
  HGDIOBJ  curBitmap = 0;
    HGDIOBJ  curRgn = 0;
    HPALETTE curPalette = 0;
};

static std::atomic<uint32_t> g_nextHandle{0x40000};

static std::mutex& GdiMutex() {
    static std::mutex m;
    return m;
}
static std::unordered_map<HGDIOBJ, ObjectState>& GdiTable() {
    static std::unordered_map<HGDIOBJ, ObjectState> t;
    return t;
}

static HGDIOBJ AllocObj(int type) {
    HGDIOBJ h = (HGDIOBJ)g_nextHandle.fetch_add(1);
    std::lock_guard<std::mutex> lk(GdiMutex());
    ObjectState s;
    s.type = type;
    GdiTable()[h] = s;
    return h;
}

// ---------------------------------------------------------------------------
// DC creation
// ---------------------------------------------------------------------------
extern "C" HDC __stdcall Shim_CreateCompatibleDC(HDC) {
    HDC h = (HDC)AllocObj(OBJ_MEMDC);
    return h;
}
extern "C" BOOL __stdcall Shim_DeleteDC(HDC hdc) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    return GdiTable().erase((HGDIOBJ)hdc) ? TRUE : FALSE;
}

// ---------------------------------------------------------------------------
// Object selection / deletion
// ---------------------------------------------------------------------------
extern "C" HGDIOBJ __stdcall Shim_SelectObject(HDC hdc, HGDIOBJ h) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return (HGDIOBJ)0;
    ObjectState& dc = it->second;
    HGDIOBJ prev = 0;
    // Determine the object type by querying its entry (if known), else fall
    // back to assuming it's a brush — many callers just call SelectObject and
    // discard the return.
    auto oit = GdiTable().find(h);
    int type = (oit != GdiTable().end()) ? oit->second.type : OBJ_BRUSH;
    switch (type) {
        case OBJ_BRUSH:  prev = dc.curBrush;   dc.curBrush = h;   break;
        case OBJ_PEN:    prev = dc.curPen;     dc.curPen = h;     break;
        case OBJ_FONT:   prev = dc.curFont;    dc.curFont = h;    break;
        case OBJ_BITMAP: prev = dc.curBitmap;  dc.curBitmap = h;  break;
        case OBJ_REGION: prev = dc.curRgn;     dc.curRgn = h;     break;
        case OBJ_PAL:    prev = (HGDIOBJ)dc.curPalette; dc.curPalette = (HPALETTE)h; break;
        default:         prev = dc.curBrush;   dc.curBrush = h;   break;
    }
    return prev ? prev : (HGDIOBJ)1;  // Return non-null to signal success.
}
extern "C" BOOL __stdcall Shim_DeleteObject(HGDIOBJ h) {
    if (!h) return FALSE;
    std::lock_guard<std::mutex> lk(GdiMutex());
    return GdiTable().erase(h) ? TRUE : FALSE;
}
extern "C" HGDIOBJ __stdcall Shim_GetStockObject(int i) {
    // Pre-create the stock objects lazily.
    static std::once_flag flag;
    static HGDIOBJ stock[32] = {0};
    std::call_once(flag, []() {
        for (int i = 0; i < 32; ++i) {
            int type = (i >= WHITE_BRUSH && i <= NULL_BRUSH) ? OBJ_BRUSH
                     : (i >= WHITE_PEN && i <= NULL_PEN) ? OBJ_PEN
                     : (i == SYSTEM_FONT) ? OBJ_FONT
                     : (i == DEFAULT_PALETTE) ? OBJ_PAL
                     : OBJ_BRUSH;
            stock[i] = AllocObj(type);
        }
    });
    if (i < 0 || i >= 32) return AllocObj(OBJ_BRUSH);
    return stock[i];
}
extern "C" HGDIOBJ __stdcall Shim_GetCurrentObject(HDC hdc, UINT uObjectType) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return (HGDIOBJ)0;
    ObjectState& dc = it->second;
    switch (uObjectType) {
        case OBJ_BRUSH:  return dc.curBrush;
        case OBJ_PEN:    return dc.curPen;
        case OBJ_FONT:   return dc.curFont;
        case OBJ_BITMAP: return dc.curBitmap;
        case OBJ_PAL:    return (HGDIOBJ)dc.curPalette;
        default:         return (HGDIOBJ)0;
    }
}

// ---------------------------------------------------------------------------
// Bitmaps
// ---------------------------------------------------------------------------
extern "C" HBITMAP __stdcall Shim_CreateCompatibleBitmap(HDC, int, int) {
    return (HBITMAP)AllocObj(OBJ_BITMAP);
}
extern "C" HBITMAP __stdcall Shim_CreateBitmap(int, int, UINT, UINT, const void*) {
    return (HBITMAP)AllocObj(OBJ_BITMAP);
}
extern "C" HBITMAP __stdcall Shim_CreateDIBSection(HDC, const BITMAPINFO*, UINT, void** ppvBits,
                                                    HANDLE, DWORD) {
    if (ppvBits) *ppvBits = nullptr;
    return (HBITMAP)AllocObj(OBJ_BITMAP);
}
extern "C" int __stdcall Shim_GetObjectW(HGDIOBJ hgdiobj, int cbBuffer, LPVOID lpvObject) {
    if (!lpvObject || cbBuffer <= 0) return 0;
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find(hgdiobj);
    if (it == GdiTable().end()) return 0;
    if (it->second.type == OBJ_FONT) {
        if (cbBuffer >= (int)sizeof(LOGFONTW)) {
            std::memcpy(lpvObject, &it->second.logFont, sizeof(LOGFONTW));
            return sizeof(LOGFONTW);
        }
    }
    return 0;
}
extern "C" UINT __stdcall Shim_GetDIBColorTable(HDC, UINT, UINT, RGBQUAD*) { return 0; }
extern "C" UINT __stdcall Shim_SetDIBColorTable(HDC, UINT, UINT, const RGBQUAD*) { return 0; }

// ---------------------------------------------------------------------------
// Brush / pen / font creation
// ---------------------------------------------------------------------------
extern "C" HBRUSH __stdcall Shim_CreateSolidBrush(COLORREF c) {
    HGDIOBJ h = AllocObj(OBJ_BRUSH);
    {
        std::lock_guard<std::mutex> lk(GdiMutex());
        GdiTable()[h].color = c;
    }
    return (HBRUSH)h;
}
extern "C" HBRUSH __stdcall Shim_CreateBrushIndirect(const LOGBRUSH* plb) {
    HGDIOBJ h = AllocObj(OBJ_BRUSH);
    if (plb) {
        std::lock_guard<std::mutex> lk(GdiMutex());
        GdiTable()[h].color = plb->lbColor;
    }
    return (HBRUSH)h;
}
extern "C" HBRUSH __stdcall Shim_CreateHatchBrush(int, COLORREF c) {
    HGDIOBJ h = AllocObj(OBJ_BRUSH);
    std::lock_guard<std::mutex> lk(GdiMutex());
    GdiTable()[h].color = c;
    return (HBRUSH)h;
}
extern "C" HBRUSH __stdcall Shim_CreatePatternBrush(HBITMAP) {
    return (HBRUSH)AllocObj(OBJ_BRUSH);
}
extern "C" HPEN __stdcall Shim_CreatePen(int, int, COLORREF c) {
    HGDIOBJ h = AllocObj(OBJ_PEN);
    std::lock_guard<std::mutex> lk(GdiMutex());
    GdiTable()[h].color = c;
    return (HPEN)h;
}
extern "C" HPEN __stdcall Shim_ExtCreatePen(DWORD, DWORD, const LOGBRUSH* plb, DWORD, const DWORD*) {
    HGDIOBJ h = AllocObj(OBJ_PEN);
    if (plb) {
        std::lock_guard<std::mutex> lk(GdiMutex());
        GdiTable()[h].color = plb->lbColor;
    }
    return (HPEN)h;
}
extern "C" HFONT __stdcall Shim_CreateFontIndirectW(const LOGFONTW* lplf) {
    HGDIOBJ h = AllocObj(OBJ_FONT);
    if (lplf) {
        std::lock_guard<std::mutex> lk(GdiMutex());
        GdiTable()[h].logFont = *lplf;
    }
    return (HFONT)h;
}
extern "C" HFONT __stdcall Shim_CreateFontW(int, int, int, int, int, DWORD, DWORD, DWORD,
                                            DWORD, DWORD, DWORD, DWORD, DWORD, LPCWSTR) {
    return (HFONT)AllocObj(OBJ_FONT);
}

// ---------------------------------------------------------------------------
// BitBlt family — no-op rendering, claim success
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD) { return TRUE; }
extern "C" BOOL __stdcall Shim_StretchBlt(HDC, int, int, int, int, HDC, int, int, int, int, DWORD) { return TRUE; }
extern "C" BOOL __stdcall Shim_AlphaBlend(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION) { return TRUE; }
extern "C" BOOL __stdcall Shim_TransparentBlt(HDC, int, int, int, int, HDC, int, int, int, int, UINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_MaskBlt(HDC, int, int, int, int, HDC, int, int, HBITMAP, int, int, DWORD) { return TRUE; }
extern "C" BOOL __stdcall Shim_PlgBlt(HDC, const POINT*, HDC, int, int, int, int, HBITMAP, int, int) { return TRUE; }
extern "C" int  __stdcall Shim_SetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int, int, UINT, UINT, const void*, const BITMAPINFO*, UINT) { return 1; }
extern "C" int  __stdcall Shim_StretchDIBits(HDC, int, int, int, int, int, int, int, int, const void*, const BITMAPINFO*, UINT, DWORD) { return 1; }
extern "C" int  __stdcall Shim_GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, LPBITMAPINFO, UINT) { return 0; }

// ---------------------------------------------------------------------------
// Pixel ops
// ---------------------------------------------------------------------------
extern "C" COLORREF __stdcall Shim_SetPixel(HDC, int, int, COLORREF c) { return c; }
extern "C" COLORREF __stdcall Shim_GetPixel(HDC, int, int) { return RGB(0, 0, 0); }

// ---------------------------------------------------------------------------
// Path / shape drawing — no-op rendering
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_MoveToEx(HDC, int, int, LPPOINT pp) {
    if (pp) { pp->x = 0; pp->y = 0; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_LineTo(HDC, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_Rectangle(HDC, int, int, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_RoundRect(HDC, int, int, int, int, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_Ellipse(HDC, int, int, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_Arc(HDC, int, int, int, int, int, int, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_Pie(HDC, int, int, int, int, int, int, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_Chord(HDC, int, int, int, int, int, int, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_Polyline(HDC, const POINT*, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_PolylineTo(HDC, const POINT*, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_Polygon(HDC, const POINT*, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_PolyBezier(HDC, const POINT*, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_PolyBezierTo(HDC, const POINT*, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_InvertRect(HDC, const RECT*) { return TRUE; }
extern "C" BOOL __stdcall Shim_DrawFocusRect(HDC, const RECT*) { return TRUE; }
extern "C" int  __stdcall Shim_FillRgn(HDC, HRGN, HBRUSH) { return 1; }
extern "C" int  __stdcall Shim_FrameRgn(HDC, HRGN, HBRUSH, int, int) { return 1; }
extern "C" int  __stdcall Shim_PaintRgn(HDC, HRGN) { return 1; }

// ---------------------------------------------------------------------------
// Regions
// ---------------------------------------------------------------------------
extern "C" HRGN __stdcall Shim_CreateRectRgn(int l, int t, int r, int b) {
    HGDIOBJ h = AllocObj(OBJ_REGION);
    (void)l; (void)t; (void)r; (void)b;
    return (HRGN)h;
}
extern "C" HRGN __stdcall Shim_CreateRectRgnIndirect(const RECT*) {
    return (HRGN)AllocObj(OBJ_REGION);
}
extern "C" HRGN __stdcall Shim_CreateEllipticRgn(int, int, int, int) {
    return (HRGN)AllocObj(OBJ_REGION);
}
extern "C" HRGN __stdcall Shim_CreateEllipticRgnIndirect(const RECT*) {
    return (HRGN)AllocObj(OBJ_REGION);
}
extern "C" HRGN __stdcall Shim_CreatePolygonRgn(const POINT*, int, int) {
    return (HRGN)AllocObj(OBJ_REGION);
}
extern "C" HRGN __stdcall Shim_CreatePolyPolygonRgn(const POINT*, const INT*, int, int) {
    return (HRGN)AllocObj(OBJ_REGION);
}
extern "C" HRGN __stdcall Shim_CreateRoundRectRgn(int, int, int, int, int, int) {
    return (HRGN)AllocObj(OBJ_REGION);
}
extern "C" HRGN __stdcall Shim_ExtCreateRegion(const XFORM*, DWORD, const RGNDATA*) {
    return (HRGN)AllocObj(OBJ_REGION);
}
extern "C" int __stdcall Shim_CombineRgn(HRGN, HRGN, HRGN, int) { return SIMPLEREGION; }
extern "C" BOOL __stdcall Shim_EqualRgn(HRGN, HRGN) { return FALSE; }
extern "C" BOOL __stdcall Shim_OffsetRgn(HRGN, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_PtInRegion(HRGN, int, int) { return FALSE; }
extern "C" BOOL __stdcall Shim_RectInRegion(HRGN, const RECT*) { return FALSE; }
extern "C" BOOL __stdcall Shim_GetRgnBox(HRGN, LPRECT lpRect) {
    if (!lpRect) return FALSE;
    lpRect->left = 0; lpRect->top = 0; lpRect->right = 1920; lpRect->bottom = 1080;
    return TRUE;
}

// ---------------------------------------------------------------------------
// Clipping
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_SelectClipRgn(HDC, HRGN) { return SIMPLEREGION; }
extern "C" int __stdcall Shim_ExtSelectClipRgn(HDC, HRGN, int) { return SIMPLEREGION; }
extern "C" int __stdcall Shim_ExcludeClipRect(HDC, int, int, int, int) { return SIMPLEREGION; }
extern "C" int __stdcall Shim_IntersectClipRect(HDC, int, int, int, int) { return SIMPLEREGION; }
extern "C" int __stdcall Shim_OffsetClipRgn(HDC, int, int) { return SIMPLEREGION; }
extern "C" BOOL __stdcall Shim_GetClipBox(HDC, LPRECT lpRect) {
    if (!lpRect) return FALSE;
    lpRect->left = 0; lpRect->top = 0; lpRect->right = 1920; lpRect->bottom = 1080;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_PtVisible(HDC, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_RectVisible(HDC, const RECT*) { return TRUE; }

// ---------------------------------------------------------------------------
// DC state save/restore
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_SaveDC(HDC) { return 1; }
extern "C" BOOL __stdcall Shim_RestoreDC(HDC, int) { return TRUE; }

// ---------------------------------------------------------------------------
// Device caps — hardcoded values
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_GetDeviceCaps(HDC hdc, int nIndex) {
    (void)hdc;
    switch (nIndex) {
        case DRIVERVERSION:    return 0;
        case TECHNOLOGY:       return 2;  // DT_RASDISPLAY
        case HORZRES:          return 1920;
        case VERTRES:          return 1080;
        case BITSPIXEL:        return 32;
        case PLANES:           return 1;
        case NUMBRUSHES:       return -1;
        case NUMPENS:          return -1;
        case NUMFONTS:         return 0;
        case ASPECTX:          return 36;
        case ASPECTY:          return 36;
        case ASPECTXY:         return 51;
        case LOGPIXELSX:       return 96;
        case LOGPIXELSY:       return 96;
        case CLIPCAPS:         return 1;
        case SIZEPALETTE:      return 0;
        case RASTERCAPS:       return 0x6A99;
        case TEXTCAPS:         return 0;
        case COLORMGMTCAPS:    return 1;
        case PHYSICALWIDTH:    return 1920;
        case PHYSICALHEIGHT:   return 1080;
        case PHYSICALOFFSETX:  return 0;
        case PHYSICALOFFSETY:  return 0;
        case VREFRESH:         return 60;
        case DESKTOPHORZRES:   return 1920;
        case DESKTOPVERTRES:   return 1080;
        case BLTALIGNMENT:     return 1;
        case SHADEBLENDCAPS:   return 0;
        default:               return 0;
    }
}

// ---------------------------------------------------------------------------
// Color / mode setters
// ---------------------------------------------------------------------------
extern "C" COLORREF __stdcall Shim_SetBkColor(HDC hdc, COLORREF c) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    COLORREF old = it->second.bkColor;
    it->second.bkColor = c;
    return old;
}
extern "C" COLORREF __stdcall Shim_SetTextColor(HDC hdc, COLORREF c) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    COLORREF old = it->second.textColor;
    it->second.textColor = c;
    return old;
}
extern "C" int __stdcall Shim_SetBkMode(HDC hdc, int mode) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    int old = it->second.bkMode;
    it->second.bkMode = mode;
    return old;
}
extern "C" int __stdcall Shim_SetROP2(HDC hdc, int r) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    int old = it->second.rop2;
    it->second.rop2 = r;
    return old;
}
extern "C" int __stdcall Shim_SetPolyFillMode(HDC hdc, int m) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    int old = it->second.polyFillMode;
    it->second.polyFillMode = m;
    return old;
}
extern "C" int __stdcall Shim_SetStretchBltMode(HDC hdc, int m) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    int old = it->second.stretchBltMode;
    it->second.stretchBltMode = m;
    return old;
}
extern "C" int __stdcall Shim_SetMapMode(HDC hdc, int m) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    int old = it->second.mapMode;
    it->second.mapMode = m;
    return old;
}
extern "C" BOOL __stdcall Shim_SetWindowExtEx(HDC, int, int, LPSIZE s) {
    if (s) { s->cx = 1920; s->cy = 1080; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_SetViewportExtEx(HDC, int, int, LPSIZE s) {
    if (s) { s->cx = 1920; s->cy = 1080; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_SetWindowOrgEx(HDC, int, int, LPPOINT p) {
    if (p) { p->x = 0; p->y = 0; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_SetViewportOrgEx(HDC, int, int, LPPOINT p) {
    if (p) { p->x = 0; p->y = 0; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_OffsetWindowOrgEx(HDC, int, int, LPPOINT p) {
    if (p) { p->x = 0; p->y = 0; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_OffsetViewportOrgEx(HDC, int, int, LPPOINT p) {
    if (p) { p->x = 0; p->y = 0; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_ScaleWindowExtEx(HDC, int, int, int, int, LPSIZE s) {
    if (s) { s->cx = 1920; s->cy = 1080; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_ScaleViewportExtEx(HDC, int, int, int, int, LPSIZE s) {
    if (s) { s->cx = 1920; s->cy = 1080; }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_DPtoLP(HDC, LPPOINT lpPoints, int nCount) {
    if (lpPoints) { for (int i = 0; i < nCount; ++i) { /* identity */ } }
    return TRUE;
}
extern "C" BOOL __stdcall Shim_LPtoDP(HDC, LPPOINT lpPoints, int nCount) {
    if (lpPoints) { for (int i = 0; i < nCount; ++i) { /* identity */ } }
    return TRUE;
}
extern "C" COLORREF __stdcall Shim_GetBkColor(HDC hdc) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    return it == GdiTable().end() ? 0 : it->second.bkColor;
}
extern "C" COLORREF __stdcall Shim_GetTextColor(HDC hdc) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    return it == GdiTable().end() ? 0 : it->second.textColor;
}
extern "C" int __stdcall Shim_GetBkMode(HDC hdc) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    return it == GdiTable().end() ? 0 : it->second.bkMode;
}
extern "C" int __stdcall Shim_GetROP2(HDC hdc) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    return it == GdiTable().end() ? 0 : it->second.rop2;
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_TextOutW(HDC, int, int, LPCWSTR, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_GetTextExtentPoint32W(HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize) {
    if (!lpSize) return FALSE;
    int n = cbString < 0 ? 0 : cbString;
    if (lpString && cbString < 0) {
        n = 0; while (lpString[n]) ++n;
    }
    // Approximate: 8px wide per char, 16px tall.
    lpSize->cx = n * 8;
    lpSize->cy = 16;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_GetTextMetricsW(HDC hdc, LPTEXTMETRICW lptm) {
    if (!lptm) return FALSE;
    std::memset(lptm, 0, sizeof(TEXTMETRICW));
    lptm->tmHeight = 16;
    lptm->tmAscent = 12;
    lptm->tmDescent = 4;
    lptm->tmAveCharWidth = 8;
    lptm->tmMaxCharWidth = 16;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_GetTextExtentExPointW(HDC, LPCWSTR, int, int, LPINT lpnFit, LPINT alpDx, LPSIZE lpSize) {
    if (lpnFit) *lpnFit = 0;
    if (lpSize) { lpSize->cx = 0; lpSize->cy = 16; }
    return TRUE;
}
extern "C" DWORD __stdcall Shim_GetGlyphOutlineW(HDC, UINT, UINT, LPGLYPHMETRICS, DWORD, LPVOID, const MAT2*) { return GDI_ERROR; }
extern "C" UINT __stdcall Shim_GetTextAlign(HDC hdc) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    return it == GdiTable().end() ? 0 : it->second.textAlign;
}
extern "C" UINT __stdcall Shim_SetTextAlign(HDC hdc, UINT align) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    UINT old = it->second.textAlign;
    it->second.textAlign = align;
    return old;
}
extern "C" int __stdcall Shim_SetTextCharacterExtra(HDC hdc, int extra) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    return extra;
}
extern "C" int __stdcall Shim_GetTextCharacterExtra(HDC) { return 0; }
extern "C" BOOL __stdcall Shim_GetCharWidth32W(HDC, UINT, UINT, LPINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_GetCharWidthW(HDC, UINT, UINT, LPINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_GetCharABCWidthsW(HDC, UINT, UINT, LPABC) { return TRUE; }
extern "C" DWORD __stdcall Shim_GetFontData(HDC, DWORD, DWORD, LPVOID, DWORD) { return GDI_ERROR; }
extern "C" int __stdcall Shim_EnumFontFamiliesExW(HDC, LPLOGFONTW, FONTENUMPROCW, LPARAM, DWORD) { return 0; }

// ---------------------------------------------------------------------------
// Palette
// ---------------------------------------------------------------------------
extern "C" HPALETTE __stdcall Shim_CreatePalette(const LOGPALETTE*) {
    return (HPALETTE)AllocObj(OBJ_PAL);
}
extern "C" HPALETTE __stdcall Shim_SelectPalette(HDC hdc, HPALETTE hpal, BOOL) {
    std::lock_guard<std::mutex> lk(GdiMutex());
    auto it = GdiTable().find((HGDIOBJ)hdc);
    if (it == GdiTable().end()) return 0;
    HPALETTE old = it->second.curPalette;
    it->second.curPalette = hpal;
    return old ? old : (HPALETTE)1;
}
extern "C" UINT __stdcall Shim_RealizePalette(HDC) { return 0; }
extern "C" UINT __stdcall Shim_SetPaletteEntries(HPALETTE, UINT, UINT, const PALETTEENTRY*) { return 0; }
extern "C" UINT __stdcall Shim_GetPaletteEntries(HPALETTE, UINT, UINT, LPPALETTEENTRY) { return 0; }
extern "C" BOOL __stdcall Shim_ResizePalette(HPALETTE, UINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_AnimatePalette(HPALETTE, UINT, UINT, const PALETTEENTRY*) { return TRUE; }

// ---------------------------------------------------------------------------
// Misc
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_GdiFlush() { return TRUE; }
extern "C" DWORD __stdcall Shim_GdiGetBatchLimit() { return 1; }
extern "C" DWORD __stdcall Shim_GdiSetBatchLimit(DWORD) { return 1; }

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("gdi32", "CreateCompatibleDC", (FARPROC)&xwr::Shim_CreateCompatibleDC);
REGISTER_SHIM("gdi32", "DeleteDC", (FARPROC)&xwr::Shim_DeleteDC);
REGISTER_SHIM("gdi32", "SelectObject", (FARPROC)&xwr::Shim_SelectObject);
REGISTER_SHIM("gdi32", "DeleteObject", (FARPROC)&xwr::Shim_DeleteObject);
REGISTER_SHIM("gdi32", "GetStockObject", (FARPROC)&xwr::Shim_GetStockObject);
REGISTER_SHIM("gdi32", "GetCurrentObject", (FARPROC)&xwr::Shim_GetCurrentObject);
REGISTER_SHIM("gdi32", "CreateCompatibleBitmap", (FARPROC)&xwr::Shim_CreateCompatibleBitmap);
REGISTER_SHIM("gdi32", "CreateBitmap", (FARPROC)&xwr::Shim_CreateBitmap);
REGISTER_SHIM("gdi32", "CreateDIBSection", (FARPROC)&xwr::Shim_CreateDIBSection);
REGISTER_SHIM("gdi32", "GetObjectW", (FARPROC)&xwr::Shim_GetObjectW);
REGISTER_SHIM("gdi32", "GetObjectA", (FARPROC)&xwr::Shim_GetObjectW);
REGISTER_SHIM("gdi32", "GetDIBColorTable", (FARPROC)&xwr::Shim_GetDIBColorTable);
REGISTER_SHIM("gdi32", "SetDIBColorTable", (FARPROC)&xwr::Shim_SetDIBColorTable);
REGISTER_SHIM("gdi32", "CreateSolidBrush", (FARPROC)&xwr::Shim_CreateSolidBrush);
REGISTER_SHIM("gdi32", "CreateBrushIndirect", (FARPROC)&xwr::Shim_CreateBrushIndirect);
REGISTER_SHIM("gdi32", "CreateHatchBrush", (FARPROC)&xwr::Shim_CreateHatchBrush);
REGISTER_SHIM("gdi32", "CreatePatternBrush", (FARPROC)&xwr::Shim_CreatePatternBrush);
REGISTER_SHIM("gdi32", "CreatePen", (FARPROC)&xwr::Shim_CreatePen);
REGISTER_SHIM("gdi32", "ExtCreatePen", (FARPROC)&xwr::Shim_ExtCreatePen);
REGISTER_SHIM("gdi32", "CreateFontIndirectW", (FARPROC)&xwr::Shim_CreateFontIndirectW);
REGISTER_SHIM("gdi32", "CreateFontIndirectA", (FARPROC)&xwr::Shim_CreateFontIndirectW);
REGISTER_SHIM("gdi32", "CreateFontW", (FARPROC)&xwr::Shim_CreateFontW);
REGISTER_SHIM("gdi32", "CreateFontA", (FARPROC)&xwr::Shim_CreateFontW);
REGISTER_SHIM("gdi32", "BitBlt", (FARPROC)&xwr::Shim_BitBlt);
REGISTER_SHIM("gdi32", "StretchBlt", (FARPROC)&xwr::Shim_StretchBlt);
REGISTER_SHIM("gdi32", "AlphaBlend", (FARPROC)&xwr::Shim_AlphaBlend);
REGISTER_SHIM("gdi32", "TransparentBlt", (FARPROC)&xwr::Shim_TransparentBlt);
REGISTER_SHIM("gdi32", "MaskBlt", (FARPROC)&xwr::Shim_MaskBlt);
REGISTER_SHIM("gdi32", "PlgBlt", (FARPROC)&xwr::Shim_PlgBlt);
REGISTER_SHIM("gdi32", "SetDIBitsToDevice", (FARPROC)&xwr::Shim_SetDIBitsToDevice);
REGISTER_SHIM("gdi32", "StretchDIBits", (FARPROC)&xwr::Shim_StretchDIBits);
REGISTER_SHIM("gdi32", "GetDIBits", (FARPROC)&xwr::Shim_GetDIBits);
REGISTER_SHIM("gdi32", "SetPixel", (FARPROC)&xwr::Shim_SetPixel);
REGISTER_SHIM("gdi32", "GetPixel", (FARPROC)&xwr::Shim_GetPixel);
REGISTER_SHIM("gdi32", "MoveToEx", (FARPROC)&xwr::Shim_MoveToEx);
REGISTER_SHIM("gdi32", "LineTo", (FARPROC)&xwr::Shim_LineTo);
REGISTER_SHIM("gdi32", "Rectangle", (FARPROC)&xwr::Shim_Rectangle);
REGISTER_SHIM("gdi32", "RoundRect", (FARPROC)&xwr::Shim_RoundRect);
REGISTER_SHIM("gdi32", "Ellipse", (FARPROC)&xwr::Shim_Ellipse);
REGISTER_SHIM("gdi32", "Arc", (FARPROC)&xwr::Shim_Arc);
REGISTER_SHIM("gdi32", "Pie", (FARPROC)&xwr::Shim_Pie);
REGISTER_SHIM("gdi32", "Chord", (FARPROC)&xwr::Shim_Chord);
REGISTER_SHIM("gdi32", "Polyline", (FARPROC)&xwr::Shim_Polyline);
REGISTER_SHIM("gdi32", "PolylineTo", (FARPROC)&xwr::Shim_PolylineTo);
REGISTER_SHIM("gdi32", "Polygon", (FARPROC)&xwr::Shim_Polygon);
REGISTER_SHIM("gdi32", "PolyBezier", (FARPROC)&xwr::Shim_PolyBezier);
REGISTER_SHIM("gdi32", "PolyBezierTo", (FARPROC)&xwr::Shim_PolyBezierTo);
REGISTER_SHIM("gdi32", "InvertRect", (FARPROC)&xwr::Shim_InvertRect);
REGISTER_SHIM("gdi32", "DrawFocusRect", (FARPROC)&xwr::Shim_DrawFocusRect);
REGISTER_SHIM("gdi32", "FillRgn", (FARPROC)&xwr::Shim_FillRgn);
REGISTER_SHIM("gdi32", "FrameRgn", (FARPROC)&xwr::Shim_FrameRgn);
REGISTER_SHIM("gdi32", "PaintRgn", (FARPROC)&xwr::Shim_PaintRgn);
REGISTER_SHIM("gdi32", "CreateRectRgn", (FARPROC)&xwr::Shim_CreateRectRgn);
REGISTER_SHIM("gdi32", "CreateRectRgnIndirect", (FARPROC)&xwr::Shim_CreateRectRgnIndirect);
REGISTER_SHIM("gdi32", "CreateEllipticRgn", (FARPROC)&xwr::Shim_CreateEllipticRgn);
REGISTER_SHIM("gdi32", "CreateEllipticRgnIndirect", (FARPROC)&xwr::Shim_CreateEllipticRgnIndirect);
REGISTER_SHIM("gdi32", "CreatePolygonRgn", (FARPROC)&xwr::Shim_CreatePolygonRgn);
REGISTER_SHIM("gdi32", "CreatePolyPolygonRgn", (FARPROC)&xwr::Shim_CreatePolyPolygonRgn);
REGISTER_SHIM("gdi32", "CreateRoundRectRgn", (FARPROC)&xwr::Shim_CreateRoundRectRgn);
REGISTER_SHIM("gdi32", "ExtCreateRegion", (FARPROC)&xwr::Shim_ExtCreateRegion);
REGISTER_SHIM("gdi32", "CombineRgn", (FARPROC)&xwr::Shim_CombineRgn);
REGISTER_SHIM("gdi32", "EqualRgn", (FARPROC)&xwr::Shim_EqualRgn);
REGISTER_SHIM("gdi32", "OffsetRgn", (FARPROC)&xwr::Shim_OffsetRgn);
REGISTER_SHIM("gdi32", "PtInRegion", (FARPROC)&xwr::Shim_PtInRegion);
REGISTER_SHIM("gdi32", "RectInRegion", (FARPROC)&xwr::Shim_RectInRegion);
REGISTER_SHIM("gdi32", "GetRgnBox", (FARPROC)&xwr::Shim_GetRgnBox);
REGISTER_SHIM("gdi32", "SelectClipRgn", (FARPROC)&xwr::Shim_SelectClipRgn);
REGISTER_SHIM("gdi32", "ExtSelectClipRgn", (FARPROC)&xwr::Shim_ExtSelectClipRgn);
REGISTER_SHIM("gdi32", "ExcludeClipRect", (FARPROC)&xwr::Shim_ExcludeClipRect);
REGISTER_SHIM("gdi32", "IntersectClipRect", (FARPROC)&xwr::Shim_IntersectClipRect);
REGISTER_SHIM("gdi32", "OffsetClipRgn", (FARPROC)&xwr::Shim_OffsetClipRgn);
REGISTER_SHIM("gdi32", "GetClipBox", (FARPROC)&xwr::Shim_GetClipBox);
REGISTER_SHIM("gdi32", "PtVisible", (FARPROC)&xwr::Shim_PtVisible);
REGISTER_SHIM("gdi32", "RectVisible", (FARPROC)&xwr::Shim_RectVisible);
REGISTER_SHIM("gdi32", "SaveDC", (FARPROC)&xwr::Shim_SaveDC);
REGISTER_SHIM("gdi32", "RestoreDC", (FARPROC)&xwr::Shim_RestoreDC);
REGISTER_SHIM("gdi32", "GetDeviceCaps", (FARPROC)&xwr::Shim_GetDeviceCaps);
REGISTER_SHIM("gdi32", "SetBkColor", (FARPROC)&xwr::Shim_SetBkColor);
REGISTER_SHIM("gdi32", "SetTextColor", (FARPROC)&xwr::Shim_SetTextColor);
REGISTER_SHIM("gdi32", "SetBkMode", (FARPROC)&xwr::Shim_SetBkMode);
REGISTER_SHIM("gdi32", "SetROP2", (FARPROC)&xwr::Shim_SetROP2);
REGISTER_SHIM("gdi32", "SetPolyFillMode", (FARPROC)&xwr::Shim_SetPolyFillMode);
REGISTER_SHIM("gdi32", "SetStretchBltMode", (FARPROC)&xwr::Shim_SetStretchBltMode);
REGISTER_SHIM("gdi32", "SetMapMode", (FARPROC)&xwr::Shim_SetMapMode);
REGISTER_SHIM("gdi32", "SetWindowExtEx", (FARPROC)&xwr::Shim_SetWindowExtEx);
REGISTER_SHIM("gdi32", "SetViewportExtEx", (FARPROC)&xwr::Shim_SetViewportExtEx);
REGISTER_SHIM("gdi32", "SetWindowOrgEx", (FARPROC)&xwr::Shim_SetWindowOrgEx);
REGISTER_SHIM("gdi32", "SetViewportOrgEx", (FARPROC)&xwr::Shim_SetViewportOrgEx);
REGISTER_SHIM("gdi32", "OffsetWindowOrgEx", (FARPROC)&xwr::Shim_OffsetWindowOrgEx);
REGISTER_SHIM("gdi32", "OffsetViewportOrgEx", (FARPROC)&xwr::Shim_OffsetViewportOrgEx);
REGISTER_SHIM("gdi32", "ScaleWindowExtEx", (FARPROC)&xwr::Shim_ScaleWindowExtEx);
REGISTER_SHIM("gdi32", "ScaleViewportExtEx", (FARPROC)&xwr::Shim_ScaleViewportExtEx);
REGISTER_SHIM("gdi32", "DPtoLP", (FARPROC)&xwr::Shim_DPtoLP);
REGISTER_SHIM("gdi32", "LPtoDP", (FARPROC)&xwr::Shim_LPtoDP);
REGISTER_SHIM("gdi32", "GetBkColor", (FARPROC)&xwr::Shim_GetBkColor);
REGISTER_SHIM("gdi32", "GetTextColor", (FARPROC)&xwr::Shim_GetTextColor);
REGISTER_SHIM("gdi32", "GetBkMode", (FARPROC)&xwr::Shim_GetBkMode);
REGISTER_SHIM("gdi32", "GetROP2", (FARPROC)&xwr::Shim_GetROP2);
REGISTER_SHIM("gdi32", "TextOutW", (FARPROC)&xwr::Shim_TextOutW);
REGISTER_SHIM("gdi32", "TextOutA", (FARPROC)&xwr::Shim_TextOutW);
REGISTER_SHIM("gdi32", "GetTextExtentPoint32W", (FARPROC)&xwr::Shim_GetTextExtentPoint32W);
REGISTER_SHIM("gdi32", "GetTextExtentPoint32A", (FARPROC)&xwr::Shim_GetTextExtentPoint32W);
REGISTER_SHIM("gdi32", "GetTextMetricsW", (FARPROC)&xwr::Shim_GetTextMetricsW);
REGISTER_SHIM("gdi32", "GetTextExtentExPointW", (FARPROC)&xwr::Shim_GetTextExtentExPointW);
REGISTER_SHIM("gdi32", "GetGlyphOutlineW", (FARPROC)&xwr::Shim_GetGlyphOutlineW);
REGISTER_SHIM("gdi32", "GetTextAlign", (FARPROC)&xwr::Shim_GetTextAlign);
REGISTER_SHIM("gdi32", "SetTextAlign", (FARPROC)&xwr::Shim_SetTextAlign);
REGISTER_SHIM("gdi32", "SetTextCharacterExtra", (FARPROC)&xwr::Shim_SetTextCharacterExtra);
REGISTER_SHIM("gdi32", "GetTextCharacterExtra", (FARPROC)&xwr::Shim_GetTextCharacterExtra);
REGISTER_SHIM("gdi32", "GetCharWidth32W", (FARPROC)&xwr::Shim_GetCharWidth32W);
REGISTER_SHIM("gdi32", "GetCharWidthW", (FARPROC)&xwr::Shim_GetCharWidthW);
REGISTER_SHIM("gdi32", "GetCharABCWidthsW", (FARPROC)&xwr::Shim_GetCharABCWidthsW);
REGISTER_SHIM("gdi32", "GetFontData", (FARPROC)&xwr::Shim_GetFontData);
REGISTER_SHIM("gdi32", "EnumFontFamiliesExW", (FARPROC)&xwr::Shim_EnumFontFamiliesExW);
REGISTER_SHIM("gdi32", "CreatePalette", (FARPROC)&xwr::Shim_CreatePalette);
REGISTER_SHIM("gdi32", "SelectPalette", (FARPROC)&xwr::Shim_SelectPalette);
REGISTER_SHIM("gdi32", "RealizePalette", (FARPROC)&xwr::Shim_RealizePalette);
REGISTER_SHIM("gdi32", "SetPaletteEntries", (FARPROC)&xwr::Shim_SetPaletteEntries);
REGISTER_SHIM("gdi32", "GetPaletteEntries", (FARPROC)&xwr::Shim_GetPaletteEntries);
REGISTER_SHIM("gdi32", "ResizePalette", (FARPROC)&xwr::Shim_ResizePalette);
REGISTER_SHIM("gdi32", "AnimatePalette", (FARPROC)&xwr::Shim_AnimatePalette);
REGISTER_SHIM("gdi32", "GdiFlush", (FARPROC)&xwr::Shim_GdiFlush);
REGISTER_SHIM("gdi32", "GdiGetBatchLimit", (FARPROC)&xwr::Shim_GdiGetBatchLimit);
REGISTER_SHIM("gdi32", "GdiSetBatchLimit", (FARPROC)&xwr::Shim_GdiSetBatchLimit);
