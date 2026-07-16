// shims/ShimRegistry.h
// Central registry of Win32 API shim functions.
//
// Each shim DLL module registers its exports into the registry at startup via
// REGISTER_SHIM("kernel32", "CreateFileW", (FARPROC)&Shim_CreateFileW).
// The PE loader queries ResolveExport("kernel32", "CreateFileW") to fill the
// IAT of a game module.
//
// Lookups are O(1) (unordered_map) and case-insensitive on both the DLL name
// and function name, matching Windows loader behavior.

#pragma once
#include <Windows.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace xwr {

class ShimRegistry {
public:
    static ShimRegistry& Instance();

    // Register a shim function under "dll.funcName".
    // `dllName` is lowercased internally; `funcName` is lowercased internally.
    void Register(const std::wstring& dllName, const char* funcName, FARPROC fn);

    // Convenience macro helper: REGISTER_SHIM(dll, name, ptr) calls this.
    void Register(const wchar_t* dllName, const wchar_t* funcName, FARPROC fn);

    // Look up a function by DLL name (any case) and function name (any case).
    // Returns 0 (nullptr) if not found.
    uint64_t ResolveExport(const std::wstring& dllName, const char* funcName);

    // True if the registry has at least one export for the given DLL.
    bool HasModule(const std::wstring& dllName);

    // For the coverage report: return every DLL we have shims for, with their
    // function counts.
    struct ModuleStats {
        std::wstring name;
        size_t        functionCount;
    };
    std::vector<ModuleStats> GetAllModuleStats();

    // Snapshot of one (dll, func, FARPROC) tuple in the registry.
    // `dll` and `func` are returned in their already-lowercased form (the form
    // used as the hash-map key). `proc` is the FARPROC value last registered.
    //
    // Used by the API-set forwarder (ApiSetForwarder.cpp) to iterate every
    // existing shim and re-register it under every api-ms-win-* DLL name.
    struct Entry {
        std::wstring dll;
        std::string  func;
        FARPROC      proc;
    };
    std::vector<Entry> GetAllEntries() const;

private:
    ShimRegistry() = default;

    struct Key {
        std::wstring dllLower;    // e.g. L"kernel32"
        std::string  funcLower;   // e.g. "createfilew"
        bool operator==(const Key& o) const {
            return dllLower == o.dllLower && funcLower == o.funcLower;
        }
    };
    struct KeyHash {
        size_t operator()(const Key& k) const {
            return std::hash<std::wstring>()(k.dllLower) ^
                   (std::hash<std::string>()(k.funcLower) << 1);
        }
    };

    std::unordered_map<Key, FARPROC, KeyHash> m_table;

    friend class PeLoader;
};

}  // namespace xwr

// Convenience macro for registering shims at namespace scope.
// Usage: REGISTER_SHIM("kernel32", "CreateFileW", (FARPROC)&Shim_CreateFileW)
//
// Uses __COUNTER__ to generate a unique variable name per registration site
// (string literals can't be token-pasted, so we don't derive the name from
// the DLL/function arguments).
#define XWR_REGID_(c) XWR_ANON_SHIM_REG_##c
#define XWR_REGID(c) XWR_REGID_(c)
#define REGISTER_SHIM(dll, name, ptr)                                          \
    static int XWR_REGID(__COUNTER__) = []() {                                 \
        ::xwr::ShimRegistry::Instance().Register(                              \
            L##dll, L##name, reinterpret_cast<FARPROC>(ptr));                  \
        return 0;                                                              \
    }()
