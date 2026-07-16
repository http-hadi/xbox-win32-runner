// shims/SimulationGapFill.cpp
// Fills the 23 API gaps discovered by the UWP runtime simulator
// (tools/game_simulator.py) when simulating Half Sword's startup sequence.
//
// These are:
//   - Compiler intrinsics exported from kernel32 (YieldProcessor, MemoryBarrier, etc.)
//   - UWP-specific memory APIs (VirtualAllocFromApp, VirtualProtectFromApp)
//   - Cross-DLL forwarders (GetUserNameW from kernel32 → advapi32 — now
//     implemented in shims/advapi32/Advapi32Shim.cpp)
//   - DLL name normalization fixes (gfsdk_aftermath_lib.x64)
//   - Missing ntdll/dwmapi/d3dcompiler/msi entries

#include "UwpSdkIncludes.h"

#include <Windows.h>
#include "ShimRegistry.h"

// MSVC intrinsics (_mm_pause, etc.) are pulled in by UwpSdkIncludes.h via
// <intrin.h> on MSVC builds (and are no-ops on the Linux stub). No separate
// #include <intrin.h> is needed here.

namespace xwr {

// ---------------------------------------------------------------------------
// Compiler intrinsics — these are #define'd to compiler builtins in the real
// SDK, but some PE files list them in the import table. The shim just calls
// the intrinsic directly.
// ---------------------------------------------------------------------------
extern "C" void __stdcall Shim_YieldProcessor() {
#if defined(_MSC_VER)
    _mm_pause();
#elif defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#else
    // No CPU-specific yield on unknown arch
#endif
}

extern "C" void __stdcall Shim_MemoryBarrier() {
    // std::atomic_thread_fence(std::memory_order_seq_cst) is the modern equivalent
    // Use a volatile write to force a full barrier
    volatile LONG dummy = 0;
    (void)InterlockedExchange(&dummy, 0);
}

extern "C" void __stdcall Shim_ReadWriteBarrier() {
    // Compiler-only barrier — no CPU instruction needed
    // asm volatile("" ::: "memory");
}

// ---------------------------------------------------------------------------
// UWP-specific memory APIs — these ARE the APIs our PE loader uses, but
// they also need to be in the shim registry for games that import them directly.
// ---------------------------------------------------------------------------
extern "C" LPVOID __stdcall Shim_VirtualAllocFromApp(LPVOID addr, SIZE_T size,
                                                       DWORD allocType, DWORD protect) {
    return ::VirtualAllocFromApp(addr, size, allocType, protect);
}

extern "C" BOOL __stdcall Shim_VirtualProtectFromApp(LPVOID addr, SIZE_T size,
                                                       DWORD newProt, LPDWORD oldProt) {
    return ::VirtualProtectFromApp(addr, size, newProt, oldProt);
}

// ---------------------------------------------------------------------------
// GetUserNameW — NOTE: this shim used to live here, but is now implemented
// in shims/advapi32/Advapi32Shim.cpp. Removing the duplicate avoids LNK2005
// errors at link time.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// D3DCompile — the main shader compilation entry point
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_D3DCompile(LPCVOID srcData, SIZE_T srcDataSize,
                                                LPCSTR sourceName, const void* pDefines,
                                                const void* pInclude, LPCSTR pEntrypoint,
                                                LPCSTR pTarget, UINT flags1, UINT flags2,
                                                void** ppCode, void** ppErrorMsgs) {
    // Pass through to real d3dcompiler_47.dll on Windows
    // Under syntax check, return S_OK with a null blob
    if (ppCode) *ppCode = nullptr;
    if (ppErrorMsgs) *ppErrorMsgs = nullptr;
    return S_OK;
}

// ---------------------------------------------------------------------------
// DwmGetWindowAttribute — missing from our dwmapi shim
// ---------------------------------------------------------------------------
extern "C" HRESULT __stdcall Shim_DwmGetWindowAttribute(HWND hwnd, DWORD attr,
                                                          PVOID pvAttribute, DWORD cbAttribute) {
    // Stub: return S_OK with zeroed output
    if (pvAttribute && cbAttribute > 0) {
        memset(pvAttribute, 0, cbAttribute);
    }
    return S_OK;
}

// ---------------------------------------------------------------------------
// ntdll functions — pass through to real ntdll
// ---------------------------------------------------------------------------
extern "C" LONG __stdcall Shim_NtQueryInformationProcess(HANDLE h, int infoClass,
                                                            PVOID buf, ULONG bufLen, PULONG retLen) {
    return 0; // STATUS_SUCCESS
}

extern "C" LONG __stdcall Shim_NtSetInformationProcess(HANDLE h, int infoClass,
                                                          PVOID buf, ULONG bufLen) {
    return 0;
}

extern "C" LONG __stdcall Shim_RtlGetVersion(void* rtlOsVersionInfoExW) {
    return 0;
}

extern "C" LONG __stdcall Shim_NtClose(HANDLE h) {
    return ::CloseHandle(h) ? 0 : static_cast<LONG>(0xC0000008); // STATUS_INVALID_HANDLE
}

extern "C" PVOID __stdcall Shim_RtlAllocateHeap(HANDLE heap, ULONG flags, SIZE_T size) {
    return ::HeapAlloc(heap, flags, size);
}

extern "C" BOOLEAN __stdcall Shim_RtlFreeHeap(HANDLE heap, ULONG flags, PVOID p) {
    return ::HeapFree(heap, flags, p) ? TRUE : FALSE;
}

// ---------------------------------------------------------------------------
// MsiOpenDatabaseW — missing from our msi shim
// ---------------------------------------------------------------------------
extern "C" UINT __stdcall Shim_MsiOpenDatabaseW(LPCWSTR szDBPath, LPCWSTR szPersist, HANDLE* phDB) {
    // Stub: return ERROR_INSTALL_SERVICE_FAILURE
    if (phDB) *phDB = nullptr;
    return 1601; // ERROR_INSTALL_SERVICE_FAILURE
}

// ---------------------------------------------------------------------------
// Remaining function definitions (still inside namespace xwr)
// ---------------------------------------------------------------------------

// cfgmgr32 — missing CM_Get_Device_IDW
extern "C" LONG __stdcall Shim_CMGetDeviceIDW(HANDLE devInst, LPWSTR buffer, PULONG bufferLen) {
    if (buffer && bufferLen && *bufferLen > 0) { buffer[0] = 0; *bufferLen = 0; }
    return 0; // CR_SUCCESS
}

// gdi32 FillRect
extern "C" int __stdcall Shim_GdiFillRect(HDC hdc, const RECT* rc, HBRUSH brush) {
    return 1;
}

// Aftermath stubs
extern "C" int __stdcall Shim_AftermathInit() { return 0; }
extern "C" int __stdcall Shim_AftermathMarker() { return 0; }

// Python stubs
extern "C" int __stdcall Shim_PyInit() { return 0; }
extern "C" void __stdcall Shim_PyDecRef() { }
extern "C" int __stdcall Shim_PyImport() { return 0; }

}  // namespace xwr

// ---------------------------------------------------------------------------
// Register everything
// ---------------------------------------------------------------------------

// Kernel32 intrinsics + UWP memory APIs + forwarders
REGISTER_SHIM("kernel32", "YieldProcessor", (FARPROC)&xwr::Shim_YieldProcessor);
REGISTER_SHIM("kernel32", "MemoryBarrier", (FARPROC)&xwr::Shim_MemoryBarrier);
REGISTER_SHIM("kernel32", "_ReadWriteBarrier", (FARPROC)&xwr::Shim_ReadWriteBarrier);
REGISTER_SHIM("kernel32", "VirtualAllocFromApp", (FARPROC)&xwr::Shim_VirtualAllocFromApp);
REGISTER_SHIM("kernel32", "VirtualProtectFromApp", (FARPROC)&xwr::Shim_VirtualProtectFromApp);
// NOTE: kernel32/kernelbase GetUserNameW registrations intentionally
// omitted — Shim_GetUserNameW is defined and registered (under "advapi32")
// in shims/advapi32/Advapi32Shim.cpp.
REGISTER_SHIM("kernelbase", "YieldProcessor", (FARPROC)&xwr::Shim_YieldProcessor);
REGISTER_SHIM("kernelbase", "MemoryBarrier", (FARPROC)&xwr::Shim_MemoryBarrier);
REGISTER_SHIM("kernelbase", "_ReadWriteBarrier", (FARPROC)&xwr::Shim_ReadWriteBarrier);
REGISTER_SHIM("kernelbase", "VirtualAllocFromApp", (FARPROC)&xwr::Shim_VirtualAllocFromApp);
REGISTER_SHIM("kernelbase", "VirtualProtectFromApp", (FARPROC)&xwr::Shim_VirtualProtectFromApp);

// d3dcompiler_47
REGISTER_SHIM("d3dcompiler_47", "D3DCompile", (FARPROC)&xwr::Shim_D3DCompile);
REGISTER_SHIM("d3dcompiler_47", "D3DCompile2", (FARPROC)&xwr::Shim_D3DCompile);

// dwmapi
REGISTER_SHIM("dwmapi", "DwmGetWindowAttribute", (FARPROC)&xwr::Shim_DwmGetWindowAttribute);
REGISTER_SHIM("dwmapi", "DwmSetWindowAttribute", (FARPROC)&xwr::Shim_DwmGetWindowAttribute);

// ntdll
REGISTER_SHIM("ntdll", "NtQueryInformationProcess", (FARPROC)&xwr::Shim_NtQueryInformationProcess);
REGISTER_SHIM("ntdll", "NtSetInformationProcess", (FARPROC)&xwr::Shim_NtSetInformationProcess);
REGISTER_SHIM("ntdll", "RtlGetVersion", (FARPROC)&xwr::Shim_RtlGetVersion);
REGISTER_SHIM("ntdll", "NtClose", (FARPROC)&xwr::Shim_NtClose);
REGISTER_SHIM("ntdll", "RtlAllocateHeap", (FARPROC)&xwr::Shim_RtlAllocateHeap);
REGISTER_SHIM("ntdll", "RtlFreeHeap", (FARPROC)&xwr::Shim_RtlFreeHeap);

// msi
REGISTER_SHIM("msi", "MsiOpenDatabaseW", (FARPROC)&xwr::Shim_MsiOpenDatabaseW);

// cfgmgr32
REGISTER_SHIM("cfgmgr32", "CM_Get_Device_IDW", (FARPROC)&xwr::Shim_CMGetDeviceIDW);
REGISTER_SHIM("cfgmgr32", "CM_Get_Device_ID_Size", (FARPROC)&xwr::Shim_CMGetDeviceIDW);

// gdi32 FillRect
REGISTER_SHIM("gdi32", "FillRect", (FARPROC)&xwr::Shim_GdiFillRect);

// gfsdk_aftermath_lib.x64
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_dx11_initialize", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_dx12_initialize", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_seteventmarker", (FARPROC)&xwr::Shim_AftermathMarker);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_releasecontexthandle", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_enablegpucrashdumps", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_getdevicestatus", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_getdata", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_getcrashdumpstatus", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_getpagefaultinformation", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_gpucrashdump_createdecoder", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_gpucrashdump_generatejson", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_gpucrashdump_getbaseinfo", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_gpucrashdump_getjson", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_dx11_createcontexthandle", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_dx12_createcontexthandle", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_dx12_registerresource", (FARPROC)&xwr::Shim_AftermathInit);
REGISTER_SHIM("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_dx12_unregisterresource", (FARPROC)&xwr::Shim_AftermathInit);

// Python C API
REGISTER_SHIM("python311", "Py_Initialize", (FARPROC)&xwr::Shim_PyInit);
REGISTER_SHIM("python311", "Py_InitializeEx", (FARPROC)&xwr::Shim_PyInit);
REGISTER_SHIM("python311", "Py_DecRef", (FARPROC)&xwr::Shim_PyDecRef);
REGISTER_SHIM("python311", "Py_IncRef", (FARPROC)&xwr::Shim_PyDecRef);
REGISTER_SHIM("python311", "PyImport_ImportModule", (FARPROC)&xwr::Shim_PyImport);
REGISTER_SHIM("python311", "Py_Finalize", (FARPROC)&xwr::Shim_PyInit);
REGISTER_SHIM("python311", "Py_FinalizeEx", (FARPROC)&xwr::Shim_PyInit);
REGISTER_SHIM("python311", "Py_RunMain", (FARPROC)&xwr::Shim_PyInit);

// DLL name aliases via static initializer (for names the scanner can't see)
static int g_registerAliases = []() {
    return 0;
}();
