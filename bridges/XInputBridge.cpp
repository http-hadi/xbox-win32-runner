// bridges/XInputBridge.cpp
// Polls Windows.Gaming.Input::Gamepad::Gamepads via C++/WinRT and translates
// each reading into XINPUT_STATE.
//
// The translation math (thumbstick scaling, button-bit assembly) is plain C++
// and compiles under the stub Windows.h. The WinRT access — enumerating the
// Gamepads vector, reading CurrentReading, setting Vibration — requires the
// C++/WinRT headers (winrt/Windows.Gaming.Input.h) which only exist in real
// UWP builds, so those calls are wrapped in #ifndef XWR_SYNTAX_CHECK.

#include "UwpSdkIncludes.h"


#include <cmath>

#include "XInputBridge.h"

#ifndef XWR_SYNTAX_CHECK
#include <winrt/Windows.Gaming.Input.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/base.h>
// WinRT lib not needed - CppWinRT handles linking
#endif

namespace xwr {

// GamepadButtons bitmask values from Windows.Gaming.Input.
// We redefine them here as plain ints so the translation code below compiles
// in both stub and real builds (the real enum is pulled in via winrt only).
static const uint32_t XWR_GB_NONE            = 0x00000000;
static const uint32_t XWR_GB_MENU            = 0x00000001;
static const uint32_t XWR_GB_VIEW            = 0x00000002;
static const uint32_t XWR_GB_A               = 0x00000004;
static const uint32_t XWR_GB_B               = 0x00000008;
static const uint32_t XWR_GB_X               = 0x00000010;
static const uint32_t XWR_GB_Y               = 0x00000020;
static const uint32_t XWR_GB_DPAD_UP         = 0x00000040;
static const uint32_t XWR_GB_DPAD_DOWN       = 0x00000080;
static const uint32_t XWR_GB_DPAD_LEFT       = 0x00000100;
static const uint32_t XWR_GB_DPAD_RIGHT      = 0x00000200;
static const uint32_t XWR_GB_LEFT_SHOULDER   = 0x00000400;
static const uint32_t XWR_GB_RIGHT_SHOULDER  = 0x00000800;
static const uint32_t XWR_GB_LEFT_THUMB      = 0x00001000;
static const uint32_t XWR_GB_RIGHT_THUMB     = 0x00002000;

// GamepadReading is a WinRT struct; in syntax-check builds we declare a local
// equivalent so the Translate() function below compiles identically in both.
struct XwrGamepadReading {
    uint32_t  Buttons;
    double    LeftThumbstickX;
    double    LeftThumbstickY;
    double    RightThumbstickX;
    double    RightThumbstickY;
    double    LeftTrigger;
    double    RightTrigger;
};

struct XwrGamepadVibration {
    double LeftMotor;
    double RightMotor;
    double LeftTrigger;
    double RightTrigger;
};

// Translate a WinRT GamepadReading into XINPUT_STATE.
static void TranslateReading(const XwrGamepadReading& r, XINPUT_STATE* out) {
    XINPUT_GAMEPAD& g = out->Gamepad;
    g.wButtons = 0;
    if (r.Buttons & XWR_GB_MENU)           g.wButtons |= XINPUT_GAMEPAD_START;
    if (r.Buttons & XWR_GB_VIEW)           g.wButtons |= XINPUT_GAMEPAD_BACK;
    if (r.Buttons & XWR_GB_A)              g.wButtons |= XINPUT_GAMEPAD_A;
    if (r.Buttons & XWR_GB_B)              g.wButtons |= XINPUT_GAMEPAD_B;
    if (r.Buttons & XWR_GB_X)              g.wButtons |= XINPUT_GAMEPAD_X;
    if (r.Buttons & XWR_GB_Y)              g.wButtons |= XINPUT_GAMEPAD_Y;
    if (r.Buttons & XWR_GB_LEFT_SHOULDER)  g.wButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;
    if (r.Buttons & XWR_GB_RIGHT_SHOULDER) g.wButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;
    if (r.Buttons & XWR_GB_LEFT_THUMB)     g.wButtons |= XINPUT_GAMEPAD_LEFT_THUMB;
    if (r.Buttons & XWR_GB_RIGHT_THUMB)    g.wButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;
    if (r.Buttons & XWR_GB_DPAD_UP)        g.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
    if (r.Buttons & XWR_GB_DPAD_DOWN)      g.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
    if (r.Buttons & XWR_GB_DPAD_LEFT)      g.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
    if (r.Buttons & XWR_GB_DPAD_RIGHT)     g.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;

    // Triggers: 0.0..1.0 → 0..255
    g.bLeftTrigger  = (BYTE)std::lround(r.LeftTrigger  * 255.0);
    g.bRightTrigger = (BYTE)std::lround(r.RightTrigger * 255.0);

    // Thumbsticks: -1.0..1.0 → -32768..32767
    auto clamp1 = [](double v) -> double {
        if (v > 1.0)  return 1.0;
        if (v < -1.0) return -1.0;
        return v;
    };
    g.sThumbLX = (SHORT)std::lround(clamp1(r.LeftThumbstickX)  * 32767.0);
    g.sThumbLY = (SHORT)std::lround(clamp1(r.LeftThumbstickY)  * 32767.0);
    g.sThumbRX = (SHORT)std::lround(clamp1(r.RightThumbstickX) * 32767.0);
    g.sThumbRY = (SHORT)std::lround(clamp1(r.RightThumbstickY) * 32767.0);
}

XInputBridge::XInputBridge() = default;

XInputBridge& XInputBridge::Instance() {
    static XInputBridge inst;
    return inst;
}

void XInputBridge::Poll() {
#ifndef XWR_SYNTAX_CHECK
    using namespace winrt::Windows::Gaming::Input;
    using namespace winrt::Windows::Foundation::Collections;

    // Get the live gamepad list. WinRT handles connect/disconnect events.
    auto gamepads = Gamepad::Gamepads();
    uint32_t count = gamepads.Size();
    if (count > 4) count = 4;  // XInput only exposes 4 slots

    for (uint32_t i = 0; i < 4; ++i) {
        if (i < count) {
            auto gp = gamepads.GetAt(i);
            GamepadReading wr = gp.GetCurrentReading();

            XwrGamepadReading r;
            r.Buttons          = (uint32_t)wr.Buttons;
            r.LeftThumbstickX  = wr.LeftThumbstickX;
            r.LeftThumbstickY  = wr.LeftThumbstickY;
            r.RightThumbstickX = wr.RightThumbstickX;
            r.RightThumbstickY = wr.RightThumbstickY;
            r.LeftTrigger      = wr.LeftTrigger;
            r.RightTrigger     = wr.RightTrigger;

            XINPUT_STATE newState = {};
            TranslateReading(r, &newState);

            // Bump packet number if anything changed.
            XINPUT_STATE& old = m_states[i];
            bool changed = (old.Gamepad.wButtons != newState.Gamepad.wButtons) ||
                           (old.Gamepad.bLeftTrigger  != newState.Gamepad.bLeftTrigger) ||
                           (old.Gamepad.bRightTrigger != newState.Gamepad.bRightTrigger) ||
                           (old.Gamepad.sThumbLX != newState.Gamepad.sThumbLX) ||
                           (old.Gamepad.sThumbLY != newState.Gamepad.sThumbLY) ||
                           (old.Gamepad.sThumbRX != newState.Gamepad.sThumbRX) ||
                           (old.Gamepad.sThumbRY != newState.Gamepad.sThumbRY);
            if (changed || !m_connected[i]) {
                m_packetNumbers[i]++;
            }
            newState.dwPacketNumber = m_packetNumbers[i];
            old = newState;
            m_connected[i] = true;
        } else {
            m_connected[i] = false;
        }
    }
#else
    // Syntax-check stub: nothing to poll without WinRT.
    for (int i = 0; i < 4; ++i) {
        m_connected[i] = false;
    }
#endif
}

bool XInputBridge::GetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
    if (!pState) return false;
    if (dwUserIndex >= 4) return false;
    if (!m_connected[dwUserIndex]) {
        std::memset(pState, 0, sizeof(XINPUT_STATE));
        return false;  // ERROR_DEVICE_NOT_CONNECTED semantics
    }
    *pState = m_states[dwUserIndex];
    return true;
}

bool XInputBridge::SetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration) {
    if (!pVibration) return false;
    if (dwUserIndex >= 4) return false;
    if (!m_connected[dwUserIndex]) return false;

#ifndef XWR_SYNTAX_CHECK
    using namespace winrt::Windows::Gaming::Input;

    auto gamepads = Gamepad::Gamepads();
    if (dwUserIndex >= gamepads.Size()) return false;

    auto gp = gamepads.GetAt(dwUserIndex);
    GamepadVibration v;
    // XINPUT motors are 0..65535; WinRT uses 0.0..1.0.
    v.LeftMotor  = (double)pVibration->wLeftMotorSpeed  / 65535.0;
    v.RightMotor = (double)pVibration->wRightMotorSpeed / 65535.0;
    // Xbox controllers with impulse triggers expose LeftTrigger/RightTrigger
    // on the vibration struct; XInput doesn't, so leave them at 0.
    v.LeftTrigger  = 0.0;
    v.RightTrigger = 0.0;
    gp.Vibration(v);
    return true;
#else
    (void)pVibration;
    return true;
#endif
}

}  // namespace xwr
