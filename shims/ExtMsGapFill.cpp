// shims/ExtMsGapFill.cpp
//
// Gap fill for ext-ms-win-* API-set DLLs that the coverage scanner
// reported as 0% covered. Closes 5 DLLs / ~221 REGISTER_SHIM entries:
//
//   ext-ms-win-wer-reporting-l1-1-0.dll          5  WER stubs (S_OK)
//   ext-ms-win-dxcore-l1-1-0.dll                 1  DXCoreCreateAdapterFactory pass-through
//   ext-ms-win-dx-d3dkmt-dxcore-l1-1-0.dll     212  ordinals 2..213 — DXGK kernel-mode stubs
//   ext-ms-win-dx-d3dkmt-dxcore-l1-1-1.dll       2  ordinals 220 / 221 — same DXGK stub
//   ext-ms-win-ntuser-uicontext-ext-l1-1-0.dll   1  ordinal 2521 — UI-context stub
//
// Rationale:
//   * WER (Windows Error Reporting) is unavailable inside an AppContainer
//     UWP game. We stub WerReportCreate / WerReportAddDump /
//     WerReportSetParameter / WerReportSubmit / WerReportCloseHandle to
//     return S_OK unconditionally; WerReportCreate also writes a fake
//     non-null HREPORT so callers that treat a null handle as failure
//     continue running. No crash dumps ever get uploaded — but no game
//     crashes harder than it needs to either.
//   * DXCoreCreateAdapterFactory is a real UWP API backed by dxcore.lib.
//     We forward the call straight through on a real Windows build; under
//     XWR_SYNTAX_CHECK we can't link the .lib, so the stub path returns S_OK.
//   * The ext-ms-win-dx-d3dkmt-dxcore-* DLLs export the DXGK (DirectX
//     Graphics Kernel) interface — kernel-mode display driver functions
//     that UWP CANNOT call (they're reserved for driver code). D3D12 pulls
//     them in via its private import table. We stub every one of the
//     imported ordinals with a single generic stub returning
//     STATUS_NOT_IMPLEMENTED (0xC0000002).
//   * ext-ms-win-ntuser-uicontext-ext-l1-1-0!ordinal:2521 is a private
//     user32 UI-context helper; we stub it to return 0 (no context).
//
// REGISTER_SHIM count: 5 + 1 + 212 + 2 + 1 = 221 entries.
//
// Added by Task ID GAP-EXT-MS-WIN.

#include "UwpSdkIncludes.h"

// ---------------------------------------------------------------------------
// MSVC: pull in the SDK headers that <windows.h> does NOT include by default.
//   * <werapi.h>  — defines HREPORT + WER_REPORT_INFORMATION
//   * <dxcore.h>  — declares DXCoreCreateAdapterFactory (may not be present on
//                   every Windows SDK install; use __has_include to detect it)
// ---------------------------------------------------------------------------
#ifdef _MSC_VER
  #include <werapi.h>
  #if __has_include(<dxcore.h>)
    #include <dxcore.h>
    #pragma comment(lib, "dxcore.lib")
    #define XWR_HAS_DXCORE 1
  #endif
#endif

#include "ShimRegistry.h"

#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif
#ifndef STATUS_NOT_IMPLEMENTED
#define STATUS_NOT_IMPLEMENTED ((NTSTATUS)0xC0000002L)
#endif

// HREPORT isn't always defined by the SDK headers on every toolchain —
// define it locally if missing so the WER shim signatures resolve.
// On MSVC with <werapi.h> included, HREPORT is typedef'd there; the
// _WERAPI_H_ guard (defined by werapi.h) ensures we don't re-typedef.
#if !defined(HREPORT_DEFINED) && !defined(_WERAPI_H_) && !defined(__WERAPI_H__)
typedef HANDLE HREPORT, *PHREPORT;
#define HREPORT_DEFINED
#endif

namespace xwr {

// ===========================================================================
// 1) ext-ms-win-wer-reporting-l1-1-0.dll — 5 WER stubs (all return S_OK).
//    WerReportCreate writes a fake non-null HREPORT so a caller that
//    treats 0/null as failure continues; the other four just return S_OK.
// ===========================================================================
extern "C" HRESULT __stdcall Shim_WerReportCreate(PCWSTR /*pwzEventType*/,
                                                  const WER_REPORT_INFORMATION* /*pReportInformation*/,
                                                  HREPORT* phReportHandle) {
    if (phReportHandle) {
        // Fake non-null handle — never dereferenced by the shim layer.
        *phReportHandle = reinterpret_cast<HREPORT>(static_cast<uintptr_t>(0xDEADC0DEULL));
    }
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_WerReportAddDump(HREPORT, HANDLE, HANDLE, DWORD,
                                                   PVOID, PVOID, DWORD, DWORD) {
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_WerReportSetParameter(HREPORT, DWORD, PCWSTR, PCWSTR) {
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_WerReportSubmit(HREPORT, DWORD, DWORD, PDWORD) {
    return S_OK;
}
extern "C" HRESULT __stdcall Shim_WerReportCloseHandle(HREPORT) {
    return S_OK;
}

// ===========================================================================
// 2) ext-ms-win-dxcore-l1-1-0.dll — DXCoreCreateAdapterFactory.
//    Pass-through to the real UWP implementation (linked via dxcore.lib on
//    a real Windows build). Under XWR_SYNTAX_CHECK we can't link the .lib,
//    so the stub path returns S_OK with *ppvFactory = nullptr.
// ===========================================================================
extern "C" HRESULT __stdcall Shim_DXCoreCreateAdapterFactory(const IID& riid, void** ppvFactory) {
#ifndef XWR_SYNTAX_CHECK
  #if defined(XWR_HAS_DXCORE)
    return ::DXCoreCreateAdapterFactory(riid, ppvFactory);
  #else
    // <dxcore.h> isn't available on this toolchain — fall back to a stub.
    (void)riid;
    if (ppvFactory) *ppvFactory = nullptr;
    return E_NOTIMPL;
  #endif
#else
    (void)riid;
    if (ppvFactory) *ppvFactory = nullptr;
    return S_OK;
#endif
}

// ===========================================================================
// 3-5) DXGK / ntuser-uicontext ordinal stubs.
//     Shim_DxgkStub returns STATUS_NOT_IMPLEMENTED; Shim_NtUserUiContext_Stub
//     returns 0 (no UI context — caller treats as "no context available").
//     Each ordinal from the gap scan is registered against the same stub.
// ===========================================================================
extern "C" NTSTATUS __stdcall Shim_DxgkStub() {
    return STATUS_NOT_IMPLEMENTED;  /* 0xC0000002 */
}
extern "C" LONG __stdcall Shim_NtUserUiContext_Stub() {
    return 0;
}

}  // namespace xwr

// ===========================================================================
// REGISTER_SHIM entries
// ===========================================================================

// ---------------------------------------------------------------------------
// 1) ext-ms-win-wer-reporting-l1-1-0.dll (5 WER functions)
// ---------------------------------------------------------------------------
REGISTER_SHIM("ext-ms-win-wer-reporting-l1-1-0", "WerReportCreate",      (FARPROC)&xwr::Shim_WerReportCreate);
REGISTER_SHIM("ext-ms-win-wer-reporting-l1-1-0", "WerReportAddDump",     (FARPROC)&xwr::Shim_WerReportAddDump);
REGISTER_SHIM("ext-ms-win-wer-reporting-l1-1-0", "WerReportSetParameter", (FARPROC)&xwr::Shim_WerReportSetParameter);
REGISTER_SHIM("ext-ms-win-wer-reporting-l1-1-0", "WerReportSubmit",      (FARPROC)&xwr::Shim_WerReportSubmit);
REGISTER_SHIM("ext-ms-win-wer-reporting-l1-1-0", "WerReportCloseHandle", (FARPROC)&xwr::Shim_WerReportCloseHandle);

// ---------------------------------------------------------------------------
// 2) ext-ms-win-dxcore-l1-1-0.dll (1 function)
// ---------------------------------------------------------------------------
REGISTER_SHIM("ext-ms-win-dxcore-l1-1-0", "DXCoreCreateAdapterFactory", (FARPROC)&xwr::Shim_DXCoreCreateAdapterFactory);

// ---------------------------------------------------------------------------
// 3) ext-ms-win-dx-d3dkmt-dxcore-l1-1-0.dll (ordinals 2..213 inclusive)
//    All 89 actual gap-scan imports live inside this range; over-registering
//    the other ordinals is harmless (registry lookups are O(1) and the extra
//    slots just sit unused). Each ordinal resolves to the same generic
//    Shim_DxgkStub() that returns STATUS_NOT_IMPLEMENTED.
// ---------------------------------------------------------------------------
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:2", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:3", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:4", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:5", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:6", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:7", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:8", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:9", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:10", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:11", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:12", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:13", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:14", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:15", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:16", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:17", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:18", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:19", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:20", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:21", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:22", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:23", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:24", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:25", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:26", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:27", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:28", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:29", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:30", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:31", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:32", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:33", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:34", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:35", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:36", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:37", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:38", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:39", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:40", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:41", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:42", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:43", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:44", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:45", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:46", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:47", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:48", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:49", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:50", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:51", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:52", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:53", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:54", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:55", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:56", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:57", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:58", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:59", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:60", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:61", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:62", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:63", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:64", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:65", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:66", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:67", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:68", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:69", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:70", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:71", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:72", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:73", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:74", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:75", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:76", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:77", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:78", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:79", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:80", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:81", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:82", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:83", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:84", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:85", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:86", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:87", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:88", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:89", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:90", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:91", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:92", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:93", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:94", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:95", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:96", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:97", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:98", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:99", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:100", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:101", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:102", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:103", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:104", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:105", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:106", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:107", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:108", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:109", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:110", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:111", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:112", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:113", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:114", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:115", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:116", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:117", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:118", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:119", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:120", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:121", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:122", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:123", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:124", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:125", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:126", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:127", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:128", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:129", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:130", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:131", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:132", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:133", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:134", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:135", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:136", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:137", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:138", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:139", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:140", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:141", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:142", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:143", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:144", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:145", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:146", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:147", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:148", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:149", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:150", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:151", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:152", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:153", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:154", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:155", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:156", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:157", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:158", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:159", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:160", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:161", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:162", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:163", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:164", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:165", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:166", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:167", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:168", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:169", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:170", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:171", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:172", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:173", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:174", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:175", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:176", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:177", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:178", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:179", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:180", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:181", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:182", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:183", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:184", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:185", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:186", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:187", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:188", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:189", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:190", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:191", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:192", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:193", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:194", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:195", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:196", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:197", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:198", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:199", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:200", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:201", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:202", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:203", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:204", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:205", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:206", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:207", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:208", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:209", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:210", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:211", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:212", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-0", "ordinal:213", (FARPROC)&xwr::Shim_DxgkStub);

// ---------------------------------------------------------------------------
// 4) ext-ms-win-dx-d3dkmt-dxcore-l1-1-1.dll (ordinals 220 / 221)
// ---------------------------------------------------------------------------
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-1", "ordinal:220", (FARPROC)&xwr::Shim_DxgkStub);
REGISTER_SHIM("ext-ms-win-dx-d3dkmt-dxcore-l1-1-1", "ordinal:221", (FARPROC)&xwr::Shim_DxgkStub);

// ---------------------------------------------------------------------------
// 5) ext-ms-win-ntuser-uicontext-ext-l1-1-0.dll (ordinal 2521)
// ---------------------------------------------------------------------------
REGISTER_SHIM("ext-ms-win-ntuser-uicontext-ext-l1-1-0", "ordinal:2521", (FARPROC)&xwr::Shim_NtUserUiContext_Stub);

// End of ExtMsGapFill.cpp
