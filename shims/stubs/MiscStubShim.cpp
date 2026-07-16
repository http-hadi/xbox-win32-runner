// shims/stubs/MiscStubShim.cpp
// Catch-all stubs for misc legacy DLLs that games sometimes import:
//   - avrt:           AvSetMmThreadCharacteristics / AvRevertMmThreadCharacteristics
//   - setupapi:       device enumeration — always fails
//   - netapi32:       NetWkstaUserGetInfo etc.
//   - qwave:          QoS — always fails
//   - ktmw32:         transaction manager — always fails
//   - uiautomationcore: accessibility — always fails
//   - dxcore:         newer DXGI factory — returns E_NOTIMPL
// All return failure values so the caller can detect "feature not present".

#include "UwpSdkIncludes.h"


#include <atomic>

#include "../ShimRegistry.h"

#ifndef ERROR_INVALID_FUNCTION
#define ERROR_INVALID_FUNCTION 1L
#endif
#ifndef ERROR_NO_MORE_ITEMS
#define ERROR_NO_MORE_ITEMS 259L
#endif
#ifndef NERR_Success
#define NERR_Success 0
#endif
#ifndef NERR_BufTooSmall
#define NERR_BufTooSmall 2123
#endif

namespace xwr {

// ---------------------------------------------------------------------------
// avrt — Multimedia class scheduling. Real impl would tag the thread.
// Return a fake handle; AvRevertMmThreadCharacteristics accepts it.
// ---------------------------------------------------------------------------
static std::atomic<uint32_t> g_avrtHandle{0x1000};
extern "C" HANDLE __stdcall Shim_AvSetMmThreadCharacteristicsW(LPCWSTR, LPDWORD lpTaskIndex) {
    if (lpTaskIndex) *lpTaskIndex = 0;
    return (HANDLE)(uintptr_t)g_avrtHandle.fetch_add(1);
}
extern "C" HANDLE __stdcall Shim_AvSetMmThreadCharacteristicsA(LPCSTR, LPDWORD lpTaskIndex) {
    if (lpTaskIndex) *lpTaskIndex = 0;
    return (HANDLE)(uintptr_t)g_avrtHandle.fetch_add(1);
}
extern "C" BOOL __stdcall Shim_AvRevertMmThreadCharacteristics(HANDLE) { return TRUE; }
extern "C" BOOL __stdcall Shim_AvSetMmThreadPriority(HANDLE, int) { return TRUE; }
extern "C" BOOL __stdcall Shim_AvRtCreateThreadOrderingGroup(HANDLE*, PLARGE_INTEGER, void*, void*) { return FALSE; }
extern "C" BOOL __stdcall Shim_AvRtJoinThreadOrderingGroup(HANDLE*, void*, BOOL) { return FALSE; }
extern "C" BOOL __stdcall Shim_AvRtLeaveThreadOrderingGroup(HANDLE) { return FALSE; }
extern "C" BOOL __stdcall Shim_AvRtDeleteThreadOrderingGroup(HANDLE) { return FALSE; }
extern "C" BOOL __stdcall Shim_AvRtWaitOnThreadOrderingGroup(HANDLE) { return FALSE; }

// ---------------------------------------------------------------------------
// setupapi — device enumeration. UWP doesn't expose this; always fail.
// Use the real SDK types (PSP_DEVICE_INTERFACE_DATA, PSP_DEVINFO_DATA, etc.)
// from <setupapi.h> so MSVC accepts the signatures.
// ---------------------------------------------------------------------------
extern "C" HDEVINFO __stdcall Shim_SetupDiGetClassDevsW(const GUID*, LPCWSTR, HWND, DWORD) {
    return (HDEVINFO)INVALID_HANDLE_VALUE;
}
extern "C" HDEVINFO __stdcall Shim_SetupDiGetClassDevsA(const GUID*, LPCSTR, HWND, DWORD) {
    return (HDEVINFO)INVALID_HANDLE_VALUE;
}
extern "C" BOOL __stdcall Shim_SetupDiDestroyDeviceInfoList(HDEVINFO) { return TRUE; }
extern "C" BOOL __stdcall Shim_SetupDiEnumDeviceInterfaces(HDEVINFO, PSP_DEVINFO_DATA, const GUID*, DWORD, PSP_DEVICE_INTERFACE_DATA) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetupDiGetDeviceInterfaceDetailW(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_W, DWORD, LPDWORD, PSP_DEVINFO_DATA) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetupDiGetDeviceInterfaceDetailA(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A, DWORD, LPDWORD, PSP_DEVINFO_DATA) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetupDiGetDeviceRegistryPropertyW(HDEVINFO, PSP_DEVINFO_DATA, DWORD, LPDWORD, PBYTE, DWORD, LPDWORD) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetupDiGetDeviceRegistryPropertyA(HDEVINFO, PSP_DEVINFO_DATA, DWORD, LPDWORD, PBYTE, DWORD, LPDWORD) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetupDiBuildDriverInfoList(HDEVINFO, PSP_DEVINFO_DATA, DWORD) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetupDiEnumDriverInfoW(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, PSP_DRVINFO_DATA_W) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetupDiEnumDriverInfoA(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, PSP_DRVINFO_DATA_A) { return FALSE; }
extern "C" BOOL __stdcall Shim_SetupDiDestroyDriverInfoList(HDEVINFO, PSP_DEVINFO_DATA, DWORD) { return TRUE; }
extern "C" BOOL __stdcall Shim_SetupDiCallClassInstaller(DWORD, HDEVINFO, PSP_DEVINFO_DATA) { return FALSE; }

// ---------------------------------------------------------------------------
// netapi32 — workstation user info.
// ---------------------------------------------------------------------------
extern "C" DWORD __stdcall Shim_NetWkstaUserGetInfo(LPCWSTR, DWORD, LPBYTE* lpBuffer) {
    (void)lpBuffer;
    return NERR_BufTooSmall;
}
extern "C" DWORD __stdcall Shim_NetWkstaUserGetInfo_local(DWORD, LPBYTE* lpBuffer) {
    (void)lpBuffer;
    return NERR_BufTooSmall;
}
extern "C" DWORD __stdcall Shim_NetWkstaGetInfo(LPCWSTR, DWORD, LPBYTE* lpBuffer) {
    (void)lpBuffer;
    return NERR_BufTooSmall;
}
extern "C" DWORD __stdcall Shim_NetApiBufferFree(LPVOID Buffer) { (void)Buffer; return NERR_Success; }
extern "C" DWORD __stdcall Shim_NetApiBufferAllocate(DWORD ByteCount, LPVOID* Buffer) {
    (void)ByteCount;
    if (Buffer) *Buffer = nullptr;
    return NERR_BufTooSmall;
}
extern "C" DWORD __stdcall Shim_NetUserGetInfo(LPCWSTR, LPCWSTR, DWORD, LPBYTE* lpBuffer) {
    (void)lpBuffer;
    return NERR_BufTooSmall;
}

// ---------------------------------------------------------------------------
// qwave — QoS. UWP doesn't expose this.
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_QOSCreateHandle(DWORD, PHANDLE phHandle) {
    if (phHandle) *phHandle = nullptr;
    return FALSE;
}
extern "C" BOOL __stdcall Shim_QOSCloseHandle(HANDLE) { return TRUE; }
extern "C" BOOL __stdcall Shim_QOSAddSocketToFlow(HANDLE, SOCKET, void*, DWORD, void*, void*) { return FALSE; }
extern "C" BOOL __stdcall Shim_QOSRemoveSocketFromFlow(HANDLE, SOCKET, void*) { return FALSE; }
extern "C" BOOL __stdcall Shim_QOSSetFlow(HANDLE, void*, DWORD, ULONG, void*, LPVOID, LPOVERLAPPED) { return FALSE; }

// ---------------------------------------------------------------------------
// ktmw32 — kernel transaction manager.
// ---------------------------------------------------------------------------
extern "C" HANDLE __stdcall Shim_CreateTransaction(void*, void*, DWORD, DWORD, DWORD, DWORD, void*) {
    return (HANDLE)INVALID_HANDLE_VALUE;
}
extern "C" BOOL __stdcall Shim_CommitTransaction(HANDLE) { return FALSE; }
extern "C" BOOL __stdcall Shim_RollbackTransaction(HANDLE) { return FALSE; }
extern "C" BOOL __stdcall Shim_CloseTransaction(HANDLE) { return TRUE; }
extern "C" HANDLE __stdcall Shim_OpenTransaction(DWORD, BOOL, void*) {
    return (HANDLE)INVALID_HANDLE_VALUE;
}

// ---------------------------------------------------------------------------
// uiautomationcore — accessibility.
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall Shim_UiaReturnRawElementProvider(HWND, WPARAM, LPARAM, void*) { return FALSE; }
extern "C" HRESULT __stdcall Shim_UiaGetRootNode(void**) { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_UiaHostProviderFromHwnd(HWND, void**) { return E_NOTIMPL; }
extern "C" HRESULT __stdcall Shim_UiaRaiseAutomationEvent(void*, int) { return E_NOTIMPL; }

// ---------------------------------------------------------------------------
// dxcore — newer factory abstraction. Not needed; we have DXGI via the bridge.
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_DXCoreCreateAdapterFactory(const IID& iid, void** ppFactory) {
    (void)iid;
    if (ppFactory) *ppFactory = nullptr;
    return E_NOTIMPL;
}

}  // namespace xwr

// ===========================================================================
// Registration — avrt
// ===========================================================================
REGISTER_SHIM("avrt", "AvSetMmThreadCharacteristicsW", (FARPROC)&xwr::Shim_AvSetMmThreadCharacteristicsW);
REGISTER_SHIM("avrt", "AvSetMmThreadCharacteristicsA", (FARPROC)&xwr::Shim_AvSetMmThreadCharacteristicsA);
REGISTER_SHIM("avrt", "AvRevertMmThreadCharacteristics", (FARPROC)&xwr::Shim_AvRevertMmThreadCharacteristics);
REGISTER_SHIM("avrt", "AvSetMmThreadPriority", (FARPROC)&xwr::Shim_AvSetMmThreadPriority);
REGISTER_SHIM("avrt", "AvRtCreateThreadOrderingGroup", (FARPROC)&xwr::Shim_AvRtCreateThreadOrderingGroup);
REGISTER_SHIM("avrt", "AvRtJoinThreadOrderingGroup", (FARPROC)&xwr::Shim_AvRtJoinThreadOrderingGroup);
REGISTER_SHIM("avrt", "AvRtLeaveThreadOrderingGroup", (FARPROC)&xwr::Shim_AvRtLeaveThreadOrderingGroup);
REGISTER_SHIM("avrt", "AvRtDeleteThreadOrderingGroup", (FARPROC)&xwr::Shim_AvRtDeleteThreadOrderingGroup);
REGISTER_SHIM("avrt", "AvRtWaitOnThreadOrderingGroup", (FARPROC)&xwr::Shim_AvRtWaitOnThreadOrderingGroup);

// ===========================================================================
// Registration — setupapi
// ===========================================================================
REGISTER_SHIM("setupapi", "SetupDiGetClassDevsW", (FARPROC)&xwr::Shim_SetupDiGetClassDevsW);
REGISTER_SHIM("setupapi", "SetupDiGetClassDevsA", (FARPROC)&xwr::Shim_SetupDiGetClassDevsA);
REGISTER_SHIM("setupapi", "SetupDiDestroyDeviceInfoList", (FARPROC)&xwr::Shim_SetupDiDestroyDeviceInfoList);
REGISTER_SHIM("setupapi", "SetupDiEnumDeviceInterfaces", (FARPROC)&xwr::Shim_SetupDiEnumDeviceInterfaces);
REGISTER_SHIM("setupapi", "SetupDiGetDeviceInterfaceDetailW", (FARPROC)&xwr::Shim_SetupDiGetDeviceInterfaceDetailW);
REGISTER_SHIM("setupapi", "SetupDiGetDeviceInterfaceDetailA", (FARPROC)&xwr::Shim_SetupDiGetDeviceInterfaceDetailA);
REGISTER_SHIM("setupapi", "SetupDiGetDeviceRegistryPropertyW", (FARPROC)&xwr::Shim_SetupDiGetDeviceRegistryPropertyW);
REGISTER_SHIM("setupapi", "SetupDiGetDeviceRegistryPropertyA", (FARPROC)&xwr::Shim_SetupDiGetDeviceRegistryPropertyA);
REGISTER_SHIM("setupapi", "SetupDiBuildDriverInfoList", (FARPROC)&xwr::Shim_SetupDiBuildDriverInfoList);
REGISTER_SHIM("setupapi", "SetupDiEnumDriverInfoW", (FARPROC)&xwr::Shim_SetupDiEnumDriverInfoW);
REGISTER_SHIM("setupapi", "SetupDiEnumDriverInfoA", (FARPROC)&xwr::Shim_SetupDiEnumDriverInfoA);
REGISTER_SHIM("setupapi", "SetupDiDestroyDriverInfoList", (FARPROC)&xwr::Shim_SetupDiDestroyDriverInfoList);
REGISTER_SHIM("setupapi", "SetupDiCallClassInstaller", (FARPROC)&xwr::Shim_SetupDiCallClassInstaller);

// ===========================================================================
// Registration — netapi32
// ===========================================================================
REGISTER_SHIM("netapi32", "NetWkstaUserGetInfo", (FARPROC)&xwr::Shim_NetWkstaUserGetInfo);
REGISTER_SHIM("netapi32", "NetWkstaGetInfo", (FARPROC)&xwr::Shim_NetWkstaGetInfo);
REGISTER_SHIM("netapi32", "NetApiBufferFree", (FARPROC)&xwr::Shim_NetApiBufferFree);
REGISTER_SHIM("netapi32", "NetApiBufferAllocate", (FARPROC)&xwr::Shim_NetApiBufferAllocate);
REGISTER_SHIM("netapi32", "NetUserGetInfo", (FARPROC)&xwr::Shim_NetUserGetInfo);

// ===========================================================================
// Registration — qwave
// ===========================================================================
REGISTER_SHIM("qwave", "QOSCreateHandle", (FARPROC)&xwr::Shim_QOSCreateHandle);
REGISTER_SHIM("qwave", "QOSCloseHandle", (FARPROC)&xwr::Shim_QOSCloseHandle);
REGISTER_SHIM("qwave", "QOSAddSocketToFlow", (FARPROC)&xwr::Shim_QOSAddSocketToFlow);
REGISTER_SHIM("qwave", "QOSRemoveSocketFromFlow", (FARPROC)&xwr::Shim_QOSRemoveSocketFromFlow);
REGISTER_SHIM("qwave", "QOSSetFlow", (FARPROC)&xwr::Shim_QOSSetFlow);

// ===========================================================================
// Registration — ktmw32
// ===========================================================================
REGISTER_SHIM("ktmw32", "CreateTransaction", (FARPROC)&xwr::Shim_CreateTransaction);
REGISTER_SHIM("ktmw32", "CommitTransaction", (FARPROC)&xwr::Shim_CommitTransaction);
REGISTER_SHIM("ktmw32", "RollbackTransaction", (FARPROC)&xwr::Shim_RollbackTransaction);
REGISTER_SHIM("ktmw32", "CloseTransaction", (FARPROC)&xwr::Shim_CloseTransaction);
REGISTER_SHIM("ktmw32", "OpenTransaction", (FARPROC)&xwr::Shim_OpenTransaction);

// ===========================================================================
// Registration — uiautomationcore
// ===========================================================================
REGISTER_SHIM("uiautomationcore", "UiaReturnRawElementProvider", (FARPROC)&xwr::Shim_UiaReturnRawElementProvider);
REGISTER_SHIM("uiautomationcore", "UiaGetRootNode", (FARPROC)&xwr::Shim_UiaGetRootNode);
REGISTER_SHIM("uiautomationcore", "UiaHostProviderFromHwnd", (FARPROC)&xwr::Shim_UiaHostProviderFromHwnd);
REGISTER_SHIM("uiautomationcore", "UiaRaiseAutomationEvent", (FARPROC)&xwr::Shim_UiaRaiseAutomationEvent);

// ===========================================================================
// Registration — dxcore
// ===========================================================================
REGISTER_SHIM("dxcore", "DXCoreCreateAdapterFactory", (FARPROC)&xwr::Shim_DXCoreCreateAdapterFactory);
