// shims/kernel32/Kernel32Shim.cpp
// Win32 kernel32 shim layer. Maps game-side kernel32 calls to UWP-equivalent
// APIs (VirtualAllocFromApp, CreateFile2, etc.) and provides in-process
// reimplementations for things UWP can't do directly.
//
// Registered via REGISTER_SHIM() macros at namespace scope. Each shim function
// has the same signature as the real Win32 function so the PE loader can drop
// its address directly into a game's IAT.

#include "UwpSdkIncludes.h"

#include <Windows.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>

#include "../ShimRegistry.h"
#include "PathTranslator.h"
#include "Win32ToUwpTranslator.h"

namespace xwr {

// ---------------------------------------------------------------------------
// File I/O — translate paths and call CreateFile2 (UWP-compatible)
// Win32 CreateFileW is BLOCKED in AppContainer; CreateFile2 is the UWP variant.
// ---------------------------------------------------------------------------

extern "C" HANDLE __stdcall Shim_CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess,
                                               DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                               DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                               HANDLE hTemplateFile) {
    if (!lpFileName) { ::SetLastError(ERROR_INVALID_PARAMETER); return INVALID_HANDLE_VALUE; }
    // Use the UWP translator (CreateFile2 + path translation)
    (void)lpSecurityAttributes; (void)hTemplateFile;
    return UwpCreateFile(lpFileName, dwDesiredAccess, dwShareMode,
                         dwCreationDisposition, dwFlagsAndAttributes);
}

extern "C" HANDLE __stdcall Shim_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess,
                                               DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                               DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                               HANDLE hTemplateFile) {
    if (!lpFileName) return INVALID_HANDLE_VALUE;
    std::wstring w;
    for (const char* p = lpFileName; *p; ++p) w.push_back(static_cast<wchar_t>(*p));
    return Shim_CreateFileW(w.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes,
                             dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

extern "C" BOOL __stdcall Shim_CloseHandle(HANDLE h) {
    return ::CloseHandle(h);
}
extern "C" BOOL __stdcall Shim_ReadFile(HANDLE h, LPVOID buf, DWORD n, LPDWORD got, LPOVERLAPPED ov) {
    return ::ReadFile(h, buf, n, got, ov);
}
extern "C" BOOL __stdcall Shim_WriteFile(HANDLE h, LPCVOID buf, DWORD n, LPDWORD got, LPOVERLAPPED ov) {
    return ::WriteFile(h, buf, n, got, ov);
}
extern "C" DWORD __stdcall Shim_SetFilePointer(HANDLE h, LONG dist, PLONG high, DWORD method) {
    return ::SetFilePointer(h, dist, high, method);
}
extern "C" BOOL __stdcall Shim_SetFilePointerEx(HANDLE h, LARGE_INTEGER dist, PLARGE_INTEGER newPos, DWORD method) {
    return ::SetFilePointerEx(h, dist, newPos, method);
}
extern "C" BOOL __stdcall Shim_FlushFileBuffers(HANDLE h) { return ::FlushFileBuffers(h); }
extern "C" BOOL __stdcall Shim_GetFileSizeEx(HANDLE h, PLARGE_INTEGER sz) { return ::GetFileSizeEx(h, sz); }
extern "C" DWORD __stdcall Shim_GetFileSize(HANDLE h, LPDWORD high) { return ::GetFileSize(h, high); }
extern "C" DWORD __stdcall Shim_GetFileAttributesW(LPCWSTR p) {
    if (!p) return INVALID_FILE_ATTRIBUTES;
    std::wstring real = PathTranslator::Instance().TranslateToReal(p);
    return ::GetFileAttributesW(real.c_str());
}
extern "C" BOOL __stdcall Shim_GetFileAttributesExW(LPCWSTR p, int infoLevel, LPVOID info) {
    if (!p) return FALSE;
    std::wstring real = PathTranslator::Instance().TranslateToReal(p);
#ifdef _MSC_VER
    // MSVC: ::GetFileAttributesExW requires GET_FILEEX_INFO_LEVELS, not int.
    return ::GetFileAttributesExW(real.c_str(),
                                   static_cast<GET_FILEEX_INFO_LEVELS>(infoLevel),
                                   info);
#else
    // Linux stub signature uses int directly.
    return ::GetFileAttributesExW(real.c_str(), infoLevel, info);
#endif
}
extern "C" BOOL __stdcall Shim_SetFileAttributesW(LPCWSTR p, DWORD a) {
    if (!p) return FALSE;
    std::wstring real = PathTranslator::Instance().TranslateToReal(p);
    return ::SetFileAttributesW(real.c_str(), a);
}
extern "C" BOOL __stdcall Shim_DeleteFileW(LPCWSTR p) {
    if (!p) return FALSE;
    std::wstring real = PathTranslator::Instance().TranslateToReal(p);
    return ::DeleteFileW(real.c_str());
}
extern "C" BOOL __stdcall Shim_MoveFileW(LPCWSTR a, LPCWSTR b) {
    if (!a || !b) return FALSE;
    std::wstring ra = PathTranslator::Instance().TranslateToReal(a);
    std::wstring rb = PathTranslator::Instance().TranslateToReal(b);
    return ::MoveFileW(ra.c_str(), rb.c_str());
}
extern "C" BOOL __stdcall Shim_CopyFileW(LPCWSTR a, LPCWSTR b, BOOL failIfExists) {
    if (!a || !b) return FALSE;
    std::wstring ra = PathTranslator::Instance().TranslateToReal(a);
    std::wstring rb = PathTranslator::Instance().TranslateToReal(b);
    return ::CopyFileW(ra.c_str(), rb.c_str(), failIfExists);
}
extern "C" BOOL __stdcall Shim_CreateDirectoryW(LPCWSTR p, LPSECURITY_ATTRIBUTES sa) {
    if (!p) return FALSE;
    std::wstring real = PathTranslator::Instance().TranslateToReal(p);
    return ::CreateDirectoryW(real.c_str(), sa);
}
extern "C" BOOL __stdcall Shim_RemoveDirectoryW(LPCWSTR p) {
    if (!p) return FALSE;
    std::wstring real = PathTranslator::Instance().TranslateToReal(p);
    return ::RemoveDirectoryW(real.c_str());
}
extern "C" HANDLE __stdcall Shim_FindFirstFileW(LPCWSTR p, LPWIN32_FIND_DATAW fd) {
    if (!p) return INVALID_HANDLE_VALUE;
    std::wstring real = PathTranslator::Instance().TranslateToReal(p);
    return ::FindFirstFileW(real.c_str(), fd);
}
extern "C" BOOL __stdcall Shim_FindNextFileW(HANDLE h, LPWIN32_FIND_DATAW fd) {
    return ::FindNextFileW(h, fd);
}
extern "C" BOOL __stdcall Shim_FindClose(HANDLE h) { return ::FindClose(h); }

// ---------------------------------------------------------------------------
// Memory - use VirtualAllocFromApp / VirtualProtectFromApp (UWP variants)
// Win32 VirtualAlloc/VirtualProtect are BLOCKED in AppContainer when the
// protection includes PAGE_EXECUTE_*; the FromApp variants allow it.
// ---------------------------------------------------------------------------

extern "C" LPVOID __stdcall Shim_VirtualAlloc(LPVOID addr, SIZE_T size, DWORD allocType, DWORD protect) {
    return static_cast<LPVOID>(UwpVirtualAlloc(addr, size, allocType, protect));
}
extern "C" LPVOID __stdcall Shim_VirtualAllocEx(HANDLE hProc, LPVOID addr, SIZE_T size, DWORD allocType, DWORD protect) {
    // Only allow in-process allocation.
    if (hProc != ::GetCurrentProcess()) { ::SetLastError(ERROR_ACCESS_DENIED); return nullptr; }
    return static_cast<LPVOID>(UwpVirtualAlloc(addr, size, allocType, protect));
}
extern "C" BOOL __stdcall Shim_VirtualProtect(LPVOID addr, SIZE_T size, DWORD newProt, LPDWORD oldProt) {
    return UwpVirtualProtect(addr, size, newProt, oldProt) ? TRUE : FALSE;
}
extern "C" BOOL __stdcall Shim_VirtualFree(LPVOID addr, SIZE_T size, DWORD freeType) {
    return ::VirtualFree(addr, size, freeType);
}
extern "C" BOOL __stdcall Shim_VirtualQuery(LPCVOID addr, PMEMORY_BASIC_INFORMATION buf, SIZE_T len) {
    return ::VirtualQuery(addr, buf, len);
}

// Heap helpers (use process heap)
extern "C" LPVOID __stdcall Shim_HeapAlloc(HANDLE heap, DWORD flags, SIZE_T size) {
    return ::HeapAlloc(heap, flags, size);
}
extern "C" LPVOID __stdcall Shim_HeapReAlloc(HANDLE heap, DWORD flags, LPVOID p, SIZE_T size) {
    return ::HeapReAlloc(heap, flags, p, size);
}
extern "C" BOOL __stdcall Shim_HeapFree(HANDLE heap, DWORD flags, LPVOID p) {
    return ::HeapFree(heap, flags, p);
}
extern "C" HANDLE __stdcall Shim_GetProcessHeap() { return ::GetProcessHeap(); }

// ---------------------------------------------------------------------------
// Threading / sync - mostly passthrough (UWP allows in-process)
// ---------------------------------------------------------------------------

extern "C" HANDLE __stdcall Shim_CreateThread(LPSECURITY_ATTRIBUTES sa, SIZE_T stack,
                                                LPTHREAD_START_ROUTINE start, LPVOID param,
                                                DWORD flags, LPDWORD tid) {
    return ::CreateThread(sa, stack, start, param, flags, tid);
}
extern "C" HANDLE __stdcall Shim_CreateEventW(LPSECURITY_ATTRIBUTES sa, BOOL manual, BOOL init, LPCWSTR name) {
    return ::CreateEventW(sa, manual, init, name);
}
extern "C" HANDLE __stdcall Shim_CreateMutexW(LPSECURITY_ATTRIBUTES sa, BOOL init, LPCWSTR name) {
    return ::CreateMutexW(sa, init, name);
}
extern "C" HANDLE __stdcall Shim_CreateSemaphoreW(LPSECURITY_ATTRIBUTES sa, LONG init, LONG max, LPCWSTR name) {
    return ::CreateSemaphoreW(sa, init, max, name);
}
extern "C" BOOL __stdcall Shim_SetEvent(HANDLE h) { return ::SetEvent(h); }
extern "C" BOOL __stdcall Shim_ResetEvent(HANDLE h) { return ::ResetEvent(h); }
extern "C" BOOL __stdcall Shim_ReleaseMutex(HANDLE h) { return ::ReleaseMutex(h); }
extern "C" BOOL __stdcall Shim_ReleaseSemaphore(HANDLE h, LONG n, LPLONG prev) { return ::ReleaseSemaphore(h, n, prev); }
extern "C" DWORD __stdcall Shim_WaitForSingleObject(HANDLE h, DWORD ms) { return ::WaitForSingleObject(h, ms); }
extern "C" DWORD __stdcall Shim_WaitForMultipleObjects(DWORD n, const HANDLE* h, BOOL all, DWORD ms) {
    return ::WaitForMultipleObjects(n, h, all, ms);
}
extern "C" void __stdcall Shim_Sleep(DWORD ms) { ::Sleep(ms); }
extern "C" DWORD __stdcall Shim_SleepEx(DWORD ms, BOOL alertable) { return ::SleepEx(ms, alertable); }
extern "C" void __stdcall Shim_InitializeCriticalSection(LPCRITICAL_SECTION cs) { ::InitializeCriticalSection(cs); }
extern "C" void __stdcall Shim_DeleteCriticalSection(LPCRITICAL_SECTION cs) { ::DeleteCriticalSection(cs); }
extern "C" void __stdcall Shim_EnterCriticalSection(LPCRITICAL_SECTION cs) { ::EnterCriticalSection(cs); }
extern "C" void __stdcall Shim_LeaveCriticalSection(LPCRITICAL_SECTION cs) { ::LeaveCriticalSection(cs); }
extern "C" BOOL __stdcall Shim_InitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION cs, DWORD spin) {
    return ::InitializeCriticalSectionAndSpinCount(cs, spin);
}
extern "C" DWORD __stdcall Shim_GetCurrentThreadId() { return ::GetCurrentThreadId(); }
extern "C" DWORD __stdcall Shim_GetCurrentProcessId() { return ::GetCurrentProcessId(); }
extern "C" HANDLE __stdcall Shim_GetCurrentProcess() { return ::GetCurrentProcess(); }
extern "C" HANDLE __stdcall Shim_GetCurrentThread() { return ::GetCurrentThread(); }
extern "C" DWORD __stdcall Shim_GetLastError() { return ::GetLastError(); }
extern "C" void __stdcall Shim_SetLastError(DWORD e) { ::SetLastError(e); }

// ---------------------------------------------------------------------------
// Module loading - intercept LoadLibrary to feed game DLLs back through
// our PE loader.
// ---------------------------------------------------------------------------

// Forward decl from PeLoader.h (we don't include it here to keep this TU
// decoupled; the linker resolves it).
namespace xwr { class PeLoader; extern PeLoader* g_peLoader; }
extern "C" uint64_t PeLoader_LoadModuleByName(const wchar_t* name) { (void)name; return 0; }
extern "C" uint64_t PeLoader_GetExportByName(const wchar_t* dll, const char* func);

extern "C" HMODULE __stdcall Shim_LoadLibraryW(LPCWSTR name) {
    if (!name) return static_cast<HMODULE>(0);
    // Try real LoadLibraryW first (for system DLLs UWP supports).
    HMODULE h = ::LoadLibraryW(name);
    if (h) return h;
    // Fall back to our PE loader for game-shipped DLLs.
    uint64_t base = PeLoader_LoadModuleByName(name);
    return reinterpret_cast<HMODULE>(base);
}
extern "C" HMODULE __stdcall Shim_LoadLibraryExW(LPCWSTR name, HANDLE hFile, DWORD flags) {
    if (!name) return static_cast<HMODULE>(0);
    HMODULE h = ::LoadLibraryExW(name, hFile, flags);
    if (h) return h;
    uint64_t base = PeLoader_LoadModuleByName(name);
    return reinterpret_cast<HMODULE>(base);
}
extern "C" FARPROC __stdcall Shim_GetProcAddress(HMODULE hMod, LPCSTR name) {
    FARPROC p = ::GetProcAddress(hMod, name);
    if (p) return p;
    // Fall back: ask PE loader for the export. We don't know the DLL name from
    // hMod alone in the simple case; the PE loader registers module handles
    // so we'd need a reverse lookup. For now, just return nullptr.
    return nullptr;
}
extern "C" HMODULE __stdcall Shim_GetModuleHandleW(LPCWSTR name) {
    return ::GetModuleHandleW(name);
}
extern "C" HMODULE __stdcall Shim_GetModuleHandleA(LPCSTR name) {
    return ::GetModuleHandleA(name);
}
extern "C" BOOL __stdcall Shim_FreeLibrary(HMODULE h) {
    return ::FreeLibrary(h);
}
extern "C" DWORD __stdcall Shim_GetModuleFileNameW(HMODULE h, LPWSTR buf, DWORD size) {
    return ::GetModuleFileNameW(h, buf, size);
}

// ---------------------------------------------------------------------------
// Time / system info
// ---------------------------------------------------------------------------

extern "C" void __stdcall Shim_GetSystemInfo(LPSYSTEM_INFO si) { ::GetSystemInfo(si); }
extern "C" BOOL __stdcall Shim_GlobalMemoryStatusEx(LPMEMORYSTATUSEX ms) { return ::GlobalMemoryStatusEx(ms); }
extern "C" DWORD __stdcall Shim_GetTickCount() { return ::GetTickCount(); }
extern "C" ULONGLONG __stdcall Shim_GetTickCount64() { return ::GetTickCount64(); }
extern "C" BOOL __stdcall Shim_QueryPerformanceCounter(LARGE_INTEGER* c) { return ::QueryPerformanceCounter(c); }
extern "C" BOOL __stdcall Shim_QueryPerformanceFrequency(LARGE_INTEGER* f) { return ::QueryPerformanceFrequency(f); }

// ---------------------------------------------------------------------------
// Environment / console / strings
// ---------------------------------------------------------------------------

extern "C" DWORD __stdcall Shim_GetEnvironmentVariableW(LPCWSTR name, LPWSTR buf, DWORD size) {
    return ::GetEnvironmentVariableW(name, buf, size);
}
extern "C" BOOL __stdcall Shim_SetEnvironmentVariableW(LPCWSTR name, LPCWSTR val) {
    return ::SetEnvironmentVariableW(name, val);
}
extern "C" LPWSTR __stdcall Shim_GetCommandLineW() { return ::GetCommandLineW(); }
extern "C" LPWCH __stdcall Shim_GetEnvironmentStringsW() { return ::GetEnvironmentStringsW(); }
extern "C" BOOL __stdcall Shim_FreeEnvironmentStringsW(LPWCH p) { return ::FreeEnvironmentStringsW(p); }
extern "C" DWORD __stdcall Shim_GetTempPathW(DWORD n, LPWSTR buf) {
    return ::GetTempPathW(n, buf);
}
extern "C" DWORD __stdcall Shim_GetTempFileNameW(LPCWSTR path, LPCWSTR prefix, UINT unique, LPWSTR buf) {
    return ::GetTempFileNameW(path, prefix, unique, buf);
}
extern "C" DWORD __stdcall Shim_FormatMessageW(DWORD flags, LPCVOID src, DWORD msgId, DWORD langId,
                                                 LPWSTR buf, DWORD size, va_list* args) {
    return ::FormatMessageW(flags, src, msgId, langId, buf, size, args);
}
extern "C" DWORD __stdcall Shim_GetCurrentDirectoryW(DWORD n, LPWSTR buf) {
    // UWP: return the LocalFolder path as the "current directory"
    const wchar_t* path = GetUwpLocalFolderPath();
    size_t len = wcslen(path);
    if (buf && n > len) {
        wcscpy(buf, path);
        return static_cast<DWORD>(len);
    }
    return static_cast<DWORD>(len + 1);
}
extern "C" BOOL __stdcall Shim_SetCurrentDirectoryW(LPCWSTR p) {
    if (!p) return FALSE;
    PathTranslator::Instance().SetVirtualCwd(p);
    return TRUE;
}
extern "C" DWORD __stdcall Shim_GetFullPathNameW(LPCWSTR p, DWORD n, LPWSTR buf, LPWSTR* filePart) {
    if (!p) return 0;
    std::wstring real = PathTranslator::Instance().TranslateToReal(p);
    return ::GetFullPathNameW(real.c_str(), n, buf, filePart);
}
extern "C" BOOL __stdcall Shim_GetVolumeInformationW(LPCWSTR root, LPWSTR nameBuf, DWORD nameSize,
                                                       LPDWORD serial, LPDWORD maxLen, LPDWORD flags,
                                                       LPWSTR fsBuf, DWORD fsSize) {
    // Stub - pretend we're NTFS with serial 0x12345678.
    if (serial) *serial = 0x12345678;
    if (maxLen) *maxLen = 255;
    if (flags) *flags = 0x000700FF;  // FILE_SUPPORTS various
    if (nameBuf && nameSize > 0) nameBuf[0] = 0;
    if (fsBuf && fsSize > 0) { LPCWSTR fs = L"NTFS"; size_t i = 0; while (i < fsSize - 1 && fs[i]) fsBuf[i++] = fs[i]; fsBuf[i] = 0; }
    return TRUE;
}
extern "C" void __stdcall Shim_OutputDebugStringW(LPCWSTR s) { ::OutputDebugStringW(s); }
extern "C" BOOL __stdcall Shim_IsDebuggerPresent() { return ::IsDebuggerPresent(); }
extern "C" void __stdcall Shim_DebugBreak() { ::DebugBreak(); }

extern "C" UINT __stdcall Shim_GetConsoleCP() { return ::GetConsoleCP(); }
extern "C" UINT __stdcall Shim_GetConsoleOutputCP() { return ::GetConsoleOutputCP(); }
extern "C" BOOL __stdcall Shim_WriteConsoleW(HANDLE h, const void* buf, DWORD n, LPDWORD got, LPVOID p) {
    return ::WriteConsoleW(h, buf, n, got, p);
}
extern "C" BOOL __stdcall Shim_AllocConsole() { return ::AllocConsole(); }
extern "C" BOOL __stdcall Shim_GetConsoleMode(HANDLE h, LPDWORD mode) { return ::GetConsoleMode(h, mode); }
extern "C" BOOL __stdcall Shim_SetConsoleMode(HANDLE h, DWORD mode) { return ::SetConsoleMode(h, mode); }
extern "C" BOOL __stdcall Shim_FlushConsoleInputBuffer(HANDLE h) { return ::FlushConsoleInputBuffer(h); }
extern "C" HANDLE __stdcall Shim_GetStdHandle(DWORD which) { return ::GetStdHandle(which); }

// ---------------------------------------------------------------------------
// Multi-byte / wide-char string helpers - pass through to OS
// ---------------------------------------------------------------------------
extern "C" int __stdcall Shim_MultiByteToWideChar(UINT cp, DWORD flags, LPCSTR src, int srcSize,
                                                    LPWSTR dst, int dstSize) {
    return ::MultiByteToWideChar(cp, flags, src, srcSize, dst, dstSize);
}
extern "C" int __stdcall Shim_WideCharToMultiByte(UINT cp, DWORD flags, LPCWSTR src, int srcSize,
                                                    LPSTR dst, int dstSize, LPCSTR def, LPBOOL usedDef) {
    return ::WideCharToMultiByte(cp, flags, src, srcSize, dst, dstSize, def, usedDef);
}
extern "C" int __stdcall Shim_lstrlenW(LPCWSTR s) { return s ? static_cast<int>(wcslen(s)) : 0; }
extern "C" int __stdcall Shim_lstrlenA(LPCSTR s) { return s ? static_cast<int>(strlen(s)) : 0; }
extern "C" LPWSTR __stdcall Shim_lstrcpyW(LPWSTR dst, LPCWSTR src) {
    if (!dst || !src) return dst;
    return wcscpy(dst, src);
}
extern "C" LPWSTR __stdcall Shim_lstrcpynW(LPWSTR dst, LPCWSTR src, int n) {
    if (!dst || !src || n <= 0) return dst;
    wcsncpy(dst, src, static_cast<size_t>(n));
    if (n > 0) dst[n - 1] = 0;
    return dst;
}
extern "C" LPWSTR __stdcall Shim_lstrcatW(LPWSTR dst, LPCWSTR src) {
    if (!dst || !src) return dst;
    return wcscat(dst, src);
}

// ---------------------------------------------------------------------------
// Registry - shim the registry into in-memory storage (see Advapi32Shim.cpp
// for the actual storage). Forward there.
// ---------------------------------------------------------------------------
extern "C" LONG __stdcall Shim_RegOpenKeyExW(HKEY, LPCWSTR, DWORD, DWORD, PHKEY);
extern "C" LONG __stdcall Shim_RegQueryValueExW(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
// (defined in Advapi32Shim.cpp; declared here only if needed.)

// ---------------------------------------------------------------------------
// Register everything
// ---------------------------------------------------------------------------
}  // namespace xwr

// REGISTER_SHIM expands to a static int with anonymous name; must live at
// namespace scope (not inside xwr namespace because of how the macro uses
// token pasting).
REGISTER_SHIM("kernel32", "CreateFileW", (FARPROC)&xwr::Shim_CreateFileW);
REGISTER_SHIM("kernel32", "CreateFileA", (FARPROC)&xwr::Shim_CreateFileA);
REGISTER_SHIM("kernel32", "CloseHandle", (FARPROC)&xwr::Shim_CloseHandle);
REGISTER_SHIM("kernel32", "ReadFile", (FARPROC)&xwr::Shim_ReadFile);
REGISTER_SHIM("kernel32", "WriteFile", (FARPROC)&xwr::Shim_WriteFile);
REGISTER_SHIM("kernel32", "SetFilePointer", (FARPROC)&xwr::Shim_SetFilePointer);
REGISTER_SHIM("kernel32", "SetFilePointerEx", (FARPROC)&xwr::Shim_SetFilePointerEx);
REGISTER_SHIM("kernel32", "FlushFileBuffers", (FARPROC)&xwr::Shim_FlushFileBuffers);
REGISTER_SHIM("kernel32", "GetFileSizeEx", (FARPROC)&xwr::Shim_GetFileSizeEx);
REGISTER_SHIM("kernel32", "GetFileSize", (FARPROC)&xwr::Shim_GetFileSize);
REGISTER_SHIM("kernel32", "GetFileAttributesW", (FARPROC)&xwr::Shim_GetFileAttributesW);
REGISTER_SHIM("kernel32", "GetFileAttributesExW", (FARPROC)&xwr::Shim_GetFileAttributesExW);
REGISTER_SHIM("kernel32", "SetFileAttributesW", (FARPROC)&xwr::Shim_SetFileAttributesW);
REGISTER_SHIM("kernel32", "DeleteFileW", (FARPROC)&xwr::Shim_DeleteFileW);
REGISTER_SHIM("kernel32", "MoveFileW", (FARPROC)&xwr::Shim_MoveFileW);
REGISTER_SHIM("kernel32", "MoveFileExW", (FARPROC)&xwr::Shim_MoveFileW);
REGISTER_SHIM("kernel32", "CopyFileW", (FARPROC)&xwr::Shim_CopyFileW);
REGISTER_SHIM("kernel32", "CopyFileExW", (FARPROC)&xwr::Shim_CopyFileW);
REGISTER_SHIM("kernel32", "CreateDirectoryW", (FARPROC)&xwr::Shim_CreateDirectoryW);
REGISTER_SHIM("kernel32", "CreateDirectoryExW", (FARPROC)&xwr::Shim_CreateDirectoryW);
REGISTER_SHIM("kernel32", "RemoveDirectoryW", (FARPROC)&xwr::Shim_RemoveDirectoryW);
REGISTER_SHIM("kernel32", "FindFirstFileW", (FARPROC)&xwr::Shim_FindFirstFileW);
REGISTER_SHIM("kernel32", "FindFirstFileExW", (FARPROC)&xwr::Shim_FindFirstFileW);
REGISTER_SHIM("kernel32", "FindNextFileW", (FARPROC)&xwr::Shim_FindNextFileW);
REGISTER_SHIM("kernel32", "FindClose", (FARPROC)&xwr::Shim_FindClose);
REGISTER_SHIM("kernel32", "VirtualAlloc", (FARPROC)&xwr::Shim_VirtualAlloc);
REGISTER_SHIM("kernel32", "VirtualAllocEx", (FARPROC)&xwr::Shim_VirtualAllocEx);
REGISTER_SHIM("kernel32", "VirtualProtect", (FARPROC)&xwr::Shim_VirtualProtect);
REGISTER_SHIM("kernel32", "VirtualProtectEx", (FARPROC)&xwr::Shim_VirtualProtect);
REGISTER_SHIM("kernel32", "VirtualFree", (FARPROC)&xwr::Shim_VirtualFree);
REGISTER_SHIM("kernel32", "VirtualFreeEx", (FARPROC)&xwr::Shim_VirtualFree);
REGISTER_SHIM("kernel32", "VirtualQuery", (FARPROC)&xwr::Shim_VirtualQuery);
REGISTER_SHIM("kernel32", "VirtualQueryEx", (FARPROC)&xwr::Shim_VirtualQuery);
REGISTER_SHIM("kernel32", "HeapAlloc", (FARPROC)&xwr::Shim_HeapAlloc);
REGISTER_SHIM("kernel32", "HeapReAlloc", (FARPROC)&xwr::Shim_HeapReAlloc);
REGISTER_SHIM("kernel32", "HeapFree", (FARPROC)&xwr::Shim_HeapFree);
REGISTER_SHIM("kernel32", "GetProcessHeap", (FARPROC)&xwr::Shim_GetProcessHeap);
REGISTER_SHIM("kernel32", "CreateThread", (FARPROC)&xwr::Shim_CreateThread);
REGISTER_SHIM("kernel32", "CreateEventW", (FARPROC)&xwr::Shim_CreateEventW);
REGISTER_SHIM("kernel32", "CreateEventExW", (FARPROC)&xwr::Shim_CreateEventW);
REGISTER_SHIM("kernel32", "CreateMutexW", (FARPROC)&xwr::Shim_CreateMutexW);
REGISTER_SHIM("kernel32", "CreateMutexExW", (FARPROC)&xwr::Shim_CreateMutexW);
REGISTER_SHIM("kernel32", "CreateSemaphoreW", (FARPROC)&xwr::Shim_CreateSemaphoreW);
REGISTER_SHIM("kernel32", "CreateSemaphoreExW", (FARPROC)&xwr::Shim_CreateSemaphoreW);
REGISTER_SHIM("kernel32", "SetEvent", (FARPROC)&xwr::Shim_SetEvent);
REGISTER_SHIM("kernel32", "ResetEvent", (FARPROC)&xwr::Shim_ResetEvent);
REGISTER_SHIM("kernel32", "ReleaseMutex", (FARPROC)&xwr::Shim_ReleaseMutex);
REGISTER_SHIM("kernel32", "ReleaseSemaphore", (FARPROC)&xwr::Shim_ReleaseSemaphore);
REGISTER_SHIM("kernel32", "WaitForSingleObject", (FARPROC)&xwr::Shim_WaitForSingleObject);
REGISTER_SHIM("kernel32", "WaitForSingleObjectEx", (FARPROC)&xwr::Shim_WaitForSingleObject);
REGISTER_SHIM("kernel32", "WaitForMultipleObjects", (FARPROC)&xwr::Shim_WaitForMultipleObjects);
REGISTER_SHIM("kernel32", "WaitForMultipleObjectsEx", (FARPROC)&xwr::Shim_WaitForMultipleObjects);
REGISTER_SHIM("kernel32", "Sleep", (FARPROC)&xwr::Shim_Sleep);
REGISTER_SHIM("kernel32", "SleepEx", (FARPROC)&xwr::Shim_SleepEx);
REGISTER_SHIM("kernel32", "InitializeCriticalSection", (FARPROC)&xwr::Shim_InitializeCriticalSection);
REGISTER_SHIM("kernel32", "DeleteCriticalSection", (FARPROC)&xwr::Shim_DeleteCriticalSection);
REGISTER_SHIM("kernel32", "EnterCriticalSection", (FARPROC)&xwr::Shim_EnterCriticalSection);
REGISTER_SHIM("kernel32", "LeaveCriticalSection", (FARPROC)&xwr::Shim_LeaveCriticalSection);
REGISTER_SHIM("kernel32", "InitializeCriticalSectionAndSpinCount", (FARPROC)&xwr::Shim_InitializeCriticalSectionAndSpinCount);
REGISTER_SHIM("kernel32", "GetCurrentThreadId", (FARPROC)&xwr::Shim_GetCurrentThreadId);
REGISTER_SHIM("kernel32", "GetCurrentProcessId", (FARPROC)&xwr::Shim_GetCurrentProcessId);
REGISTER_SHIM("kernel32", "GetCurrentProcess", (FARPROC)&xwr::Shim_GetCurrentProcess);
REGISTER_SHIM("kernel32", "GetCurrentThread", (FARPROC)&xwr::Shim_GetCurrentThread);
REGISTER_SHIM("kernel32", "GetLastError", (FARPROC)&xwr::Shim_GetLastError);
REGISTER_SHIM("kernel32", "SetLastError", (FARPROC)&xwr::Shim_SetLastError);
REGISTER_SHIM("kernel32", "LoadLibraryW", (FARPROC)&xwr::Shim_LoadLibraryW);
REGISTER_SHIM("kernel32", "LoadLibraryExW", (FARPROC)&xwr::Shim_LoadLibraryExW);
REGISTER_SHIM("kernel32", "GetProcAddress", (FARPROC)&xwr::Shim_GetProcAddress);
REGISTER_SHIM("kernel32", "GetModuleHandleW", (FARPROC)&xwr::Shim_GetModuleHandleW);
REGISTER_SHIM("kernel32", "GetModuleHandleA", (FARPROC)&xwr::Shim_GetModuleHandleA);
REGISTER_SHIM("kernel32", "FreeLibrary", (FARPROC)&xwr::Shim_FreeLibrary);
REGISTER_SHIM("kernel32", "GetModuleFileNameW", (FARPROC)&xwr::Shim_GetModuleFileNameW);
REGISTER_SHIM("kernel32", "GetSystemInfo", (FARPROC)&xwr::Shim_GetSystemInfo);
REGISTER_SHIM("kernel32", "GlobalMemoryStatusEx", (FARPROC)&xwr::Shim_GlobalMemoryStatusEx);
REGISTER_SHIM("kernel32", "GetTickCount", (FARPROC)&xwr::Shim_GetTickCount);
REGISTER_SHIM("kernel32", "GetTickCount64", (FARPROC)&xwr::Shim_GetTickCount64);
REGISTER_SHIM("kernel32", "QueryPerformanceCounter", (FARPROC)&xwr::Shim_QueryPerformanceCounter);
REGISTER_SHIM("kernel32", "QueryPerformanceFrequency", (FARPROC)&xwr::Shim_QueryPerformanceFrequency);
REGISTER_SHIM("kernel32", "GetEnvironmentVariableW", (FARPROC)&xwr::Shim_GetEnvironmentVariableW);
REGISTER_SHIM("kernel32", "SetEnvironmentVariableW", (FARPROC)&xwr::Shim_SetEnvironmentVariableW);
REGISTER_SHIM("kernel32", "GetCommandLineW", (FARPROC)&xwr::Shim_GetCommandLineW);
REGISTER_SHIM("kernel32", "GetEnvironmentStringsW", (FARPROC)&xwr::Shim_GetEnvironmentStringsW);
REGISTER_SHIM("kernel32", "FreeEnvironmentStringsW", (FARPROC)&xwr::Shim_FreeEnvironmentStringsW);
REGISTER_SHIM("kernel32", "GetTempPathW", (FARPROC)&xwr::Shim_GetTempPathW);
REGISTER_SHIM("kernel32", "GetTempFileNameW", (FARPROC)&xwr::Shim_GetTempFileNameW);
REGISTER_SHIM("kernel32", "FormatMessageW", (FARPROC)&xwr::Shim_FormatMessageW);
REGISTER_SHIM("kernel32", "GetCurrentDirectoryW", (FARPROC)&xwr::Shim_GetCurrentDirectoryW);
REGISTER_SHIM("kernel32", "SetCurrentDirectoryW", (FARPROC)&xwr::Shim_SetCurrentDirectoryW);
REGISTER_SHIM("kernel32", "GetFullPathNameW", (FARPROC)&xwr::Shim_GetFullPathNameW);
REGISTER_SHIM("kernel32", "GetVolumeInformationW", (FARPROC)&xwr::Shim_GetVolumeInformationW);
REGISTER_SHIM("kernel32", "OutputDebugStringW", (FARPROC)&xwr::Shim_OutputDebugStringW);
REGISTER_SHIM("kernel32", "OutputDebugStringA", (FARPROC)&xwr::Shim_OutputDebugStringW);
REGISTER_SHIM("kernel32", "IsDebuggerPresent", (FARPROC)&xwr::Shim_IsDebuggerPresent);
REGISTER_SHIM("kernel32", "DebugBreak", (FARPROC)&xwr::Shim_DebugBreak);
REGISTER_SHIM("kernel32", "GetConsoleCP", (FARPROC)&xwr::Shim_GetConsoleCP);
REGISTER_SHIM("kernel32", "GetConsoleOutputCP", (FARPROC)&xwr::Shim_GetConsoleOutputCP);
REGISTER_SHIM("kernel32", "WriteConsoleW", (FARPROC)&xwr::Shim_WriteConsoleW);
REGISTER_SHIM("kernel32", "AllocConsole", (FARPROC)&xwr::Shim_AllocConsole);
REGISTER_SHIM("kernel32", "GetConsoleMode", (FARPROC)&xwr::Shim_GetConsoleMode);
REGISTER_SHIM("kernel32", "SetConsoleMode", (FARPROC)&xwr::Shim_SetConsoleMode);
REGISTER_SHIM("kernel32", "FlushConsoleInputBuffer", (FARPROC)&xwr::Shim_FlushConsoleInputBuffer);
REGISTER_SHIM("kernel32", "GetStdHandle", (FARPROC)&xwr::Shim_GetStdHandle);
REGISTER_SHIM("kernel32", "MultiByteToWideChar", (FARPROC)&xwr::Shim_MultiByteToWideChar);
REGISTER_SHIM("kernel32", "WideCharToMultiByte", (FARPROC)&xwr::Shim_WideCharToMultiByte);
REGISTER_SHIM("kernel32", "lstrlenW", (FARPROC)&xwr::Shim_lstrlenW);
REGISTER_SHIM("kernel32", "lstrlenA", (FARPROC)&xwr::Shim_lstrlenA);
REGISTER_SHIM("kernel32", "lstrcpyW", (FARPROC)&xwr::Shim_lstrcpyW);
REGISTER_SHIM("kernel32", "lstrcpynW", (FARPROC)&xwr::Shim_lstrcpynW);
REGISTER_SHIM("kernel32", "lstrcatW", (FARPROC)&xwr::Shim_lstrcatW);
// kernel32 forwards many entries to kernelbase (same shim).
REGISTER_SHIM("kernelbase", "CreateFileW", (FARPROC)&xwr::Shim_CreateFileW);
REGISTER_SHIM("kernelbase", "CloseHandle", (FARPROC)&xwr::Shim_CloseHandle);
REGISTER_SHIM("kernelbase", "ReadFile", (FARPROC)&xwr::Shim_ReadFile);
REGISTER_SHIM("kernelbase", "WriteFile", (FARPROC)&xwr::Shim_WriteFile);
REGISTER_SHIM("kernelbase", "VirtualAlloc", (FARPROC)&xwr::Shim_VirtualAlloc);
REGISTER_SHIM("kernelbase", "VirtualProtect", (FARPROC)&xwr::Shim_VirtualProtect);
REGISTER_SHIM("kernelbase", "VirtualFree", (FARPROC)&xwr::Shim_VirtualFree);
REGISTER_SHIM("kernelbase", "VirtualQuery", (FARPROC)&xwr::Shim_VirtualQuery);
REGISTER_SHIM("kernelbase", "GetCurrentThreadId", (FARPROC)&xwr::Shim_GetCurrentThreadId);
REGISTER_SHIM("kernelbase", "GetCurrentProcessId", (FARPROC)&xwr::Shim_GetCurrentProcessId);
REGISTER_SHIM("kernelbase", "GetLastError", (FARPROC)&xwr::Shim_GetLastError);
REGISTER_SHIM("kernelbase", "SetLastError", (FARPROC)&xwr::Shim_SetLastError);
REGISTER_SHIM("kernelbase", "Sleep", (FARPROC)&xwr::Shim_Sleep);
REGISTER_SHIM("kernelbase", "SleepEx", (FARPROC)&xwr::Shim_SleepEx);
REGISTER_SHIM("kernelbase", "QueryPerformanceCounter", (FARPROC)&xwr::Shim_QueryPerformanceCounter);
REGISTER_SHIM("kernelbase", "QueryPerformanceFrequency", (FARPROC)&xwr::Shim_QueryPerformanceFrequency);
REGISTER_SHIM("kernelbase", "GetTickCount", (FARPROC)&xwr::Shim_GetTickCount);
REGISTER_SHIM("kernelbase", "GetTickCount64", (FARPROC)&xwr::Shim_GetTickCount64);
REGISTER_SHIM("kernelbase", "WaitForSingleObject", (FARPROC)&xwr::Shim_WaitForSingleObject);
REGISTER_SHIM("kernelbase", "WaitForMultipleObjects", (FARPROC)&xwr::Shim_WaitForMultipleObjects);
REGISTER_SHIM("kernelbase", "InitializeCriticalSection", (FARPROC)&xwr::Shim_InitializeCriticalSection);
REGISTER_SHIM("kernelbase", "DeleteCriticalSection", (FARPROC)&xwr::Shim_DeleteCriticalSection);
REGISTER_SHIM("kernelbase", "EnterCriticalSection", (FARPROC)&xwr::Shim_EnterCriticalSection);
REGISTER_SHIM("kernelbase", "LeaveCriticalSection", (FARPROC)&xwr::Shim_LeaveCriticalSection);
REGISTER_SHIM("kernelbase", "GetEnvironmentVariableW", (FARPROC)&xwr::Shim_GetEnvironmentVariableW);
REGISTER_SHIM("kernelbase", "SetEnvironmentVariableW", (FARPROC)&xwr::Shim_SetEnvironmentVariableW);
REGISTER_SHIM("kernelbase", "GetModuleHandleW", (FARPROC)&xwr::Shim_GetModuleHandleW);
REGISTER_SHIM("kernelbase", "GetProcAddress", (FARPROC)&xwr::Shim_GetProcAddress);
REGISTER_SHIM("kernelbase", "LoadLibraryW", (FARPROC)&xwr::Shim_LoadLibraryW);
REGISTER_SHIM("kernelbase", "LoadLibraryExW", (FARPROC)&xwr::Shim_LoadLibraryExW);
REGISTER_SHIM("kernelbase", "GetSystemInfo", (FARPROC)&xwr::Shim_GetSystemInfo);
REGISTER_SHIM("kernelbase", "GlobalMemoryStatusEx", (FARPROC)&xwr::Shim_GlobalMemoryStatusEx);
REGISTER_SHIM("kernelbase", "MultiByteToWideChar", (FARPROC)&xwr::Shim_MultiByteToWideChar);
REGISTER_SHIM("kernelbase", "WideCharToMultiByte", (FARPROC)&xwr::Shim_WideCharToMultiByte);
REGISTER_SHIM("kernelbase", "OutputDebugStringW", (FARPROC)&xwr::Shim_OutputDebugStringW);
REGISTER_SHIM("kernelbase", "HeapAlloc", (FARPROC)&xwr::Shim_HeapAlloc);
REGISTER_SHIM("kernelbase", "HeapFree", (FARPROC)&xwr::Shim_HeapFree);
REGISTER_SHIM("kernelbase", "GetProcessHeap", (FARPROC)&xwr::Shim_GetProcessHeap);
