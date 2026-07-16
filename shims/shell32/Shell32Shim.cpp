// shims/shell32/Shell32Shim.cpp
// Win32 shell32 shim layer. Maps CSIDL_APPDATA / CSIDL_LOCAL_APPDATA etc.
// to PathTranslator's physical root + subfolder so legacy games see paths
// that they can actually read/write under AppContainer.

#include "UwpSdkIncludes.h"


#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstring>

#include "../ShimRegistry.h"
#include "../kernel32/PathTranslator.h"

#ifndef ERROR_ALREADY_EXISTS
#define ERROR_ALREADY_EXISTS 183L
#endif

// Known-folder GUIDs (subset most games care about). Defined inline so we
// don't need to drag in knownfolders.h (which the UWP SDK doesn't expose to
// AppContainer code).
static const GUID XWR_FOLDERID_RoamingAppData    = { 0x3EB685DB, 0x65F9, 0x4F6D, { 0x8A, 0xDF, 0xE6, 0xDE, 0x9B, 0x60, 0x37, 0x6C } };
static const GUID XWR_FOLDERID_LocalAppData      = { 0xF1B32785, 0x6FBA, 0x4FCF, { 0x9D, 0x55, 0x7B, 0x8E, 0x7F, 0x15, 0x70, 0x92 } };
static const GUID XWR_FOLDERID_ProgramFiles      = { 0x905e63b6, 0xc1bf, 0x494e, { 0xb2, 0x9c, 0xad, 0x65, 0x69, 0xe0, 0x87, 0x88 } };
static const GUID XWR_FOLDERID_ProgramFilesX86   = { 0x7c5a40ef, 0xa0fb, 0x4bfc, { 0x97, 0x4d, 0xf1, 0xa1, 0xc3, 0x44, 0x3a, 0x4f } };
static const GUID XWR_FOLDERID_Windows           = { 0xf38bf404, 0x1d43, 0x42f2, { 0x93, 0x15, 0xc7, 0x59, 0xde, 0x57, 0x66, 0x8c } };
static const GUID XWR_FOLDERID_System            = { 0x1ac14e77, 0x02e7, 0x4e5d, { 0xb7, 0x44, 0x2e, 0xb1, 0xae, 0x51, 0x98, 0xb7 } };
static const GUID XWR_FOLDERID_Profile           = { 0x5e6c858f, 0x0e22, 0x476f, { 0x8a, 0xf3, 0xe2, 0x09, 0x49, 0x1f, 0x21, 0x43 } };
static const GUID XWR_FOLDERID_Documents         = { 0xfdd39ad0, 0x238f, 0x46af, { 0xad, 0xb4, 0x6c, 0x85, 0x48, 0x03, 0x69, 0xe7 } };
static const GUID XWR_FOLDERID_Music             = { 0x4bd8d571, 0x6d19, 0x48d3, { 0xbe, 0x97, 0x42, 0x23, 0x20, 0x0e, 0x71, 0xc1 } };
static const GUID XWR_FOLDERID_Pictures          = { 0x33e28130, 0x4e1e, 0x4676, { 0x83, 0x5a, 0x98, 0x39, 0x5c, 0x3b, 0xc3, 0xbb } };
static const GUID XWR_FOLDERID_Videos            = { 0x18989b1d, 0x99b5, 0x455b, { 0x84, 0x1c, 0xab, 0x7c, 0x74, 0xe4, 0x1f, 0xfc } };
static const GUID XWR_FOLDERID_ProgramData       = { 0x62ab5d82, 0xfdc1, 0x4dc3, { 0xa9, 0xdd, 0x07, 0x0d, 0x1d, 0xe5, 0x9f, 0x95 } };

static bool GuidEq(const GUID& a, const GUID& b) {
    return std::memcmp(&a, &b, sizeof(GUID)) == 0;
}

namespace xwr {

// Append a subfolder to the PathTranslator physical root, ensuring the result
// ends with a single backslash.
static std::wstring AppendToPhysicalRoot(const std::wstring& sub) {
    std::wstring root = PathTranslator::Instance().GetPhysicalRoot();
    if (root.empty()) root = L"C:\\XboxRunner";
    if (!root.empty() && root.back() != L'\\' && root.back() != L'/') root.push_back(L'\\');
    root += sub;
    return root;
}

extern "C" HRESULT __stdcall Shim_SHGetFolderPathW(HWND, int csidl, HANDLE, DWORD, LPWSTR pszPath) {
    if (!pszPath) return E_INVALIDARG;
    std::wstring sub;
    switch (csidl) {
        case CSIDL_APPDATA:          sub = L"AppData\\Roaming"; break;
        case CSIDL_LOCAL_APPDATA:    sub = L"AppData\\Local";   break;
        case CSIDL_PROGRAM_FILES:    sub = L"Program Files";    break;
        case CSIDL_PROGRAM_FILES_COMMON: sub = L"Program Files\\Common"; break;
        case CSIDL_WINDOWS:          sub = L"Windows";          break;
        case CSIDL_SYSTEM:           sub = L"System";           break;
        case CSIDL_PROFILE:          sub = L"Profile";          break;
        case CSIDL_PROGRAMS:         sub = L"Programs";         break;
        case CSIDL_STARTUP:          sub = L"Startup";          break;
        case CSIDL_MYDOCUMENTS:      sub = L"Documents";        break;
        case CSIDL_MYMUSIC:          sub = L"Music";            break;
        case CSIDL_MYPICTURES:       sub = L"Pictures";         break;
        case CSIDL_MYVIDEO:          sub = L"Videos";           break;
        default:                     sub = L"AppData\\Local";   break;
    }
    std::wstring path = AppendToPhysicalRoot(sub);
    if (path.size() >= MAX_PATH) return E_FAIL;
    std::memcpy(pszPath, path.c_str(), (path.size() + 1) * sizeof(wchar_t));
    return S_OK;
}

extern "C" HRESULT __stdcall Shim_SHGetKnownFolderPath(const GUID* rfid, DWORD, HANDLE, PWSTR* ppszPath) {
    if (!rfid || !ppszPath) return E_INVALIDARG;
    std::wstring sub;
    if (GuidEq(*rfid, XWR_FOLDERID_RoamingAppData))        sub = L"AppData\\Roaming";
    else if (GuidEq(*rfid, XWR_FOLDERID_LocalAppData))     sub = L"AppData\\Local";
    else if (GuidEq(*rfid, XWR_FOLDERID_ProgramFiles))     sub = L"Program Files";
    else if (GuidEq(*rfid, XWR_FOLDERID_ProgramFilesX86))  sub = L"Program Files (x86)";
    else if (GuidEq(*rfid, XWR_FOLDERID_Windows))          sub = L"Windows";
    else if (GuidEq(*rfid, XWR_FOLDERID_System))           sub = L"System";
    else if (GuidEq(*rfid, XWR_FOLDERID_Profile))          sub = L"Profile";
    else if (GuidEq(*rfid, XWR_FOLDERID_Documents))        sub = L"Documents";
    else if (GuidEq(*rfid, XWR_FOLDERID_Music))            sub = L"Music";
    else if (GuidEq(*rfid, XWR_FOLDERID_Pictures))         sub = L"Pictures";
    else if (GuidEq(*rfid, XWR_FOLDERID_Videos))           sub = L"Videos";
    else if (GuidEq(*rfid, XWR_FOLDERID_ProgramData))      sub = L"ProgramData";
    else                                                   sub = L"AppData\\Local";
    std::wstring path = AppendToPhysicalRoot(sub);
    // SHGetKnownFolderPath expects CoTaskMemAlloc'd memory; we approximate by
    // using malloc and assuming the caller uses CoTaskMemFree / free.
    size_t bytes = (path.size() + 1) * sizeof(wchar_t);
    PWSTR out = (PWSTR)std::malloc(bytes);
    if (!out) return E_OUTOFMEMORY;
    std::memcpy(out, path.c_str(), bytes);
    *ppszPath = out;
    return S_OK;
}

extern "C" BOOL __stdcall Shim_ShellExecuteExW(SHELLEXECUTEINFOW* pExecInfo) {
    if (!pExecInfo) return FALSE;
    // Pretend the operation was launched.
    return TRUE;
}
extern "C" HINSTANCE __stdcall Shim_ShellExecuteW(HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT) {
    return (HINSTANCE)32;  // ShellExecute returns >32 on success.
}
extern "C" int __stdcall Shim_SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp) {
    (void)lpFileOp;
    return 0;  // success
}
extern "C" HRESULT __stdcall Shim_SHCreateDirectoryExW(LPCWSTR pszPath, LPCWSTR pszSecurity,
                                                        LPSECURITY_ATTRIBUTES) {
    (void)pszSecurity;
    if (!pszPath) return E_INVALIDARG;
    std::wstring real = PathTranslator::Instance().TranslateToReal(pszPath);
    if (::CreateDirectoryW(real.c_str(), nullptr)) return S_OK;
    DWORD err = ::GetLastError();
    if (err == ERROR_ALREADY_EXISTS) return S_OK;
    return E_FAIL;
}
extern "C" DWORD __stdcall Shim_SHAutoComplete(HWND, DWORD) { return 0; }
extern "C" BOOL __stdcall Shim_IsUserAnAdmin() { return FALSE; }
extern "C" UINT __stdcall Shim_DragQueryFileW(HDROP, UINT, LPWSTR, UINT) { return 0; }
extern "C" BOOL __stdcall Shim_DragFinish(HDROP) { return TRUE; }
extern "C" void __stdcall Shim_SHAddToRecentDocs(UINT, LPCVOID) { }
extern "C" BOOL __stdcall Shim_SHGetPathFromIDListW(const void*, LPWSTR pszPath) {
    if (pszPath) pszPath[0] = L'\0';
    return FALSE;
}
extern "C" HRESULT __stdcall Shim_ILCreateFromPathW(LPCWSTR) {
    return E_NOTIMPL;
}
extern "C" void __stdcall Shim_ILFree(void*) { }

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("shell32", "SHGetFolderPathW", (FARPROC)&xwr::Shim_SHGetFolderPathW);
REGISTER_SHIM("shell32", "SHGetFolderPathA", (FARPROC)&xwr::Shim_SHGetFolderPathW);
REGISTER_SHIM("shell32", "SHGetKnownFolderPath", (FARPROC)&xwr::Shim_SHGetKnownFolderPath);
REGISTER_SHIM("shell32", "ShellExecuteW", (FARPROC)&xwr::Shim_ShellExecuteW);
REGISTER_SHIM("shell32", "ShellExecuteA", (FARPROC)&xwr::Shim_ShellExecuteW);
REGISTER_SHIM("shell32", "ShellExecuteExW", (FARPROC)&xwr::Shim_ShellExecuteExW);
REGISTER_SHIM("shell32", "ShellExecuteExA", (FARPROC)&xwr::Shim_ShellExecuteExW);
REGISTER_SHIM("shell32", "SHFileOperationW", (FARPROC)&xwr::Shim_SHFileOperationW);
REGISTER_SHIM("shell32", "SHFileOperationA", (FARPROC)&xwr::Shim_SHFileOperationW);
REGISTER_SHIM("shell32", "SHCreateDirectoryExW", (FARPROC)&xwr::Shim_SHCreateDirectoryExW);
REGISTER_SHIM("shell32", "SHAutoComplete", (FARPROC)&xwr::Shim_SHAutoComplete);
REGISTER_SHIM("shell32", "IsUserAnAdmin", (FARPROC)&xwr::Shim_IsUserAnAdmin);
REGISTER_SHIM("shell32", "DragQueryFileW", (FARPROC)&xwr::Shim_DragQueryFileW);
REGISTER_SHIM("shell32", "DragQueryFileA", (FARPROC)&xwr::Shim_DragQueryFileW);
REGISTER_SHIM("shell32", "DragFinish", (FARPROC)&xwr::Shim_DragFinish);
REGISTER_SHIM("shell32", "SHAddToRecentDocs", (FARPROC)&xwr::Shim_SHAddToRecentDocs);
REGISTER_SHIM("shell32", "SHGetPathFromIDListW", (FARPROC)&xwr::Shim_SHGetPathFromIDListW);
REGISTER_SHIM("shell32", "ILCreateFromPathW", (FARPROC)&xwr::Shim_ILCreateFromPathW);
REGISTER_SHIM("shell32", "ILFree", (FARPROC)&xwr::Shim_ILFree);
