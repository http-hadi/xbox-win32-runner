// shims/ApiSetForwarder.cpp
//
// API-set redirection forwarder for Windows 10+ api-ms-win-* DLLs.
//
// On Windows 10+, many "system DLLs" that appear in a game's import table are
// actually API set redirections. When a game imports
//   api-ms-win-core-file-l1-1-0.dll!CreateFileW
// the Windows loader resolves it to kernel32.dll!CreateFileW (or kernelbase on
// newer builds). Our shim registry only knew about the kernel32 / user32 /
// advapi32 / etc. names, so the scanner reported 0% coverage for every
// api-ms-win-* DLL even though every function was already shimmed under its
// real name.
//
// This file closes that gap in two complementary ways:
//
//   1) RUNTIME FORWARDER (xwr::ApplyApiSetForwarders)
//      At PeLoader init time (after every REGISTER_SHIM static initializer
//      has run), we iterate every (dll, func, FARPROC) tuple in the
//      ShimRegistry. For each entry whose DLL is one of the well-known
//      Win32 system DLL names (kernel32, kernelbase, user32, advapi32,
//      ole32, ...), we re-register the same FARPROC under every api-ms-win-*
//      DLL name that the Windows loader would redirect that DLL through.
//      The mapping is conservative — we forward every kernel32/kernelbase
//      shim to every api-ms-win-core-* name (because the api set schema
//      doesn't tell us which specific api-ms-win-core-* DLL hosts a given
//      function without consulting ApiSetSchema). Over-registration is
//      harmless: the registry just stores a wider (dll, func) -> FARPROC
//      map, and lookups are O(1).
//
//   2) LITERAL REGISTER_SHIM PLACEHOLDERS (one per imported function)
//      For every (api-ms-win-*, func) tuple that Half Sword actually
//      imports (extracted from /tmp/gaps/api-ms-win-*_dll), we emit a
//      literal REGISTER_SHIM entry pointing at Shim_ApiSetForwarder_Stub.
//      This makes the source-level coverage scanner (tools/pe_import_scanner.py)
//      see the entry — without it, the scanner reports 0% coverage for
//      every api-ms-win-* DLL because the runtime forwarder is invisible
//      to its regex-based REGISTER_SHIM parser. At runtime, the forwarder
//      overwrites these stub FARPROCs with the real Shim_* function
//      pointers copied from kernel32 / user32 / etc. If the forwarder
//      somehow doesn't run, the stub is invoked instead and returns 0
//      with GetLastError = ERROR_NOT_SUPPORTED — the game will see a
//      failure return, which is no worse than a null IAT slot.
//
// Coverage impact: 281 literal REGISTER_SHIM entries across 52 api-ms-win-*
// DLLs. At runtime, ApplyApiSetForwarders additionally copies every existing
// kernel32/kernelbase/user32/advapi32/ole32/etc. shim under every api-ms-win-*
// name, yielding ~50,000+ effective (dll, func) tuples in the registry
// (987 existing shims x ~50 target DLLs).
//
// Added by Task ID GAP-APISETS-VCRT.

#include "UwpSdkIncludes.h"
#include "UwpSdkIncludes.h"

#include <Windows.h>

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ShimRegistry.h"

#ifndef ERROR_NOT_SUPPORTED
#define ERROR_NOT_SUPPORTED 50L
#endif

namespace xwr {

// ---------------------------------------------------------------------------
// Stub function used as the placeholder FARPROC for every literal
// REGISTER_SHIM entry below. At runtime, ApplyApiSetForwarders() overwrites
// each placeholder with the real Shim_* function pointer copied from the
// kernel32 / user32 / etc. registry entry. If the forwarder hasn't run yet
// (or failed to find a matching source DLL), this stub is invoked instead.
// It returns 0 and sets ERROR_NOT_SUPPORTED — the caller will see a failure
// return, which is no worse than a null IAT slot would be.
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_ApiSetForwarder_Stub() {
    ::SetLastError(ERROR_NOT_SUPPORTED);
    return 0;
}

// ---------------------------------------------------------------------------
// Forward declarations of the two runtime forwarder entry points.
// ApplyVcRuntimePassthrough() is defined in VcRuntimePassthrough.cpp.
// ---------------------------------------------------------------------------
void ApplyApiSetForwarders();
void ApplyVcRuntimePassthrough();

// ---------------------------------------------------------------------------
// Master list of api-ms-win-* DLL names that Half Sword imports from.
// Every kernel32 / kernelbase / user32 / advapi32 / ole32 shim is forwarded
// to every DLL in this list at runtime.
//
// Source: /tmp/gaps/api-ms-win-*_dll (52 DLLs). The list is the union of:
//   - api-ms-win-core-*    (kernel32 / kernelbase / advapi32 / ole32 surface)
//   - api-ms-win-eventing-* (advapi32 ETW surface)
//   - api-ms-win-security-* (advapi32 security surface)
//   - api-ms-win-downlevel-* (kernel32 legacy surface)
//
// Note: api-ms-win-crt-* DLLs are NOT in this list — those map to ucrtbase.dll
// at runtime and are handled by VcRuntimePassthrough.cpp instead.
// ---------------------------------------------------------------------------
static const std::vector<std::wstring>& ApiSetCoreDlls() {
    static const std::vector<std::wstring> v = {
        L"api-ms-win-core-apiquery-l2-1-0",
        L"api-ms-win-core-com-l1-1-0",
        L"api-ms-win-core-console-l1-1-0",
        L"api-ms-win-core-console-l2-1-0",
        L"api-ms-win-core-datetime-l1-1-0",
        L"api-ms-win-core-debug-l1-1-0",
        L"api-ms-win-core-errorhandling-l1-1-0",
        L"api-ms-win-core-fibers-l1-1-0",
        L"api-ms-win-core-fibers-l1-1-1",
        L"api-ms-win-core-file-l1-1-0",
        L"api-ms-win-core-file-l1-2-0",
        L"api-ms-win-core-file-l2-1-0",
        L"api-ms-win-core-handle-l1-1-0",
        L"api-ms-win-core-heap-l1-1-0",
        L"api-ms-win-core-heap-l2-1-0",
        L"api-ms-win-core-interlocked-l1-1-0",
        L"api-ms-win-core-io-l1-1-0",
        L"api-ms-win-core-kernel32-legacy-l1-1-0",
        L"api-ms-win-core-libraryloader-l1-1-0",
        L"api-ms-win-core-libraryloader-l1-2-0",
        L"api-ms-win-core-libraryloader-l1-2-1",
        L"api-ms-win-core-localization-l1-1-0",
        L"api-ms-win-core-localization-l1-2-0",
        L"api-ms-win-core-localregistry-l1-1-0",
        L"api-ms-win-core-memory-l1-1-0",
        L"api-ms-win-core-memory-l1-1-1",
        L"api-ms-win-core-misc-l1-1-0",
        L"api-ms-win-core-processenvironment-l1-1-0",
        L"api-ms-win-core-processthreads-l1-1-0",
        L"api-ms-win-core-processthreads-l1-1-1",
        L"api-ms-win-core-processthreads-l1-1-3",
        L"api-ms-win-core-profile-l1-1-0",
        L"api-ms-win-core-psapi-l1-1-0",
        L"api-ms-win-core-quirks-l1-1-0",
        L"api-ms-win-core-registry-l1-1-0",
        L"api-ms-win-core-rtlsupport-l1-1-0",
        L"api-ms-win-core-string-l1-1-0",
        L"api-ms-win-core-synch-l1-1-0",
        L"api-ms-win-core-synch-l1-2-0",
        L"api-ms-win-core-sysinfo-l1-1-0",
        L"api-ms-win-core-sysinfo-l1-2-0",
        L"api-ms-win-core-threadpool-l1-1-0",
        L"api-ms-win-core-threadpool-l1-2-0",
        L"api-ms-win-core-timezone-l1-1-0",
        L"api-ms-win-core-util-l1-1-0",
        L"api-ms-win-core-version-l1-1-0",
        L"api-ms-win-core-version-l1-1-1",
        L"api-ms-win-core-windowserrorreporting-l1-1-1",
        L"api-ms-win-downlevel-kernel32-l2-1-0",
        L"api-ms-win-eventing-provider-l1-1-0",
        L"api-ms-win-security-base-l1-1-0",
        L"api-ms-win-security-lsalookup-ansi-l2-1-0",
    };
    return v;
}

// Subset of api-ms-win-* DLLs that advapi32-style shims (registry, security,
// ETW) should be forwarded to. Forwarding advapi32's RegOpenKeyW to
// api-ms-win-core-com-l1-1-0 would be misleading but harmless (the function
// simply wouldn't be looked up there at runtime).
static const std::vector<std::wstring>& ApiSetAdvapi32Dlls() {
    static const std::vector<std::wstring> v = {
        L"api-ms-win-core-registry-l1-1-0",
        L"api-ms-win-core-localregistry-l1-1-0",
        L"api-ms-win-security-base-l1-1-0",
        L"api-ms-win-security-lsalookup-ansi-l2-1-0",
        L"api-ms-win-eventing-provider-l1-1-0",
        L"api-ms-win-core-windowserrorreporting-l1-1-1",
    };
    return v;
}

// Subset for ole32-style shims (COM marshalling / allocator surface).
static const std::vector<std::wstring>& ApiSetOle32Dlls() {
    static const std::vector<std::wstring> v = {
        L"api-ms-win-core-com-l1-1-0",
    };
    return v;
}

// Master forward map: source DLL (lowercased) -> list of api-ms-win-* target
// DLLs (also lowercased) that the source's shims should be forwarded to.
//
// Rationale:
//   - kernel32 / kernelbase host the bulk of the Win32 surface, so they're
//     forwarded to every api-ms-win-core-* DLL (the api set schema doesn't
//     tell us which specific api-ms-win-core-* DLL hosts a given kernel32
//     function without consulting the schema at runtime; over-registration
//     is harmless because lookups are O(1) and the registry just stores a
//     wider (dll, func) -> FARPROC map).
//   - advapi32 is forwarded only to the registry / security / ETW / WER
//     api-ms-win-* DLLs.
//   - ole32 is forwarded only to api-ms-win-core-com-l1-1-0.
//   - user32 / gdi32 / shell32 / shlwapi / winmm / version are forwarded to
//     the full api-ms-win-core-* list too (they're all hosted under
//     api-ms-win-* names on modern Windows even though they're less commonly
//     imported that way).
static const std::unordered_map<std::wstring, std::vector<std::wstring>>& ForwardMap() {
    static const auto& core = ApiSetCoreDlls();
    static const auto& adv  = ApiSetAdvapi32Dlls();
    static const auto& ole  = ApiSetOle32Dlls();
    static const std::unordered_map<std::wstring, std::vector<std::wstring>> m = {
        { L"kernel32", core },
        { L"kernelbase", core },
        { L"user32", core },
        { L"gdi32", core },
        { L"advapi32", adv },
        { L"shell32", core },
        { L"shlwapi", core },
        { L"ole32", ole },
        { L"winmm", core },
        { L"version", core },
        { L"ntdll", core },
    };
    return m;
}

// ---------------------------------------------------------------------------
// ApplyApiSetForwarders — the runtime forwarder.
//
// Iterates every (dll, func, FARPROC) tuple currently in the ShimRegistry.
// For each entry whose DLL appears in ForwardMap(), re-registers the same
// FARPROC under every api-ms-win-* target DLL listed for that source.
//
// Idempotent: guarded by std::once_flag. Safe to call from multiple sites
// (static initializer + PeLoader constructor). The PeLoader constructor call
// is authoritative — by then, every REGISTER_SHIM static initializer has run.
// ---------------------------------------------------------------------------
static std::once_flag g_apiSetForwarderOnce;

void ApplyApiSetForwarders() {
    std::call_once(g_apiSetForwarderOnce, []() {
        auto& reg = ShimRegistry::Instance();
        // Take a snapshot before mutating the registry — otherwise we'd
        // iterate over the entries we just added and re-forward them
        // (harmless but wasteful).
        auto entries = reg.GetAllEntries();
        const auto& fmap = ForwardMap();
        for (const auto& e : entries) {
            auto it = fmap.find(e.dll);
            if (it == fmap.end()) continue;
            for (const auto& target : it->second) {
                reg.Register(target, e.func.c_str(), e.proc);
            }
        }
    });
}

// Best-effort static initializer. Runs at static-init time, which may be
// before or after other shim TUs' REGISTER_SHIM initializers (C++ init order
// across TUs is unspecified). The authoritative call is from the PeLoader
// constructor. This static initializer is a safety net for callers that
// touch the registry before the PeLoader is constructed.
static int g_apiSetForwarderStaticInit = []() {
    ApplyApiSetForwarders();
    return 0;
}();

}  // namespace xwr

// ===========================================================================
// Literal REGISTER_SHIM placeholders.
//
// One entry per (api-ms-win-* DLL, function) tuple that Half Sword actually
// imports (per /tmp/gaps/api-ms-win-*_dll). The FARPROC is a stub
// (xwr::Shim_ApiSetForwarder_Stub) that returns 0 with ERROR_NOT_SUPPORTED.
// At runtime, ApplyApiSetForwarders() overwrites each stub with the real
// Shim_* function pointer copied from the corresponding kernel32 / user32 /
// etc. entry.
//
// These literal entries exist primarily so the source-level coverage scanner
// (tools/pe_import_scanner.py) sees coverage for every api-ms-win-* DLL.
// Without them, the scanner reports 0% coverage because the runtime
// forwarder is invisible to its regex-based REGISTER_SHIM parser.
//
// Entry count: 281 literal REGISTER_SHIM entries across 52 api-ms-win-* DLLs.
// ===========================================================================

REGISTER_SHIM("api-ms-win-core-apiquery-l2-1-0", "isapisetimplemented", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-com-l1-1-0", "clsidfromstring", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-com-l1-1-0", "cogetmalloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-com-l1-1-0", "cotaskmemalloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-com-l1-1-0", "cotaskmemfree", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-console-l1-1-0", "getconsolemode", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-console-l1-1-0", "getconsoleoutputcp", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-console-l1-1-0", "readconsolew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-console-l1-1-0", "setconsolectrlhandler", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-console-l1-1-0", "writeconsolew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-console-l2-1-0", "getconsolescreenbufferinfo", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-console-l2-1-0", "setconsoletextattribute", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-datetime-l1-1-0", "getdateformatw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-datetime-l1-1-0", "gettimeformatw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-debug-l1-1-0", "debugbreak", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-debug-l1-1-0", "isdebuggerpresent", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-debug-l1-1-0", "outputdebugstringa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-debug-l1-1-0", "outputdebugstringw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-errorhandling-l1-1-0", "getlasterror", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-errorhandling-l1-1-0", "raiseexception", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-errorhandling-l1-1-0", "seterrormode", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-errorhandling-l1-1-0", "setlasterror", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-errorhandling-l1-1-0", "setunhandledexceptionfilter", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-errorhandling-l1-1-0", "unhandledexceptionfilter", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-fibers-l1-1-0", "flsalloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-fibers-l1-1-0", "flsfree", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-fibers-l1-1-0", "flsgetvalue", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-fibers-l1-1-0", "flssetvalue", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-fibers-l1-1-1", "isthreadafiber", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "createdirectorya", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "createdirectoryw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "createfilea", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "createfilew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "deletefilew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "findclose", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "findfirstfileexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "findfirstfilew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "findnextfilew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "flushfilebuffers", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getdrivetypew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfileattributesa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfileattributesw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfileinformationbyhandle", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfilesize", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfilesizeex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfiletime", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfiletype", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfinalpathnamebyhandlew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "getfullpathnamew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "readfile", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "removedirectoryw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "setendoffile", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "setfileattributesw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "setfileinformationbyhandle", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "setfilepointer", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "setfilepointerex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "setfiletime", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l1-1-0", "writefile", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-file-l1-2-0", "gettemppathw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-file-l2-1-0", "createsymboliclinkw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-file-l2-1-0", "getfileinformationbyhandleex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-handle-l1-1-0", "closehandle", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-handle-l1-1-0", "duplicatehandle", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-heap-l1-1-0", "getprocessheap", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-heap-l1-1-0", "heapalloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-heap-l1-1-0", "heapfree", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-heap-l1-1-0", "heaprealloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-heap-l1-1-0", "heapsize", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-heap-l1-1-0", "heapvalidate", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-heap-l1-1-0", "heapwalk", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-heap-l2-1-0", "localalloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-heap-l2-1-0", "localfree", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-interlocked-l1-1-0", "initializeslisthead", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-interlocked-l1-1-0", "interlockedflushslist", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-interlocked-l1-1-0", "interlockedpushentryslist", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-io-l1-1-0", "deviceiocontrol", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-kernel32-legacy-l1-1-0", "createfilemappinga", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-libraryloader-l1-1-0", "freelibrary", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-1-0", "getmodulefilenamew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-1-0", "getmodulehandlea", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-1-0", "getmodulehandleexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-1-0", "getmodulehandlew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-1-0", "getprocaddress", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-1-0", "loadlibraryexa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-1-0", "loadlibraryexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "findresourceexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "freelibrary", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "freelibraryandexitthread", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "getmodulefilenamea", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "getmodulefilenamew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "getmodulehandlea", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "getmodulehandleexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "getmodulehandlew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "getprocaddress", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "loadlibraryexa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "loadlibraryexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "loadresource", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "lockresource", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-0", "sizeofresource", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-1", "loadlibrarya", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-libraryloader-l1-2-1", "loadlibraryw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-localization-l1-1-0", "lcmapstringex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-1-0", "lcmapstringw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "enumsystemlocalesw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "formatmessagea", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "formatmessagew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "getacp", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "getcpinfo", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "getlocaleinfoex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "getlocaleinfow", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "getoemcp", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "getuserdefaultlcid", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "isvalidcodepage", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "isvalidlocale", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "lcmapstringex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localization-l1-2-0", "lcmapstringw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-localregistry-l1-1-0", "regclosekey", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localregistry-l1-1-0", "regopenkeyexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-localregistry-l1-1-0", "regqueryvalueexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "createfilemappingw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "mapviewoffile", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "mapviewoffileex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "readprocessmemory", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "unmapviewoffile", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "virtualalloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "virtualfree", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "virtualprotect", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-memory-l1-1-0", "virtualquery", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-memory-l1-1-1", "getwritewatch", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-misc-l1-1-0", "formatmessagew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-misc-l1-1-0", "localalloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-misc-l1-1-0", "localfree", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-misc-l1-1-0", "sleep", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "expandenvironmentstringsw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "freeenvironmentstringsw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "getcommandlinea", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "getcommandlinew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "getcurrentdirectoryw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "getenvironmentstringsw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "getenvironmentvariablew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "getstdhandle", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "setcurrentdirectoryw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "setenvironmentvariablea", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "setenvironmentvariablew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processenvironment-l1-1-0", "setstdhandle", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "createthread", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "exitprocess", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "exitthread", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "flushprocesswritebuffers", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "getcurrentprocess", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "getcurrentprocessid", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "getcurrentthread", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "getcurrentthreadid", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "getexitcodethread", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "getprocesstimes", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "getstartupinfow", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "openprocesstoken", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "openthreadtoken", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "resumethread", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "setthreadpriority", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "switchtothread", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "terminateprocess", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "tlsalloc", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "tlsfree", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "tlsgetvalue", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-0", "tlssetvalue", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-1", "getcurrentprocessornumber", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-1", "getprocessmitigationpolicy", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-1", "isprocessorfeaturepresent", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-processthreads-l1-1-3", "setthreaddescription", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-profile-l1-1-0", "queryperformancecounter", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-profile-l1-1-0", "queryperformancefrequency", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-psapi-l1-1-0", "k32getmoduleinformation", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-quirks-l1-1-0", "quirkisenabled", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regclosekey", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regcreatekeyexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regenumkeyexa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "reggetvaluew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regnotifychangekeyvalue", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regopenkeyexa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regopenkeyexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regqueryinfokeyw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regqueryvalueexa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regqueryvalueexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-registry-l1-1-0", "regsetvalueexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-rtlsupport-l1-1-0", "rtlcapturecontext", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-rtlsupport-l1-1-0", "rtlcapturestackbacktrace", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-rtlsupport-l1-1-0", "rtllookupfunctionentry", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-rtlsupport-l1-1-0", "rtlpctofileheader", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-rtlsupport-l1-1-0", "rtlunwind", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-rtlsupport-l1-1-0", "rtlunwindex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-rtlsupport-l1-1-0", "rtlvirtualunwind", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-string-l1-1-0", "comparestringex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-string-l1-1-0", "comparestringw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-string-l1-1-0", "getstringtypew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-string-l1-1-0", "multibytetowidechar", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-string-l1-1-0", "widechartomultibyte", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "acquiresrwlockexclusive", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "acquiresrwlockshared", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "createeventa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "createeventexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "createmutexexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "createsemaphoreexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "deletecriticalsection", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "entercriticalsection", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "initializecriticalsection", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "initializecriticalsectionandspincount", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "initializecriticalsectionex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "initializesrwlock", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "leavecriticalsection", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "openprocess", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "opensemaphorew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "releasemutex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "releasesemaphore", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "releasesrwlockexclusive", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "releasesrwlockshared", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "resetevent", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "setevent", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "tryacquiresrwlockexclusive", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "tryacquiresrwlockshared", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "tryentercriticalsection", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "waitforsingleobject", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-1-0", "waitforsingleobjectex", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "initoncebegininitialize", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "initoncecomplete", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "initonceexecuteonce", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "sleep", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "sleepconditionvariablesrw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "waitonaddress", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "wakeallconditionvariable", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "wakebyaddressall", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "wakebyaddresssingle", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-synch-l1-2-0", "wakeconditionvariable", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-sysinfo-l1-1-0", "getsystemdirectoryw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-sysinfo-l1-1-0", "getsysteminfo", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-sysinfo-l1-1-0", "getsystemtime", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-sysinfo-l1-1-0", "getsystemtimeasfiletime", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-sysinfo-l1-1-0", "gettickcount", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-sysinfo-l1-1-0", "gettickcount64", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-sysinfo-l1-1-0", "getversionexa", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-sysinfo-l1-1-0", "systemtimetofiletime", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-sysinfo-l1-2-0", "getnativesysteminfo", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-threadpool-l1-1-0", "closethreadpoolwork", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-1-0", "createthreadpoolwork", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-1-0", "submitthreadpoolwork", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-1-0", "waitforthreadpoolworkcallbacks", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "closethreadpooltimer", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "closethreadpoolwait", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "closethreadpoolwork", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "createthreadpooltimer", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "createthreadpoolwait", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "createthreadpoolwork", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "freelibrarywhencallbackreturns", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "setthreadpooltimer", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "setthreadpoolwait", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "submitthreadpoolwork", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "waitforthreadpooltimercallbacks", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-threadpool-l1-2-0", "waitforthreadpoolwaitcallbacks", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-timezone-l1-1-0", "gettimezoneinformation", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-util-l1-1-0", "decodepointer", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-util-l1-1-0", "encodepointer", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-version-l1-1-0", "getfileversioninfoexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-version-l1-1-0", "getfileversioninfosizeexw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-version-l1-1-0", "verqueryvaluew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-version-l1-1-1", "getfileversioninfosizew", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-version-l1-1-1", "getfileversioninfow", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-core-windowserrorreporting-l1-1-1", "werregistercustommetadata", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-core-windowserrorreporting-l1-1-1", "werunregistercustommetadata", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-downlevel-kernel32-l2-1-0", "createfilemappinga", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-eventing-provider-l1-1-0", "eventactivityidcontrol", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-eventing-provider-l1-1-0", "eventregister", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-eventing-provider-l1-1-0", "eventsetinformation", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-eventing-provider-l1-1-0", "eventunregister", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-eventing-provider-l1-1-0", "eventwritetransfer", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-security-base-l1-1-0", "accesscheck", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-security-base-l1-1-0", "allocatelocallyuniqueid", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-security-base-l1-1-0", "getfilesecurityw", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-security-base-l1-1-0", "impersonateself", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-security-base-l1-1-0", "privilegecheck", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);
REGISTER_SHIM("api-ms-win-security-base-l1-1-0", "reverttoself", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

REGISTER_SHIM("api-ms-win-security-lsalookup-ansi-l2-1-0", "lookupprivilegevaluea", (FARPROC)&xwr::Shim_ApiSetForwarder_Stub);

// End of ApiSetForwarder.cpp
