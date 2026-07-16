// bridges/KeyboardBridge.h
// Translates UWP CoreWindow key events into the Win32 message queue so that
// games pumping messages via PeekMessage/GetMessage see WM_KEYDOWN /
// WM_KEYUP / WM_CHAR exactly as if they were running on the desktop.
//
// The UWP shell registers handlers for CoreWindow::KeyDown / KeyUp /
// CharacterReceived and forwards each event here. We:
//   1) Update the 256-byte key-state array (used by GetAsyncKeyState /
//      GetKeyState / GetKeyboardState, which the user32 shim implements).
//   2) Post WM_KEYDOWN / WM_KEYUP / WM_CHAR into the active window's
//      message queue via PostMessageW.
//
// lParam packing for WM_KEY* messages (Win32 convention):
//   bits  0..15 : repeat count (1)
//   bits 16..23 : scan code
//   bit     24  : extended-key flag
//   bits 25..28 : reserved
//   bit     29  : context code (0 for WM_KEYDOWN, 1 if Alt held for WM_KEYUP)
//   bit     30  : previous key state (0=was up, 1=was down)
//   bit     31  : transition state (0=going down, 1=going up)

#pragma once
#include <Windows.h>

#include <cstdint>

namespace xwr {

class KeyboardBridge {
public:
    static KeyboardBridge& Instance();

    // Called by the UWP shell on CoreWindow key events.
    void OnKeyDown(UINT virtualKey, UINT scanCode, BOOL wasKeyDown);
    void OnKeyUp(UINT virtualKey, UINT scanCode, BOOL wasKeyDown);
    void OnCharacterReceived(UINT codePoint);

    // Track pressed/released state for GetAsyncKeyState (user32 shim).
    short GetAsyncKeyState(int vKey) const;
    short GetKeyState(int vKey) const;
    BOOL GetKeyboardState(PBYTE lpKeyState) const;

    // Set the HWND that keyboard messages are posted to. The UWP shell should
    // call this once it has created the main game window.
    void SetTargetWindow(HWND hwnd) { m_targetHwnd = hwnd; }

private:
    KeyboardBridge();
    KeyboardBridge(const KeyboardBridge&) = delete;
    KeyboardBridge& operator=(const KeyboardBridge&) = delete;

    uint8_t m_keyState[256] = {};
    HWND    m_targetHwnd = 0;
};

}  // namespace xwr
