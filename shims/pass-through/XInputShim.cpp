// shims/pass-through/XInputShim.cpp
// Pass-through XInput shim. Forwards to the real UWP XInput APIs.

#include "UwpSdkIncludes.h"


#include "../ShimRegistry.h"

namespace xwr {

extern "C" DWORD __stdcall Shim_XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
    return ::XInputGetState(dwUserIndex, pState);
}
extern "C" DWORD __stdcall Shim_XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration) {
    return ::XInputSetState(dwUserIndex, pVibration);
}
extern "C" DWORD __stdcall Shim_XInputGetCapabilities(DWORD dwUserIndex, DWORD dwFlags,
                                                       XINPUT_CAPABILITIES* pCapabilities) {
    return ::XInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);
}
extern "C" DWORD __stdcall Shim_XInputEnable(BOOL bEnable) {
    ::XInputEnable(bEnable);
    return ERROR_SUCCESS;
}
extern "C" DWORD __stdcall Shim_XInputGetBatteryInformation(DWORD dwUserIndex, BYTE devType,
                                                             XINPUT_BATTERY_INFORMATION* pBatteryInfo) {
    return ::XInputGetBatteryInformation(dwUserIndex, devType, pBatteryInfo);
}
extern "C" DWORD __stdcall Shim_XInputGetKeystroke(DWORD dwUserIndex, DWORD dwReserved, PXINPUT_KEYSTROKE pKeystroke) {
    return ::XInputGetKeystroke(dwUserIndex, dwReserved, pKeystroke);
}

}  // namespace xwr

// ===========================================================================
// Registration — under all four XInput DLL names. The most-recent
// implementation lives in xinput1_4; older games use 1_3 / 1_2 / 9_1_0.
// ===========================================================================
#define XWR_REG_XINPUT(dll)                                                    \
    REGISTER_SHIM(dll, "XInputGetState", (FARPROC)&xwr::Shim_XInputGetState); \
    REGISTER_SHIM(dll, "XInputSetState", (FARPROC)&xwr::Shim_XInputSetState); \
    REGISTER_SHIM(dll, "XInputGetCapabilities", (FARPROC)&xwr::Shim_XInputGetCapabilities); \
    REGISTER_SHIM(dll, "XInputEnable", (FARPROC)&xwr::Shim_XInputEnable); \
    REGISTER_SHIM(dll, "XInputGetBatteryInformation", (FARPROC)&xwr::Shim_XInputGetBatteryInformation); \
    REGISTER_SHIM(dll, "XInputGetKeystroke", (FARPROC)&xwr::Shim_XInputGetKeystroke);

XWR_REG_XINPUT("xinput1_4")
XWR_REG_XINPUT("xinput1_3")
XWR_REG_XINPUT("xinput1_2")
XWR_REG_XINPUT("xinput9_1_0")
