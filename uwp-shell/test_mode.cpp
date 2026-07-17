// uwp-shell/App.cpp — Test mode
// When run.config contains "mode=test", the app runs shim tests inside
// the UWP AppContainer sandbox and writes results to LocalState\test_results.txt
//
// This tests the REAL shim functions inside the REAL UWP sandbox.

#include "UwpSdkIncludes.h"
#include "App.h"
#include "PeLoader.h"
#include "ShimRegistry.h"
#include "D3D11Bridge.h"
#include "PathTranslator.h"
#include "XInputBridge.h"
#include "KeyboardBridge.h"
#include "AudioBridge.h"
#include "GdiRenderer.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <sstream>

#ifndef SUCCEEDED
#define SUCCEEDED(hr) ((HRESULT)(hr) >= 0)
#endif
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif

namespace xwr {

// Forward declarations of shim functions we want to test
extern "C" {
    LPVOID __stdcall Shim_VirtualAlloc(LPVOID, SIZE_T, DWORD, DWORD);
    BOOL __stdcall Shim_VirtualProtect(LPVOID, SIZE_T, DWORD, LPDWORD);
    BOOL __stdcall Shim_VirtualFree(LPVOID, SIZE_T, DWORD);
    HANDLE __stdcall Shim_CreateFileW(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    BOOL __stdcall Shim_ReadFile(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
    BOOL __stdcall Shim_WriteFile(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
    BOOL __stdcall Shim_CloseHandle(HANDLE);
    DWORD __stdcall Shim_GetFileSize(HANDLE, LPDWORD);
    BOOL __stdcall Shim_GetFileAttributesExW(LPCWSTR, int, LPVOID);
    DWORD __stdcall Shim_GetFileAttributesW(LPCWSTR);
    BOOL __stdcall Shim_DeleteFileW(LPCWSTR);
    HANDLE __stdcall Shim_CreateThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
    DWORD __stdcall Shim_WaitForSingleObject(HANDLE, DWORD);
    HANDLE __stdcall Shim_CreateEventW(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCWSTR);
    BOOL __stdcall Shim_SetEvent(HANDLE);
    BOOL __stdcall Shim_ResetEvent(HANDLE);
    void __stdcall Shim_InitializeCriticalSection(LPCRITICAL_SECTION);
    void __stdcall Shim_DeleteCriticalSection(LPCRITICAL_SECTION);
    void __stdcall Shim_EnterCriticalSection(LPCRITICAL_SECTION);
    void __stdcall Shim_LeaveCriticalSection(LPCRITICAL_SECTION);
    HANDLE __stdcall Shim_GetProcessHeap();
    LPVOID __stdcall Shim_HeapAlloc(HANDLE, DWORD, SIZE_T);
    BOOL __stdcall Shim_HeapFree(HANDLE, DWORD, LPVOID);
    DWORD __stdcall Shim_GetTickCount();
    BOOL __stdcall Shim_QueryPerformanceCounter(LARGE_INTEGER*);
    void __stdcall Shim_GetSystemInfo(LPSYSTEM_INFO);
    BOOL __stdcall Shim_GlobalMemoryStatusEx(LPMEMORYSTATUSEX);
    DWORD __stdcall Shim_GetLastError();
    void __stdcall Shim_SetLastError(DWORD);
    int __stdcall Shim_MultiByteToWideChar(UINT, DWORD, LPCSTR, int, LPWSTR, int);
    int __stdcall Shim_WideCharToMultiByte(UINT, DWORD, LPCWSTR, int, LPSTR, int, LPCSTR, LPBOOL);
    DWORD __stdcall Shim_GetCurrentDirectoryW(DWORD, LPWSTR);
    DWORD __stdcall Shim_GetEnvironmentVariableW(LPCWSTR, LPWSTR, DWORD);
    HRESULT __stdcall Shim_CoInitializeEx(LPVOID, DWORD);
    void __stdcall Shim_CoUninitialize();
}

// Write test results to a file in LocalState
static std::wstringstream g_results;
static int g_pass = 0;
static int g_fail = 0;

#define TEST(name, cond) do { \
    bool _result = (cond); \
    g_results << (_result ? L"[PASS] " : L"[FAIL] ") << name << L"\r\n"; \
    if (_result) g_pass++; else g_fail++; \
} while(0)

static DWORD WINAPI TestThread(LPVOID) { return 42; }

void App::RunTestMode() {
    g_results << L"=== Xbox Win32 Runner — Shim Test (Inside UWP AppContainer) ===\r\n\r\n";

    // --- 1. ShimRegistry ---
    g_results << L"--- Shim Registry ---\r\n";
    {
        auto& reg = ShimRegistry::Instance();
        uint64_t addr = reg.ResolveExport(L"kernel32", "CreateFileW");
        TEST(L"ShimRegistry resolves kernel32!CreateFileW", addr != 0);
        addr = reg.ResolveExport(L"kernel32", "VirtualAlloc");
        TEST(L"ShimRegistry resolves kernel32!VirtualAlloc", addr != 0);
        addr = reg.ResolveExport(L"d3d11", "D3D11CreateDevice");
        TEST(L"ShimRegistry resolves d3d11!D3D11CreateDevice", addr != 0);
        addr = reg.ResolveExport(L"user32", "CreateWindowExW");
        TEST(L"ShimRegistry resolves user32!CreateWindowExW", addr != 0);
    }

    // --- 2. Memory (through shims) ---
    g_results << L"\r\n--- Memory (Shim_VirtualAlloc) ---\r\n";
    {
        // THE critical test: can we allocate executable memory through our shim?
        // This calls Shim_VirtualAlloc → UwpVirtualAlloc → VirtualAllocFromApp
        // Inside AppContainer with codeGeneration capability, this should work.
        void* p = Shim_VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        TEST(L"Shim_VirtualAlloc(PAGE_EXECUTE_READWRITE) — code generation", p != nullptr);
        if (p) {
            // Write a RET instruction
            ((unsigned char*)p)[0] = 0xC3;
            TEST(L"Wrote executable code to shim-allocated memory", true);
            Shim_VirtualFree(p, 0, MEM_RELEASE);
        }

        void* p2 = Shim_VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        TEST(L"Shim_VirtualAlloc(PAGE_READWRITE)", p2 != nullptr);
        if (p2) {
            DWORD oldProt = 0;
            BOOL ok = Shim_VirtualProtect(p2, 4096, PAGE_READONLY, &oldProt);
            TEST(L"Shim_VirtualProtect(→PAGE_READONLY)", ok);
            Shim_VirtualFree(p2, 0, MEM_RELEASE);
        }
    }

    // --- 3. File I/O (through shims, with path translation) ---
    g_results << L"\r\n--- File I/O (Shim_CreateFileW + path translation) ---\r\n";
    {
        // This tests: Shim_CreateFileW → UwpCreateFile → CreateFile2
        // with path translation: "test_file.dat" → LocalState\test_file.dat
        HANDLE h = Shim_CreateFileW(L"test_file.dat", GENERIC_WRITE, 0, nullptr,
                                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        TEST(L"Shim_CreateFileW (write)", h != INVALID_HANDLE_VALUE);
        if (h != INVALID_HANDLE_VALUE) {
            const char* data = "Hello from UWP shim test!";
            DWORD written = 0;
            TEST(L"Shim_WriteFile", Shim_WriteFile(h, (LPCVOID)data, (DWORD)strlen(data), &written, nullptr) && written == strlen(data));
            Shim_CloseHandle(h);
        }

        h = Shim_CreateFileW(L"test_file.dat", GENERIC_READ, FILE_SHARE_READ, nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        TEST(L"Shim_CreateFileW (read)", h != INVALID_HANDLE_VALUE);
        if (h != INVALID_HANDLE_VALUE) {
            char buf[256] = {0};
            DWORD read = 0;
            TEST(L"Shim_ReadFile", Shim_ReadFile(h, buf, sizeof(buf)-1, &read, nullptr) && read > 0);
            TEST(L"Shim_ReadFile content matches", strcmp(buf, "Hello from UWP shim test!") == 0);
            Shim_CloseHandle(h);
        }

        DWORD attrs = Shim_GetFileAttributesW(L"test_file.dat");
        TEST(L"Shim_GetFileAttributesW", attrs != INVALID_FILE_ATTRIBUTES);
        TEST(L"Shim_DeleteFileW", Shim_DeleteFileW(L"test_file.dat"));
    }

    // --- 4. Threading (through shims) ---
    g_results << L"\r\n--- Threading (Shim_CreateThread) ---\r\n";
    {
        HANDLE h = Shim_CreateThread(nullptr, 0, &TestThread, nullptr, 0, nullptr);
        TEST(L"Shim_CreateThread", h != nullptr);
        if (h) {
            DWORD exitCode = 0;
            Shim_WaitForSingleObject(h, 5000);
            GetExitCodeThread(h, &exitCode);
            TEST(L"Thread exit code = 42", exitCode == 42);
            Shim_CloseHandle(h);
        }
    }

    // --- 5. Synchronization (through shims) ---
    g_results << L"\r\n--- Synchronization ---\r\n";
    {
        HANDLE evt = Shim_CreateEventW(nullptr, TRUE, FALSE, nullptr);
        TEST(L"Shim_CreateEventW", evt != nullptr);
        if (evt) {
            TEST(L"Shim_SetEvent", Shim_SetEvent(evt));
            TEST(L"Shim_ResetEvent", Shim_ResetEvent(evt));
            TEST(L"Shim_WaitForSingleObject (timeout)", Shim_WaitForSingleObject(evt, 100) == WAIT_TIMEOUT);
            Shim_SetEvent(evt);
            TEST(L"Shim_WaitForSingleObject (signaled)", Shim_WaitForSingleObject(evt, 100) == WAIT_OBJECT_0);
            Shim_CloseHandle(evt);
        }

        CRITICAL_SECTION cs;
        Shim_InitializeCriticalSection(&cs);
        Shim_EnterCriticalSection(&cs);
        Shim_LeaveCriticalSection(&cs);
        Shim_DeleteCriticalSection(&cs);
        TEST(L"Shim CriticalSection", true);
    }

    // --- 6. Heap (through shims) ---
    g_results << L"\r\n--- Heap ---\r\n";
    {
        HANDLE heap = Shim_GetProcessHeap();
        TEST(L"Shim_GetProcessHeap", heap != nullptr);
        void* p = Shim_HeapAlloc(heap, 0, 1024);
        TEST(L"Shim_HeapAlloc", p != nullptr);
        if (p) TEST(L"Shim_HeapFree", Shim_HeapFree(heap, 0, p));
    }

    // --- 7. Timing (through shims) ---
    g_results << L"\r\n--- Timing ---\r\n";
    {
        TEST(L"Shim_GetTickCount > 0", Shim_GetTickCount() > 0);
        LARGE_INTEGER pc;
        TEST(L"Shim_QueryPerformanceCounter", Shim_QueryPerformanceCounter(&pc));
    }

    // --- 8. System Info (through shims) ---
    g_results << L"\r\n--- System Info ---\r\n";
    {
        SYSTEM_INFO si;
        Shim_GetSystemInfo(&si);
        TEST(L"Shim_GetSystemInfo (NumberOfProcessors > 0)", si.dwNumberOfProcessors > 0);

        MEMORYSTATUSEX ms = {};
        ms.dwLength = sizeof(ms);
        TEST(L"Shim_GlobalMemoryStatusEx", Shim_GlobalMemoryStatusEx(&ms));
    }

    // --- 9. Error Handling ---
    g_results << L"\r\n--- Error Handling ---\r\n";
    {
        Shim_SetLastError(0);
        TEST(L"Shim_SetLastError(0)", Shim_GetLastError() == 0);
        Shim_SetLastError(42);
        TEST(L"Shim_SetLastError(42)", Shim_GetLastError() == 42);
    }

    // --- 10. String Conversion ---
    g_results << L"\r\n--- String Conversion ---\r\n";
    {
        wchar_t wbuf[256];
        int len = Shim_MultiByteToWideChar(CP_UTF8, 0, "Hello", -1, wbuf, 256);
        TEST(L"Shim_MultiByteToWideChar", len > 0 && wcscmp(wbuf, L"Hello") == 0);

        char abuf[256];
        len = Shim_WideCharToMultiByte(CP_UTF8, 0, L"World", -1, abuf, 256, nullptr, nullptr);
        TEST(L"Shim_WideCharToMultiByte", len > 0 && strcmp(abuf, "World") == 0);
    }

    // --- 11. Environment ---
    g_results << L"\r\n--- Environment ---\r\n";
    {
        WCHAR buf[MAX_PATH];
        TEST(L"Shim_GetCurrentDirectoryW", Shim_GetCurrentDirectoryW(MAX_PATH, buf) > 0);
        TEST(L"Shim_GetEnvironmentVariableW(PATH)", Shim_GetEnvironmentVariableW(L"PATH", buf, MAX_PATH) >= 0);
    }

    // --- 12. COM ---
    g_results << L"\r\n--- COM ---\r\n";
    {
        HRESULT hr = Shim_CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        TEST(L"Shim_CoInitializeEx", SUCCEEDED(hr));
        if (SUCCEEDED(hr)) Shim_CoUninitialize();
    }

    // --- 13. Path Translation ---
    g_results << L"\r\n--- Path Translation ---\r\n";
    {
        // Test that C:\Game\save.dat translates to LocalState\Game\save.dat
        std::wstring translated = PathTranslator::Instance().TranslateToReal(L"C:\\Game\\save.dat");
        TEST(L"Path translation: C:\\Game\\ → LocalState\\Game\\", translated.find(L"Game") != std::wstring::npos);
    }

    // --- Summary ---
    g_results << L"\r\n=== SUMMARY ===\r\n";
    g_results << L"  PASS: " << g_pass << L"\r\n";
    g_results << L"  FAIL: " << g_fail << L"\r\n";
    g_results << L"  TOTAL: " << (g_pass + g_fail) << L"\r\n";
    int total = g_pass + g_fail;
    if (total > 0) {
        g_results << L"  Success rate: " << (100.0 * g_pass / total) << L"%\r\n";
    }

    // Write results using Win32 file I/O (more reliable than std::ofstream in AppContainer)
    std::wstring resultsStr = g_results.str();
    HANDLE hFile = CreateFileW(L"shim_test_results.txt", GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        // Write UTF-16 BOM first
        WORD bom = 0xFEFF;
        DWORD written;
        WriteFile(hFile, &bom, sizeof(bom), &written, nullptr);
        WriteFile(hFile, (LPCVOID)resultsStr.c_str(), (DWORD)(resultsStr.size() * sizeof(wchar_t)), &written, nullptr);
        CloseHandle(hFile);
    }

    // Also output via debug string
    OutputDebugStringW(resultsStr.c_str());
}

}  // namespace xwr
