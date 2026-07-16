// shims/ShimRegistry.cpp
#include "UwpSdkIncludes.h"

#include "ShimRegistry.h"

#include <algorithm>
#include <locale>

namespace xwr {

ShimRegistry& ShimRegistry::Instance() {
    static ShimRegistry inst;
    return inst;
}

static std::wstring LowerW(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
    return r;
}
static std::string LowerA(const char* s) {
    std::string r;
    for (const char* p = s; *p; ++p) r.push_back(static_cast<char>(::tolower(static_cast<unsigned char>(*p))));
    return r;
}

void ShimRegistry::Register(const std::wstring& dllName, const char* funcName, FARPROC fn) {
    Key k{ LowerW(dllName), LowerA(funcName) };
    m_table[k] = fn;
}
void ShimRegistry::Register(const wchar_t* dllName, const wchar_t* funcName, FARPROC fn) {
    std::wstring wd(dllName);
    std::string  af;
    for (const wchar_t* p = funcName; *p; ++p) af.push_back(static_cast<char>(*p));
    Register(wd, af.c_str(), fn);
}

uint64_t ShimRegistry::ResolveExport(const std::wstring& dllName, const char* funcName) {
    Key k{ LowerW(dllName), LowerA(funcName) };
    auto it = m_table.find(k);
    if (it == m_table.end()) return 0;
    return reinterpret_cast<uint64_t>(it->second);
}

bool ShimRegistry::HasModule(const std::wstring& dllName) {
    auto dl = LowerW(dllName);
    for (const auto& kv : m_table) {
        if (kv.first.dllLower == dl) return true;
    }
    return false;
}

std::vector<ShimRegistry::ModuleStats> ShimRegistry::GetAllModuleStats() {
    std::unordered_map<std::wstring, size_t> counts;
    for (const auto& kv : m_table) counts[kv.first.dllLower]++;
    std::vector<ModuleStats> r;
    r.reserve(counts.size());
    for (const auto& c : counts) r.push_back({c.first, c.second});
    std::sort(r.begin(), r.end(),
              [](const ModuleStats& a, const ModuleStats& b) {
                  return a.name < b.name;
              });
    return r;
}

std::vector<ShimRegistry::Entry> ShimRegistry::GetAllEntries() const {
    std::vector<Entry> r;
    r.reserve(m_table.size());
    for (const auto& kv : m_table) {
        r.push_back(Entry{kv.first.dllLower, kv.first.funcLower, kv.second});
    }
    return r;
}

}  // namespace xwr
