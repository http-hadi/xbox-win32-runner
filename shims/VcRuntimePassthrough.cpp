// shims/VcRuntimePassthrough.cpp
//
// Pass-through registration for the Visual C++ runtime DLLs and the
// Universal CRT (api-ms-win-crt-*) API set DLLs.
//
// THE PROBLEM
// -----------
// Half Sword (and most modern Windows games built with MSVC) imports from:
//
//   MSVCP140.dll            (~258 functions: C++ stdlib — basic_iostream,
//                                                  basic_streambuf, locale,
//                                                  codecvt, ctype, etc.)
//   VCRUNTIME140.dll         (~32 functions: C runtime — _CxxThrowException,
//                                                  __C_specific_handler,
//                                                  memcpy/memset/strstr, etc.)
//   VCRUNTIME140_1.dll       (1 function: __CxxFrameHandler4)
//   MSVCP140_ATOMIC_WAIT.dll (5 functions: __std_*_threadpool_work)
//   api-ms-win-crt-*.dll     (14 DLLs, 380 functions: Universal CRT — fopen,
//                                                  malloc, strlen, _beginthreadex,
//                                                  __stdio_common_vfprintf, etc.)
//
// All of these are pure C/C++ runtime — they're not Win32 API surface and we
// can't reasonably reimplement them. The good news: UWP supports them
// natively (the Visual C++ Redistributable is AppContainer-compatible, and
// ucrtbase.dll ships in every Windows 10+ install). So the right answer is
// to pass these imports through to the real system DLLs at runtime.
//
// THE SOLUTION
// ------------
// This file does two things:
//
//   1) LITERAL REGISTER_SHIM PLACEHOLDERS (one per imported function)
//      For every (MSVCP140 / VCRUNTIME140 / VCRUNTIME140_1 /
//      MSVCP140_ATOMIC_WAIT / api-ms-win-crt-*, func) tuple that Half Sword
//      actually imports (extracted from /tmp/gaps/*_dll), we emit a literal
//      REGISTER_SHIM entry pointing at Shim_VcRuntime_Stub.
//      This makes the source-level coverage scanner
//      (tools/pe_import_scanner.py) see the entry — without it, the scanner
//      reports 0% coverage and flags the DLL as "uncovered".
//
//   2) RUNTIME PASSTHROUGH (xwr::ApplyVcRuntimePassthrough)
//      At PeLoader init time, we walk the ShimRegistry and for every entry
//      whose DLL is one of the VC runtime / CRT API set DLLs, we:
//         a) LoadLibraryW the real underlying system DLL
//            (MSVCP140.dll, VCRUNTIME140.dll, VCRUNTIME140_1.dll,
//             MSVCP140_ATOMIC_WAIT.dll, or ucrtbase.dll for api-ms-win-crt-*)
//         b) GetProcAddress the function by name
//         c) If found, re-register the (dll, func) tuple with the real
//            function pointer (overwriting the stub).
//
//      This means the runtime PE loader, when resolving a game's static
//      import of e.g. MSVCP140!??0?$basic_ios@..., finds the entry in the
//      ShimRegistry (because ApplyVcRuntimePassthrough already overwrote
//      the stub with the real FARPROC), and writes the real function
//      address into the game's IAT. The game then calls the real MSVCP140
//      function transparently.
//
//   3) PE LOADER FALLBACK (in PeLoader.cpp ResolveImport)
//      As a defense-in-depth, the PE loader's ResolveImport has a fallback
//      step: if a function from a known passthrough DLL (MSVCP140, etc.)
//      isn't in the ShimRegistry, it calls LoadLibraryW + GetProcAddress
//      directly. This handles the case where ApplyVcRuntimePassthrough
//      hasn't run yet, or where the function wasn't in /tmp/gaps (i.e.
//      not imported by Half Sword but imported by some other game).
//
// WHY NOT JUST USE LoadLibraryW VIA THE PE LOADER?
// ------------------------------------------------
// The PE loader's EnsureDependencyLoaded only looks at the game's DLL search
// dirs (m_searchDirs). It does NOT call the OS LoadLibraryW for system DLLs.
// So without the registration in step (1), the PE loader's HasModule() check
// returns false for MSVCP140, and LoadModule falls through to a file search
// that won't find MSVCP140.dll (it lives in System32, not the game dir).
// With the registration in step (1), HasModule() returns true and
// EnsureDependencyLoaded short-circuits — so we MUST also do step (2) to
// populate the FARPROC, otherwise the IAT slot stays pointing at the stub.
//
// Added by Task ID GAP-APISETS-VCRT.

#include "UwpSdkIncludes.h"
#include "UwpSdkIncludes.h"

#include <Windows.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ShimRegistry.h"

#ifndef ERROR_NOT_SUPPORTED
#define ERROR_NOT_SUPPORTED 50L
#endif
#ifndef ERROR_PROC_NOT_FOUND
#define ERROR_PROC_NOT_FOUND 127L
#endif
#ifndef ERROR_MOD_NOT_FOUND
#define ERROR_MOD_NOT_FOUND 126L
#endif

namespace xwr {

// Forward declarations — defined in ApiSetForwarder.cpp.
void ApplyApiSetForwarders();

// ---------------------------------------------------------------------------
// Stub function used as the placeholder FARPROC for every literal
// REGISTER_SHIM entry below. At runtime, ApplyVcRuntimePassthrough()
// overwrites each placeholder with the real function pointer fetched from
// the real system DLL via LoadLibraryW + GetProcAddress. If the passthrough
// hasn't run (or failed to find the function in the real DLL), this stub is
// invoked instead and returns 0 with GetLastError = ERROR_PROC_NOT_FOUND.
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_VcRuntime_Stub() {
    ::SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

// ---------------------------------------------------------------------------
// Mapping: shim DLL name (lowercased) -> real system DLL to LoadLibraryW.
//
// - The four VC runtime DLLs map to themselves (with .dll appended).
// - Every api-ms-win-crt-* DLL maps to ucrtbase.dll (the Universal CRT
//   host — Windows 10+ redirects all api-ms-win-crt-* lookups to it).
// ---------------------------------------------------------------------------
static const std::unordered_map<std::wstring, std::wstring>& VcRuntimeRealDllMap() {
    static const std::unordered_map<std::wstring, std::wstring> m = {
        { L"msvcp140",               L"MSVCP140.dll" },
        { L"vcruntime140",           L"VCRUNTIME140.dll" },
        { L"vcruntime140_1",         L"VCRUNTIME140_1.dll" },
        { L"msvcp140_atomic_wait",   L"MSVCP140_ATOMIC_WAIT.dll" },
        // api-ms-win-crt-* all redirect to ucrtbase.dll at runtime.
        { L"api-ms-win-crt-conio-l1-1-0",       L"ucrtbase.dll" },
        { L"api-ms-win-crt-convert-l1-1-0",     L"ucrtbase.dll" },
        { L"api-ms-win-crt-environment-l1-1-0", L"ucrtbase.dll" },
        { L"api-ms-win-crt-filesystem-l1-1-0",  L"ucrtbase.dll" },
        { L"api-ms-win-crt-heap-l1-1-0",        L"ucrtbase.dll" },
        { L"api-ms-win-crt-locale-l1-1-0",      L"ucrtbase.dll" },
        { L"api-ms-win-crt-math-l1-1-0",        L"ucrtbase.dll" },
        { L"api-ms-win-crt-multibyte-l1-1-0",   L"ucrtbase.dll" },
        { L"api-ms-win-crt-private-l1-1-0",     L"ucrtbase.dll" },
        { L"api-ms-win-crt-runtime-l1-1-0",     L"ucrtbase.dll" },
        { L"api-ms-win-crt-stdio-l1-1-0",       L"ucrtbase.dll" },
        { L"api-ms-win-crt-string-l1-1-0",      L"ucrtbase.dll" },
        { L"api-ms-win-crt-time-l1-1-0",        L"ucrtbase.dll" },
        { L"api-ms-win-crt-utility-l1-1-0",     L"ucrtbase.dll" },
    };
    return m;
}

// Set of lowercased DLL names that should always pass through to the real
// system DLL (used by the PeLoader's ResolveImport fallback).
static const std::unordered_set<std::wstring>& PassthroughDllSet() {
    static const std::unordered_set<std::wstring> s = {
        L"msvcp140",
        L"vcruntime140",
        L"vcruntime140_1",
        L"msvcp140_atomic_wait",
        L"ucrtbase",
        L"ucrtbase_enclave",
        L"api-ms-win-crt-conio-l1-1-0",
        L"api-ms-win-crt-convert-l1-1-0",
        L"api-ms-win-crt-environment-l1-1-0",
        L"api-ms-win-crt-filesystem-l1-1-0",
        L"api-ms-win-crt-heap-l1-1-0",
        L"api-ms-win-crt-locale-l1-1-0",
        L"api-ms-win-crt-math-l1-1-0",
        L"api-ms-win-crt-multibyte-l1-1-0",
        L"api-ms-win-crt-private-l1-1-0",
        L"api-ms-win-crt-runtime-l1-1-0",
        L"api-ms-win-crt-stdio-l1-1-0",
        L"api-ms-win-crt-string-l1-1-0",
        L"api-ms-win-crt-time-l1-1-0",
        L"api-ms-win-crt-utility-l1-1-0",
    };
    return s;
}

// Public helper: is the given DLL (lowercased basename) a VC runtime / CRT
// passthrough DLL? Used by PeLoader.cpp's ResolveImport fallback.
bool IsVcRuntimePassthroughDll(const std::wstring& dllLower) {
    return PassthroughDllSet().count(dllLower) > 0;
}

// Public helper: given a passthrough DLL (lowercased basename), return the
// real system DLL name to LoadLibraryW. Returns empty string if not a
// passthrough DLL.
std::wstring GetRealDllForPassthrough(const std::wstring& dllLower) {
    const auto& m = VcRuntimeRealDllMap();
    auto it = m.find(dllLower);
    if (it != m.end()) return it->second;
    // ucrtbase / ucrtbase_enclave — pass through directly.
    if (dllLower == L"ucrtbase" || dllLower == L"ucrtbase_enclave") {
        return dllLower + L".dll";
    }
    return std::wstring();
}

// ---------------------------------------------------------------------------
// ApplyVcRuntimePassthrough — the runtime resolver.
//
// For every (dll, func, stub_proc) entry in the ShimRegistry whose DLL is in
// VcRuntimeRealDllMap(), look up the real function from the real system DLL
// via LoadLibraryW + GetProcAddress, and re-register the entry with the real
// function pointer (overwriting the stub). This makes the runtime PE
// loader's ResolveImport find the real function directly in the registry.
//
// Idempotent: guarded by std::once_flag. Safe to call from multiple sites.
// ---------------------------------------------------------------------------
static std::once_flag g_vcRuntimePassthroughOnce;

void ApplyVcRuntimePassthrough() {
    std::call_once(g_vcRuntimePassthroughOnce, []() {
        auto& reg = ShimRegistry::Instance();
        const auto& realMap = VcRuntimeRealDllMap();

        // Group entries by source DLL so we LoadLibraryW each real DLL at
        // most once.
        std::unordered_map<std::wstring, std::vector<ShimRegistry::Entry>> byDll;
        for (const auto& e : reg.GetAllEntries()) {
            if (realMap.count(e.dll)) {
                byDll[e.dll].push_back(e);
            }
        }

        for (const auto& kv : byDll) {
            const std::wstring& shimDll = kv.first;
            const std::wstring& realDll = realMap.at(shimDll);
            HMODULE h = ::LoadLibraryW(realDll.c_str());
            if (!h) continue;  // real DLL not available — stubs stay in place
            for (const auto& e : kv.second) {
                // GetProcAddress takes a char* — e.func is already a std::string.
                FARPROC real = ::GetProcAddress(h, e.func.c_str());
                if (real) {
                    reg.Register(shimDll, e.func.c_str(), real);
                }
                // If real is null, the stub stays — caller will hit
                // Shim_VcRuntime_Stub which returns 0 / ERROR_PROC_NOT_FOUND.
            }
            // Intentionally leak `h` — keep the real DLL loaded for the
            // lifetime of the process so the function pointers stay valid.
            // (FreeLibrary here would invalidate the FARPROC values we just
            // registered.)
        }
    });
}

// Best-effort static initializer. The authoritative call is from the
// PeLoader constructor (after all REGISTER_SHIM static initializers have
// run). This static initializer is a safety net for callers that touch the
// registry before the PeLoader is constructed.
static int g_vcRuntimePassthroughStaticInit = []() {
    ApplyVcRuntimePassthrough();
    return 0;
}();

}  // namespace xwr

// ===========================================================================
// Literal REGISTER_SHIM placeholders — VC runtime DLLs.
//
// One entry per (MSVCP140 / VCRUNTIME140 / VCRUNTIME140_1 /
// MSVCP140_ATOMIC_WAIT, function) tuple that Half Sword imports. The FARPROC
// is a stub (xwr::Shim_VcRuntime_Stub). At runtime, ApplyVcRuntimePassthrough
// overwrites each stub with the real function pointer fetched from the real
// system DLL via LoadLibraryW + GetProcAddress.
//
// Entry count: 296 literal REGISTER_SHIM entries across 4 VC runtime DLLs.
// ===========================================================================

REGISTER_SHIM("msvcp140_atomic_wait", "__std_bulk_submit_threadpool_work", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140_atomic_wait", "__std_close_threadpool_work", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140_atomic_wait", "__std_create_threadpool_work", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140_atomic_wait", "__std_parallel_algorithms_hw_threads", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140_atomic_wait", "__std_wait_for_threadpool_work_callbacks", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("msvcp140", "??0?$basic_ios@_wu?$char_traits@_w@std@@@std@@ieaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0?$basic_ios@du?$char_traits@d@std@@@std@@ieaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0?$basic_iostream@du?$char_traits@d@std@@@std@@qeaa@peav?$basic_streambuf@du?$char_traits@d@std@@@1@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0?$basic_istream@_wu?$char_traits@_w@std@@@std@@qeaa@peav?$basic_streambuf@_wu?$char_traits@_w@std@@@1@_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0?$basic_istream@du?$char_traits@d@std@@@std@@qeaa@peav?$basic_streambuf@du?$char_traits@d@std@@@1@_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0?$basic_ostream@du?$char_traits@d@std@@@std@@qeaa@peav?$basic_streambuf@du?$char_traits@d@std@@@1@_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@ieaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0?$codecvt@_wdu_mbstatet@@@std@@qeaa@_k@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0_locinfo@std@@qeaa@pebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0_lockit@std@@qeaa@h@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??0facet@locale@std@@ieaa@_k@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$basic_ios@_wu?$char_traits@_w@std@@@std@@ueaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$basic_ios@du?$char_traits@d@std@@@std@@ueaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$basic_iostream@du?$char_traits@d@std@@@std@@ueaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$basic_istream@_wu?$char_traits@_w@std@@@std@@ueaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$basic_istream@du?$char_traits@d@std@@@std@@ueaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$basic_ostream@du?$char_traits@d@std@@@std@@ueaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@ueaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$basic_streambuf@du?$char_traits@d@std@@@std@@ueaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1?$codecvt@_wdu_mbstatet@@@std@@meaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1_locinfo@std@@qeaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1_lockit@std@@qeaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??1facet@locale@std@@meaa@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??4?$_yarn@d@std@@qeaaaeav01@pebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??5?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav01@aea_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??5?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav01@aeaf@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??5?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav01@aeag@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??5?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav01@aeah@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??5?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav01@aeai@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??5?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav01@aeam@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@_k@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@f@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@g@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@h@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@i@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@k@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@m@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@o@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@p6aaeav01@aeav01@@z@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@p6aaeavios_base@1@aeav21@@z@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??6?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav01@pebx@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??7ios_base@std@@qeba_nxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_7?$basic_ios@du?$char_traits@d@std@@@std@@6b@", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_7?$basic_iostream@du?$char_traits@d@std@@@std@@6b@", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_7?$basic_istream@du?$char_traits@d@std@@@std@@6b@", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_7?$basic_ostream@du?$char_traits@d@std@@@std@@6b@", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_7?$basic_streambuf@du?$char_traits@d@std@@@std@@6b@", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_7?$ctype@d@std@@6b@", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_7_facet_base@std@@6b@", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_7ios_base@std@@6b@", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_d?$basic_istream@du?$char_traits@d@std@@@std@@qeaaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??_d?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??bid@locale@std@@qeaa_kxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "??bios_base@std@@qeba_nxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?__exceptionptrassign@@yaxpeaxpebx@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?__exceptionptrcopy@@yaxpeaxpebx@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?__exceptionptrcopyexception@@yaxpeaxpebx1@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?__exceptionptrcreate@@yaxpeax@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?__exceptionptrcurrentexception@@yaxpeax@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?__exceptionptrdestroy@@yaxpeax@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?__exceptionptrrethrow@@yaxpebx@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?__exceptionptrtobool@@ya_npebx@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_addfac@_locimp@locale@std@@aeaaxpeavfacet@23@_k@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_decref@facet@locale@std@@ueaapeav_facet_base@3@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_execute_once@std@@yahaeauonce_flag@1@p6ahpeax1peapeax@z1@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_fiopen@std@@yapeau_iobuf@@peb_whh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_fiopen@std@@yapeau_iobuf@@pebdhh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_getcat@?$codecvt@_wdu_mbstatet@@@std@@sa_kpeapebvfacet@locale@2@pebv42@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_getcat@?$codecvt@ddu_mbstatet@@@std@@sa_kpeapebvfacet@locale@2@pebv42@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_getcat@?$ctype@d@std@@sa_kpeapebvfacet@locale@2@pebv42@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_getcat@?$time_put@dv?$ostreambuf_iterator@du?$char_traits@d@std@@@std@@@std@@sa_kpeapebvfacet@locale@2@pebv42@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_getcoll@_locinfo@std@@qeba?au_collvec@@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_getgloballocale@locale@std@@capeav_locimp@12@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_gnavail@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieba_jxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_gndec@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@ieaapea_wxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_gndec@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_gninc@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@ieaapea_wxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_gninc@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_id_cnt@id@locale@std@@0ha", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_incref@facet@locale@std@@ueaaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_init@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@ieaaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_init@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaaxpeapead0peah001@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_init@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_init@locale@std@@capeav_locimp@12@_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_ios_base_dtor@ios_base@std@@caxpeav12@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_ipfx@?$basic_istream@_wu?$char_traits@_w@std@@@std@@qeaa_n_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_ipfx@?$basic_istream@du?$char_traits@d@std@@@std@@qeaa_n_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_locimp_addfac@_locimp@locale@std@@caxpeav123@peavfacet@23@_k@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_locinfo_ctor@_locinfo@std@@saxpeav12@pebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_locinfo_dtor@_locinfo@std@@saxpeav12@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_lock@?$basic_streambuf@du?$char_traits@d@std@@@std@@ueaaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_new_locimp@_locimp@locale@std@@capeav123@aebv123@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_osfx@?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_pnavail@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieba_jxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_pninc@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@ieaapea_wxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_pninc@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_raise_handler@std@@3p6axaebvexception@stdext@@@zea", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_random_device@std@@yaixz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_rethrow_future_exception@std@@yaxvexception_ptr@1@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_syserror_map@std@@yapebdh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_throw_c_error@std@@yaxh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_throw_cpp_error@std@@yaxh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_throw_future_error@std@@yaxaebverror_code@1@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_unlock@?$basic_streambuf@du?$char_traits@d@std@@@std@@ueaaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_winerror_map@std@@yahh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_xbad_alloc@std@@yaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_xbad_function_call@std@@yaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_xgetlasterror@std@@yaxxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_xinvalid_argument@std@@yaxpebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_xlength_error@std@@yaxpebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_xout_of_range@std@@yaxpebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?_xregex_error@std@@yaxw4error_type@regex_constants@1@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?always_noconv@codecvt_base@std@@qeba_nxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?bad@ios_base@std@@qeba_nxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?c_str@?$_yarn@d@std@@qebapebdxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?cerr@std@@3v?$basic_ostream@du?$char_traits@d@std@@@1@a", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?clear@?$basic_ios@du?$char_traits@d@std@@@std@@qeaaxh_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?cout@std@@3v?$basic_ostream@du?$char_traits@d@std@@@1@a", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?do_narrow@?$ctype@d@std@@mebaddd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?do_narrow@?$ctype@d@std@@mebapebdpebd0dpead@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?do_tolower@?$ctype@d@std@@mebadd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?do_tolower@?$ctype@d@std@@mebapebdpeadpebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?do_toupper@?$ctype@d@std@@mebadd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?do_toupper@?$ctype@d@std@@mebapebdpeadpebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?do_widen@?$ctype@d@std@@mebadd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?do_widen@?$ctype@d@std@@mebapebdpebd0pead@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?eback@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@iebapea_wxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?eback@?$basic_streambuf@du?$char_traits@d@std@@@std@@iebapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?egptr@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@iebapea_wxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?egptr@?$basic_streambuf@du?$char_traits@d@std@@@std@@iebapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?epptr@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@iebapea_wxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?epptr@?$basic_streambuf@du?$char_traits@d@std@@@std@@iebapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?fail@ios_base@std@@qeba_nxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?fill@?$basic_ios@du?$char_traits@d@std@@@std@@qeaadd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?fill@?$basic_ios@du?$char_traits@d@std@@@std@@qebadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?flags@ios_base@std@@qebahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?flush@?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav12@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?gbump@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaaxh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?gcount@?$basic_istream@du?$char_traits@d@std@@@std@@qeba_jxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?getloc@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@qeba?avlocale@2@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?getloc@?$basic_streambuf@du?$char_traits@d@std@@@std@@qeba?avlocale@2@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?getloc@ios_base@std@@qeba?avlocale@2@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?good@ios_base@std@@qeba_nxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?gptr@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@iebapea_wxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?gptr@?$basic_streambuf@du?$char_traits@d@std@@@std@@iebapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?id@?$codecvt@_wdu_mbstatet@@@std@@2v0locale@2@a", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?id@?$codecvt@ddu_mbstatet@@@std@@2v0locale@2@a", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?id@?$collate@d@std@@2v0locale@2@a", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?id@?$ctype@d@std@@2v0locale@2@a", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?id@?$time_put@dv?$ostreambuf_iterator@du?$char_traits@d@std@@@std@@@std@@2v0locale@2@a", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?imbue@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaaxaebvlocale@2@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?in@?$codecvt@_wdu_mbstatet@@@std@@qebahaeau_mbstatet@@pebd1aeapebdpea_w3aeapea_w@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?in@?$codecvt@ddu_mbstatet@@@std@@qebahaeau_mbstatet@@pebd1aeapebdpead3aeapead@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?iword@ios_base@std@@qeaaaeajh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?out@?$codecvt@_wdu_mbstatet@@@std@@qebahaeau_mbstatet@@peb_w1aeapeb_wpead3aeapead@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?out@?$codecvt@ddu_mbstatet@@@std@@qebahaeau_mbstatet@@pebd1aeapebdpead3aeapead@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?overflow@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaahh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?pbackfail@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaahh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?pbase@?$basic_streambuf@du?$char_traits@d@std@@@std@@iebapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?pbump@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaaxh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?peek@?$basic_istream@du?$char_traits@d@std@@@std@@qeaahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?pptr@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@iebapea_wxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?pptr@?$basic_streambuf@du?$char_traits@d@std@@@std@@iebapeadxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?precision@ios_base@std@@qeaa_j_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?precision@ios_base@std@@qeba_jxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?pubsync@?$basic_streambuf@du?$char_traits@d@std@@@std@@qeaahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?put@?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav12@d@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?put@?$time_put@dv?$ostreambuf_iterator@du?$char_traits@d@std@@@std@@@std@@qeba?av?$ostreambuf_iterator@du?$char_traits@d@std@@@2@v32@aeavios_base@2@dpebutm@@pebd3@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?pword@ios_base@std@@qeaaaeapeaxh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?rdbuf@?$basic_ios@_wu?$char_traits@_w@std@@@std@@qebapeav?$basic_streambuf@_wu?$char_traits@_w@std@@@2@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?rdbuf@?$basic_ios@du?$char_traits@d@std@@@std@@qeaapeav?$basic_streambuf@du?$char_traits@d@std@@@2@peav32@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?rdbuf@?$basic_ios@du?$char_traits@d@std@@@std@@qebapeav?$basic_streambuf@du?$char_traits@d@std@@@2@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?read@?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav12@pead_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?sbumpc@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@qeaagxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?sbumpc@?$basic_streambuf@du?$char_traits@d@std@@@std@@qeaahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?seekg@?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav12@_jh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?seekg@?$basic_istream@du?$char_traits@d@std@@@std@@qeaaaeav12@v?$fpos@u_mbstatet@@@2@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?seekoff@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaa?av?$fpos@u_mbstatet@@@2@_jhh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?seekp@?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav12@_jh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?seekp@?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav12@v?$fpos@u_mbstatet@@@2@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?seekpos@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaa?av?$fpos@u_mbstatet@@@2@v32@h@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setbuf@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaapeav12@pead_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setf@ios_base@std@@qeaahh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setf@ios_base@std@@qeaahhh@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setg@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@ieaaxpea_w00@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setg@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaaxpead00@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setiosflags@std@@ya?au?$_smanip@h@1@h@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setp@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaaxpead00@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setp@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaaxpead0@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setprecision@std@@ya?au?$_smanip@_j@1@_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setstate@?$basic_ios@_wu?$char_traits@_w@std@@@std@@qeaaxh_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setstate@?$basic_ios@du?$char_traits@d@std@@@std@@qeaaxh_n@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?setw@std@@ya?au?$_smanip@_j@1@_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?sgetc@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@qeaagxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?sgetc@?$basic_streambuf@du?$char_traits@d@std@@@std@@qeaahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?sgetn@?$basic_streambuf@du?$char_traits@d@std@@@std@@qeaa_jpead_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?showmanyc@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@meaa_jxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?showmanyc@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaa_jxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?snextc@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@qeaagxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?snextc@?$basic_streambuf@du?$char_traits@d@std@@@std@@qeaahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?sputc@?$basic_streambuf@du?$char_traits@d@std@@@std@@qeaahd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?sputn@?$basic_streambuf@du?$char_traits@d@std@@@std@@qeaa_jpebd_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?swap@?$basic_iostream@du?$char_traits@d@std@@@std@@ieaaxaeav12@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?swap@?$basic_streambuf@du?$char_traits@d@std@@@std@@ieaaxaeav12@@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?sync@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?tellg@?$basic_istream@du?$char_traits@d@std@@@std@@qeaa?av?$fpos@u_mbstatet@@@2@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?tellp@?$basic_ostream@du?$char_traits@d@std@@@std@@qeaa?av?$fpos@u_mbstatet@@@2@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?tie@?$basic_ios@du?$char_traits@d@std@@@std@@qebapeav?$basic_ostream@du?$char_traits@d@std@@@2@xz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?tolower@?$ctype@d@std@@qebadd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?tolower@?$ctype@d@std@@qebapebdpeadpebd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?uflow@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?uncaught_exception@std@@ya_nxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?uncaught_exceptions@std@@yahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?underflow@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?unshift@?$codecvt@_wdu_mbstatet@@@std@@qebahaeau_mbstatet@@pead1aeapead@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?unshift@?$codecvt@ddu_mbstatet@@@std@@qebahaeau_mbstatet@@pead1aeapead@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?widen@?$basic_ios@_wu?$char_traits@_w@std@@@std@@qeba_wd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?widen@?$basic_ios@du?$char_traits@d@std@@@std@@qebadd@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?width@ios_base@std@@qeaa_j_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?width@ios_base@std@@qeba_jxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?write@?$basic_ostream@du?$char_traits@d@std@@@std@@qeaaaeav12@pebd_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?xalloc@ios_base@std@@sahxz", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?xsgetn@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@meaa_jpea_w_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?xsgetn@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaa_jpead_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?xsputn@?$basic_streambuf@_wu?$char_traits@_w@std@@@std@@meaa_jpeb_w_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "?xsputn@?$basic_streambuf@du?$char_traits@d@std@@@std@@meaa_jpebd_j@z", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_broadcast", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_destroy_in_situ", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_do_broadcast_at_thread_exit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_init_in_situ", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_register_at_thread_exit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_signal", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_timedwait", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_unregister_at_thread_exit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_cnd_wait", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_getctype", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_lock_shared_ptr_spin_lock", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_mtx_current_owns", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_mtx_destroy_in_situ", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_mtx_init_in_situ", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_mtx_lock", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_mtx_trylock", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_mtx_unlock", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_query_perf_counter", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_query_perf_frequency", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_strcoll", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_strxfrm", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_thrd_detach", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_thrd_id", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_thrd_join", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_thrd_sleep", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_thrd_yield", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_unlock_shared_ptr_spin_lock", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("msvcp140", "_xtime_get_ticks", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("vcruntime140_1", "__cxxframehandler4", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("vcruntime140", "__c_specific_handler", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__current_exception", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__current_exception_context", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__cxxframehandler3", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__intrinsic_setjmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__rtcasttovoid", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__rtdynamiccast", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__rttypeid", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__std_exception_copy", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__std_exception_destroy", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__std_terminate", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__std_type_info_compare", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__std_type_info_destroy_list", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__std_type_info_hash", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__std_type_info_name", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__telemetry_main_invoke_trigger", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "__telemetry_main_return_trigger", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "_cxxthrowexception", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "_purecall", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "_set_purecall_handler", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "longjmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "memchr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "memcmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "memcpy", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "memmove", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "memset", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "strchr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "strrchr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "strstr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "wcschr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "wcsrchr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("vcruntime140", "wcsstr", (FARPROC)&xwr::Shim_VcRuntime_Stub);

// ===========================================================================
// Literal REGISTER_SHIM placeholders — Universal CRT API set DLLs
// (api-ms-win-crt-*).
//
// One entry per (api-ms-win-crt-*, function) tuple that Half Sword imports.
// At runtime, ApplyVcRuntimePassthrough overwrites each stub with the real
// function pointer fetched from ucrtbase.dll.
//
// Entry count: 380 literal REGISTER_SHIM entries across 14 api-ms-win-crt-* DLLs.
// ===========================================================================

REGISTER_SHIM("api-ms-win-crt-conio-l1-1-0", "_cputs", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "_wcstoi64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "_wcstoui64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "_wtof", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "_wtoi", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "_wtoi64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "atof", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "atoi", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "mbstowcs_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "strtod", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "strtol", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "strtoll", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "strtoul", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "strtoull", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "wcstod", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "wcstol", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "wcstombs", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-convert-l1-1-0", "wcstoul", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-environment-l1-1-0", "__p__environ", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-environment-l1-1-0", "_putenv_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-environment-l1-1-0", "_wgetenv_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-environment-l1-1-0", "getenv", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-environment-l1-1-0", "getenv_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_access", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_fstat64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_lock_file", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_stat64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_stat64i32", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_unlink", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_unlock_file", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_waccess", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "_wsplitpath_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-filesystem-l1-1-0", "remove", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "_aligned_free", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "_aligned_malloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "_aligned_msize", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "_aligned_realloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "_callnewh", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "_get_heap_handle", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "_heapchk", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "_set_new_mode", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "calloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "free", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "malloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-heap-l1-1-0", "realloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-locale-l1-1-0", "___lc_codepage_func", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-locale-l1-1-0", "__initialize_lconv_for_unsigned_char", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-locale-l1-1-0", "_configthreadlocale", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-locale-l1-1-0", "_lock_locales", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-locale-l1-1-0", "_unlock_locales", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-locale-l1-1-0", "localeconv", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-locale-l1-1-0", "setlocale", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "__setusermatherr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "_dclass", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "_dsign", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "_dtest", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "_fdclass", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "_fdopen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "_fdtest", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "_finite", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "_isnan", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "acos", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "acosf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "asin", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "asinf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "atan", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "atan2", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "atan2f", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "atanf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "cbrt", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "ceil", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "ceilf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "cos", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "cosf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "exp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "exp2", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "exp2f", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "expf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "fabs", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "floor", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "floorf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "fmax", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "fmod", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "fmodf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "frexp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "hypot", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "ldexp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "log", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "log10", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "log2", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "log2f", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "logbf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "logf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "lroundf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "modf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "modff", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "nextafter", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "nextafterf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "pow", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "powf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "round", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "roundf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "sin", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "sinf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "sinhf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "sqrt", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "sqrtf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "tan", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "tanf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "tanhf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "trunc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-math-l1-1-0", "truncf", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-multibyte-l1-1-0", "_mbspbrk", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "__c_specific_handler", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "__cxxframehandler3", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "__uncaught_exception", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "__undname", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "__undnameex", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_cxxthrowexception", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o____lc_codepage_func", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o____lc_locale_name_func", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o____mb_cur_max_func", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___pctype_func", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___std_exception_copy", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___std_exception_destroy", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___std_type_info_destroy_list", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___stdio_common_vfprintf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___stdio_common_vsnprintf_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___stdio_common_vsnwprintf_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___stdio_common_vsprintf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___stdio_common_vsprintf_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___stdio_common_vsscanf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___stdio_common_vswprintf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o___stdio_common_vswprintf_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__aligned_free", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__aligned_malloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__callnewh", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__calloc_base", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__cexit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__close", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__configure_narrow_argv", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__crt_atexit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__errno", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__execute_onexit_table", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__filelengthi64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__free_base", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__fullpath", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__initialize_narrow_environment", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__initialize_onexit_table", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__invalid_parameter_noinfo", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__invalid_parameter_noinfo_noreturn", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__itoa_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__lseeki64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__ltoa", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__malloc_base", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__mbscmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__memicmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__open_osfhandle", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__purecall", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__read", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__register_onexit_function", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__seh_filter_dll", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__splitpath_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__stricmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__strlwr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__strnicmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wcsdup", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wcsicmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wcslwr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wcsnicmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wctime64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wdupenv_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wfsopen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wfullpath", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wgetenv", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wmakepath_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wsplitpath_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o__wtoi", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_abort", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_atoi", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_atol", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_bsearch", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_calloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_ceilf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_fclose", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_fflush", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_fread", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_free", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_frexp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_fseek", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_ftell", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_isspace", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_iswprint", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_iswspace", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_iswxdigit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_localeconv", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_malloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_qsort", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_realloc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_setlocale", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_strcat_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_strcpy_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_strncat_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_strncpy_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_terminate", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_tolower", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_towlower", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_wcscat_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_wcscpy_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_wcsncat_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_wcsncpy_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_wcstoul", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "_o_wmemcpy_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "memcmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "memcpy", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "memmove", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "strchr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "strrchr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "strstr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "wcschr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "wcsrchr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-private-l1-1-0", "wcsstr", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "__doserrno", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "__sys_errlist", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "__sys_nerr", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_beginthreadex", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_c_exit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_cexit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_configure_narrow_argv", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_control87", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_crt_at_quick_exit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_crt_atexit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_endthreadex", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_errno", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_execute_onexit_table", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_exit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_get_narrow_winmain_command_line", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_getpid", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_initialize_narrow_environment", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_initialize_onexit_table", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_initterm", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_initterm_e", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_invalid_parameter_noinfo", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_invalid_parameter_noinfo_noreturn", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_invoke_watson", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_register_onexit_function", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_register_thread_local_exe_atexit_callback", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_seh_filter_dll", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_seh_filter_exe", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_set_app_type", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_set_invalid_parameter_handler", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "_wassert", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "abort", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "exit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "raise", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "signal", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "strerror", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "strerror_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-runtime-l1-1-0", "terminate", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__acrt_iob_func", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__p__commode", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vfprintf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vfprintf_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vfwprintf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vsnprintf_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vsprintf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vsprintf_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vsscanf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vswprintf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "__stdio_common_vswprintf_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_close", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_creat", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_dup", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_dup2", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_fileno", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_fseeki64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_get_osfhandle", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_get_stream_buffer_pointers", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_getcwd", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_isatty", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_lseeki64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_open", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_open_osfhandle", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_read", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_set_fmode", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_setmode", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_wfopen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_wfsopen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_wopen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "_write", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fclose", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "feof", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "ferror", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fflush", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fgetc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fgetpos", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fgets", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fgetwc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fopen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fopen_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fputc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fputs", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fputwc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fread", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fseek", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fsetpos", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "ftell", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "fwrite", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "getchar", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "putchar", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "puts", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "setvbuf", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "tmpnam", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "ungetc", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-stdio-l1-1-0", "ungetwc", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "_strdup", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "_stricmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "_strnicmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "_wcsdup", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "_wcsicmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "_wcsnicmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "isalnum", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "isalpha", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iscntrl", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "isdigit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "islower", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "isprint", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "ispunct", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "isspace", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "isupper", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswalnum", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswalpha", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswcntrl", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswdigit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswlower", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswprint", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswpunct", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswspace", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswupper", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "iswxdigit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "isxdigit", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "memset", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strcat_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strcmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strcpy_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strcspn", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strlen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strncat", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strncmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strncpy", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strncpy_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strnlen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strpbrk", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strspn", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "strxfrm", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "tolower", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "toupper", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "towlower", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "wcscat_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "wcscmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "wcscpy_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "wcsncmp", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "wcsncpy", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "wcsnlen", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-string-l1-1-0", "wcsxfrm", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "__timezone", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "__tzname", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "_ctime64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "_gmtime64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "_gmtime64_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "_localtime64_s", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "_time64", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "_tzset", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "clock", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-time-l1-1-0", "strftime", (FARPROC)&xwr::Shim_VcRuntime_Stub);

REGISTER_SHIM("api-ms-win-crt-utility-l1-1-0", "bsearch", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-utility-l1-1-0", "div", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-utility-l1-1-0", "qsort", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-utility-l1-1-0", "rand", (FARPROC)&xwr::Shim_VcRuntime_Stub);
REGISTER_SHIM("api-ms-win-crt-utility-l1-1-0", "srand", (FARPROC)&xwr::Shim_VcRuntime_Stub);

// End of VcRuntimePassthrough.cpp
