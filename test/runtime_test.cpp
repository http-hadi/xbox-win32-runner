// test/runtime_test.cpp
// Runtime test that verifies each Win32 API our shim layer uses actually works.
// This is a STANDALONE console app — it does NOT include UwpSdkIncludes.h or
// CommonPre.h. It uses the real Windows SDK headers directly.
//
// Built with: cl /std:c++17 /EHsc runtime_test.cpp /link kernel32.lib ...

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <d3d11.h>
#include <xinput.h>
#include <mmsystem.h>
#include <objbase.h>
#include <stdio.h>

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")

static int g_pass = 0;
static int g_fail = 0;
static FILE* g_log = nullptr;

#define TEST(name, cond) do { \
    bool result = (cond); \
    const char* status = result ? "PASS" : "FAIL"; \
    printf("[%s] %s\n", status, name); \
    if (g_log) fprintf(g_log, "[%s] %s\n", status, name); \
    if (result) g_pass++; else g_fail++; \
} while(0)

int main() {
    g_log = fopen("runtime_test_results.txt", "w");

    printf("=== Xbox Win32 Runner — Runtime API Test ===\n");
    printf("Testing each Win32 API our shim layer uses.\n\n");

    // --- 1. Memory: VirtualAllocFromApp with PAGE_EXECUTE_READWRITE ---
    printf("\n--- Memory ---\n");
    {
        void* p = VirtualAllocFromApp(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        TEST("VirtualAllocFromApp(PAGE_EXECUTE_READWRITE) — code generation", p != nullptr);
        if (p) {
            ((unsigned char*)p)[0] = 0xC3;
            TEST("Wrote executable code to allocated memory", true);
            VirtualFree(p, 0, MEM_RELEASE);
        }

        void* p2 = VirtualAllocFromApp(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        TEST("VirtualAllocFromApp(PAGE_READWRITE)", p2 != nullptr);
        if (p2) VirtualFree(p2, 0, MEM_RELEASE);

        void* p3 = VirtualAllocFromApp(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        DWORD oldProt = 0;
        BOOL protOk = VirtualProtectFromApp(p3, 4096, PAGE_READONLY, &oldProt);
        TEST("VirtualProtectFromApp(PAGE_READWRITE -> PAGE_READONLY)", protOk);
        if (p3) VirtualFree(p3, 0, MEM_RELEASE);
    }

    // --- 2. File I/O ---
    printf("\n--- File I/O ---\n");
    {
        HANDLE h = CreateFileW(L"runtime_test_file.dat", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        TEST("CreateFileW (write)", h != INVALID_HANDLE_VALUE);
        if (h != INVALID_HANDLE_VALUE) {
            const char* data = "Hello from Xbox Win32 Runner!";
            DWORD written = 0;
            TEST("WriteFile", WriteFile(h, data, (DWORD)strlen(data), &written, nullptr) && written == strlen(data));
            CloseHandle(h);
        }

        h = CreateFileW(L"runtime_test_file.dat", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        TEST("CreateFileW (read)", h != INVALID_HANDLE_VALUE);
        if (h != INVALID_HANDLE_VALUE) {
            char buf[256] = {0};
            DWORD read = 0;
            TEST("ReadFile", ReadFile(h, buf, sizeof(buf)-1, &read, nullptr) && read > 0);
            TEST("ReadFile content matches", strcmp(buf, "Hello from Xbox Win32 Runner!") == 0);
            CloseHandle(h);
        }

        DWORD attrs = GetFileAttributesW(L"runtime_test_file.dat");
        TEST("GetFileAttributesW", attrs != INVALID_FILE_ATTRIBUTES);
        TEST("DeleteFileW", DeleteFileW(L"runtime_test_file.dat"));
    }

    // --- 3. Threading ---
    printf("\n--- Threading ---\n");
    {
        HANDLE h = CreateThread(nullptr, 0, [](LPVOID) -> DWORD { return 42; }, nullptr, 0, nullptr);
        TEST("CreateThread", h != nullptr);
        if (h) {
            DWORD exitCode = 0;
            WaitForSingleObject(h, 5000);
            GetExitCodeThread(h, &exitCode);
            TEST("Thread exit code = 42", exitCode == 42);
            CloseHandle(h);
        }
    }

    // --- 4. Synchronization ---
    printf("\n--- Synchronization ---\n");
    {
        HANDLE evt = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        TEST("CreateEventW", evt != nullptr);
        if (evt) {
            TEST("SetEvent", SetEvent(evt));
            TEST("ResetEvent", ResetEvent(evt));
            TEST("WaitForSingleObject (timeout)", WaitForSingleObject(evt, 100) == WAIT_TIMEOUT);
            SetEvent(evt);
            TEST("WaitForSingleObject (signaled)", WaitForSingleObject(evt, 100) == WAIT_OBJECT_0);
            CloseHandle(evt);
        }

        CRITICAL_SECTION cs;
        InitializeCriticalSection(&cs);
        EnterCriticalSection(&cs);
        LeaveCriticalSection(&cs);
        DeleteCriticalSection(&cs);
        TEST("CriticalSection (init/enter/leave/delete)", true);
    }

    // --- 5. Heap ---
    printf("\n--- Heap ---\n");
    {
        HANDLE heap = GetProcessHeap();
        TEST("GetProcessHeap", heap != nullptr);
        void* p = HeapAlloc(heap, 0, 1024);
        TEST("HeapAlloc", p != nullptr);
        if (p) TEST("HeapFree", HeapFree(heap, 0, p));
    }

    // --- 6. Timing ---
    printf("\n--- Timing ---\n");
    {
        TEST("GetTickCount > 0", GetTickCount() > 0);
        TEST("GetTickCount64 > 0", GetTickCount64() > 0);
        LARGE_INTEGER pc, pf;
        TEST("QueryPerformanceCounter", QueryPerformanceCounter(&pc));
        TEST("QueryPerformanceFrequency", QueryPerformanceFrequency(&pf) && pf.QuadPart > 0);
    }

    // --- 7. System Info ---
    printf("\n--- System Info ---\n");
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        TEST("GetSystemInfo (NumberOfProcessors > 0)", si.dwNumberOfProcessors > 0);

        MEMORYSTATUSEX ms = {};
        ms.dwLength = sizeof(ms);
        TEST("GlobalMemoryStatusEx", GlobalMemoryStatusEx(&ms));
        TEST("Memory > 1GB", ms.ullTotalPhys > 1024ULL * 1024 * 1024);
    }

    // --- 8. Error Handling ---
    printf("\n--- Error Handling ---\n");
    {
        SetLastError(0);
        TEST("SetLastError(0)", GetLastError() == 0);
        SetLastError(42);
        TEST("SetLastError(42) / GetLastError", GetLastError() == 42);
    }

    // --- 9. String Conversion ---
    printf("\n--- String Conversion ---\n");
    {
        wchar_t wbuf[256];
        int len = MultiByteToWideChar(CP_UTF8, 0, "Hello", -1, wbuf, 256);
        TEST("MultiByteToWideChar", len > 0 && wcscmp(wbuf, L"Hello") == 0);

        char abuf[256];
        len = WideCharToMultiByte(CP_UTF8, 0, L"World", -1, abuf, 256, nullptr, nullptr);
        TEST("WideCharToMultiByte", len > 0 && strcmp(abuf, "World") == 0);
    }

    // --- 10. D3D11 ---
    printf("\n--- D3D11 ---\n");
    {
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* ctx = nullptr;
        D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
        D3D_FEATURE_LEVEL obtainedFl = D3D_FEATURE_LEVEL_11_0;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &fl, 1, 7, &device, &obtainedFl, &ctx);
        TEST("D3D11CreateDevice (FL 11.0)", SUCCEEDED(hr) && device != nullptr);
        if (device) {
            TEST("D3D11 feature level is 11.0", obtainedFl == D3D_FEATURE_LEVEL_11_0);
            if (ctx) ctx->Release();
            device->Release();
        }
    }

    // --- 11. XInput ---
    printf("\n--- XInput ---\n");
    {
        XINPUT_STATE state;
        DWORD result = XInputGetState(0, &state);
        TEST("XInputGetState (API callable)", result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED);
    }

    // --- 12. Registry ---
    printf("\n--- Registry ---\n");
    {
        HKEY hKey;
        LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);
        TEST("RegOpenKeyExW", result == ERROR_SUCCESS);
        if (result == ERROR_SUCCESS) {
            WCHAR buf[256];
            DWORD bufSize = sizeof(buf);
            DWORD type;
            TEST("RegQueryValueExW(ProductName)", RegQueryValueExW(hKey, L"ProductName", nullptr, &type, (LPBYTE)buf, &bufSize) == ERROR_SUCCESS);
            RegCloseKey(hKey);
        }
    }

    // --- 13. Networking ---
    printf("\n--- Networking ---\n");
    {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        TEST("WSAStartup", result == 0);
        if (result == 0) {
            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            TEST("socket(AF_INET, SOCK_STREAM)", s != INVALID_SOCKET);
            if (s != INVALID_SOCKET) closesocket(s);
            WSACleanup();
        }
    }

    // --- 14. Multimedia ---
    printf("\n--- Multimedia ---\n");
    {
        TEST("timeGetTime > 0", timeGetTime() > 0);
    }

    // --- 15. Environment ---
    printf("\n--- Environment ---\n");
    {
        WCHAR buf[MAX_PATH];
        TEST("GetCurrentDirectoryW", GetCurrentDirectoryW(MAX_PATH, buf) > 0);
        TEST("GetEnvironmentVariableW(PATH)", GetEnvironmentVariableW(L"PATH", buf, MAX_PATH) > 0);
        TEST("GetTempPathW", GetTempPathW(MAX_PATH, buf) > 0);
    }

    // --- 16. Console ---
    printf("\n--- Console ---\n");
    {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        TEST("GetStdHandle", h != INVALID_HANDLE_VALUE);
        DWORD mode;
        TEST("GetConsoleMode", GetConsoleMode(h, &mode));
    }

    // --- 17. COM ---
    printf("\n--- COM ---\n");
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        TEST("CoInitializeEx", SUCCEEDED(hr));
        if (SUCCEEDED(hr)) CoUninitialize();
    }

    // --- Summary ---
    printf("\n=== SUMMARY ===\n");
    printf("  PASS: %d\n", g_pass);
    printf("  FAIL: %d\n", g_fail);
    printf("  TOTAL: %d\n", g_pass + g_fail);
    printf("  Success rate: %.1f%%\n", 100.0 * g_pass / (g_pass + g_fail));
    if (g_log) {
        fprintf(g_log, "\n=== SUMMARY ===\n");
        fprintf(g_log, "  PASS: %d\n", g_pass);
        fprintf(g_log, "  FAIL: %d\n", g_fail);
        fprintf(g_log, "  TOTAL: %d\n", g_pass + g_fail);
        fprintf(g_log, "  Success rate: %.1f%%\n", 100.0 * g_pass / (g_pass + g_fail));
        fclose(g_log);
    }
    return g_fail > 0 ? 1 : 0;
}
