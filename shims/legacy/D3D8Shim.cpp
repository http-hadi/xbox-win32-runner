// shims/legacy/D3D8Shim.cpp
// Stub for d3d8. Direct3DCreate8 returns a fake IDirect3D8 pointer.

#include "UwpSdkIncludes.h"
#include <Windows.h>
#include <atomic>

#include "../ShimRegistry.h"

namespace xwr {

static std::atomic<uint32_t> g_d3d8Fake{0x5000};

extern "C" void* __stdcall Shim_Direct3DCreate8(UINT SDKVersion) {
    (void)SDKVersion;
    return (void*)(uintptr_t)g_d3d8Fake.fetch_add(1);
}

extern "C" void __stdcall Shim_DebugSetMute() { }

}  // namespace xwr

REGISTER_SHIM("d3d8", "Direct3DCreate8", (FARPROC)&xwr::Shim_Direct3DCreate8);
REGISTER_SHIM("d3d8", "DebugSetMute", (FARPROC)&xwr::Shim_DebugSetMute);
