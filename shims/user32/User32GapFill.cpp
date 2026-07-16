// shims/user32/User32GapFill.cpp
// Implements the remaining 77 USER32.dll functions that the runner imports
// but were missing from User32Shim.cpp.
//
// Most are direct pass-throughs to the real Win32 API (UWP supports them).
// A few are stubs returning reasonable defaults for APIs that UWP blocks:
//   - Raw input (GetRawInputData, GetRawInputDeviceInfoA, GetRawInputDeviceList,
//     RegisterRawInputDevices)
//   - Touch input (CloseTouchInputHandle, GetTouchInputInfo, RegisterTouchWindow)
//   - Display config (DisplayConfigGetDeviceInfo, GetDisplayConfigBufferSizes,
//     QueryDisplayConfig)
//   - Display mode changes (ChangeDisplaySettings{,Ex}W)
//   - SendInput (Xbox has no physical mouse/keyboard — input bridge handles it)
//
// Each function is `extern "C" __stdcall Shim_<OriginalName>` so the PE loader
// can drop its address directly into a game's IAT.

#include "UwpSdkIncludes.h"


#include <atomic>
#include <mutex>
#include <cstring>
#include <cstdarg>

#include "../ShimRegistry.h"

namespace xwr {

// ===========================================================================
// Keyboard layout / input
// ===========================================================================
extern "C" HKL    __stdcall Shim_ActivateKeyboardLayout(HKL hkl, UINT flags)      { return ::ActivateKeyboardLayout(hkl, flags); }
extern "C" BOOL   __stdcall Shim_AllowSetForegroundWindow(DWORD dwProcId)         { return ::AllowSetForegroundWindow(dwProcId); }
extern "C" HKL    __stdcall Shim_GetKeyboardLayout(DWORD idThread)                { return ::GetKeyboardLayout(idThread); }
extern "C" int    __stdcall Shim_GetKeyboardLayoutList(int nBuff, HKL* lpList)    { return ::GetKeyboardLayoutList(nBuff, lpList); }
extern "C" LPARAM __stdcall Shim_GetMessageExtraInfo()                            { return ::GetMessageExtraInfo(); }

// ===========================================================================
// Clipboard / hooks
// ===========================================================================
extern "C" BOOL    __stdcall Shim_AddClipboardFormatListener(HWND hwnd)           { return ::AddClipboardFormatListener(hwnd); }
extern "C" LRESULT __stdcall Shim_CallNextHookEx(HHOOK hhk, int code, WPARAM w, LPARAM l) { return ::CallNextHookEx(hhk, code, w, l); }
extern "C" HHOOK   __stdcall Shim_SetWindowsHookExW(int idHook, HOOKPROC lpfn, HINSTANCE hMod, DWORD dwTid) { return ::SetWindowsHookExW(idHook, lpfn, hMod, dwTid); }
extern "C" BOOL    __stdcall Shim_UnhookWindowsHookEx(HHOOK hhk)                  { return ::UnhookWindowsHookEx(hhk); }

// ===========================================================================
// Display settings / display config
// ===========================================================================
extern "C" LONG __stdcall Shim_ChangeDisplaySettingsExW(LPCWSTR lpszDeviceName, LPDEVMODEW lpDevMode,
                                                        HWND hwnd, DWORD dwFlags, LPVOID lParam) {
    return ::ChangeDisplaySettingsExW(lpszDeviceName, lpDevMode, hwnd, dwFlags, lParam);
}
extern "C" LONG __stdcall Shim_ChangeDisplaySettingsW(LPDEVMODEW lpDevMode, DWORD dwFlags) {
    return ::ChangeDisplaySettingsW(lpDevMode, dwFlags);
}
extern "C" BOOL __stdcall Shim_EnumDisplayDevicesA(LPCSTR lpDevice, DWORD iDevNum,
                                                   PDISPLAY_DEVICEA lpDisplayDevice, DWORD dwFlags) {
    return ::EnumDisplayDevicesA(lpDevice, iDevNum, lpDisplayDevice, dwFlags);
}
extern "C" BOOL __stdcall Shim_EnumDisplayDevicesW(LPCWSTR lpDevice, DWORD iDevNum,
                                                   PDISPLAY_DEVICEW lpDisplayDevice, DWORD dwFlags) {
    return ::EnumDisplayDevicesW(lpDevice, iDevNum, lpDisplayDevice, dwFlags);
}
extern "C" BOOL __stdcall Shim_EnumDisplaySettingsW(LPCWSTR lpszDeviceName, DWORD iModeNum, PDEVMODEW lpDevMode) {
    return ::EnumDisplaySettingsW(lpszDeviceName, iModeNum, lpDevMode);
}

// Display config APIs — UWP blocks these. Stub with ERROR_NOT_SUPPORTED.
extern "C" LONG __stdcall Shim_DisplayConfigGetDeviceInfo(DISPLAYCONFIG_DEVICE_INFO_HEADER* pRequest) {
    (void)pRequest;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return ERROR_NOT_SUPPORTED;
}
extern "C" LONG __stdcall Shim_GetDisplayConfigBufferSizes(UINT32 flags, UINT32* pNumPathArray, UINT32* pNumModeInfoArray) {
    if (pNumPathArray)    *pNumPathArray = 0;
    if (pNumModeInfoArray) *pNumModeInfoArray = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return ERROR_NOT_SUPPORTED;
}
extern "C" LONG __stdcall Shim_QueryDisplayConfig(UINT32 flags, UINT32* pNumPathArray, DISPLAYCONFIG_PATH_INFO* pPathInfoArray,
                                                  UINT32* pNumModeInfoArray, DISPLAYCONFIG_MODE_INFO* pModeInfoArray, HWND* pCurrentTopologyId) {
    if (pNumPathArray)     *pNumPathArray = 0;
    if (pNumModeInfoArray) *pNumModeInfoArray = 0;
    (void)flags; (void)pPathInfoArray; (void)pModeInfoArray; (void)pCurrentTopologyId;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return ERROR_NOT_SUPPORTED;
}

// ===========================================================================
// Touch input — UWP blocks touch APIs. Stub as if no touch hardware present.
// ===========================================================================
extern "C" BOOL __stdcall Shim_CloseTouchInputHandle(HTOUCHINPUT hTouchInput) {
    (void)hTouchInput;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
extern "C" BOOL __stdcall Shim_GetTouchInputInfo(HTOUCHINPUT hTouchInput, UINT cInputs, PTOUCHINPUT pInputs, int cbSize) {
    (void)hTouchInput; (void)cInputs; (void)pInputs; (void)cbSize;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}
extern "C" BOOL __stdcall Shim_RegisterTouchWindow(HWND hwnd, ULONG ulFlags) {
    (void)hwnd; (void)ulFlags;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

// ===========================================================================
// Raw input — UWP blocks raw input APIs.
// ===========================================================================
extern "C" UINT __stdcall Shim_GetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) {
    (void)hRawInput; (void)uiCommand; (void)pData;
    if (pcbSize) *pcbSize = cbSizeHeader;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return (UINT)-1;
}
extern "C" UINT __stdcall Shim_GetRawInputDeviceInfoA(HANDLE hDevice, UINT uiCommand, LPVOID pData, PINT pcbSize) {
    (void)hDevice; (void)uiCommand; (void)pData;
    if (pcbSize) *pcbSize = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return (UINT)-1;
}
extern "C" UINT __stdcall Shim_GetRawInputDeviceList(PRAWINPUTDEVICELIST pRawInputDeviceList, PUINT puiNumDevices, UINT cbSize) {
    (void)pRawInputDeviceList; (void)cbSize;
    if (puiNumDevices) *puiNumDevices = 0;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return (UINT)-1;
}
extern "C" BOOL __stdcall Shim_RegisterRawInputDevices(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize) {
    (void)pRawInputDevices; (void)uiNumDevices; (void)cbSize;
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}

// ===========================================================================
// SendInput — Xbox has no physical mouse/keyboard; input bridge handles it.
// Pretend all inputs were successfully injected.
// ===========================================================================
extern "C" UINT __stdcall Shim_SendInput(UINT cInputs, LPINPUT pInputs, int cbSize) {
    (void)pInputs; (void)cbSize;
    return cInputs;
}

// ===========================================================================
// Dialogs / accelerators / menus
// ===========================================================================
extern "C" HWND  __stdcall Shim_CreateDialogParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
    return ::CreateDialogParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}
extern "C" BOOL __stdcall Shim_DestroyAcceleratorTable(HACCEL hAccel) { return ::DestroyAcceleratorTable(hAccel); }
extern "C" BOOL __stdcall Shim_IsDialogMessageW(HWND hDlg, LPMSG lpMsg) { return ::IsDialogMessageW(hDlg, lpMsg); }
extern "C" HACCEL __stdcall Shim_LoadAcceleratorsW(HINSTANCE hInstance, LPCWSTR lpTableName) { return ::LoadAcceleratorsW(hInstance, lpTableName); }
extern "C" HMENU __stdcall Shim_GetSystemMenu(HWND hWnd, BOOL bRevert) { return ::GetSystemMenu(hWnd, bRevert); }
extern "C" BOOL __stdcall Shim_EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable) { return ::EnableMenuItem(hMenu, uIDEnableItem, uEnable); }
extern "C" BOOL __stdcall Shim_InsertMenuItemW(HMENU hMenu, UINT uItem, BOOL fByPosition, LPCMENUITEMINFOW lpmii) { return ::InsertMenuItemW(hMenu, uItem, fByPosition, lpmii); }
extern "C" BOOL __stdcall Shim_TrackPopupMenu(HMENU hMenu, UINT uFlags, int x, int y, int nReserved, HWND hWnd, const RECT* prcRect) {
    return ::TrackPopupMenu(hMenu, uFlags, x, y, nReserved, hWnd, prcRect);
}

// ===========================================================================
// Window state / classification
// ===========================================================================
extern "C" HWND  __stdcall Shim_GetActiveWindow()                  { return ::GetActiveWindow(); }
extern "C" HWND  __stdcall Shim_SetActiveWindow(HWND hWnd)         { return ::SetActiveWindow(hWnd); }
extern "C" HWND  __stdcall Shim_GetCapture()                       { return ::GetCapture(); }
extern "C" HWND  __stdcall Shim_SetCapture(HWND hWnd)              { return ::SetCapture(hWnd); }
extern "C" BOOL  __stdcall Shim_ReleaseCapture()                   { return ::ReleaseCapture(); }
extern "C" HWND  __stdcall Shim_GetTopWindow(HWND hWnd)            { return ::GetTopWindow(hWnd); }
extern "C" HWND  __stdcall Shim_GetDlgItem(HWND hDlg, int nIDDlgItem) { return ::GetDlgItem(hDlg, nIDDlgItem); }
extern "C" UINT  __stdcall Shim_GetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int cchMax) {
    return ::GetDlgItemTextW(hDlg, nIDDlgItem, lpString, cchMax);
}
extern "C" BOOL  __stdcall Shim_SetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString) {
    return ::SetDlgItemTextW(hDlg, nIDDlgItem, lpString);
}
extern "C" BOOL  __stdcall Shim_EnableWindow(HWND hWnd, BOOL bEnable) { return ::EnableWindow(hWnd, bEnable); }
extern "C" BOOL  __stdcall Shim_IsIconic(HWND hWnd)                 { return ::IsIconic(hWnd); }
extern "C" BOOL  __stdcall Shim_IsZoomed(HWND hWnd)                 { return ::IsZoomed(hWnd); }
extern "C" BOOL  __stdcall Shim_FlashWindowEx(PFLASHWINFO pfwi)     { return ::FlashWindowEx(pfwi); }
extern "C" HWND  __stdcall Shim_FindWindowExA(HWND hWndParent, HWND hWndChildAfter, LPCSTR lpszClass, LPCSTR lpszWindow) {
    return ::FindWindowExA(hWndParent, hWndChildAfter, lpszClass, lpszWindow);
}
extern "C" BOOL  __stdcall Shim_EnumChildWindows(HWND hWndParent, WNDENUMPROC lpEnumFunc, LPARAM lParam) {
    return ::EnumChildWindows(hWndParent, lpEnumFunc, lParam);
}
extern "C" ULONG_PTR __stdcall Shim_GetClassLongPtrW(HWND hWnd, int nIndex) { return ::GetClassLongPtrW(hWnd, nIndex); }
extern "C" BOOL  __stdcall Shim_GetWindowInfo(HWND hWnd, PWINDOWINFO pwi) { return ::GetWindowInfo(hWnd, pwi); }
extern "C" BOOL  __stdcall Shim_GetWindowDisplayAffinity(HWND hWnd, DWORD* pdwAffinity) { return ::GetWindowDisplayAffinity(hWnd, pdwAffinity); }
extern "C" BOOL  __stdcall Shim_SetWindowDisplayAffinity(HWND hWnd, DWORD dwAffinity) { return ::SetWindowDisplayAffinity(hWnd, dwAffinity); }
extern "C" BOOL  __stdcall Shim_SetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags) {
    return ::SetLayeredWindowAttributes(hwnd, crKey, bAlpha, dwFlags);
}
extern "C" VOID  __stdcall Shim_DisableProcessWindowsGhosting()    { ::DisableProcessWindowsGhosting(); }
extern "C" BOOL  __stdcall Shim_UnregisterClassA(LPCSTR lpClassName, HINSTANCE hInstance) {
    return ::UnregisterClassA(lpClassName, hInstance);
}

// ===========================================================================
// Window station / user object
// ===========================================================================
extern "C" HWINSTA __stdcall Shim_GetProcessWindowStation()        { return ::GetProcessWindowStation(); }
extern "C" BOOL    __stdcall Shim_GetUserObjectInformationW(HWINSTA hObj, int nIndex, LPVOID pvInfo, DWORD nLength, LPDWORD lpnLenNeeded) {
    return ::GetUserObjectInformationW(hObj, nIndex, pvInfo, nLength, lpnLenNeeded);
}

// ===========================================================================
// DC / monitor / icon / drawing
// ===========================================================================
extern "C" HDC       __stdcall Shim_GetDCEx(HWND hWnd, HRGN hrgnClip, DWORD flags) { return ::GetDCEx(hWnd, hrgnClip, flags); }
extern "C" BOOL      __stdcall Shim_GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi) { return ::GetMonitorInfoA(hMonitor, lpmi); }
extern "C" HMONITOR  __stdcall Shim_MonitorFromRect(LPCRECT lprc, DWORD dwFlags) { return ::MonitorFromRect(lprc, dwFlags); }
extern "C" HICON     __stdcall Shim_CreateIconIndirect(PICONINFO piconinfo) { return ::CreateIconIndirect(piconinfo); }
extern "C" BOOL      __stdcall Shim_DrawIconEx(HDC hdc, int x, int y, HICON hicon, int cx, int cy, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags) {
    return ::DrawIconEx(hdc, x, y, hicon, cx, cy, istepIfAniCur, hbrFlickerFreeDraw, diFlags);
}
extern "C" BOOL      __stdcall Shim_DrawStateW(HDC hdc, HBRUSH hbrFore, DRAWSTATEPROC qfnCallBack, LPARAM lData, WPARAM wData, int x, int y, int cx, int cy, UINT uFlags) {
    return ::DrawStateW(hdc, hbrFore, qfnCallBack, lData, wData, x, y, cx, cy, uFlags);
}
extern "C" HBITMAP   __stdcall Shim_LoadBitmapW(HINSTANCE hInstance, LPCWSTR lpBitmapName) { return ::LoadBitmapW(hInstance, lpBitmapName); }
extern "C" HCURSOR   __stdcall Shim_LoadCursorFromFileW(LPCWSTR lpFileName) { return ::LoadCursorFromFileW(lpFileName); }
extern "C" int       __stdcall Shim_MapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints) {
    return ::MapWindowPoints(hWndFrom, hWndTo, lpPoints, cPoints);
}
extern "C" BOOL      __stdcall Shim_PtInRect(const RECT* lprc, POINT pt) { return ::PtInRect(lprc, pt); }
extern "C" HWND      __stdcall Shim_WindowFromPoint(POINT pt) { return ::WindowFromPoint(pt); }
extern "C" void      __stdcall Shim_TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack) { ::TrackMouseEvent(lpEventTrack); }

// ===========================================================================
// System colors
// ===========================================================================
extern "C" DWORD  __stdcall Shim_GetSysColor(int nIndex)              { return ::GetSysColor(nIndex); }
extern "C" HBRUSH __stdcall Shim_GetSysColorBrush(int nIndex)         { return ::GetSysColorBrush(nIndex); }

// ===========================================================================
// Messages / wait
// ===========================================================================
extern "C" UINT  __stdcall Shim_RegisterWindowMessageW(LPCWSTR lpString) { return ::RegisterWindowMessageW(lpString); }
extern "C" BOOL  __stdcall Shim_PostThreadMessageW(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam) {
    return ::PostThreadMessageW(idThread, Msg, wParam, lParam);
}
extern "C" DWORD __stdcall Shim_MsgWaitForMultipleObjects(DWORD nCount, const HANDLE* pHandles, BOOL fWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask) {
    return ::MsgWaitForMultipleObjects(nCount, pHandles, fWaitAll, dwMilliseconds, dwWakeMask);
}
extern "C" DWORD __stdcall Shim_MsgWaitForMultipleObjectsEx(DWORD nCount, const HANDLE* pHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags) {
    return ::MsgWaitForMultipleObjectsEx(nCount, pHandles, dwMilliseconds, dwWakeMask, dwFlags);
}
extern "C" DWORD __stdcall Shim_WaitForInputIdle(HANDLE hProcess, DWORD dwMilliseconds) {
    return ::WaitForInputIdle(hProcess, dwMilliseconds);
}

// ===========================================================================
// wsprintfW / wvsprintfW — variadic. wsprintf forwards to wvsprintf via va_list.
// ===========================================================================
extern "C" int __cdecl Shim_wsprintfW(LPWSTR buffer, LPCWSTR format, ...) {
    va_list args;
    va_start(args, format);
    int r = ::wvsprintfW(buffer, format, args);
    va_end(args);
    return r;
}

}  // namespace xwr

// ===========================================================================
// REGISTER_SHIM entries — 77 entries.
// ===========================================================================
#define XWR_REG_U32(name) \
    REGISTER_SHIM("user32", #name, (FARPROC)&xwr::Shim_##name);

// Keyboard layout / input (5)
XWR_REG_U32(ActivateKeyboardLayout)
XWR_REG_U32(AllowSetForegroundWindow)
XWR_REG_U32(GetKeyboardLayout)
XWR_REG_U32(GetKeyboardLayoutList)
XWR_REG_U32(GetMessageExtraInfo)

// Clipboard / hooks (5)
XWR_REG_U32(AddClipboardFormatListener)
XWR_REG_U32(CallNextHookEx)
XWR_REG_U32(SetWindowsHookExW)
XWR_REG_U32(UnhookWindowsHookEx)

// Display settings (5)
XWR_REG_U32(ChangeDisplaySettingsExW)
XWR_REG_U32(ChangeDisplaySettingsW)
XWR_REG_U32(EnumDisplayDevicesA)
XWR_REG_U32(EnumDisplayDevicesW)
XWR_REG_U32(EnumDisplaySettingsW)

// Display config stubs (3)
XWR_REG_U32(DisplayConfigGetDeviceInfo)
XWR_REG_U32(GetDisplayConfigBufferSizes)
XWR_REG_U32(QueryDisplayConfig)

// Touch stubs (3)
XWR_REG_U32(CloseTouchInputHandle)
XWR_REG_U32(GetTouchInputInfo)
XWR_REG_U32(RegisterTouchWindow)

// Raw input stubs (4)
XWR_REG_U32(GetRawInputData)
XWR_REG_U32(GetRawInputDeviceInfoA)
XWR_REG_U32(GetRawInputDeviceList)
XWR_REG_U32(RegisterRawInputDevices)

// SendInput stub (1)
XWR_REG_U32(SendInput)

// Dialogs / accelerators / menus (8)
XWR_REG_U32(CreateDialogParamW)
XWR_REG_U32(DestroyAcceleratorTable)
XWR_REG_U32(IsDialogMessageW)
XWR_REG_U32(LoadAcceleratorsW)
XWR_REG_U32(GetSystemMenu)
XWR_REG_U32(EnableMenuItem)
XWR_REG_U32(InsertMenuItemW)
XWR_REG_U32(TrackPopupMenu)

// Window state / classification (19)
XWR_REG_U32(GetActiveWindow)
XWR_REG_U32(SetActiveWindow)
XWR_REG_U32(GetCapture)
XWR_REG_U32(SetCapture)
XWR_REG_U32(ReleaseCapture)
XWR_REG_U32(GetTopWindow)
XWR_REG_U32(GetDlgItem)
XWR_REG_U32(GetDlgItemTextW)
XWR_REG_U32(SetDlgItemTextW)
XWR_REG_U32(EnableWindow)
XWR_REG_U32(IsIconic)
XWR_REG_U32(IsZoomed)
XWR_REG_U32(FlashWindowEx)
XWR_REG_U32(FindWindowExA)
XWR_REG_U32(EnumChildWindows)
XWR_REG_U32(GetClassLongPtrW)
XWR_REG_U32(GetWindowInfo)
XWR_REG_U32(GetWindowDisplayAffinity)
XWR_REG_U32(SetWindowDisplayAffinity)

// Layered / ghost / unregister (3)
XWR_REG_U32(SetLayeredWindowAttributes)
XWR_REG_U32(DisableProcessWindowsGhosting)
XWR_REG_U32(UnregisterClassA)

// Window station / user object (2)
XWR_REG_U32(GetProcessWindowStation)
XWR_REG_U32(GetUserObjectInformationW)

// DC / monitor / icon / drawing (12)
XWR_REG_U32(GetDCEx)
XWR_REG_U32(GetMonitorInfoA)
XWR_REG_U32(MonitorFromRect)
XWR_REG_U32(CreateIconIndirect)
XWR_REG_U32(DrawIconEx)
XWR_REG_U32(DrawStateW)
XWR_REG_U32(LoadBitmapW)
XWR_REG_U32(LoadCursorFromFileW)
XWR_REG_U32(MapWindowPoints)
XWR_REG_U32(PtInRect)
XWR_REG_U32(WindowFromPoint)
XWR_REG_U32(TrackMouseEvent)

// System colors (2)
XWR_REG_U32(GetSysColor)
XWR_REG_U32(GetSysColorBrush)

// Messages / wait (5)
XWR_REG_U32(RegisterWindowMessageW)
XWR_REG_U32(PostThreadMessageW)
XWR_REG_U32(MsgWaitForMultipleObjects)
XWR_REG_U32(MsgWaitForMultipleObjectsEx)
XWR_REG_U32(WaitForInputIdle)

// String format (1)
XWR_REG_U32(wsprintfW)

#undef XWR_REG_U32
