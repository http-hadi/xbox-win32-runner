// bridges/XInputBridge.h
// Translates Windows.Gaming.Input (WinRT) gamepad state into XINPUT_STATE
// for legacy games that call XInputGetState / XInputSetState.
//
// The UWP shell's input thread calls Poll() at ~250 Hz. The XInput shim's
// Shim_XInputGetState reads the latest cached state from GetState().
//
// Xbox controller button mapping (Windows.Gaming.Input.GamepadButtons → XINPUT):
//   Menu              → XINPUT_GAMEPAD_START
//   View              → XINPUT_GAMEPAD_BACK
//   A                 → XINPUT_GAMEPAD_A
//   B                 → XINPUT_GAMEPAD_B
//   X                 → XINPUT_GAMEPAD_X
//   Y                 → XINPUT_GAMEPAD_Y
//   LeftShoulder      → XINPUT_GAMEPAD_LEFT_SHOULDER
//   RightShoulder     → XINPUT_GAMEPAD_RIGHT_SHOULDER
//   LeftThumbstick    → XINPUT_GAMEPAD_LEFT_THUMB
//   RightThumbstick   → XINPUT_GAMEPAD_RIGHT_THUMB
//   DPadUp/Down/Left/Right → XINPUT_GAMEPAD_DPAD_*
//
// Triggers (0.0..1.0) → BYTE (0..255); thumbsticks (-1.0..1.0) → SHORT (±32767).

#pragma once
#include <Windows.h>

#include <cstdint>

namespace xwr {

class XInputBridge {
public:
    static XInputBridge& Instance();

    // Poll all 4 controller slots. Called by shim_XInputGetState at ~250 Hz.
    void Poll();

    // Get the latest state for slot [0..3]. Returns false if no controller
    // is connected at that index.
    bool GetState(DWORD dwUserIndex, XINPUT_STATE* pState);

    // Set vibration. Scales 0..65535 → 0.0..1.0.
    bool SetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);

private:
    XInputBridge();
    XInputBridge(const XInputBridge&) = delete;
    XInputBridge& operator=(const XInputBridge&) = delete;

    // One cached state per slot. dwPacketNumber is bumped on every change.
    XINPUT_STATE m_states[4] = {};
    // True if a gamepad was seen at this slot during the last Poll().
    bool m_connected[4] = {};
    // Packet number — incremented every Poll() where the state changed.
    uint32_t m_packetNumbers[4] = {1, 1, 1, 1};
};

}  // namespace xwr
