// shims/stubs/DiscordShim.cpp
// Stub for discord_game_sdk. All functions return failure (1) or no-op.

#include "UwpSdkIncludes.h"
#include <Windows.h>

#include "../ShimRegistry.h"

namespace xwr {

extern "C" int  __stdcall Shim_Discord_Initialize(const char*, void*, int, const char*) { return 1; }
extern "C" void __stdcall Shim_Discord_Shutdown() { }
extern "C" void __stdcall Shim_Discord_RunCallbacks() { }
extern "C" int  __stdcall Shim_Discord_RegisterW(const wchar_t*, const wchar_t*) { return 0; }
extern "C" int  __stdcall Shim_Discord_Register(const char*, const char*) { return 0; }
extern "C" void* __stdcall Shim_DiscordCreate(int, void*) { return nullptr; }
extern "C" int  __stdcall Shim_Discord_UpdatePresence(const void*) { return 1; }
extern "C" int  __stdcall Shim_Discord_ClearPresence() { return 1; }
extern "C" void __stdcall Shim_Discord_UpdateHandlers(void*) { }

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================
REGISTER_SHIM("discord_game_sdk", "Discord_Initialize", (FARPROC)&xwr::Shim_Discord_Initialize);
REGISTER_SHIM("discord_game_sdk", "Discord_Shutdown", (FARPROC)&xwr::Shim_Discord_Shutdown);
REGISTER_SHIM("discord_game_sdk", "Discord_RunCallbacks", (FARPROC)&xwr::Shim_Discord_RunCallbacks);
REGISTER_SHIM("discord_game_sdk", "Discord_RegisterW", (FARPROC)&xwr::Shim_Discord_RegisterW);
REGISTER_SHIM("discord_game_sdk", "Discord_Register", (FARPROC)&xwr::Shim_Discord_Register);
REGISTER_SHIM("discord_game_sdk", "DiscordCreate", (FARPROC)&xwr::Shim_DiscordCreate);
REGISTER_SHIM("discord_game_sdk", "Discord_UpdatePresence", (FARPROC)&xwr::Shim_Discord_UpdatePresence);
REGISTER_SHIM("discord_game_sdk", "Discord_ClearPresence", (FARPROC)&xwr::Shim_Discord_ClearPresence);
REGISTER_SHIM("discord_game_sdk", "Discord_UpdateHandlers", (FARPROC)&xwr::Shim_Discord_UpdateHandlers);
