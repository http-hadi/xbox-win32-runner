// test/stub_win32_impl.cpp
// Minimal stub implementations of the Win32 functions that test_app_wiring.cpp
// transitively calls. This lets us LINK the test on Linux (not just syntax-check).
//
// The stubs just return success/failure values; they don't do real work.

#include <Windows.h>
#include <cstring>

// PathTranslator calls these
extern "C" {

DWORD WINAPI GetCurrentDirectoryW(DWORD n, LPWSTR buf) {
    if (buf && n > 0) {
        const wchar_t* cwd = L"/tmp";
        size_t len = wcslen(cwd);
        if (len >= n) return len + 1;
        wcscpy(buf, cwd);
        return static_cast<DWORD>(len);
    }
    return 5;  // length of "/tmp"
}

// ShimRegistry doesn't call any Win32 functions at construction time.
// PeLoader calls CreateFileW / ReadFile / GetFileSizeEx / CloseHandle —
// but the test doesn't instantiate PeLoader, so we don't need these.

}  // extern "C"
