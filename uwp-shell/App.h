// uwp-shell/App.h
// Main application class for the XboxWin32Runner UWP shell.
//
// Lifecycle:
//   1) App::Main() constructs an App instance.
//   2) LoadConfig("run.config") reads the small text file that tells us
//      which .exe to launch, where its working directory is, what command-
//      line args to pass, and what extra DLL search dirs to register.
//   3) Initialize() brings up the D3D11 bridge, ShimRegistry singleton,
//      and PathTranslator. Real UWP / CoreWindow / WinRT calls are wrapped
//      in #ifndef XWR_SYNTAX_CHECK so the file compiles on Linux.
//   4) Run() spawns a background thread that calls PeLoader.LoadModuleFromPath
//      + PeLoader.RunExe. On the main thread it runs a GetMessage /
//      TranslateMessage / DispatchMessage pump and calls
//      D3D11Bridge::Instance().Present(1, 0) each frame.

#pragma once
#include <Windows.h>
#include <atomic>
#include <string>

namespace xwr {

class App {
public:
    // Single-shot entry point used by wWinMain / main().
    static int Main();

    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // Read run.config to find the .exe path, cwd, args, and dll_search_path.
    bool LoadConfig(const std::wstring& configPath);

    // Initialize the D3D11 bridge, shim registry, PE loader.
    // (On real builds, also creates the CoreWindow and obtains its HWND.)
    bool Initialize();

    // Load and run the configured .exe on a background thread, then run the
    // message pump on the calling thread until WM_QUIT.
    int Run();

    // XInput polling thread context (public so the thread function can access it).
    struct XInputPollCtx {
        std::atomic<bool> stop{false};
    };

private:
    std::wstring m_exePath;        // e.g. C:\Game\HalfSword.exe
    std::wstring m_workingDir;     // exe's directory
    std::wstring m_cmdLine;        // command-line args (e.g. "-dx11")
    std::wstring m_dllSearchPath;  // ;-separated list of additional DLL dirs
    bool         m_initialized = false;

    // Window handle for the main UWP window. Set by Initialize() via
    // CoreWindow interop on real builds; 0 if interop fails.
    HWND m_hwnd = 0;

    // XInput polling thread (250Hz). Started by Initialize(), stopped by Run().
    XInputPollCtx* m_xinputPollCtx = nullptr;
    HANDLE         m_xinputThread  = nullptr;
};

}  // namespace xwr
