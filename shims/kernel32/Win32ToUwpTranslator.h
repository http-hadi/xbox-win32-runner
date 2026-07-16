// shims/kernel32/Win32ToUwpTranslator.h
// Win32 → UWP API translation layer.
//
// Every Win32 API that has a UWP-specific variant is translated here:
//   CreateFileW       → CreateFile2          (UWP-compatible file open)
//   CreateFileA       → CreateFile2 + A→W conversion
//   VirtualAlloc      → VirtualAllocFromApp  (UWP allows code generation)
//   VirtualProtect    → VirtualProtectFromApp
//   GetCurrentDirectoryW → ApplicationData::Current().LocalFolder.Path
//   GetTempPathW      → ApplicationData::Current().LocalFolder.Path + "\\Temp"
//   GetSystemInfo     → GetNativeSystemInfo (same, but explicit)
//
// On real Windows/Xbox builds, these call the real UWP APIs.
// On Linux syntax-check builds, the WinRTStubs.h provides stub types
// so the code compiles and the translation logic is verifiable.

#pragma once
#include <Windows.h>
#include <string>

namespace xwr {

// Initialize the UWP translation layer. Must be called once at startup
// before any shim function is invoked. Queries ApplicationData::Current()
// .LocalFolder to establish the physical root for path translation.
bool InitializeUwpTranslator();

// Get the UWP local folder path (the physical root for C:\ translation).
const wchar_t* GetUwpLocalFolderPath();

// Translate a Win32 path to a UWP-local path.
//   C:\Game\save.dat  →  C:\Users\...\LocalState\Game\save.dat
//   \Game\save.dat    →  ...same...
//   save.dat          →  ...same + virtual cwd
std::wstring TranslateWin32PathToUwp(const std::wstring& win32Path);

// UWP-compatible file open. Wraps CreateFile2 with path translation.
HANDLE UwpCreateFile(const wchar_t* path, DWORD access, DWORD share,
                     DWORD creationDisposition, DWORD flagsAndAttributes);

// UWP-compatible memory allocation. Uses VirtualAllocFromApp.
void* UwpVirtualAlloc(void* addr, size_t size, DWORD allocType, DWORD protect);

// UWP-compatible memory protection. Uses VirtualProtectFromApp.
bool UwpVirtualProtect(void* addr, size_t size, DWORD newProt, DWORD* oldProt);

// UWP-compatible registry: HKCU writes go to LocalState\registry.dat,
// HKLM reads return from an in-memory snapshot.
bool UwpRegOpenKey(HKEY root, const wchar_t* subkey, HKEY* outHandle);

// UWP-compatible GetUserName: returns "XboxUser" (AppContainer has no user name).
bool UwpGetUserName(wchar_t* buf, DWORD* size);

// UWP-compatible GetComputerName: returns "XboxSeriesX" or "XboxSeriesS".
bool UwpGetComputerName(wchar_t* buf, DWORD* size);

}  // namespace xwr
