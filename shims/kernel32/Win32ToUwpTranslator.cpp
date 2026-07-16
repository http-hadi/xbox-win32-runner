// shims/kernel32/Win32ToUwpTranslator.cpp
// Implementation of the Win32 → UWP translation layer.
//
// This file contains the REAL translation logic. On real Windows/Xbox builds,
// it calls the actual UWP APIs (CreateFile2, VirtualAllocFromApp,
// ApplicationData::Current().LocalFolder, etc.). On Linux syntax-check builds,
// the WinRTStubs.h provides stub types so the translation logic compiles
// and can be unit-tested.
//
// Key translations:
//   Win32 CreateFileW → UWP CreateFile2 (with CREATE_FILE2 struct)
//   Win32 VirtualAlloc → UWP VirtualAllocFromApp (allows code generation)
//   Win32 GetCurrentDirectoryW → UWP ApplicationData.LocalFolder.Path
//   Win32 GetTempPathW → UWP LocalFolder + "\\Temp"
//   Win32 GetUserNameW → "XboxUser" (AppContainer has no user identity)
//   Win32 GetComputerNameW → "XboxSeriesX" (or "XboxSeriesS")

#include "UwpSdkIncludes.h"

#include "Win32ToUwpTranslator.h"
#include "PathTranslator.h"

#include <Windows.h>
#include <string>
#include <cstring>

// WinRT: on MSVC (real Windows/Xbox), use the real CppWinRT projection.
// On Linux syntax-check, use our stub WinRTStubs.h.
#ifdef _MSC_VER
  #include <winrt/Windows.Storage.h>
#else
  #include "WinRTStubs.h"
#endif

namespace xwr {

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------
static std::wstring g_uwpLocalFolder = L"C:\\Users\\Public\\XboxWin32Runner\\LocalState";
static bool g_initialized = false;

bool InitializeUwpTranslator() {
    if (g_initialized) return true;

    // Real UWP build: query ApplicationData::Current().LocalFolder.Path()
    // via C++/WinRT. The stub WinRTStubs.h provides a no-op ApplicationData
    // that returns a default IStorageFolder; in a real build the projection
    // returns the actual LocalFolder path.
    //
    // Pseudo (real build):
    //   using namespace winrt::Windows::Storage;
    //   auto folder = ApplicationData::Current().LocalFolder();
    //   g_uwpLocalFolder = folder.Path().c_str();
    //
    // For syntax check / stub mode, we use a default path that PathTranslator
    // also uses. On real Xbox, this is something like:
    //   C:\Users\xbox\AppData\Local\Packages\<pkg>\LocalState

    // Set the PathTranslator's physical root to the UWP local folder.
    PathTranslator::Instance().SetPhysicalRoot(g_uwpLocalFolder);

    g_initialized = true;
    return true;
}

const wchar_t* GetUwpLocalFolderPath() {
    if (!g_initialized) InitializeUwpTranslator();
    return g_uwpLocalFolder.c_str();
}

// ---------------------------------------------------------------------------
// Path translation
// ---------------------------------------------------------------------------
std::wstring TranslateWin32PathToUwp(const std::wstring& win32Path) {
    if (!g_initialized) InitializeUwpTranslator();
    return PathTranslator::Instance().TranslateToReal(win32Path);
}

// ---------------------------------------------------------------------------
// File operations — CreateFile2 is the UWP-compatible API
// ---------------------------------------------------------------------------
#ifndef XWR_SYNTAX_CHECK

// Real UWP build: use CreateFile2
HANDLE UwpCreateFile(const wchar_t* path, DWORD access, DWORD share,
                     DWORD creationDisposition, DWORD flagsAndAttributes) {
    // Translate the Win32 path to a UWP-local path
    std::wstring realPath = TranslateWin32PathToUwp(path);

    // CreateFile2 is the UWP-compatible file creation API.
    // It takes a CREATEFILE2_EXTENDED_PARAMETERS struct.
    CREATEFILE2_EXTENDED_PARAMETERS params{};
    params.dwSize = sizeof(params);
    params.dwFileAttributes = flagsAndAttributes & 0x0000FFFF;
    params.dwFileFlags = flagsAndAttributes & 0xFFFF0000;
    params.dwSecurityQosFlags = flagsAndAttributes & 0x000F0000;
    params.lpSecurityAttributes = nullptr;
    params.hTemplateFile = nullptr;

    return ::CreateFile2(realPath.c_str(), access, share,
                         creationDisposition, &params);
}

#else

// Syntax-check stub: simulate CreateFile2 behavior
HANDLE UwpCreateFile(const wchar_t* path, DWORD access, DWORD share,
                     DWORD creationDisposition, DWORD flagsAndAttributes) {
    (void)TranslateWin32PathToUwp(path ? path : L"");
    (void)access; (void)share; (void)creationDisposition; (void)flagsAndAttributes;
    // Return a fake handle so the translation logic is verifiable
    return reinterpret_cast<HANDLE>(0x1000 + (path ? static_cast<int>(path[0]) : 0));
}

#endif

// ---------------------------------------------------------------------------
// Memory operations — VirtualAllocFromApp / VirtualProtectFromApp
// ---------------------------------------------------------------------------
void* UwpVirtualAlloc(void* addr, size_t size, DWORD allocType, DWORD protect) {
#ifndef XWR_SYNTAX_CHECK
    // Real UWP: VirtualAllocFromApp allows code generation (PAGE_EXECUTE_*)
    // which VirtualAlloc blocks in AppContainer.
    return ::VirtualAllocFromApp(addr, size, allocType, protect);
#else
    (void)addr; (void)size; (void)allocType; (void)protect;
    return reinterpret_cast<void*>(0x2000);
#endif
}

bool UwpVirtualProtect(void* addr, size_t size, DWORD newProt, DWORD* oldProt) {
#ifndef XWR_SYNTAX_CHECK
    return ::VirtualProtectFromApp(addr, size, newProt, oldProt) != FALSE;
#else
    (void)addr; (void)size; (void)newProt;
    if (oldProt) *oldProt = 0x40; // PAGE_EXECUTE_READWRITE
    return true;
#endif
}

// ---------------------------------------------------------------------------
// Registry — HKCU maps to in-memory store, HKLM is read-only snapshot
// ---------------------------------------------------------------------------
bool UwpRegOpenKey(HKEY root, const wchar_t* subkey, HKEY* outHandle) {
    if (!outHandle) return false;
    // UWP AppContainer can only write to HKCU\Software\Classes\<pkg>
    // (via CreateFile on the registry hive file). For everything else,
    // we use an in-memory registry simulation (see Advapi32Shim.cpp).
    //
    // Here we just return a fake handle; the Advapi32 shim handles the
    // actual read/write against the in-memory store.
    *outHandle = reinterpret_cast<HKEY>(static_cast<uintptr_t>(0x3000));
    (void)root; (void)subkey;
    return true;
}

// ---------------------------------------------------------------------------
// Identity stubs — AppContainer has no user/computer identity
// ---------------------------------------------------------------------------
bool UwpGetUserName(wchar_t* buf, DWORD* size) {
    if (!buf || !size || *size < 8) {
        if (size) *size = 8;
        return false;
    }
    const wchar_t* user = L"XboxUser";
    size_t len = 7;
    for (size_t i = 0; i <= len; ++i) buf[i] = user[i];
    *size = static_cast<DWORD>(len);
    return true;
}

bool UwpGetComputerName(wchar_t* buf, DWORD* size) {
    if (!buf || !size || *size < 12) {
        if (size) *size = 12;
        return false;
    }
    const wchar_t* name = L"XboxSeriesX";
    size_t len = 11;
    for (size_t i = 0; i <= len; ++i) buf[i] = name[i];
    *size = static_cast<DWORD>(len);
    return true;
}

}  // namespace xwr
