// shims/version/VersionShim.cpp
// Win32 version.dll shim layer. Most games only need GetFileVersionInfo to
// succeed (or fail consistently) so we just return FALSE/0 to keep them
// moving. WinVerifyTrust always returns S_OK so installers don't refuse to
// run code-signed binaries that the UWP trust subsystem can't validate.

#include "UwpSdkIncludes.h"


#include <Windows.h>
#include <string>

#include "../ShimRegistry.h"

namespace xwr {

extern "C" BOOL __stdcall Shim_GetFileVersionInfoW(LPCWSTR, DWORD, DWORD, LPVOID) { return FALSE; }
extern "C" BOOL __stdcall Shim_GetFileVersionInfoSizeW(LPCWSTR, LPDWORD lpdwLen) {
    if (lpdwLen) *lpdwLen = 0;
    return FALSE;
}
extern "C" BOOL __stdcall Shim_GetFileVersionInfoSizeExW(DWORD, LPCWSTR, LPDWORD lpdwLen) {
    if (lpdwLen) *lpdwLen = 0;
    return FALSE;
}
extern "C" BOOL __stdcall Shim_VerQueryValueW(LPCVOID, LPCWSTR, LPVOID*, PUINT) { return FALSE; }
extern "C" BOOL __stdcall Shim_VerInstallFileW(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT) { return FALSE; }
extern "C" DWORD __stdcall Shim_VerLanguageNameW(DWORD dwLang, LPWSTR szLang, DWORD nSize) {
    if (!szLang || nSize == 0) return 0;
    // Map the LangID low word to a friendlier English string.
    const wchar_t* name = L"English (United States)";
    switch (LOWORD(dwLang)) {
        case 0x0409: name = L"English (United States)"; break;
        case 0x040C: name = L"French (France)"; break;
        case 0x0407: name = L"German (Germany)"; break;
        case 0x0410: name = L"Italian (Italy)"; break;
        case 0x040A: name = L"Spanish (Spain)"; break;
        case 0x0411: name = L"Japanese"; break;
        case 0x0412: name = L"Korean"; break;
        case 0x0804: name = L"Chinese (Simplified)"; break;
        case 0x0404: name = L"Chinese (Traditional)"; break;
        case 0x0419: name = L"Russian"; break;
        default:     name = L"English (United States)"; break;
    }
    size_t len = std::wcslen(name);
    if (len + 1 > nSize) {
        std::wcsncpy(szLang, name, nSize - 1);
        szLang[nSize - 1] = L'\0';
        return (DWORD)(nSize - 1);
    }
    std::wcscpy(szLang, name);
    return (DWORD)len;
}
extern "C" BOOL __stdcall Shim_VerFindFileW(DWORD, LPCWSTR, LPCWSTR, LPCWSTR, LPWSTR, PUINT, LPWSTR, PUINT) { return FALSE; }
extern "C" HRESULT __stdcall Shim_WinVerifyTrust(HWND, const GUID*, LPVOID) { return S_OK; }

}  // namespace xwr

// ===========================================================================
// Registration — also covers "version.dll" alias.
// ===========================================================================
REGISTER_SHIM("version", "GetFileVersionInfoW", (FARPROC)&xwr::Shim_GetFileVersionInfoW);
REGISTER_SHIM("version", "GetFileVersionInfoA", (FARPROC)&xwr::Shim_GetFileVersionInfoW);
REGISTER_SHIM("version", "GetFileVersionInfoSizeW", (FARPROC)&xwr::Shim_GetFileVersionInfoSizeW);
REGISTER_SHIM("version", "GetFileVersionInfoSizeA", (FARPROC)&xwr::Shim_GetFileVersionInfoSizeW);
REGISTER_SHIM("version", "GetFileVersionInfoSizeExW", (FARPROC)&xwr::Shim_GetFileVersionInfoSizeExW);
REGISTER_SHIM("version", "GetFileVersionInfoSizeExA", (FARPROC)&xwr::Shim_GetFileVersionInfoSizeExW);
REGISTER_SHIM("version", "VerQueryValueW", (FARPROC)&xwr::Shim_VerQueryValueW);
REGISTER_SHIM("version", "VerQueryValueA", (FARPROC)&xwr::Shim_VerQueryValueW);
REGISTER_SHIM("version", "VerInstallFileW", (FARPROC)&xwr::Shim_VerInstallFileW);
REGISTER_SHIM("version", "VerLanguageNameW", (FARPROC)&xwr::Shim_VerLanguageNameW);
REGISTER_SHIM("version", "VerFindFileW", (FARPROC)&xwr::Shim_VerFindFileW);
REGISTER_SHIM("version", "WinVerifyTrust", (FARPROC)&xwr::Shim_WinVerifyTrust);
