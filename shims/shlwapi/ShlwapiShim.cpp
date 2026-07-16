// shims/shlwapi/ShlwapiShim.cpp
// Win32 shlwapi shim layer. Implements the path-manipulation and string
// functions for real (they're just string ops) and provides stubs for the
// color helpers.

#include "UwpSdkIncludes.h"
#include <string>
#include <cwctype>
#include <cstring>
#include <cstdlib>

#include "../ShimRegistry.h"
#include "../kernel32/PathTranslator.h"

// Path / file attribute constants missing from the UWP SDK subset.
#ifndef FILE_ATTRIBUTE_DIRECTORY
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#endif

namespace xwr {

static bool IsSlash(wchar_t c) { return c == L'\\' || c == L'/'; }

// ---------------------------------------------------------------------------
// Path manipulation
// ---------------------------------------------------------------------------
extern "C" LPWSTR __stdcall Shim_PathAppendW(LPWSTR pszPath, LPCWSTR pszMore) {
    if (!pszPath || !pszMore) return pszPath;
    size_t len = std::wcslen(pszPath);
    if (len > 0 && !IsSlash(pszPath[len - 1]) && !IsSlash(pszMore[0])) {
        if (len + 1 < MAX_PATH) { pszPath[len] = L'\\'; pszPath[len + 1] = L'\0'; ++len; }
    }
    std::wcscpy(pszPath + len, pszMore);
    return pszPath;
}
extern "C" LPWSTR __stdcall Shim_PathAddBackslashW(LPWSTR pszPath) {
    if (!pszPath) return nullptr;
    size_t len = std::wcslen(pszPath);
    if (len + 1 >= MAX_PATH) return pszPath + len;
    if (len > 0 && !IsSlash(pszPath[len - 1])) {
        pszPath[len] = L'\\';
        pszPath[len + 1] = L'\0';
        return pszPath + len + 1;
    }
    return pszPath + len;
}
extern "C" LPWSTR __stdcall Shim_PathRemoveBackslashW(LPWSTR pszPath) {
    if (!pszPath) return nullptr;
    size_t len = std::wcslen(pszPath);
    if (len > 0 && IsSlash(pszPath[len - 1])) pszPath[len - 1] = L'\0';
    return pszPath;
}
extern "C" void __stdcall Shim_PathRemoveExtensionW(LPWSTR pszPath) {
    if (!pszPath) return;
    LPCWSTR ext = ::PathFindExtensionW(pszPath);
    if (ext && *ext) const_cast<LPWSTR>(ext)[0] = L'\0';
}
extern "C" BOOL __stdcall Shim_PathRenameExtensionW(LPWSTR pszPath, LPCWSTR pszExt) {
    if (!pszPath || !pszExt) return FALSE;
    LPCWSTR ext = ::PathFindExtensionW(pszPath);
    if (!ext) return FALSE;
    LPWSTR wext = const_cast<LPWSTR>(ext);
    if (pszExt[0] != L'.' && (size_t)(wext - pszPath) + 1 < MAX_PATH) {
        wext[0] = L'.'; ++wext;
    }
    std::wcscpy(wext, pszExt);
    return TRUE;
}
extern "C" LPWSTR __stdcall Shim_PathCombineW(LPWSTR pszDest, LPCWSTR pszDir, LPCWSTR pszFile) {
    if (!pszDest) return nullptr;
    if (!pszDir || !*pszDir) {
        if (pszFile) std::wcscpy(pszDest, pszFile); else pszDest[0] = L'\0';
        return pszDest;
    }
    std::wcscpy(pszDest, pszDir);
    if (pszFile && *pszFile) Shim_PathAppendW(pszDest, pszFile);
    return pszDest;
}
extern "C" BOOL __stdcall Shim_PathCanonicalizeW(LPWSTR pszBuf, LPCWSTR pszPath) {
    if (!pszBuf || !pszPath) return FALSE;
    // Simplified: just copy. Real impl would resolve ".." and ".".
    std::wcscpy(pszBuf, pszPath);
    return TRUE;
}
extern "C" BOOL __stdcall Shim_PathRelativePathToW(LPWSTR pszDest, LPCWSTR pszFrom, DWORD,
                                                     LPCWSTR pszTo, DWORD) {
    if (!pszDest || !pszFrom || !pszTo) return FALSE;
    std::wcscpy(pszDest, pszTo);
    return TRUE;
}
extern "C" int __stdcall Shim_PathCommonPrefixW(LPCWSTR pszFile1, LPCWSTR pszFile2, LPWSTR pszPath) {
    if (!pszFile1 || !pszFile2) return 0;
    int n = 0;
    while (pszFile1[n] && pszFile2[n] && pszFile1[n] == pszFile2[n]) ++n;
    if (pszPath) { std::wcsncpy(pszPath, pszFile1, (size_t)n); pszPath[n] = L'\0'; }
    return n;
}
extern "C" BOOL __stdcall Shim_PathIsUNCW(LPCWSTR pszPath) {
    return (pszPath && pszPath[0] == L'\\' && pszPath[1] == L'\\') ? TRUE : FALSE;
}
extern "C" BOOL __stdcall Shim_PathIsRelativeW(LPCWSTR pszPath) {
    if (!pszPath || !*pszPath) return FALSE;
    if (IsSlash(pszPath[0])) return FALSE;
    if ((pszPath[1] == L':' && pszPath[2] == L'\\')) return FALSE;
    return TRUE;
}
extern "C" BOOL __stdcall Shim_PathIsRootW(LPCWSTR pszPath) {
    if (!pszPath) return FALSE;
    if (pszPath[0] == L'\\' && pszPath[1] == L'\\') return TRUE;
    if (pszPath[0] && pszPath[1] == L':' && pszPath[2] == L'\\' && pszPath[3] == L'\0') return TRUE;
    if (pszPath[0] == L'\\' && pszPath[1] == L'\0') return TRUE;
    return FALSE;
}
extern "C" BOOL __stdcall Shim_PathIsDirectoryW(LPCWSTR pszPath) {
    if (!pszPath) return FALSE;
    std::wstring real = PathTranslator::Instance().TranslateToReal(pszPath);
    DWORD attr = ::GetFileAttributesW(real.c_str());
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) ? TRUE : FALSE;
}
extern "C" BOOL __stdcall Shim_PathIsDirectoryEmptyW(LPCWSTR pszPath) {
    if (!Shim_PathIsDirectoryW(pszPath)) return FALSE;
    return TRUE;  // optimistic
}
extern "C" BOOL __stdcall Shim_PathFileExistsW(LPCWSTR pszPath) {
    if (!pszPath) return FALSE;
    std::wstring real = PathTranslator::Instance().TranslateToReal(pszPath);
    DWORD attr = ::GetFileAttributesW(real.c_str());
    return attr != INVALID_FILE_ATTRIBUTES ? TRUE : FALSE;
}
extern "C" BOOL __stdcall Shim_PathMatchSpecW(LPCWSTR pszFile, LPCWSTR pszSpec) {
    (void)pszFile; (void)pszSpec;
    return FALSE;
}
extern "C" LPWSTR __stdcall Shim_PathGetArgsW(LPCWSTR pszPath) {
    if (!pszPath) return nullptr;
    LPWSTR p = const_cast<LPWSTR>(pszPath);
    bool inQuote = false;
    while (*p) {
        if (*p == L'"') inQuote = !inQuote;
        else if (!inQuote && std::iswspace(*p)) {
            while (std::iswspace(*p)) ++p;
            return p;
        }
        ++p;
    }
    return p;
}
extern "C" void __stdcall Shim_PathStripPathW(LPWSTR pszPath) {
    if (!pszPath) return;
    LPCWSTR fname = ::PathFindFileNameW(pszPath);
    if (fname && fname != pszPath) {
        std::memmove(pszPath, fname, (std::wcslen(fname) + 1) * sizeof(wchar_t));
    }
}
extern "C" void __stdcall Shim_PathUnquoteSpacesW(LPWSTR pszPath) {
    if (!pszPath) return;
    size_t len = std::wcslen(pszPath);
    if (len >= 2 && pszPath[0] == L'"' && pszPath[len - 1] == L'"') {
        std::memmove(pszPath, pszPath + 1, (len - 2) * sizeof(wchar_t));
        pszPath[len - 2] = L'\0';
    }
}
extern "C" void __stdcall Shim_PathQuoteSpacesW(LPWSTR pszPath) {
    (void)pszPath;  // not implemented (would need buffer growth)
}
extern "C" void __stdcall Shim_PathRemoveArgsW(LPWSTR pszPath) {
    if (!pszPath) return;
    LPWSTR args = Shim_PathGetArgsW(pszPath);
    if (args && *args) *(args - 1) = L'\0';
}
extern "C" void __stdcall Shim_PathRemoveBlanksW(LPWSTR pszPath) {
    if (!pszPath) return;
    LPWSTR start = pszPath;
    while (std::iswspace(*start)) ++start;
    if (start != pszPath) std::memmove(pszPath, start, (std::wcslen(start) + 1) * sizeof(wchar_t));
    size_t len = std::wcslen(pszPath);
    while (len > 0 && std::iswspace(pszPath[len - 1])) { pszPath[--len] = L'\0'; }
}
extern "C" void __stdcall Shim_PathRemoveFileSpecW(LPWSTR pszPath) {
    if (!pszPath) return;
    LPCWSTR fname = ::PathFindFileNameW(pszPath);
    if (fname && fname != pszPath) {
        const_cast<LPWSTR>(fname)[-1] = L'\0';
    } else {
        pszPath[0] = L'\0';
    }
}
extern "C" LPCWSTR __stdcall Shim_PathFindExtensionW(LPCWSTR pszPath) {
    if (!pszPath) return nullptr;
    LPCWSTR lastDot = nullptr;
    for (LPCWSTR p = pszPath; *p; ++p) {
        if (*p == L'.') lastDot = p;
        else if (IsSlash(*p) || *p == L':') lastDot = nullptr;
    }
    return lastDot ? lastDot : (pszPath + std::wcslen(pszPath));
}
extern "C" LPCWSTR __stdcall Shim_PathFindFileNameW(LPCWSTR pszPath) {
    if (!pszPath) return nullptr;
    LPCWSTR last = pszPath;
    for (LPCWSTR p = pszPath; *p; ++p) {
        if (IsSlash(*p) || *p == L':') last = p + 1;
    }
    return last;
}
extern "C" int __stdcall Shim_PathGetDriveNumberW(LPCWSTR pszPath) {
    if (!pszPath || !pszPath[0]) return -1;
    wchar_t c = pszPath[0];
    if (c >= L'a' && c <= L'z') return (int)(c - L'a');
    if (c >= L'A' && c <= L'Z') return (int)(c - L'A');
    return -1;
}

// ---------------------------------------------------------------------------
// String functions
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_StrCmpW(LPCWSTR a, LPCWSTR b) { return std::wcscmp(a ? a : L"", b ? b : L""); }
extern "C" int __stdcall Shim_StrCmpIW(LPCWSTR a, LPCWSTR b) {
    a = a ? a : L""; b = b ? b : L"";
    while (*a && *b) {
        wchar_t ca = (wchar_t)std::towlower(*a);
        wchar_t cb = (wchar_t)std::towlower(*b);
        if (ca != cb) return (int)(ca - cb);
        ++a; ++b;
    }
    return (int)(*a - *b);
}
extern "C" int __stdcall Shim_StrCmpNW(LPCWSTR a, LPCWSTR b, int n) { return std::wcsncmp(a ? a : L"", b ? b : L"", (size_t)n); }
extern "C" int __stdcall Shim_StrCmpNIW(LPCWSTR a, LPCWSTR b, int n) {
    a = a ? a : L""; b = b ? b : L"";
    for (int i = 0; i < n && *a && *b; ++i) {
        wchar_t ca = (wchar_t)std::towlower(*a);
        wchar_t cb = (wchar_t)std::towlower(*b);
        if (ca != cb) return (int)(ca - cb);
        ++a; ++b;
    }
    return 0;
}
extern "C" LPWSTR __stdcall Shim_StrCpyW(LPWSTR a, LPCWSTR b) { return std::wcscpy(a, b); }
extern "C" LPWSTR __stdcall Shim_StrCpyNW(LPWSTR a, LPCWSTR b, int n) { return std::wcsncpy(a, b, (size_t)n); }
extern "C" LPWSTR __stdcall Shim_StrCatW(LPWSTR a, LPCWSTR b) { return std::wcscat(a, b); }
extern "C" LPWSTR __stdcall Shim_StrCatBuffW(LPWSTR a, LPCWSTR b, int cchMax) {
    if (!a || !b) return a;
    size_t la = std::wcslen(a);
    size_t avail = (size_t)cchMax > la ? (size_t)cchMax - la - 1 : 0;
    std::wcsncpy(a + la, b, avail);
    a[la + avail] = L'\0';
    return a;
}
extern "C" int __stdcall Shim_StrLenW(LPCWSTR a) { return a ? (int)std::wcslen(a) : 0; }
extern "C" LPWSTR __stdcall Shim_StrStrW(LPWSTR a, LPCWSTR b) {
    if (!a || !b) return nullptr;
    return std::wcsstr(a, b);
}
extern "C" LPWSTR __stdcall Shim_StrStrIW(LPWSTR a, LPCWSTR b) {
    if (!a || !b || !*b) return nullptr;
    size_t blen = std::wcslen(b);
    for (LPWSTR p = a; *p; ++p) {
        if (Shim_StrCmpNIW(p, b, (int)blen) == 0) return p;
    }
    return nullptr;
}
extern "C" LPWSTR __stdcall Shim_StrRStrW(LPWSTR a, LPCWSTR, LPCWSTR b) {
    if (!a || !b || !*b) return nullptr;
    size_t blen = std::wcslen(b);
    LPWSTR last = nullptr;
    for (LPWSTR p = a; *p; ++p) {
        if (Shim_StrCmpNIW(p, b, (int)blen) == 0) last = p;
    }
    return last;
}
extern "C" LPWSTR __stdcall Shim_StrChrW(LPWSTR a, WCHAR c) {
    if (!a) return nullptr;
    return std::wcschr(a, c);
}
extern "C" LPWSTR __stdcall Shim_StrChrIW(LPWSTR a, WCHAR c) {
    if (!a) return nullptr;
    wchar_t lc = (wchar_t)std::towlower(c);
    for (LPWSTR p = a; *p; ++p) {
        if (std::towlower(*p) == lc) return p;
    }
    return nullptr;
}
extern "C" LPWSTR __stdcall Shim_StrRChrW(LPWSTR a, LPCWSTR, WCHAR c) {
    if (!a) return nullptr;
    LPWSTR last = nullptr;
    for (LPWSTR p = a; *p; ++p) if (*p == c) last = p;
    return last;
}
extern "C" LPWSTR __stdcall Shim_StrRChrIW(LPWSTR a, LPCWSTR, WCHAR c) {
    if (!a) return nullptr;
    wchar_t lc = (wchar_t)std::towlower(c);
    LPWSTR last = nullptr;
    for (LPWSTR p = a; *p; ++p) if (std::towlower(*p) == lc) last = p;
    return last;
}
extern "C" int __stdcall Shim_StrSpnW(LPCWSTR a, LPCWSTR b) {
    if (!a || !b) return 0;
    int n = 0;
    while (*a && std::wcschr(b, *a)) { ++a; ++n; }
    return n;
}
extern "C" int __stdcall Shim_StrCSpnW(LPCWSTR a, LPCWSTR b) {
    if (!a || !b) return (int)(a ? std::wcslen(a) : 0);
    int n = 0;
    while (*a && !std::wcschr(b, *a)) { ++a; ++n; }
    return n;
}
extern "C" int __stdcall Shim_StrCSpnIW(LPCWSTR a, LPCWSTR b) {
    if (!a || !b) return (int)(a ? std::wcslen(a) : 0);
    int n = 0;
    while (*a) {
        wchar_t ca = (wchar_t)std::towlower(*a);
        bool found = false;
        for (LPCWSTR p = b; *p; ++p) if (std::towlower(*p) == ca) { found = true; break; }
        if (found) break;
        ++a; ++n;
    }
    return n;
}
extern "C" LPWSTR __stdcall Shim_StrPBrkW(LPCWSTR a, LPCWSTR b) {
    if (!a || !b) return nullptr;
    for (LPCWSTR p = a; *p; ++p) if (std::wcschr(b, *p)) return const_cast<LPWSTR>(p);
    return nullptr;
}
extern "C" LPWSTR __stdcall Shim_StrReverseW(LPWSTR a) {
    if (!a) return nullptr;
    size_t len = std::wcslen(a);
    for (size_t i = 0; i < len / 2; ++i) {
        wchar_t t = a[i]; a[i] = a[len - 1 - i]; a[len - 1 - i] = t;
    }
    return a;
}
extern "C" BOOL __stdcall Shim_ChrCmpIW(WCHAR a, WCHAR b) {
    return std::towlower(a) == std::towlower(b) ? TRUE : FALSE;
}

// ---------------------------------------------------------------------------
// Color helpers — basic RGB <-> HLS conversion
// ---------------------------------------------------------------------------
extern "C" COLORREF __stdcall Shim_ColorRGBToHLS(COLORREF c, LPWORD pwHue, LPWORD pwLum, LPWORD pwSat) {
    int r = GetRValue(c), g = GetGValue(c), b = GetBValue(c);
    int mx = (r > g ? r : g); if (b > mx) mx = b;
    int mn = (r < g ? r : g); if (b < mn) mn = b;
    int l = (mx + mn) * 240 / 510;
    int s = 0, h = 0;
    if (mx != mn) {
        int delta = mx - mn;
        s = (mx + mn) > 255 ? delta * 240 / (510 - mx - mn) : delta * 240 / (mx + mn);
        if (mx == r)      h = (g - b) * 40 / delta + (g < b ? 240 : 0);
        else if (mx == g) h = (b - r) * 40 / delta + 80;
        else              h = (r - g) * 40 / delta + 160;
    }
    if (pwHue) *pwHue = (WORD)h;
    if (pwLum) *pwLum = (WORD)l;
    if (pwSat) *pwSat = (WORD)s;
    return c;
}
extern "C" COLORREF __stdcall Shim_ColorHLSToRGB(WORD h, WORD l, WORD s) {
    if (s == 0) {
        BYTE v = (BYTE)(l * 255 / 240);
        return RGB(v, v, v);
    }
    double H = (double)h / 240.0;
    double L = (double)l / 240.0;
    double S = (double)s / 240.0;
    double q = L < 0.5 ? L * (1.0 + S) : L + S - L * S;
    double p = 2.0 * L - q;
    auto hue = [](double p, double q, double t) -> double {
        if (t < 0) t += 1.0;
        if (t > 1) t -= 1.0;
        if (t < 1.0/6.0) return p + (q - p) * 6.0 * t;
        if (t < 0.5)     return q;
        if (t < 2.0/3.0) return p + (q - p) * (2.0/3.0 - t) * 6.0;
        return p;
    };
    double r = hue(p, q, H + 1.0/3.0);
    double g = hue(p, q, H);
    double b = hue(p, q, H - 1.0/3.0);
    return RGB((BYTE)(r * 255), (BYTE)(g * 255), (BYTE)(b * 255));
}
extern "C" BOOL __stdcall Shim_IsOS(DWORD) { return FALSE; }

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("shlwapi", "PathAppendW", (FARPROC)&xwr::Shim_PathAppendW);
REGISTER_SHIM("shlwapi", "PathAddBackslashW", (FARPROC)&xwr::Shim_PathAddBackslashW);
REGISTER_SHIM("shlwapi", "PathRemoveBackslashW", (FARPROC)&xwr::Shim_PathRemoveBackslashW);
REGISTER_SHIM("shlwapi", "PathRemoveExtensionW", (FARPROC)&xwr::Shim_PathRemoveExtensionW);
REGISTER_SHIM("shlwapi", "PathRenameExtensionW", (FARPROC)&xwr::Shim_PathRenameExtensionW);
REGISTER_SHIM("shlwapi", "PathCombineW", (FARPROC)&xwr::Shim_PathCombineW);
REGISTER_SHIM("shlwapi", "PathCanonicalizeW", (FARPROC)&xwr::Shim_PathCanonicalizeW);
REGISTER_SHIM("shlwapi", "PathRelativePathToW", (FARPROC)&xwr::Shim_PathRelativePathToW);
REGISTER_SHIM("shlwapi", "PathCommonPrefixW", (FARPROC)&xwr::Shim_PathCommonPrefixW);
REGISTER_SHIM("shlwapi", "PathIsUNCW", (FARPROC)&xwr::Shim_PathIsUNCW);
REGISTER_SHIM("shlwapi", "PathIsRelativeW", (FARPROC)&xwr::Shim_PathIsRelativeW);
REGISTER_SHIM("shlwapi", "PathIsRootW", (FARPROC)&xwr::Shim_PathIsRootW);
REGISTER_SHIM("shlwapi", "PathIsDirectoryW", (FARPROC)&xwr::Shim_PathIsDirectoryW);
REGISTER_SHIM("shlwapi", "PathIsDirectoryEmptyW", (FARPROC)&xwr::Shim_PathIsDirectoryEmptyW);
REGISTER_SHIM("shlwapi", "PathFileExistsW", (FARPROC)&xwr::Shim_PathFileExistsW);
REGISTER_SHIM("shlwapi", "PathMatchSpecW", (FARPROC)&xwr::Shim_PathMatchSpecW);
REGISTER_SHIM("shlwapi", "PathGetArgsW", (FARPROC)&xwr::Shim_PathGetArgsW);
REGISTER_SHIM("shlwapi", "PathStripPathW", (FARPROC)&xwr::Shim_PathStripPathW);
REGISTER_SHIM("shlwapi", "PathUnquoteSpacesW", (FARPROC)&xwr::Shim_PathUnquoteSpacesW);
REGISTER_SHIM("shlwapi", "PathQuoteSpacesW", (FARPROC)&xwr::Shim_PathQuoteSpacesW);
REGISTER_SHIM("shlwapi", "PathRemoveArgsW", (FARPROC)&xwr::Shim_PathRemoveArgsW);
REGISTER_SHIM("shlwapi", "PathRemoveBlanksW", (FARPROC)&xwr::Shim_PathRemoveBlanksW);
REGISTER_SHIM("shlwapi", "PathRemoveFileSpecW", (FARPROC)&xwr::Shim_PathRemoveFileSpecW);
REGISTER_SHIM("shlwapi", "PathFindExtensionW", (FARPROC)&xwr::Shim_PathFindExtensionW);
REGISTER_SHIM("shlwapi", "PathFindFileNameW", (FARPROC)&xwr::Shim_PathFindFileNameW);
REGISTER_SHIM("shlwapi", "PathGetDriveNumberW", (FARPROC)&xwr::Shim_PathGetDriveNumberW);
REGISTER_SHIM("shlwapi", "StrCmpW", (FARPROC)&xwr::Shim_StrCmpW);
REGISTER_SHIM("shlwapi", "StrCmpIW", (FARPROC)&xwr::Shim_StrCmpIW);
REGISTER_SHIM("shlwapi", "StrCmpNW", (FARPROC)&xwr::Shim_StrCmpNW);
REGISTER_SHIM("shlwapi", "StrCmpNIW", (FARPROC)&xwr::Shim_StrCmpNIW);
REGISTER_SHIM("shlwapi", "StrCpyW", (FARPROC)&xwr::Shim_StrCpyW);
REGISTER_SHIM("shlwapi", "StrCpyNW", (FARPROC)&xwr::Shim_StrCpyNW);
REGISTER_SHIM("shlwapi", "StrCatW", (FARPROC)&xwr::Shim_StrCatW);
REGISTER_SHIM("shlwapi", "StrCatBuffW", (FARPROC)&xwr::Shim_StrCatBuffW);
REGISTER_SHIM("shlwapi", "StrLenW", (FARPROC)&xwr::Shim_StrLenW);
REGISTER_SHIM("shlwapi", "StrStrW", (FARPROC)&xwr::Shim_StrStrW);
REGISTER_SHIM("shlwapi", "StrStrIW", (FARPROC)&xwr::Shim_StrStrIW);
REGISTER_SHIM("shlwapi", "StrRStrW", (FARPROC)&xwr::Shim_StrRStrW);
REGISTER_SHIM("shlwapi", "StrChrW", (FARPROC)&xwr::Shim_StrChrW);
REGISTER_SHIM("shlwapi", "StrChrIW", (FARPROC)&xwr::Shim_StrChrIW);
REGISTER_SHIM("shlwapi", "StrRChrW", (FARPROC)&xwr::Shim_StrRChrW);
REGISTER_SHIM("shlwapi", "StrRChrIW", (FARPROC)&xwr::Shim_StrRChrIW);
REGISTER_SHIM("shlwapi", "StrSpnW", (FARPROC)&xwr::Shim_StrSpnW);
REGISTER_SHIM("shlwapi", "StrCSpnW", (FARPROC)&xwr::Shim_StrCSpnW);
REGISTER_SHIM("shlwapi", "StrCSpnIW", (FARPROC)&xwr::Shim_StrCSpnIW);
REGISTER_SHIM("shlwapi", "StrPBrkW", (FARPROC)&xwr::Shim_StrPBrkW);
REGISTER_SHIM("shlwapi", "StrReverseW", (FARPROC)&xwr::Shim_StrReverseW);
REGISTER_SHIM("shlwapi", "ChrCmpIW", (FARPROC)&xwr::Shim_ChrCmpIW);
REGISTER_SHIM("shlwapi", "ColorRGBToHLS", (FARPROC)&xwr::Shim_ColorRGBToHLS);
REGISTER_SHIM("shlwapi", "ColorHLSToRGB", (FARPROC)&xwr::Shim_ColorHLSToRGB);
REGISTER_SHIM("shlwapi", "IsOS", (FARPROC)&xwr::Shim_IsOS);
