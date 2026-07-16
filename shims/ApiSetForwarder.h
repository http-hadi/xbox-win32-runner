// shims/ApiSetForwarder.h
//
// Declarations for the API-set / VC-runtime forwarder and passthrough
// helpers. Definitions live in shims/ApiSetForwarder.cpp and
// shims/VcRuntimePassthrough.cpp.
//
// See the corresponding .cpp files for the full design rationale.

#pragma once
#include <Windows.h>

#include <string>

namespace xwr {

// ---------------------------------------------------------------------------
// API-set forwarder (ApiSetForwarder.cpp)
//
// At PeLoader init time (after every REGISTER_SHIM static initializer has
// run), iterate every (dll, func, FARPROC) tuple in the ShimRegistry. For
// each entry whose DLL is one of the well-known Win32 system DLL names
// (kernel32, kernelbase, user32, advapi32, ole32, ...), re-register the
// same FARPROC under every api-ms-win-* DLL name that the Windows loader
// would redirect that DLL through.
//
// Idempotent: guarded by std::once_flag. Safe to call from multiple sites
// (static initializer + PeLoader constructor).
// ---------------------------------------------------------------------------
void ApplyApiSetForwarders();

// ---------------------------------------------------------------------------
// VC runtime passthrough (VcRuntimePassthrough.cpp)
//
// For every (MSVCP140 / VCRUNTIME140 / VCRUNTIME140_1 / MSVCP140_ATOMIC_WAIT
// / api-ms-win-crt-*, func) entry in the ShimRegistry, LoadLibraryW the real
// underlying system DLL (the VC runtime DLL itself, or ucrtbase.dll for
// api-ms-win-crt-*), GetProcAddress the function, and re-register the entry
// with the real function pointer (overwriting the stub).
//
// Idempotent: guarded by std::once_flag.
// ---------------------------------------------------------------------------
void ApplyVcRuntimePassthrough();

// ---------------------------------------------------------------------------
// Helpers for the PeLoader's ResolveImport fallback.
// ---------------------------------------------------------------------------

// True if `dllLower` (lowercased basename, no extension) is a VC runtime /
// Universal CRT passthrough DLL — i.e. the PE loader should fall back to
// LoadLibraryW + GetProcAddress when the ShimRegistry doesn't have the
// specific function.
bool IsVcRuntimePassthroughDll(const std::wstring& dllLower);

// Given a passthrough DLL (lowercased basename, no extension), return the
// real system DLL name (with .dll extension) to LoadLibraryW. Returns empty
// string if `dllLower` is not a passthrough DLL.
std::wstring GetRealDllForPassthrough(const std::wstring& dllLower);

}  // namespace xwr
