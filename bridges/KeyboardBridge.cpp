// bridges/KeyboardBridge.cpp
// Converts UWP CoreWindow key events to WM_KEYDOWN/WM_KEYUP/WM_CHAR messages
// and maintains the 256-byte key-state array consumed by the user32 shim's
// GetAsyncKeyState / GetKeyState / GetKeyboardState.
//
// All Win32 calls used here (PostMessageW, GetForegroundWindow) are declared
// in winheaders/Windows.h, so this file compiles in both syntax-check and
// real-build modes without any #ifdef.

#include "UwpSdkIncludes.h"


#include <cstring>

#include "KeyboardBridge.h"

namespace xwr {

// Build the lParam for a WM_KEYDOWN / WM_KEYUP message.
//   repeatCount = 1
//   scanCode in bits 16..23
//   extended flag in bit 24 (set for arrow keys, Numpad, etc. — caller can extend)
//   prev-state bit 30: 0 if previously up, 1 if previously down
//   transition bit 31: 0 for WM_KEYDOWN, 1 for WM_KEYUP
static LPARAM MakeKeyLParam(UINT scanCode, BOOL wasKeyDown, BOOL goingUp) {
    LPARAM lp = 0;
    lp |= (LPARAM)1;                       // repeat count = 1
    lp |= ((LPARAM)(scanCode & 0xFF)) << 16;
    // bit 24 (extended) left at 0 — caller could OR in 0x01000000 for extended keys
    // bit 29 (context code) = 0 (Alt not held — Alt+key produces WM_SYSKEYDOWN instead)
    if (wasKeyDown) lp |= (LPARAM)1 << 30;  // previous state: was down
    if (goingUp)    lp |= (LPARAM)1 << 31;  // transition: 1 = going up
    return lp;
}

KeyboardBridge::KeyboardBridge() = default;

KeyboardBridge& KeyboardBridge::Instance() {
    static KeyboardBridge inst;
    return inst;
}

void KeyboardBridge::OnKeyDown(UINT virtualKey, UINT scanCode, BOOL wasKeyDown) {
    if (virtualKey >= 256) return;

    // Update key-state array: bit 0x80 = "currently pressed".
    m_keyState[virtualKey] = 0x80;

    // Pick a target window: prefer the explicitly-set target, else the
    // foreground window (which on UWP is the CoreWindow's HWND).
    HWND target = m_targetHwnd;
    if (!target) {
        target = GetForegroundWindow();
    }
    if (target) {
        LPARAM lp = MakeKeyLParam(scanCode, wasKeyDown, /*goingUp=*/FALSE);
        PostMessageW(target, WM_KEYDOWN, (WPARAM)virtualKey, lp);
    }
}

void KeyboardBridge::OnKeyUp(UINT virtualKey, UINT scanCode, BOOL wasKeyDown) {
    if (virtualKey >= 256) return;

    // Clear the pressed bit but keep bit 0x01 ("toggled") for keys like CapsLock.
    m_keyState[virtualKey] = (uint8_t)(m_keyState[virtualKey] & ~0x80);

    HWND target = m_targetHwnd;
    if (!target) {
        target = GetForegroundWindow();
    }
    if (target) {
        LPARAM lp = MakeKeyLParam(scanCode, wasKeyDown, /*goingUp=*/TRUE);
        PostMessageW(target, WM_KEYUP, (WPARAM)virtualKey, lp);
    }
}

void KeyboardBridge::OnCharacterReceived(UINT codePoint) {
    HWND target = m_targetHwnd;
    if (!target) {
        target = GetForegroundWindow();
    }
    if (target) {
        // WM_CHAR lParam mirrors the WM_KEYDOWN that produced the character.
        // We don't have the originating scan code here, so pass repeatCount=1
        // and leave the rest zero.
        LPARAM lp = 1;
        PostMessageW(target, WM_CHAR, (WPARAM)codePoint, lp);
    }
}

short KeyboardBridge::GetAsyncKeyState(int vKey) const {
    if (vKey < 0 || vKey >= 256) return 0;
    // MSB (0x8000) = "currently pressed".
    // LSB (0x0001) = "pressed since last query" — we don't track this, leave 0.
    return (m_keyState[vKey] & 0x80) ? (short)0x8000 : (short)0;
}

short KeyboardBridge::GetKeyState(int vKey) const {
    if (vKey < 0 || vKey >= 256) return 0;
    short result = 0;
    // MSB (0x8000) = "currently pressed".
    if (m_keyState[vKey] & 0x80) result |= 0x8000;
    // LSB (0x0001) = "toggled" (CapsLock / NumLock / ScrollLock).
    // We don't maintain toggle state, so always 0 — the user32 shim could
    // refine this later if a game actually reads CapsLock toggle.
    return result;
}

BOOL KeyboardBridge::GetKeyboardState(PBYTE lpKeyState) const {
    if (!lpKeyState) return FALSE;
    std::memcpy(lpKeyState, m_keyState, 256);
    return TRUE;
}

}  // namespace xwr
