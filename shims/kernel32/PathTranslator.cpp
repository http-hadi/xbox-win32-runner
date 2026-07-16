// shims/kernel32/PathTranslator.cpp
#include "UwpSdkIncludes.h"

#include "PathTranslator.h"
#include <algorithm>
#include <cwctype>

namespace xwr {

PathTranslator& PathTranslator::Instance() {
    static PathTranslator inst;
    return inst;
}

PathTranslator::PathTranslator() {
    // Default: use current process directory as physical root.
    // On real UWP this would be the LocalFolder; the UWP shell sets it.
    WCHAR buf[MAX_PATH] = {0};
    ::GetCurrentDirectoryW(MAX_PATH, buf);
    m_physicalRoot = buf;
    if (!m_physicalRoot.empty() && m_physicalRoot.back() != L'\\')
        m_physicalRoot.push_back(L'\\');
    m_virtualCwd = L"\\";
}

static std::wstring Lower(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
    return r;
}

std::wstring PathTranslator::TranslateToReal(const std::wstring& virtualPath) const {
    if (virtualPath.empty()) return m_physicalRoot;

    // Case 1: drive-relative path like "C:\foo\bar" or "c:/foo/bar"
    if (virtualPath.size() >= 2 && virtualPath[1] == L':') {
        std::wstring drive = Lower(virtualPath.substr(0, 1));
        std::wstring rest = virtualPath.substr(2);
        // Replace forward slashes with backslashes
        for (auto& c : rest) if (c == L'/') c = L'\\';
        if (!rest.empty() && rest[0] == L'\\') rest = rest.substr(1);
        std::wstring result = m_physicalRoot;
        if (drive != L"c") {
            result += drive;
            result += L"\\";
        }
        result += rest;
        return result;
    }

    // Case 2: UNC path like "\\server\share\..."
    if (virtualPath.size() >= 2 && virtualPath[0] == L'\\' && virtualPath[1] == L'\\') {
        // Treat as: <root>\UNC\<rest>
        std::wstring rest = virtualPath.substr(2);
        std::wstring result = m_physicalRoot;
        result += L"UNC\\";
        result += rest;
        for (auto& c : result) if (c == L'/') c = L'\\';
        return result;
    }

    // Case 3: absolute path without drive like "\foo\bar"
    if (virtualPath[0] == L'\\' || virtualPath[0] == L'/') {
        std::wstring rest = virtualPath.substr(1);
        for (auto& c : rest) if (c == L'/') c = L'\\';
        return m_physicalRoot + rest;
    }

    // Case 4: relative path - prepend virtualCwd
    std::wstring cwd = m_virtualCwd;
    if (cwd.empty() || cwd.back() != L'\\') cwd.push_back(L'\\');
    std::wstring result = cwd + virtualPath;
    for (auto& c : result) if (c == L'/') c = L'\\';
    // Translate the combined path (which is now absolute virtual) to real
    if (!result.empty() && result[0] == L'\\') {
        return m_physicalRoot + result.substr(1);
    }
    return m_physicalRoot + result;
}

std::wstring PathTranslator::TranslateToVirtual(const std::wstring& realPath) const {
    std::wstring lowerReal = Lower(realPath);
    std::wstring lowerRoot = Lower(m_physicalRoot);
    if (lowerReal.find(lowerRoot) == 0) {
        std::wstring rest = realPath.substr(m_physicalRoot.size());
        return L"C:\\" + rest;
    }
    return realPath;
}

}  // namespace xwr
