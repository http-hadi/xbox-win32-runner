// shims/stubs/SteamShim.cpp
// Stub for steam_api64 / steam_api. Every function returns false / 0 / nullptr
// so the game degrades gracefully (no Steam running, achievements disabled,
// etc.).

#include "UwpSdkIncludes.h"
#include <Windows.h>

#include "../ShimRegistry.h"

namespace xwr {

// SteamAPI_*
extern "C" int  __stdcall Shim_SteamAPI_Init() { return 0; }              // bool
extern "C" void __stdcall Shim_SteamAPI_Shutdown() { }
extern "C" void __stdcall Shim_SteamAPI_RunCallbacks() { }
extern "C" int  __stdcall Shim_SteamAPI_RestartAppIfNecessary(uint32_t) { return 0; }  // bool false
extern "C" void __stdcall Shim_SteamAPI_SetMiniDumpComment(const char*) { }
extern "C" void __stdcall Shim_SteamAPI_WriteMiniDump(uint32_t, void*, uint32_t) { }
extern "C" void __stdcall Shim_SteamAPI_RegisterCallback(void*, int) { }
extern "C" void __stdcall Shim_SteamAPI_UnregisterCallback(void*) { }
extern "C" void __stdcall Shim_SteamAPI_RegisterCallResult(void*, uint64_t) { }
extern "C" void __stdcall Shim_SteamAPI_UnregisterCallResult(void*, uint64_t) { }
extern "C" void __stdcall Shim_SteamAPI_ReleaseCurrentThreadMemory() { }
extern "C" int  __stdcall Shim_SteamAPI_IsSteamRunning() { return 0; }
extern "C" const char* __stdcall Shim_SteamAPI_GetSteamInstallPath() { return ""; }
extern "C" void* __stdcall Shim_SteamInternal_CreateInterface() { return nullptr; }
extern "C" int  __stdcall Shim_SteamInternal_FindOrCreateUserInterface(uint32_t, const char*) { return 0; }
extern "C" int  __stdcall Shim_SteamInternal_FindOrCreateGameServerInterface(uint32_t, const char*) { return 0; }
extern "C" void* __stdcall Shim_SteamApps() { return nullptr; }
extern "C" void* __stdcall Shim_SteamUser() { return nullptr; }
extern "C" void* __stdcall Shim_SteamFriends() { return nullptr; }
extern "C" void* __stdcall Shim_SteamUserStats() { return nullptr; }
extern "C" void* __stdcall Shim_SteamMatchmaking() { return nullptr; }
extern "C" void* __stdcall Shim_SteamMatchmakingServers() { return nullptr; }
extern "C" void* __stdcall Shim_SteamGameServer() { return nullptr; }
extern "C" void* __stdcall Shim_SteamGameServerStats() { return nullptr; }
extern "C" void* __stdcall Shim_SteamNetworking() { return nullptr; }
extern "C" void* __stdcall Shim_SteamRemoteStorage() { return nullptr; }
extern "C" void* __stdcall Shim_SteamScreenshots() { return nullptr; }
extern "C" void* __stdcall Shim_SteamHTTP() { return nullptr; }
extern "C" void* __stdcall Shim_SteamController() { return nullptr; }
extern "C" void* __stdcall Shim_SteamUGC() { return nullptr; }
extern "C" void* __stdcall Shim_SteamAppList() { return nullptr; }
extern "C" void* __stdcall Shim_SteamMusic() { return nullptr; }
extern "C" void* __stdcall Shim_SteamMusicRemote() { return nullptr; }
extern "C" void* __stdcall Shim_SteamHTMLSurface() { return nullptr; }
extern "C" void* __stdcall Shim_SteamInventory() { return nullptr; }
extern "C" void* __stdcall Shim_SteamVideo() { return nullptr; }
extern "C" void* __stdcall Shim_SteamParentalSettings() { return nullptr; }
extern "C" void* __stdcall Shim_SteamUtils() { return nullptr; }

}  // namespace xwr

// ===========================================================================
// Registration — also covers "steam_api" (32-bit alias).
// ===========================================================================
#define XWR_REG_STEAM(dll)                                                    \
    REGISTER_SHIM(dll, "SteamAPI_Init", (FARPROC)&xwr::Shim_SteamAPI_Init); \
    REGISTER_SHIM(dll, "SteamAPI_Shutdown", (FARPROC)&xwr::Shim_SteamAPI_Shutdown); \
    REGISTER_SHIM(dll, "SteamAPI_RunCallbacks", (FARPROC)&xwr::Shim_SteamAPI_RunCallbacks); \
    REGISTER_SHIM(dll, "SteamAPI_RestartAppIfNecessary", (FARPROC)&xwr::Shim_SteamAPI_RestartAppIfNecessary); \
    REGISTER_SHIM(dll, "SteamAPI_SetMiniDumpComment", (FARPROC)&xwr::Shim_SteamAPI_SetMiniDumpComment); \
    REGISTER_SHIM(dll, "SteamAPI_WriteMiniDump", (FARPROC)&xwr::Shim_SteamAPI_WriteMiniDump); \
    REGISTER_SHIM(dll, "SteamAPI_RegisterCallback", (FARPROC)&xwr::Shim_SteamAPI_RegisterCallback); \
    REGISTER_SHIM(dll, "SteamAPI_UnregisterCallback", (FARPROC)&xwr::Shim_SteamAPI_UnregisterCallback); \
    REGISTER_SHIM(dll, "SteamAPI_RegisterCallResult", (FARPROC)&xwr::Shim_SteamAPI_RegisterCallResult); \
    REGISTER_SHIM(dll, "SteamAPI_UnregisterCallResult", (FARPROC)&xwr::Shim_SteamAPI_UnregisterCallResult); \
    REGISTER_SHIM(dll, "SteamAPI_ReleaseCurrentThreadMemory", (FARPROC)&xwr::Shim_SteamAPI_ReleaseCurrentThreadMemory); \
    REGISTER_SHIM(dll, "SteamAPI_IsSteamRunning", (FARPROC)&xwr::Shim_SteamAPI_IsSteamRunning); \
    REGISTER_SHIM(dll, "SteamAPI_GetSteamInstallPath", (FARPROC)&xwr::Shim_SteamAPI_GetSteamInstallPath); \
    REGISTER_SHIM(dll, "SteamInternal_CreateInterface", (FARPROC)&xwr::Shim_SteamInternal_CreateInterface); \
    REGISTER_SHIM(dll, "SteamInternal_FindOrCreateUserInterface", (FARPROC)&xwr::Shim_SteamInternal_FindOrCreateUserInterface); \
    REGISTER_SHIM(dll, "SteamInternal_FindOrCreateGameServerInterface", (FARPROC)&xwr::Shim_SteamInternal_FindOrCreateGameServerInterface); \
    REGISTER_SHIM(dll, "SteamApps", (FARPROC)&xwr::Shim_SteamApps); \
    REGISTER_SHIM(dll, "SteamUser", (FARPROC)&xwr::Shim_SteamUser); \
    REGISTER_SHIM(dll, "SteamFriends", (FARPROC)&xwr::Shim_SteamFriends); \
    REGISTER_SHIM(dll, "SteamUserStats", (FARPROC)&xwr::Shim_SteamUserStats); \
    REGISTER_SHIM(dll, "SteamMatchmaking", (FARPROC)&xwr::Shim_SteamMatchmaking); \
    REGISTER_SHIM(dll, "SteamMatchmakingServers", (FARPROC)&xwr::Shim_SteamMatchmakingServers); \
    REGISTER_SHIM(dll, "SteamGameServer", (FARPROC)&xwr::Shim_SteamGameServer); \
    REGISTER_SHIM(dll, "SteamGameServerStats", (FARPROC)&xwr::Shim_SteamGameServerStats); \
    REGISTER_SHIM(dll, "SteamNetworking", (FARPROC)&xwr::Shim_SteamNetworking); \
    REGISTER_SHIM(dll, "SteamRemoteStorage", (FARPROC)&xwr::Shim_SteamRemoteStorage); \
    REGISTER_SHIM(dll, "SteamScreenshots", (FARPROC)&xwr::Shim_SteamScreenshots); \
    REGISTER_SHIM(dll, "SteamHTTP", (FARPROC)&xwr::Shim_SteamHTTP); \
    REGISTER_SHIM(dll, "SteamController", (FARPROC)&xwr::Shim_SteamController); \
    REGISTER_SHIM(dll, "SteamUGC", (FARPROC)&xwr::Shim_SteamUGC); \
    REGISTER_SHIM(dll, "SteamAppList", (FARPROC)&xwr::Shim_SteamAppList); \
    REGISTER_SHIM(dll, "SteamMusic", (FARPROC)&xwr::Shim_SteamMusic); \
    REGISTER_SHIM(dll, "SteamMusicRemote", (FARPROC)&xwr::Shim_SteamMusicRemote); \
    REGISTER_SHIM(dll, "SteamHTMLSurface", (FARPROC)&xwr::Shim_SteamHTMLSurface); \
    REGISTER_SHIM(dll, "SteamInventory", (FARPROC)&xwr::Shim_SteamInventory); \
    REGISTER_SHIM(dll, "SteamVideo", (FARPROC)&xwr::Shim_SteamVideo); \
    REGISTER_SHIM(dll, "SteamParentalSettings", (FARPROC)&xwr::Shim_SteamParentalSettings); \
    REGISTER_SHIM(dll, "SteamUtils", (FARPROC)&xwr::Shim_SteamUtils);

XWR_REG_STEAM("steam_api64")
XWR_REG_STEAM("steam_api")
