// uwp-shell/main.cpp
// Entry point for the XboxWin32Runner UWP app.
//
// On real Windows / Xbox builds, wWinMain is the standard UWP entry point
// (linked with /SUBSYSTEM:WINDOWS). On Linux syntax-check builds, g++ has
// no wWinMain, so we also expose a plain main() that the syntax-check
// harness can compile.
//
// The real UWP startup (CoreApplication / IFrameworkViewSource) is owned by
// App::Main(); this file is intentionally tiny so the lifecycle is easy to
// follow.

#include "UwpSdkIncludes.h"
#include "App.h"

#ifdef XWR_SYNTAX_CHECK
// g++ -fsyntax-check has no wWinMain concept; provide a main() so the TU
// compiles. This is never linked into the real UWP binary.
int main(int /*argc*/, char** /*argv*/) {
    return xwr::App::Main();
}
#else
int WINAPI wWinMain(HINSTANCE /*hInstance*/,
                    HINSTANCE /*hPrevInstance*/,
                    PWSTR    /*lpCmdLine*/,
                    int      /*nCmdShow*/) {
    return xwr::App::Main();
}
#endif
