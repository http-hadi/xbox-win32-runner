// shims/user32/User32Shim.cpp
// Win32 user32 shim layer. Provides an in-process virtual HWND table and a
// per-thread message queue so simple message-pump loops compile and run.
//
// All rendering / input is no-op or hardcoded; the real input + swap-chain
// surface is driven by the D3D11 bridge and input bridge.

#include "UwpSdkIncludes.h"


#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <queue>
#include <cstring>

#include "../ShimRegistry.h"

// Local definitions of constants that the UWP SDK's user32 omits.
#ifndef SM_CXSCREEN
#define SM_CXSCREEN 0
#endif
#ifndef SM_CYSCREEN
#define SM_CYSCREEN 1
#endif
#ifndef SM_CXVSCROLL
#define SM_CXVSCROLL 2
#endif
#ifndef SM_CYHSCROLL
#define SM_CYHSCROLL 3
#endif
#ifndef SM_CYCAPTION
#define SM_CYCAPTION 4
#endif
#ifndef SM_CXBORDER
#define SM_CXBORDER 5
#endif
#ifndef SM_CYBORDER
#define SM_CYBORDER 6
#endif
#ifndef SM_CXDLGFRAME
#define SM_CXDLGFRAME 7
#endif
#ifndef SM_CYDLGFRAME
#define SM_CYDLGFRAME 8
#endif
#ifndef SM_CYVTHUMB
#define SM_CYVTHUMB 9
#endif
#ifndef SM_CXHTHUMB
#define SM_CXHTHUMB 10
#endif
#ifndef SM_CXICON
#define SM_CXICON 11
#endif
#ifndef SM_CYICON
#define SM_CYICON 12
#endif
#ifndef SM_CXCURSOR
#define SM_CXCURSOR 13
#endif
#ifndef SM_CYCURSOR
#define SM_CYCURSOR 14
#endif
#ifndef SM_CYMENU
#define SM_CYMENU 15
#endif
#ifndef SM_CXFULLSCREEN
#define SM_CXFULLSCREEN 16
#endif
#ifndef SM_CYFULLSCREEN
#define SM_CYFULLSCREEN 17
#endif
#ifndef SM_CYKANJIWINDOW
#define SM_CYKANJIWINDOW 18
#endif
#ifndef SM_MOUSEPRESENT
#define SM_MOUSEPRESENT 19
#endif
#ifndef SM_CYVSCROLL
#define SM_CYVSCROLL 20
#endif
#ifndef SM_CXHSCROLL
#define SM_CXHSCROLL 21
#endif
#ifndef SM_DEBUG
#define SM_DEBUG 22
#endif
#ifndef SM_SWAPBUTTON
#define SM_SWAPBUTTON 23
#endif
#ifndef SM_CXMIN
#define SM_CXMIN 28
#endif
#ifndef SM_CYMIN
#define SM_CYMIN 29
#endif
#ifndef SM_CXSIZE
#define SM_CXSIZE 30
#endif
#ifndef SM_CYSIZE
#define SM_CYSIZE 31
#endif
#ifndef SM_CXFRAME
#define SM_CXFRAME 32
#endif
#ifndef SM_CYFRAME
#define SM_CYFRAME 33
#endif
#ifndef SM_CXMINTRACK
#define SM_CXMINTRACK 34
#endif
#ifndef SM_CYMINTRACK
#define SM_CYMINTRACK 35
#endif
#ifndef SM_CXDOUBLECLK
#define SM_CXDOUBLECLK 36
#endif
#ifndef SM_CYDOUBLECLK
#define SM_CYDOUBLECLK 37
#endif
#ifndef SM_CXICONSPACING
#define SM_CXICONSPACING 38
#endif
#ifndef SM_CYICONSPACING
#define SM_CYICONSPACING 39
#endif
#ifndef SM_MENUDROPALIGNMENT
#define SM_MENUDROPALIGNMENT 40
#endif
#ifndef SM_SHOWSOUNDS
#define SM_SHOWSOUNDS 41
#endif
#ifndef SM_CMONITORS
#define SM_CMONITORS 80
#endif
#ifndef SM_CXDIGITIZER
#define SM_CXDIGITIZER 94
#endif
#ifndef SM_CYDIGITIZER
#define SM_CYDIGITIZER 95
#endif
#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif
#ifndef SM_DIGITIZER
#define SM_DIGITIZER 94
#endif
#ifndef SM_MAXIMUMTOUCHES
#define SM_MAXIMUMTOUCHES 95
#endif

namespace xwr {

// ---------------------------------------------------------------------------
// Virtual HWND table
// ---------------------------------------------------------------------------
// Ensure WNDPROC is defined (it should be from <winuser.h> via UwpSdkIncludes.h,
// but some SDK configs may not have it at this point).
#ifndef WNDPROC
typedef LRESULT (CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
#endif
struct WindowState {
    WNDPROC      wndProc = nullptr;
    HINSTANCE    hInstance = 0;
    HWND         parent = 0;
    std::wstring className;
    std::wstring title;
    DWORD        style = 0;
    DWORD        exStyle = 0;
    int          x = 0, y = 0, w = 0, h = 0;
    LONG_PTR     userData = 0;
    LONG_PTR     wndLong[GWL_WNDPROC + 32] = {0};  // simple per-window storage
};

static std::atomic<uint32_t> g_nextHwnd{0x10000};
static std::atomic<uint32_t> g_nextAtom{0xC000};
static std::atomic<uint32_t> g_nextTimer{0x20000};

static std::mutex& HwndMutex() {
    static std::mutex m;
    return m;
}
static std::unordered_map<HWND, WindowState>& HwndTable() {
    static std::unordered_map<HWND, WindowState> t;
    return t;
}

static WindowState* LookupHwnd(HWND h) {
    if (!h) return nullptr;
    std::lock_guard<std::mutex> lk(HwndMutex());
    auto it = HwndTable().find(h);
    return it == HwndTable().end() ? nullptr : &it->second;
}

// ---------------------------------------------------------------------------
// Per-thread message queue
// ---------------------------------------------------------------------------
struct ThreadMsgQueue {
    std::queue<MSG> q;
};
static thread_local ThreadMsgQueue tls_queue;

// ---------------------------------------------------------------------------
// Window creation / destruction
// ---------------------------------------------------------------------------
extern "C" HWND __stdcall Shim_CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName,
                                                LPCWSTR lpWindowName, DWORD dwStyle,
                                                int X, int Y, int nWidth, int nHeight,
                                                HWND hWndParent, HMENU hMenu,
                                                HINSTANCE hInstance, LPVOID lpParam) {
    HWND h = (HWND)g_nextHwnd.fetch_add(1);
    WindowState ws;
    ws.exStyle = dwExStyle;
    ws.style = dwStyle;
    ws.parent = hWndParent;
    ws.hInstance = hInstance;
    ws.x = X; ws.y = Y; ws.w = nWidth; ws.h = nHeight;
    if (lpClassName) ws.className = lpClassName;
    if (lpWindowName) ws.title = lpWindowName;
    {
        std::lock_guard<std::mutex> lk(HwndMutex());
        HwndTable()[h] = ws;
    }
    // Send WM_CREATE — game might bail out if it returns -1.
    MSG create{};
    create.hwnd = h;
    create.message = WM_CREATE;
    create.wParam = 0;
    create.lParam = reinterpret_cast<LPARAM>(lpParam);
    // Skip dispatch — many games don't expect a WndProc at all.
    return h;
}

extern "C" HWND __stdcall Shim_CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName,
                                                LPCSTR lpWindowName, DWORD dwStyle,
                                                int X, int Y, int nWidth, int nHeight,
                                                HWND hWndParent, HMENU hMenu,
                                                HINSTANCE hInstance, LPVOID lpParam) {
    std::wstring cls, title;
    if (lpClassName) for (const char* p = lpClassName; *p; ++p) cls.push_back((wchar_t)*p);
    if (lpWindowName) for (const char* p = lpWindowName; *p; ++p) title.push_back((wchar_t)*p);
    return Shim_CreateWindowExW(dwExStyle, cls.c_str(), title.c_str(), dwStyle,
                                 X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

extern "C" BOOL __stdcall Shim_DestroyWindow(HWND hWnd) {
    std::lock_guard<std::mutex> lk(HwndMutex());
    return HwndTable().erase(hWnd) ? TRUE : FALSE;
}

extern "C" BOOL __stdcall Shim_ShowWindow(HWND, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_UpdateWindow(HWND) { return TRUE; }
extern "C" BOOL __stdcall Shim_InvalidateRect(HWND, const RECT*, BOOL) { return TRUE; }
extern "C" BOOL __stdcall Shim_ValidateRect(HWND, const RECT*) { return TRUE; }
extern "C" BOOL __stdcall Shim_RedrawWindow(HWND, const RECT*, HRGN, UINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_MoveWindow(HWND, int, int, int, int, BOOL) { return TRUE; }
extern "C" BOOL __stdcall Shim_BringWindowToTop(HWND) { return TRUE; }
extern "C" BOOL __stdcall Shim_SetForegroundWindow(HWND) { return TRUE; }
extern "C" HWND __stdcall Shim_GetForegroundWindow() { return 0; }
extern "C" HWND __stdcall Shim_GetFocus() { return 0; }
extern "C" BOOL __stdcall Shim_SetFocus(HWND) { return TRUE; }
extern "C" BOOL __stdcall Shim_ShowCursor(BOOL) { return TRUE; }
extern "C" BOOL __stdcall Shim_SetCursor(HCURSOR) { return TRUE; }
extern "C" HCURSOR __stdcall Shim_LoadCursorW(HINSTANCE, LPCWSTR) { return (HCURSOR)0x100; }
extern "C" HICON __stdcall Shim_LoadIconW(HINSTANCE, LPCWSTR) { return (HICON)0x101; }
extern "C" HICON __stdcall Shim_LoadImageW(HINSTANCE, LPCWSTR, UINT, int, int, UINT) { return (HICON)0x102; }

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_GetClientRect(HWND hWnd, LPRECT lpRect) {
    if (!lpRect) return FALSE;
    lpRect->left = 0; lpRect->top = 0;
    lpRect->right = 1920; lpRect->bottom = 1080;
    (void)hWnd;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_GetWindowRect(HWND hWnd, LPRECT lpRect) {
    if (!lpRect) return FALSE;
    lpRect->left = 0; lpRect->top = 0;
    lpRect->right = 1920; lpRect->bottom = 1080;
    (void)hWnd;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_AdjustWindowRect(LPRECT lpRect, DWORD, BOOL) {
    return lpRect ? TRUE : FALSE;
}
extern "C" BOOL __stdcall Shim_AdjustWindowRectEx(LPRECT lpRect, DWORD, BOOL, DWORD) {
    return lpRect ? TRUE : FALSE;
}
extern "C" BOOL __stdcall Shim_ClientToScreen(HWND, LPPOINT p) {
    if (!p) return FALSE; p->x += 0; p->y += 0; return TRUE;
}
extern "C" BOOL __stdcall Shim_ScreenToClient(HWND, LPPOINT p) {
    if (!p) return FALSE; p->x -= 0; p->y -= 0; return TRUE;
}

// ---------------------------------------------------------------------------
// Device context
// ---------------------------------------------------------------------------
extern "C" HDC __stdcall Shim_GetDC(HWND) { return (HDC)1; }
extern "C" int __stdcall Shim_ReleaseDC(HWND, HDC) { return 1; }
extern "C" HDC __stdcall Shim_BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint) {
    if (lpPaint) {
        std::memset(lpPaint, 0, sizeof(PAINTSTRUCT));
        lpPaint->hdc = (HDC)1;
        lpPaint->rcPaint.left = 0; lpPaint->rcPaint.top = 0;
        lpPaint->rcPaint.right = 1920; lpPaint->rcPaint.bottom = 1080;
    }
    (void)hWnd;
    return (HDC)1;
}
extern "C" BOOL __stdcall Shim_EndPaint(HWND, const PAINTSTRUCT*) { return TRUE; }

// ---------------------------------------------------------------------------
// Message queue
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin,
                                             UINT wMsgFilterMax, UINT wRemoveMsg) {
    (void)hWnd; (void)wMsgFilterMin; (void)wMsgFilterMax; (void)wRemoveMsg;
    if (!lpMsg) return FALSE;
    if (tls_queue.q.empty()) {
        // Synthesize a WM_PAINT to keep loops alive.
        lpMsg->hwnd = hWnd;
        lpMsg->message = WM_PAINT;
        lpMsg->wParam = 0;
        lpMsg->lParam = 0;
        lpMsg->time = 0;
        lpMsg->pt.x = 0; lpMsg->pt.y = 0;
        return FALSE;
    }
    *lpMsg = tls_queue.q.front();
    if (wRemoveMsg & 0x0001) tls_queue.q.pop();  // PM_REMOVE
    return TRUE;
}
extern "C" BOOL __stdcall Shim_GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    (void)hWnd; (void)wMsgFilterMin; (void)wMsgFilterMax;
    if (!lpMsg) return FALSE;
    if (tls_queue.q.empty()) {
        std::memset(lpMsg, 0, sizeof(MSG));
        return TRUE;  // pretend we got WM_NULL
    }
    *lpMsg = tls_queue.q.front();
    tls_queue.q.pop();
    return TRUE;
}
extern "C" BOOL __stdcall Shim_PostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    MSG m{};
    m.hwnd = hWnd;
    m.message = Msg;
    m.wParam = wParam;
    m.lParam = lParam;
    tls_queue.q.push(m);
    return TRUE;
}
extern "C" LRESULT __stdcall Shim_SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    (void)hWnd; (void)Msg; (void)wParam; (void)lParam;
    return 0;
}
extern "C" BOOL __stdcall Shim_TranslateMessage(const MSG*) { return TRUE; }
extern "C" LRESULT __stdcall Shim_DispatchMessageW(const MSG* lpMsg) {
    if (!lpMsg) return 0;
    if (lpMsg->message == WM_QUIT) return 0;
    return 0;
}
extern "C" LRESULT __stdcall Shim_DefWindowProcW(HWND, UINT, WPARAM, LPARAM) { return 0; }
extern "C" void __stdcall Shim_PostQuitMessage(int nExitCode) {
    MSG m{};
    m.message = WM_QUIT;
    m.wParam = (WPARAM)nExitCode;
    tls_queue.q.push(m);
}
extern "C" BOOL __stdcall Shim_WaitMessage() { return TRUE; }
extern "C" BOOL __stdcall Shim_GetInputState() { return FALSE; }
extern "C" DWORD __stdcall Shim_GetQueueStatus(UINT) { return 0; }
extern "C" BOOL __stdcall Shim_TranslateAcceleratorW(HWND, HACCEL, LPMSG) { return FALSE; }

// ---------------------------------------------------------------------------
// Window class registration
// ---------------------------------------------------------------------------
extern "C" ATOM __stdcall Shim_RegisterClassW(const WNDCLASSW* lpwc) {
    (void)lpwc;
    return (ATOM)g_nextAtom.fetch_add(1);
}
extern "C" ATOM __stdcall Shim_RegisterClassExW(const WNDCLASSEXW* lpwcx) {
    (void)lpwcx;
    return (ATOM)g_nextAtom.fetch_add(1);
}
extern "C" BOOL __stdcall Shim_UnregisterClassW(LPCWSTR, HINSTANCE) { return TRUE; }

// ---------------------------------------------------------------------------
// System metrics
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_GetSystemMetrics(int nIndex) {
    switch (nIndex) {
        case SM_CXSCREEN: return 1920;
        case SM_CYSCREEN: return 1080;
        case SM_CXFULLSCREEN: return 1920;
        case SM_CYFULLSCREEN: return 1040;
        case SM_CYCAPTION: return 23;
        case SM_CYMENU: return 23;
        case SM_CXFRAME: return 4;
        case SM_CYFRAME: return 4;
        case SM_CXBORDER: return 1;
        case SM_CYBORDER: return 1;
        case SM_CXVSCROLL: return 17;
        case SM_CYHSCROLL: return 17;
        case SM_CXICON: return 32;
        case SM_CYICON: return 32;
        case SM_CXCURSOR: return 32;
        case SM_CYCURSOR: return 32;
        case SM_CXMIN: return 112;
        case SM_CYMIN: return 27;
        case SM_CXMINTRACK: return 136;
        case SM_CYMINTRACK: return 39;
        case SM_CXSIZE: return 22;
        case SM_CYSIZE: return 22;
        case SM_MOUSEPRESENT: return 1;
        case SM_CMONITORS: return 1;
        case SM_MAXIMUMTOUCHES: return 0;
        case SM_SWAPBUTTON: return 0;
        case SM_REMOTESESSION: return 0;
        default: return 0;
    }
}

// ---------------------------------------------------------------------------
// Timers
// ---------------------------------------------------------------------------
extern "C" UINT_PTR __stdcall Shim_SetTimer(HWND, UINT_PTR nIDEvent, UINT, TIMERPROC) {
    if (nIDEvent == 0) return (UINT_PTR)g_nextTimer.fetch_add(1);
    return nIDEvent;
}
extern "C" BOOL __stdcall Shim_KillTimer(HWND, UINT_PTR) { return TRUE; }

// ---------------------------------------------------------------------------
// Message boxes, dialogs, etc.
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_MessageBoxW(HWND, LPCWSTR, LPCWSTR, UINT) { return IDOK; }
extern "C" int __stdcall Shim_MessageBoxA(HWND, LPCSTR, LPCSTR, UINT) { return IDOK; }
extern "C" BOOL __stdcall Shim_MessageBeep(UINT) { return TRUE; }

// ---------------------------------------------------------------------------
// Window longs
// ---------------------------------------------------------------------------
extern "C" LONG __stdcall Shim_GetWindowLongW(HWND hWnd, int nIndex) {
    WindowState* ws = LookupHwnd(hWnd);
    if (!ws) return 0;
    if (nIndex == GWL_USERDATA) return (LONG)ws->userData;
    if (nIndex == GWL_STYLE) return (LONG)ws->style;
    if (nIndex == GWL_EXSTYLE) return (LONG)ws->exStyle;
    if (nIndex == GWL_HINSTANCE) return (LONG)ws->hInstance;
    if (nIndex == GWL_HWNDPARENT) return (LONG)ws->parent;
    return 0;
}
extern "C" LONG __stdcall Shim_SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong) {
    WindowState* ws = LookupHwnd(hWnd);
    if (!ws) return 0;
    LONG old = 0;
    if (nIndex == GWL_USERDATA) { old = (LONG)ws->userData; ws->userData = dwNewLong; }
    else if (nIndex == GWL_STYLE) { old = (LONG)ws->style; ws->style = (DWORD)dwNewLong; }
    else if (nIndex == GWL_EXSTYLE) { old = (LONG)ws->exStyle; ws->exStyle = (DWORD)dwNewLong; }
    return old;
}
extern "C" LONG_PTR __stdcall Shim_GetWindowLongPtrW(HWND hWnd, int nIndex) {
    return (LONG_PTR)Shim_GetWindowLongW(hWnd, nIndex);
}
extern "C" LONG_PTR __stdcall Shim_SetWindowLongPtrW(HWND hWnd, int nIndex, LONG_PTR dwNewLong) {
    return (LONG_PTR)Shim_SetWindowLongW(hWnd, nIndex, (LONG)dwNewLong);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_GetCursorPos(LPPOINT lpPoint) {
    if (!lpPoint) return FALSE;
    lpPoint->x = 960; lpPoint->y = 540;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_SetCursorPos(int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_GetClipCursor(LPRECT lpRect) {
    if (!lpRect) return FALSE;
    lpRect->left = 0; lpRect->top = 0;
    lpRect->right = 1920; lpRect->bottom = 1080;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_ClipCursor(const RECT*) { return TRUE; }
extern "C" short __stdcall Shim_GetAsyncKeyState(int) { return 0; }
extern "C" short __stdcall Shim_GetKeyState(int) { return 0; }
extern "C" BOOL __stdcall Shim_GetKeyboardState(PBYTE) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetKeyboardState(const BYTE*) { return FALSE; }
extern "C" int __stdcall Shim_ToUnicodeEx(UINT, UINT, const BYTE*, LPWSTR, int, UINT, HKL) { return 0; }
extern "C" int __stdcall Shim_ToAsciiEx(UINT, UINT, const BYTE*, LPWORD, UINT, HKL) { return 0; }
extern "C" int __stdcall Shim_MapVirtualKeyW(UINT, UINT) { return 0; }
extern "C" int __stdcall Shim_MapVirtualKeyA(UINT, UINT) { return 0; }
extern "C" int __stdcall Shim_GetKeyNameTextW(LONG, LPWSTR, int) { return 0; }

// ---------------------------------------------------------------------------
// Misc window queries
// ---------------------------------------------------------------------------
extern "C" DWORD __stdcall Shim_GetWindowThreadProcessId(HWND hWnd, LPDWORD lpdwProcessId) {
    if (lpdwProcessId) *lpdwProcessId = ::GetCurrentProcessId();
    (void)hWnd;
    return ::GetCurrentThreadId();
}
extern "C" BOOL __stdcall Shim_IsWindow(HWND) { return TRUE; }
extern "C" BOOL __stdcall Shim_IsWindowVisible(HWND) { return TRUE; }
extern "C" BOOL __stdcall Shim_IsWindowEnabled(HWND) { return TRUE; }
extern "C" BOOL __stdcall Shim_IsChild(HWND, HWND) { return FALSE; }
extern "C" HWND __stdcall Shim_GetParent(HWND hWnd) {
    WindowState* ws = LookupHwnd(hWnd);
    return ws ? ws->parent : (HWND)0;
}
extern "C" HWND __stdcall Shim_SetParent(HWND hWnd, HWND hWndNewParent) {
    WindowState* ws = LookupHwnd(hWnd);
    if (!ws) return 0;
    HWND old = ws->parent;
    ws->parent = hWndNewParent;
    return old;
}
extern "C" HWND __stdcall Shim_GetDesktopWindow() { return (HWND)1; }
extern "C" HWND __stdcall Shim_GetShellWindow() { return (HWND)2; }
extern "C" HWND __stdcall Shim_FindWindowW(LPCWSTR, LPCWSTR) { return (HWND)0; }
extern "C" HWND __stdcall Shim_FindWindowExW(HWND, HWND, LPCWSTR, LPCWSTR) { return (HWND)0; }
extern "C" int __stdcall Shim_GetWindowTextW(HWND, LPWSTR lpString, int nMaxCount) {
    if (lpString && nMaxCount > 0) lpString[0] = L'\0';
    return 0;
}
extern "C" int __stdcall Shim_GetWindowTextLengthW(HWND) { return 0; }
extern "C" BOOL __stdcall Shim_SetWindowTextW(HWND, LPCWSTR) { return TRUE; }
extern "C" BOOL __stdcall Shim_GetWindowPlacement(HWND, WINDOWPLACEMENT* lpwndpl) {
    if (!lpwndpl) return FALSE;
    std::memset(lpwndpl, 0, sizeof(WINDOWPLACEMENT));
    lpwndpl->length = sizeof(WINDOWPLACEMENT);
    lpwndpl->showCmd = 1;  // SW_SHOWNORMAL
    lpwndpl->rcNormalPosition.left = 0;
    lpwndpl->rcNormalPosition.top = 0;
    lpwndpl->rcNormalPosition.right = 1920;
    lpwndpl->rcNormalPosition.bottom = 1080;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_SetWindowPlacement(HWND, const WINDOWPLACEMENT*) { return TRUE; }
extern "C" BOOL __stdcall Shim_ShowOwnedPopups(HWND, BOOL) { return TRUE; }
extern "C" BOOL __stdcall Shim_SetWindowRgn(HWND, HRGN, BOOL) { return TRUE; }
extern "C" int __stdcall Shim_GetUpdateRgn(HWND, HRGN, BOOL) { return 0; }
extern "C" BOOL __stdcall Shim_GetUpdateRect(HWND, LPRECT, BOOL) { return FALSE; }
extern "C" BOOL __stdcall Shim_ScrollWindowEx(HWND, int, int, const RECT*, const RECT*, HRGN, LPRECT, UINT) { return TRUE; }

// ---------------------------------------------------------------------------
// Menus
// ---------------------------------------------------------------------------
extern "C" HMENU __stdcall Shim_CreateMenu() { return (HMENU)0x30000; }
extern "C" HMENU __stdcall Shim_CreatePopupMenu() { return (HMENU)0x30001; }
extern "C" BOOL __stdcall Shim_DestroyMenu(HMENU) { return TRUE; }
extern "C" BOOL __stdcall Shim_AppendMenuW(HMENU, UINT, UINT_PTR, LPCWSTR) { return TRUE; }
extern "C" BOOL __stdcall Shim_InsertMenuW(HMENU, UINT, UINT, UINT_PTR, LPCWSTR) { return TRUE; }
extern "C" BOOL __stdcall Shim_DeleteMenu(HMENU, UINT, UINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_TrackPopupMenuEx(HMENU, UINT, int, int, HWND, LPTPMPARAMS) { return TRUE; }

// ---------------------------------------------------------------------------
// Monitor / DPI
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_GetMonitorInfoW(HMONITOR, LPMONITORINFO lpmi) {
    if (!lpmi) return FALSE;
    std::memset(lpmi, 0, sizeof(MONITORINFO));
    lpmi->cbSize = sizeof(MONITORINFO);
    lpmi->rcMonitor.left = 0; lpmi->rcMonitor.top = 0;
    lpmi->rcMonitor.right = 1920; lpmi->rcMonitor.bottom = 1080;
    lpmi->rcWork = lpmi->rcMonitor;
    lpmi->dwFlags = 1;  // MONITORINFOF_PRIMARY
    return TRUE;
}
extern "C" HMONITOR __stdcall Shim_MonitorFromWindow(HWND, DWORD) { return (HMONITOR)1; }
extern "C" HMONITOR __stdcall Shim_MonitorFromPoint(POINT, DWORD) { return (HMONITOR)1; }
extern "C" BOOL __stdcall Shim_EnumDisplayMonitors(HDC, LPCRECT, MONITORENUMPROC lpfnEnum, LPARAM lParam) {
    if (lpfnEnum) {
        RECT rc{0, 0, 1920, 1080};
        lpfnEnum((HMONITOR)1, (HDC)1, &rc, lParam);
    }
    return TRUE;
}
extern "C" int __stdcall Shim_GetDpiForWindow(HWND) { return 96; }
extern "C" int __stdcall Shim_GetDpiForSystem() { return 96; }
extern "C" BOOL __stdcall Shim_SetProcessDPIAware() { return TRUE; }
extern "C" BOOL __stdcall Shim_SetProcessDpiAwareness(int) { return TRUE; }
extern "C" BOOL __stdcall Shim_SystemParametersInfoW(UINT, UINT, PVOID, UINT) { return TRUE; }

// ---------------------------------------------------------------------------
// Clipboard
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_OpenClipboard(HWND) { return TRUE; }
extern "C" BOOL __stdcall Shim_CloseClipboard() { return TRUE; }
extern "C" BOOL __stdcall Shim_EmptyClipboard() { return TRUE; }
extern "C" HANDLE __stdcall Shim_SetClipboardData(UINT, HANDLE) { return nullptr; }
extern "C" HANDLE __stdcall Shim_GetClipboardData(UINT) { return nullptr; }
extern "C" BOOL __stdcall Shim_IsClipboardFormatAvailable(UINT) { return FALSE; }
extern "C" UINT __stdcall Shim_RegisterClipboardFormatW(LPCWSTR) { return 0; }
extern "C" int __stdcall Shim_GetClipboardFormatNameW(UINT, LPWSTR, int) { return 0; }
extern "C" int __stdcall Shim_CountClipboardFormats() { return 0; }
extern "C" UINT __stdcall Shim_EnumClipboardFormats(UINT) { return 0; }

// ---------------------------------------------------------------------------
// Atoms
// ---------------------------------------------------------------------------
extern "C" ATOM __stdcall Shim_GlobalAddAtomW(LPCWSTR) { return (ATOM)g_nextAtom.fetch_add(1); }
extern "C" ATOM __stdcall Shim_GlobalFindAtomW(LPCWSTR) { return 0; }
extern "C" ATOM __stdcall Shim_GlobalDeleteAtom(ATOM) { return 0; }

// ---------------------------------------------------------------------------
// Icons / cursors
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_DrawIconW(HDC, int, int, HICON) { return TRUE; }
extern "C" BOOL __stdcall Shim_DrawIconExW(HDC, int, int, HICON, int, int, UINT, HBRUSH, UINT) { return TRUE; }
extern "C" BOOL __stdcall Shim_DestroyIcon(HICON) { return TRUE; }
extern "C" BOOL __stdcall Shim_DestroyCursor(HCURSOR) { return TRUE; }

// ---------------------------------------------------------------------------
// Drawing helpers (also forwarded from gdi32)
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_FillRect(HDC, const RECT*, HBRUSH) { return 1; }
extern "C" int __stdcall Shim_FrameRect(HDC, const RECT*, HBRUSH) { return 1; }
extern "C" int __stdcall Shim_DrawTextW(HDC, LPCWSTR, int, LPRECT, UINT) { return 0; }
extern "C" int __stdcall Shim_DrawTextExW(HDC, LPWSTR, int, LPRECT, UINT, LPDRAWTEXTPARAMS) { return 0; }
extern "C" BOOL __stdcall Shim_GrayStringW(HDC, HBRUSH, GRAYSTRINGPROC, LPARAM, int, int, int, int, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_TabbedTextOutW(HDC, int, int, LPCWSTR, int, int, const INT*, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_ExtTextOutW(HDC, int, int, UINT, const RECT*, LPCWSTR, UINT, const INT*) { return TRUE; }

}  // namespace xwr

// ===========================================================================
// Registration — must be at namespace scope (not inside xwr).
// ===========================================================================
REGISTER_SHIM("user32", "CreateWindowExW", (FARPROC)&xwr::Shim_CreateWindowExW);
REGISTER_SHIM("user32", "CreateWindowExA", (FARPROC)&xwr::Shim_CreateWindowExA);
REGISTER_SHIM("user32", "DestroyWindow", (FARPROC)&xwr::Shim_DestroyWindow);
REGISTER_SHIM("user32", "ShowWindow", (FARPROC)&xwr::Shim_ShowWindow);
REGISTER_SHIM("user32", "UpdateWindow", (FARPROC)&xwr::Shim_UpdateWindow);
REGISTER_SHIM("user32", "InvalidateRect", (FARPROC)&xwr::Shim_InvalidateRect);
REGISTER_SHIM("user32", "ValidateRect", (FARPROC)&xwr::Shim_ValidateRect);
REGISTER_SHIM("user32", "RedrawWindow", (FARPROC)&xwr::Shim_RedrawWindow);
REGISTER_SHIM("user32", "SetWindowPos", (FARPROC)&xwr::Shim_SetWindowPos);
REGISTER_SHIM("user32", "MoveWindow", (FARPROC)&xwr::Shim_MoveWindow);
REGISTER_SHIM("user32", "BringWindowToTop", (FARPROC)&xwr::Shim_BringWindowToTop);
REGISTER_SHIM("user32", "SetForegroundWindow", (FARPROC)&xwr::Shim_SetForegroundWindow);
REGISTER_SHIM("user32", "GetForegroundWindow", (FARPROC)&xwr::Shim_GetForegroundWindow);
REGISTER_SHIM("user32", "GetFocus", (FARPROC)&xwr::Shim_GetFocus);
REGISTER_SHIM("user32", "SetFocus", (FARPROC)&xwr::Shim_SetFocus);
REGISTER_SHIM("user32", "ShowCursor", (FARPROC)&xwr::Shim_ShowCursor);
REGISTER_SHIM("user32", "SetCursor", (FARPROC)&xwr::Shim_SetCursor);
REGISTER_SHIM("user32", "LoadCursorW", (FARPROC)&xwr::Shim_LoadCursorW);
REGISTER_SHIM("user32", "LoadIconW", (FARPROC)&xwr::Shim_LoadIconW);
REGISTER_SHIM("user32", "LoadImageW", (FARPROC)&xwr::Shim_LoadImageW);
REGISTER_SHIM("user32", "GetClientRect", (FARPROC)&xwr::Shim_GetClientRect);
REGISTER_SHIM("user32", "GetWindowRect", (FARPROC)&xwr::Shim_GetWindowRect);
REGISTER_SHIM("user32", "AdjustWindowRect", (FARPROC)&xwr::Shim_AdjustWindowRect);
REGISTER_SHIM("user32", "AdjustWindowRectEx", (FARPROC)&xwr::Shim_AdjustWindowRectEx);
REGISTER_SHIM("user32", "ClientToScreen", (FARPROC)&xwr::Shim_ClientToScreen);
REGISTER_SHIM("user32", "ScreenToClient", (FARPROC)&xwr::Shim_ScreenToClient);
REGISTER_SHIM("user32", "GetDC", (FARPROC)&xwr::Shim_GetDC);
REGISTER_SHIM("user32", "ReleaseDC", (FARPROC)&xwr::Shim_ReleaseDC);
REGISTER_SHIM("user32", "BeginPaint", (FARPROC)&xwr::Shim_BeginPaint);
REGISTER_SHIM("user32", "EndPaint", (FARPROC)&xwr::Shim_EndPaint);
REGISTER_SHIM("user32", "PeekMessageW", (FARPROC)&xwr::Shim_PeekMessageW);
REGISTER_SHIM("user32", "PeekMessageA", (FARPROC)&xwr::Shim_PeekMessageW);
REGISTER_SHIM("user32", "GetMessageW", (FARPROC)&xwr::Shim_GetMessageW);
REGISTER_SHIM("user32", "GetMessageA", (FARPROC)&xwr::Shim_GetMessageW);
REGISTER_SHIM("user32", "PostMessageW", (FARPROC)&xwr::Shim_PostMessageW);
REGISTER_SHIM("user32", "PostMessageA", (FARPROC)&xwr::Shim_PostMessageW);
REGISTER_SHIM("user32", "SendMessageW", (FARPROC)&xwr::Shim_SendMessageW);
REGISTER_SHIM("user32", "SendMessageA", (FARPROC)&xwr::Shim_SendMessageW);
REGISTER_SHIM("user32", "TranslateMessage", (FARPROC)&xwr::Shim_TranslateMessage);
REGISTER_SHIM("user32", "DispatchMessageW", (FARPROC)&xwr::Shim_DispatchMessageW);
REGISTER_SHIM("user32", "DispatchMessageA", (FARPROC)&xwr::Shim_DispatchMessageW);
REGISTER_SHIM("user32", "DefWindowProcW", (FARPROC)&xwr::Shim_DefWindowProcW);
REGISTER_SHIM("user32", "DefWindowProcA", (FARPROC)&xwr::Shim_DefWindowProcW);
REGISTER_SHIM("user32", "PostQuitMessage", (FARPROC)&xwr::Shim_PostQuitMessage);
REGISTER_SHIM("user32", "WaitMessage", (FARPROC)&xwr::Shim_WaitMessage);
REGISTER_SHIM("user32", "GetInputState", (FARPROC)&xwr::Shim_GetInputState);
REGISTER_SHIM("user32", "GetQueueStatus", (FARPROC)&xwr::Shim_GetQueueStatus);
REGISTER_SHIM("user32", "TranslateAcceleratorW", (FARPROC)&xwr::Shim_TranslateAcceleratorW);
REGISTER_SHIM("user32", "RegisterClassW", (FARPROC)&xwr::Shim_RegisterClassW);
REGISTER_SHIM("user32", "RegisterClassExW", (FARPROC)&xwr::Shim_RegisterClassExW);
REGISTER_SHIM("user32", "RegisterClassA", (FARPROC)&xwr::Shim_RegisterClassW);
REGISTER_SHIM("user32", "RegisterClassExA", (FARPROC)&xwr::Shim_RegisterClassExW);
REGISTER_SHIM("user32", "UnregisterClassW", (FARPROC)&xwr::Shim_UnregisterClassW);
REGISTER_SHIM("user32", "GetSystemMetrics", (FARPROC)&xwr::Shim_GetSystemMetrics);
REGISTER_SHIM("user32", "SetTimer", (FARPROC)&xwr::Shim_SetTimer);
REGISTER_SHIM("user32", "KillTimer", (FARPROC)&xwr::Shim_KillTimer);
REGISTER_SHIM("user32", "MessageBoxW", (FARPROC)&xwr::Shim_MessageBoxW);
REGISTER_SHIM("user32", "MessageBoxA", (FARPROC)&xwr::Shim_MessageBoxA);
REGISTER_SHIM("user32", "MessageBeep", (FARPROC)&xwr::Shim_MessageBeep);
REGISTER_SHIM("user32", "GetWindowLongW", (FARPROC)&xwr::Shim_GetWindowLongW);
REGISTER_SHIM("user32", "SetWindowLongW", (FARPROC)&xwr::Shim_SetWindowLongW);
REGISTER_SHIM("user32", "GetWindowLongPtrW", (FARPROC)&xwr::Shim_GetWindowLongPtrW);
REGISTER_SHIM("user32", "SetWindowLongPtrW", (FARPROC)&xwr::Shim_SetWindowLongPtrW);
REGISTER_SHIM("user32", "GetCursorPos", (FARPROC)&xwr::Shim_GetCursorPos);
REGISTER_SHIM("user32", "SetCursorPos", (FARPROC)&xwr::Shim_SetCursorPos);
REGISTER_SHIM("user32", "GetClipCursor", (FARPROC)&xwr::Shim_GetClipCursor);
REGISTER_SHIM("user32", "ClipCursor", (FARPROC)&xwr::Shim_ClipCursor);
REGISTER_SHIM("user32", "GetAsyncKeyState", (FARPROC)&xwr::Shim_GetAsyncKeyState);
REGISTER_SHIM("user32", "GetKeyState", (FARPROC)&xwr::Shim_GetKeyState);
REGISTER_SHIM("user32", "GetKeyboardState", (FARPROC)&xwr::Shim_GetKeyboardState);
REGISTER_SHIM("user32", "SetKeyboardState", (FARPROC)&xwr::Shim_SetKeyboardState);
REGISTER_SHIM("user32", "ToUnicodeEx", (FARPROC)&xwr::Shim_ToUnicodeEx);
REGISTER_SHIM("user32", "ToAsciiEx", (FARPROC)&xwr::Shim_ToAsciiEx);
REGISTER_SHIM("user32", "MapVirtualKeyW", (FARPROC)&xwr::Shim_MapVirtualKeyW);
REGISTER_SHIM("user32", "MapVirtualKeyA", (FARPROC)&xwr::Shim_MapVirtualKeyA);
REGISTER_SHIM("user32", "GetKeyNameTextW", (FARPROC)&xwr::Shim_GetKeyNameTextW);
REGISTER_SHIM("user32", "GetWindowThreadProcessId", (FARPROC)&xwr::Shim_GetWindowThreadProcessId);
REGISTER_SHIM("user32", "IsWindow", (FARPROC)&xwr::Shim_IsWindow);
REGISTER_SHIM("user32", "IsWindowVisible", (FARPROC)&xwr::Shim_IsWindowVisible);
REGISTER_SHIM("user32", "IsWindowEnabled", (FARPROC)&xwr::Shim_IsWindowEnabled);
REGISTER_SHIM("user32", "IsChild", (FARPROC)&xwr::Shim_IsChild);
REGISTER_SHIM("user32", "GetParent", (FARPROC)&xwr::Shim_GetParent);
REGISTER_SHIM("user32", "SetParent", (FARPROC)&xwr::Shim_SetParent);
REGISTER_SHIM("user32", "GetDesktopWindow", (FARPROC)&xwr::Shim_GetDesktopWindow);
REGISTER_SHIM("user32", "GetShellWindow", (FARPROC)&xwr::Shim_GetShellWindow);
REGISTER_SHIM("user32", "FindWindowW", (FARPROC)&xwr::Shim_FindWindowW);
REGISTER_SHIM("user32", "FindWindowExW", (FARPROC)&xwr::Shim_FindWindowExW);
REGISTER_SHIM("user32", "GetWindowTextW", (FARPROC)&xwr::Shim_GetWindowTextW);
REGISTER_SHIM("user32", "GetWindowTextLengthW", (FARPROC)&xwr::Shim_GetWindowTextLengthW);
REGISTER_SHIM("user32", "SetWindowTextW", (FARPROC)&xwr::Shim_SetWindowTextW);
REGISTER_SHIM("user32", "GetWindowPlacement", (FARPROC)&xwr::Shim_GetWindowPlacement);
REGISTER_SHIM("user32", "SetWindowPlacement", (FARPROC)&xwr::Shim_SetWindowPlacement);
REGISTER_SHIM("user32", "ShowOwnedPopups", (FARPROC)&xwr::Shim_ShowOwnedPopups);
REGISTER_SHIM("user32", "SetWindowRgn", (FARPROC)&xwr::Shim_SetWindowRgn);
REGISTER_SHIM("user32", "GetUpdateRgn", (FARPROC)&xwr::Shim_GetUpdateRgn);
REGISTER_SHIM("user32", "GetUpdateRect", (FARPROC)&xwr::Shim_GetUpdateRect);
REGISTER_SHIM("user32", "ScrollWindowEx", (FARPROC)&xwr::Shim_ScrollWindowEx);
REGISTER_SHIM("user32", "CreateMenu", (FARPROC)&xwr::Shim_CreateMenu);
REGISTER_SHIM("user32", "CreatePopupMenu", (FARPROC)&xwr::Shim_CreatePopupMenu);
REGISTER_SHIM("user32", "DestroyMenu", (FARPROC)&xwr::Shim_DestroyMenu);
REGISTER_SHIM("user32", "AppendMenuW", (FARPROC)&xwr::Shim_AppendMenuW);
REGISTER_SHIM("user32", "InsertMenuW", (FARPROC)&xwr::Shim_InsertMenuW);
REGISTER_SHIM("user32", "DeleteMenu", (FARPROC)&xwr::Shim_DeleteMenu);
REGISTER_SHIM("user32", "TrackPopupMenuEx", (FARPROC)&xwr::Shim_TrackPopupMenuEx);
REGISTER_SHIM("user32", "GetMonitorInfoW", (FARPROC)&xwr::Shim_GetMonitorInfoW);
REGISTER_SHIM("user32", "MonitorFromWindow", (FARPROC)&xwr::Shim_MonitorFromWindow);
REGISTER_SHIM("user32", "MonitorFromPoint", (FARPROC)&xwr::Shim_MonitorFromPoint);
REGISTER_SHIM("user32", "EnumDisplayMonitors", (FARPROC)&xwr::Shim_EnumDisplayMonitors);
REGISTER_SHIM("user32", "GetDpiForWindow", (FARPROC)&xwr::Shim_GetDpiForWindow);
REGISTER_SHIM("user32", "GetDpiForSystem", (FARPROC)&xwr::Shim_GetDpiForSystem);
REGISTER_SHIM("user32", "SetProcessDPIAware", (FARPROC)&xwr::Shim_SetProcessDPIAware);
REGISTER_SHIM("user32", "SetProcessDpiAwareness", (FARPROC)&xwr::Shim_SetProcessDpiAwareness);
REGISTER_SHIM("user32", "SystemParametersInfoW", (FARPROC)&xwr::Shim_SystemParametersInfoW);
REGISTER_SHIM("user32", "OpenClipboard", (FARPROC)&xwr::Shim_OpenClipboard);
REGISTER_SHIM("user32", "CloseClipboard", (FARPROC)&xwr::Shim_CloseClipboard);
REGISTER_SHIM("user32", "EmptyClipboard", (FARPROC)&xwr::Shim_EmptyClipboard);
REGISTER_SHIM("user32", "SetClipboardData", (FARPROC)&xwr::Shim_SetClipboardData);
REGISTER_SHIM("user32", "GetClipboardData", (FARPROC)&xwr::Shim_GetClipboardData);
REGISTER_SHIM("user32", "IsClipboardFormatAvailable", (FARPROC)&xwr::Shim_IsClipboardFormatAvailable);
REGISTER_SHIM("user32", "RegisterClipboardFormatW", (FARPROC)&xwr::Shim_RegisterClipboardFormatW);
REGISTER_SHIM("user32", "GetClipboardFormatNameW", (FARPROC)&xwr::Shim_GetClipboardFormatNameW);
REGISTER_SHIM("user32", "CountClipboardFormats", (FARPROC)&xwr::Shim_CountClipboardFormats);
REGISTER_SHIM("user32", "EnumClipboardFormats", (FARPROC)&xwr::Shim_EnumClipboardFormats);
REGISTER_SHIM("user32", "GlobalAddAtomW", (FARPROC)&xwr::Shim_GlobalAddAtomW);
REGISTER_SHIM("user32", "GlobalFindAtomW", (FARPROC)&xwr::Shim_GlobalFindAtomW);
REGISTER_SHIM("user32", "GlobalDeleteAtom", (FARPROC)&xwr::Shim_GlobalDeleteAtom);
REGISTER_SHIM("user32", "DrawIconW", (FARPROC)&xwr::Shim_DrawIconW);
REGISTER_SHIM("user32", "DrawIconExW", (FARPROC)&xwr::Shim_DrawIconExW);
REGISTER_SHIM("user32", "DestroyIcon", (FARPROC)&xwr::Shim_DestroyIcon);
REGISTER_SHIM("user32", "DestroyCursor", (FARPROC)&xwr::Shim_DestroyCursor);
REGISTER_SHIM("user32", "FillRect", (FARPROC)&xwr::Shim_FillRect);
REGISTER_SHIM("user32", "FrameRect", (FARPROC)&xwr::Shim_FrameRect);
REGISTER_SHIM("user32", "DrawTextW", (FARPROC)&xwr::Shim_DrawTextW);
REGISTER_SHIM("user32", "DrawTextExW", (FARPROC)&xwr::Shim_DrawTextExW);
REGISTER_SHIM("user32", "GrayStringW", (FARPROC)&xwr::Shim_GrayStringW);
REGISTER_SHIM("user32", "TabbedTextOutW", (FARPROC)&xwr::Shim_TabbedTextOutW);
REGISTER_SHIM("user32", "ExtTextOutW", (FARPROC)&xwr::Shim_ExtTextOutW);
