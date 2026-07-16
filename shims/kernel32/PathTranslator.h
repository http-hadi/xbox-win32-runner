// shims/kernel32/PathTranslator.h
// Translates Win32 paths (e.g. "C:\Game\save.dat") to UWP-local-folder paths
// so game file I/O works transparently under AppContainer.
//
// Strategy: maintain a virtual "C:\" drive root that maps to the UWP local
// folder. Other drives (D:\, E:\) map to subfolders. The first call lazily
// queries Windows.ApplicationData.Current.LocalFolder via C++/WinRT.
//
// Games mostly use relative paths (resolved via SetCurrentDirectoryW) which
// are stored as the "current virtual directory" and prepended to relative paths.

#pragma once
#include <Windows.h>
#include <string>

namespace xwr {

class PathTranslator {
public:
    static PathTranslator& Instance();

    // Set the virtual "current directory" (game-relative path).
    void SetVirtualCwd(const std::wstring& cwd) { m_virtualCwd = cwd; }
    const std::wstring& GetVirtualCwd() const { return m_virtualCwd; }

    // Set the root that "C:\" maps to (typically the UWP local folder).
    void SetPhysicalRoot(const std::wstring& root) { m_physicalRoot = root; }
    const std::wstring& GetPhysicalRoot() const { return m_physicalRoot; }

    // Translate a Win32 path to a real UWP path.
    // Handles: drive letters, UNC, relative paths (against virtualCwd).
    std::wstring TranslateToReal(const std::wstring& virtualPath) const;

    // Inverse - mostly used for diagnostic output.
    std::wstring TranslateToVirtual(const std::wstring& realPath) const;

private:
    PathTranslator();
    std::wstring m_physicalRoot;  // e.g. C:/Users/.../LocalCache/Local
    std::wstring m_virtualCwd;    // e.g. /Game  (relative to C:)
};

}  // namespace xwr
