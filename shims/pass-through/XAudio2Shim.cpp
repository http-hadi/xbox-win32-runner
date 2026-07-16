// shims/pass-through/XAudio2Shim.cpp
// Pass-through XAudio2 shim. Forwards to the real UWP XAudio2Create.

#include "UwpSdkIncludes.h"


#include "../ShimRegistry.h"

namespace xwr {

extern "C" HRESULT __stdcall Shim_XAudio2Create(IXAudio2** ppXAudio2, UINT Flags, XAUDIO2_PROCESSOR XAudio2Processor) {
    return ::XAudio2Create(ppXAudio2, Flags, XAudio2Processor);
}

}  // namespace xwr

REGISTER_SHIM("xaudio2_9", "XAudio2Create", (FARPROC)&xwr::Shim_XAudio2Create);
REGISTER_SHIM("xaudio2_8", "XAudio2Create", (FARPROC)&xwr::Shim_XAudio2Create);
