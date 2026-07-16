// shims/stubs/GameDllStubs.cpp
//
// Stubs for game-shipped DLLs that we can't load from the game folder.
// These DLLs either call external services (Discord, Steam backend,
// NVIDIA Reflex / Aftermath) or are optional features (Python scripting,
// Intel denoiser) — every export returns failure / false / 0 / nullptr so
// the game degrades gracefully.
//
// DLLs covered (REGISTER_SHIM entries):
//   discord_partner_sdk.dll        477  generic stub (returns 0)
//   python311.dll                  133  generic stub (returns 0)
//   GFSDK_Aftermath_Lib.x64.dll     17  generic stub (returns 0)
//   NvLowLatencyVk.dll              10  generic stub (returns 0)
//   OpenImageDenoise.dll             9  generic stub (returns 0)
//   discord-rpc.dll                  6  initialize returns 0 (failure)
//   steam_api64.dll                  6  additional funcs beyond SteamShim.cpp
//   IPHLPAPI.DLL                     8  pass-through to real UWP iphlpapi APIs
//
// Total REGISTER_SHIM entries: 477 + 133 + 17 + 10 + 9 + 6 + 6 + 8 = 666
//
// Generic-stub rationale:
//   * discord_partner_sdk.dll exposes the Discord Partner SDK surface
//     (477 C-style wrappers around Discord activity / lobby / voice /
//     relationship / store / overlay / network / achievement APIs).
//     Returning 0 from every entry makes the game's Discord integration
//     fail-fast at startup; the game continues to run without it.
//   * python311.dll is the embedded CPython 3.11 interpreter. UE5 games
//     pull it in for editor-side Python scripting but it isn't required
//     at runtime — every Py_* / _Py_* function returns 0 / nullptr.
//   * GFSDK_Aftermath_Lib.x64.dll is NVIDIA's GPU crash-dump SDK. Not
//     needed; every GFSDK_Aftermath_* function returns 0 (failure).
//   * NvLowLatencyVk.dll is NVIDIA Reflex for Vulkan. We're on D3D11
//     anyway; every NVLL_VK_* function returns 0.
//   * OpenImageDenoise.dll is Intel's Open Image Denoiser. Visual
//     feature; game runs without it. Every oidn* function returns 0.
//
// Pass-through rationale:
//   * IPHLPAPI.DLL exports IP-helper functions (GetAdaptersAddresses,
//     GetAdaptersInfo, IcmpCreateFile, IcmpSendEcho, etc.) that ARE
//     available to UWP apps via iphlpapi.lib. We forward the calls
//     straight through on a real Windows build. Under XWR_SYNTAX_CHECK
//     we can't link iphlpapi.lib, so the stub path returns failure
//     values that the caller will treat as "no network adapters / no
//     ICMP reply".
//
// Added by Task ID GAP-GAME-DLL-STUBS.

#include "UwpSdkIncludes.h"


#include "../ShimRegistry.h"

#ifndef S_OK
#define S_OK ((HRESULT)0L)
#endif

// IP_ADDR is not a standard Windows SDK type — the real <iphlpapi.h> uses
// IPAddr (typedef IPAddr = ULONG). On toolchains where IP_ADDR isn't
// defined (and to keep the IcmpSendEcho shim signature consistent with
// the real signature that takes an IPAddr/DWORD destination), alias it
// locally. Under XWR_SYNTAX_CHECK our stub Windows.h already typedefs
// IP_ADDR; this guard makes both paths safe.
#ifndef IP_ADDR_DEFINED
typedef ULONG IP_ADDR, *PIP_ADDR;
#define IP_ADDR_DEFINED
#endif

namespace xwr {

// ===========================================================================
// 1) discord_partner_sdk.dll — 477 C-style functions.
//    One generic stub returning 0 (failure) is registered under every
//    exported function name. Game's Discord integration silently no-ops.
// ===========================================================================
extern "C" int __stdcall Shim_DiscordPartner_Stub() { return 0; }

// ===========================================================================
// 2) python311.dll — 133 Python C API functions (Py_*, _Py*).
//    One generic stub returning 0 is registered under every name. Game's
//    Python scripting won't initialize but UE5 games run without it.
// ===========================================================================
extern "C" int __stdcall Shim_Python311_Stub() { return 0; }

// ===========================================================================
// 3) GFSDK_Aftermath_Lib.x64.dll — 17 NVIDIA Aftermath crash-dump SDK
//    functions. One generic stub returning 0 (failure).
// ===========================================================================
extern "C" int __stdcall Shim_Aftermath_Stub() { return 0; }

// ===========================================================================
// 4) NvLowLatencyVk.dll — 10 NVIDIA Reflex for Vulkan functions.
//    One generic stub returning 0. We're on D3D11 anyway.
// ===========================================================================
extern "C" int __stdcall Shim_NvLowLatencyVk_Stub() { return 0; }

// ===========================================================================
// 5) OpenImageDenoise.dll — 9 Intel denoiser functions.
//    One generic stub returning 0 (failure). Visual feature; game runs
//    without it.
// ===========================================================================
extern "C" int __stdcall Shim_OIDN_Stub() { return 0; }

// ===========================================================================
// 6) discord-rpc.dll — 6 functions. discord_initialize returns 0 (failure)
//    so the game knows Discord isn't available. The rest are no-ops.
// ===========================================================================
extern "C" int  __stdcall Shim_DiscordRpc_Initialize(const char*, void*, int, const char*) { return 0; }
extern "C" int  __stdcall Shim_DiscordRpc_ClearPresence() { return 0; }
extern "C" int  __stdcall Shim_DiscordRpc_Respond(const char*, int) { return 0; }
extern "C" void __stdcall Shim_DiscordRpc_RunCallbacks() { }
extern "C" void __stdcall Shim_DiscordRpc_Shutdown() { }
extern "C" int  __stdcall Shim_DiscordRpc_UpdatePresence(const void*) { return 0; }

// ===========================================================================
// 7) steam_api64.dll — 6 additional functions beyond what SteamShim.cpp
//    already covers. These are the SteamGameServer_* and SteamInternal_*
//    entry points the gap scanner flagged.
// ===========================================================================
extern "C" uint32_t __stdcall Shim_SteamAPI_GetHSteamUser() { return 0; }
extern "C" uint32_t __stdcall Shim_SteamGameServer_GetHSteamUser() { return 0; }
extern "C" void     __stdcall Shim_SteamGameServer_RunCallbacks() { }
extern "C" void     __stdcall Shim_SteamGameServer_Shutdown() { }
extern "C" void*    __stdcall Shim_SteamInternal_ContextInit(void*) { return nullptr; }
extern "C" int      __stdcall Shim_SteamInternal_GameServer_Init(uint32_t, uint16_t, uint16_t, int, int, const char*) { return 0; }

// ===========================================================================
// 8) IPHLPAPI.DLL — pass-through to the real UWP iphlpapi API. Under
//    XWR_SYNTAX_CHECK we can't link iphlpapi.lib, so the stub path
//    returns failure values (0 / INVALID_HANDLE_VALUE / FALSE).
// ===========================================================================
extern "C" ULONG __stdcall Shim_GetCurrentThreadCompartmentId() {
#ifndef XWR_SYNTAX_CHECK
    return ::GetCurrentThreadCompartmentId();
#else
    return 0;
#endif
}
extern "C" DWORD __stdcall Shim_SetCurrentThreadCompartmentId(ULONG CompartmentId) {
#ifndef XWR_SYNTAX_CHECK
    return ::SetCurrentThreadCompartmentId(CompartmentId);
#else
    (void)CompartmentId;
    return (DWORD)1L;  // ERROR_INVALID_FUNCTION
#endif
}
extern "C" ULONG __stdcall Shim_GetAdaptersAddresses(ULONG Family, ULONG Flags, PVOID Reserved,
                                                      PIP_ADAPTER_ADDRESSES AdapterAddresses,
                                                      PULONG SizePointer) {
#ifndef XWR_SYNTAX_CHECK
    return ::GetAdaptersAddresses(Family, Flags, Reserved, AdapterAddresses, SizePointer);
#else
    (void)Family; (void)Flags; (void)Reserved; (void)AdapterAddresses;
    if (SizePointer) *SizePointer = 0;
    return (ULONG)232L;  // ERROR_NO_DATA
#endif
}
extern "C" DWORD __stdcall Shim_GetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen) {
#ifndef XWR_SYNTAX_CHECK
    return ::GetAdaptersInfo(pAdapterInfo, pOutBufLen);
#else
    (void)pAdapterInfo;
    if (pOutBufLen) *pOutBufLen = 0;
    return (DWORD)111L;  // ERROR_BUFFER_OVERFLOW
#endif
}
extern "C" HANDLE __stdcall Shim_IcmpCreateFile() {
#ifndef XWR_SYNTAX_CHECK
    return ::IcmpCreateFile();
#else
    return INVALID_HANDLE_VALUE;
#endif
}
extern "C" BOOL __stdcall Shim_IcmpCloseHandle(HANDLE IcmpHandle) {
#ifndef XWR_SYNTAX_CHECK
    return ::IcmpCloseHandle(IcmpHandle);
#else
    (void)IcmpHandle;
    return FALSE;
#endif
}
extern "C" DWORD __stdcall Shim_IcmpSendEcho(HANDLE IcmpHandle, IP_ADDR DestinationAddress,
                                              LPVOID RequestData, WORD RequestSize,
                                              PIP_OPTION_INFORMATION RequestOptions,
                                              LPVOID ReplyBuffer, DWORD ReplySize, DWORD Timeout) {
#ifndef XWR_SYNTAX_CHECK
    return ::IcmpSendEcho(IcmpHandle, DestinationAddress, RequestData, RequestSize,
                          RequestOptions, ReplyBuffer, ReplySize, Timeout);
#else
    (void)IcmpHandle; (void)DestinationAddress; (void)RequestData; (void)RequestSize;
    (void)RequestOptions; (void)ReplyBuffer; (void)ReplySize; (void)Timeout;
    return 0;  // no replies
#endif
}
extern "C" NET_IFINDEX __stdcall Shim_if_nametoindex(PCSTR InterfaceName) {
#ifndef XWR_SYNTAX_CHECK
    return ::if_nametoindex(InterfaceName);
#else
    (void)InterfaceName;
    return (NET_IFINDEX)0;  // NET_IFINDEX_UNSPECIFIED
#endif
}

}  // namespace xwr

// ===========================================================================
// Registration
// ===========================================================================

// ---------------------------------------------------------------------------
// 1) discord_partner_sdk.dll (477 functions)
// ---------------------------------------------------------------------------
REGISTER_SHIM("discord_partner_sdk", "discord_activity_addbutton", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_applicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_assets", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_details", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_detailsurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_equals", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_getbuttons", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_name", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_parentapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_party", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_secrets", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setassets", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setdetails", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setdetailsurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setname", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setparentapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setparty", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setsecrets", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setstate", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setstateurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setstatusdisplaytype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_setsupportedplatforms", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_settimestamps", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_settype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_state", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_stateurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_statusdisplaytype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_supportedplatforms", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_timestamps", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activity_type", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_invitecoverimage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_largeimage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_largetext", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_largeurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_setinvitecoverimage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_setlargeimage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_setlargetext", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_setlargeurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_setsmallimage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_setsmalltext", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_setsmallurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_smallimage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_smalltext", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityassets_smallurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitybutton_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitybutton_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitybutton_label", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitybutton_setlabel", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitybutton_seturl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitybutton_url", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_applicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_channelid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_isvalid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_messageid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_parentapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_partyid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_senderid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_sessionid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_setapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_setchannelid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_setisvalid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_setmessageid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_setparentapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_setpartyid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_setsenderid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_setsessionid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_settype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityinvite_type", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_currentsize", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_maxsize", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_privacy", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_setcurrentsize", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_setid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_setmaxsize", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activityparty_setprivacy", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitysecrets_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitysecrets_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitysecrets_join", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitysecrets_setjoin", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitytimestamps_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitytimestamps_end", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitytimestamps_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitytimestamps_setend", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitytimestamps_setstart", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_activitytimestamps_start", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_count", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_equals", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_setcount", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_settitle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_settype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_title", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_type", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_additionalcontent_typetostring", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_alloc", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_audiodevice_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_audiodevice_equals", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_audiodevice_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_audiodevice_isdefault", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_audiodevice_name", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_audiodevice_setid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_audiodevice_setisdefault", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_audiodevice_setname", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_clientid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_codechallenge", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_customschemeparam", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_integrationtype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_nonce", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_scopes", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_setclientid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_setcodechallenge", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_setcustomschemeparam", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_setintegrationtype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_setnonce", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_setscopes", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_setstate", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationargs_state", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodechallenge_challenge", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodechallenge_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodechallenge_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodechallenge_method", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodechallenge_setchallenge", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodechallenge_setmethod", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodeverifier_challenge", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodeverifier_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodeverifier_setchallenge", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodeverifier_setverifier", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_authorizationcodeverifier_verifier", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_errortostring", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getaudiomode", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getchannelid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getguildid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getlocalmute", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getparticipants", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getparticipantvolume", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getpttactive", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getpttreleasedelay", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getselfdeaf", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getselfmute", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getstatus", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getvadthreshold", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_getvoicestatehandle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setaudiomode", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setlocalmute", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setonvoicestatechangedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setparticipantchangedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setparticipantvolume", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setpttactive", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setpttreleasedelay", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setselfdeaf", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setselfmute", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setspeakingstatuschangedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setstatuschangedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_setvadthreshold", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_call_statustostring", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_callinfohandle_channelid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_callinfohandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_callinfohandle_getparticipants", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_callinfohandle_getvoicestatehandle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_callinfohandle_guildid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_channelhandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_channelhandle_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_channelhandle_name", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_channelhandle_recipients", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_channelhandle_type", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_abortauthorize", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_abortgettokenfromdevice", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_acceptactivityinvite", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_acceptdiscordfriendrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_acceptgamefriendrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_addlogcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_addvoicelogcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_authorize", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_blockuser", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_canceldiscordfriendrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_cancelgamefriendrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_canopenmessageindiscord", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_clearrichpresence", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_closeauthorizedevicescreen", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_connect", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_createauthorizationcodeverifier", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_createorjoinlobby", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_createorjoinlobbywithmetadata", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_deleteusermessage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_disconnect", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_editusermessage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_endcall", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_endcalls", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_errortostring", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_exchangechildtoken", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_fetchcurrentuser", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getcall", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getcalls", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getchannelhandle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getcurrentinputdevice", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getcurrentoutputdevice", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getcurrentuser", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getcurrentuserv2", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getdefaultaudiodeviceid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getdefaultcommunicationscopes", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getdefaultpresencescopes", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getdiscordclientconnecteduser", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getguildchannels", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getinputdevices", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getinputvolume", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getlobbyhandle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getlobbyids", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getlobbymessageswithlimit", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getmessagehandle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getoutputdevices", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getoutputvolume", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getprovisionaltoken", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getrelationshiphandle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getrelationships", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getrelationshipsbygroup", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getselfdeafall", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getselfmuteall", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getstatus", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_gettoken", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_gettokenfromdevice", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_gettokenfromdeviceprovisionalmerge", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_gettokenfromprovisionalmerge", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getuser", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getuserguilds", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getusermessagesummaries", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getusermessageswithlimit", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getversionhash", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getversionmajor", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getversionminor", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_getversionpatch", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_initwithbases", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_initwithoptions", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_isauthenticated", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_joinlinkedlobbyguild", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_leavelobby", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_linkchanneltolobby", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_openauthorizedevicescreen", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_openconnectedgamessettingsindiscord", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_openmessageindiscord", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_provisionalusermergecompleted", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_refreshtoken", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_registerauthorizerequestcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_registerlaunchcommand", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_registerlaunchsteamapplication", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_rejectdiscordfriendrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_rejectgamefriendrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_removeauthorizerequestcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_removediscordandgamefriend", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_removegamefriend", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_revoketoken", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_searchfriendsbyusername", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendactivityinvite", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendactivityjoinrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendactivityjoinrequestreply", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_senddiscordfriendrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_senddiscordfriendrequestbyid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendgamefriendrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendgamefriendrequestbyid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendlobbymessage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendlobbymessagewithmetadata", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendusermessage", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sendusermessagewithmetadata", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setactivityinvitecreatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setactivityinviteupdatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setactivityjoincallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setactivityjoinwithapplicationcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setaecdump", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setauthorizedevicescreenclosedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setautomaticgaincontrol", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setdevicechangecallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setechocancellation", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setenginemanagedaudiosession", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setgamewindowpid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_sethttprequesttimeout", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setinputdevice", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setinputvolume", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setlobbycreatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setlobbydeletedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setlobbymemberaddedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setlobbymemberremovedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setlobbymemberupdatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setlobbyupdatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setlogdir", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setmessagecreatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setmessagedeletedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setmessageupdatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setnoaudioinputcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setnoaudioinputthreshold", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setnoisesuppression", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setonlinestatus", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setopushardwarecoding", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setoutputdevice", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setoutputvolume", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setrelationshipcreatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setrelationshipdeletedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setrelationshipgroupsupdatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setselfdeafall", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setselfmuteall", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setshowingchat", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setspeakermode", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setstatuschangedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setthreadpriority", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_settokenexpirationcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setuserupdatedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setvoicelogdir", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_setvoiceparticipantchangedcallback", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_showaudioroutepicker", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_startcall", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_startcallwithaudiocallbacks", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_statustostring", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_threadtostring", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_unblockuser", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_unlinkchannelfromlobby", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_unmergeintoprovisionalaccount", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_updateprovisionalaccountdisplayname", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_updaterichpresence", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_client_updatetoken", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_apibase", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_cpuaffinitymask", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_experimentalandroidpreventcommsforbluetooth", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_experimentalaudiosystem", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_setapibase", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_setcpuaffinitymask", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_setexperimentalandroidpreventcommsforbluetooth", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_setexperimentalaudiosystem", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_setwebbase", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientcreateoptions_webbase", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_error", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_errorcode", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_responsebody", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_retryable", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_retryafter", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_seterror", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_seterrorcode", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_setresponsebody", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_setretryable", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_setretryafter", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_setstatus", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_setsuccessful", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_settype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_status", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_successful", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_tostring", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_clientresult_type", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_deviceauthorizationargs_clientid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_deviceauthorizationargs_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_deviceauthorizationargs_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_deviceauthorizationargs_scopes", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_deviceauthorizationargs_setclientid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_deviceauthorizationargs_setscopes", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_free", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_freeproperties", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_islinkable", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_isviewableandwriteablebyallmembers", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_linkedlobby", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_name", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_parentid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_position", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_setid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_setislinkable", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_setisviewableandwriteablebyallmembers", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_setlinkedlobby", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_setname", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_setparentid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_setposition", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_settype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildchannel_type", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildminimal_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildminimal_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildminimal_name", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildminimal_setid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_guildminimal_setname", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedchannel_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedchannel_guildid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedchannel_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedchannel_name", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedchannel_setguildid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedchannel_setid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedchannel_setname", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedlobby_applicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedlobby_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedlobby_init", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedlobby_lobbyid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedlobby_setapplicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_linkedlobby_setlobbyid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbyhandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbyhandle_getcallinfohandle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbyhandle_getlobbymemberhandle", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbyhandle_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbyhandle_linkedchannel", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbyhandle_lobbymemberids", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbyhandle_lobbymembers", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbyhandle_metadata", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbymemberhandle_canlinklobby", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbymemberhandle_connected", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbymemberhandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbymemberhandle_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbymemberhandle_metadata", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_lobbymemberhandle_user", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_additionalcontent", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_applicationid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_author", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_authorid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_channel", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_channelid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_content", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_disclosuretype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_editedtimestamp", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_lobby", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_metadata", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_rawcontent", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_recipient", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_recipientid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_sentfromgame", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_messagehandle_senttimestamp", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_relationshiphandle_discordrelationshiptype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_relationshiphandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_relationshiphandle_gamerelationshiptype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_relationshiphandle_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_relationshiphandle_isspamrequest", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_relationshiphandle_user", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userapplicationprofilehandle_avatarhash", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userapplicationprofilehandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userapplicationprofilehandle_metadata", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userapplicationprofilehandle_providerid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userapplicationprofilehandle_providerissueduserid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userapplicationprofilehandle_providertype", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userapplicationprofilehandle_username", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_avatar", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_avatartypetostring", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_avatarurl", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_displayname", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_gameactivity", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_globalname", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_id", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_isprovisional", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_relationship", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_status", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_userapplicationprofiles", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_userhandle_username", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_usermessagesummary_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_usermessagesummary_lastmessageid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_usermessagesummary_userid", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_vadthresholdsettings_automatic", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_vadthresholdsettings_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_vadthresholdsettings_setautomatic", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_vadthresholdsettings_setvadthreshold", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_vadthresholdsettings_vadthreshold", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_voicestatehandle_drop", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_voicestatehandle_selfdeaf", (FARPROC)&xwr::Shim_DiscordPartner_Stub);
REGISTER_SHIM("discord_partner_sdk", "discord_voicestatehandle_selfmute", (FARPROC)&xwr::Shim_DiscordPartner_Stub);

// ---------------------------------------------------------------------------
// 2) python311.dll (133 functions)
// ---------------------------------------------------------------------------
REGISTER_SHIM("python311", "_py_dealloc", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "_py_nonestruct", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "_py_notimplementedstruct", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "_pyobject_new", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "_pytype_lookup", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "py_buildvalue", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyarg_parsetupleandkeywords", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pybaseobject_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pybool_fromlong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pybool_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pybytes_asstring", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pybytes_size", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pycallable_check", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pycfunction_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pycmethod_new", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pycomplex_imagasdouble", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pycomplex_realasdouble", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pycomplex_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_clear", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_copy", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_getitem", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_getitemstring", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_items", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_keys", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_new", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_size", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_update", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pydict_values", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_clear", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_exceptionmatches", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_format", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_newexception", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_nomemory", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_occurred", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_setobject", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_setstring", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyerr_warnex", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyeval_getglobals", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyexc_attributeerror", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyexc_indexerror", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyexc_overflowerror", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyexc_referenceerror", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyexc_runtimeerror", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyexc_stopiteration", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyexc_typeerror", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyexc_valueerror", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyfloat_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyimport_importmodule", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyiter_next", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylist_append", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylist_insert", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylist_new", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylist_reverse", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylist_sort", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylist_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_aslong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_aslonglong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_asssize_t", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_asunsignedlong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_asunsignedlonglong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_fromlong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_fromlonglong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_fromunsignedlong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_fromunsignedlonglong", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pylong_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pymem_free", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pymem_malloc", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pymethod_new", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pymethod_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pymodule_create2", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pymodule_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_add", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_and", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_floordivide", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplaceadd", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplaceand", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplacefloordivide", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplacelshift", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplacemultiply", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplaceor", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplaceremainder", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplacershift", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplacesubtract", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_inplacexor", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_lshift", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_multiply", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_or", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_remainder", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_rshift", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_subtract", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pynumber_xor", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_call", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_callfunction", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_callmethod", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_clearweakrefs", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_delitem", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_getattr", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_getattrstring", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_getitem", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_init", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_isinstance", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_istrue", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_richcompare", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_setattr", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_setattrstring", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_setitem", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyobject_size", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyproperty_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyrun_fileexflags", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyrun_stringflags", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyslice_new", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pystaticmethod_new", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pystaticmethod_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pytuple_getitem", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pytuple_new", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pytuple_size", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pytuple_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pytype_genericalloc", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pytype_issubtype", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pytype_ready", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pytype_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_asutf8", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_asutf8string", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_aswidecharstring", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_fromencodedobject", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_fromformat", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_fromstring", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_fromstringandsize", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_fsconverter", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_internfromstring", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyunicode_type", (FARPROC)&xwr::Shim_Python311_Stub);
REGISTER_SHIM("python311", "pyweakref_newref", (FARPROC)&xwr::Shim_Python311_Stub);

// ---------------------------------------------------------------------------
// 3) GFSDK_Aftermath_Lib.x64.dll (17 functions)
// ---------------------------------------------------------------------------
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_dx11_createcontexthandle", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_dx11_initialize", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_dx12_createcontexthandle", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_dx12_initialize", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_dx12_registerresource", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_dx12_unregisterresource", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_enablegpucrashdumps", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_getcrashdumpstatus", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_getdata", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_getdevicestatus", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_getpagefaultinformation", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_gpucrashdump_createdecoder", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_gpucrashdump_generatejson", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_gpucrashdump_getbaseinfo", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_gpucrashdump_getjson", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_releasecontexthandle", (FARPROC)&xwr::Shim_Aftermath_Stub);
REGISTER_SHIM("GFSDK_Aftermath_Lib", "gfsdk_aftermath_seteventmarker", (FARPROC)&xwr::Shim_Aftermath_Stub);

// ---------------------------------------------------------------------------
// 4) NvLowLatencyVk.dll (10 functions)
// ---------------------------------------------------------------------------
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_destroylowlatencydevice", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_getlatency", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_getsleepstatus", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_initialize", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_initlowlatencydevice", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_notifyoutofbandqueue", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_setlatencymarker", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_setsleepmode", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_sleep", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);
REGISTER_SHIM("NvLowLatencyVk", "nvll_vk_unload", (FARPROC)&xwr::Shim_NvLowLatencyVk_Stub);

// ---------------------------------------------------------------------------
// 5) OpenImageDenoise.dll (9 functions)
// ---------------------------------------------------------------------------
REGISTER_SHIM("OpenImageDenoise", "oidncommitdevice", (FARPROC)&xwr::Shim_OIDN_Stub);
REGISTER_SHIM("OpenImageDenoise", "oidncommitfilter", (FARPROC)&xwr::Shim_OIDN_Stub);
REGISTER_SHIM("OpenImageDenoise", "oidnexecutefilter", (FARPROC)&xwr::Shim_OIDN_Stub);
REGISTER_SHIM("OpenImageDenoise", "oidnnewdevice", (FARPROC)&xwr::Shim_OIDN_Stub);
REGISTER_SHIM("OpenImageDenoise", "oidnnewfilter", (FARPROC)&xwr::Shim_OIDN_Stub);
REGISTER_SHIM("OpenImageDenoise", "oidnreleasedevice", (FARPROC)&xwr::Shim_OIDN_Stub);
REGISTER_SHIM("OpenImageDenoise", "oidnreleasefilter", (FARPROC)&xwr::Shim_OIDN_Stub);
REGISTER_SHIM("OpenImageDenoise", "oidnsetfilter1b", (FARPROC)&xwr::Shim_OIDN_Stub);
REGISTER_SHIM("OpenImageDenoise", "oidnsetsharedfilterimage", (FARPROC)&xwr::Shim_OIDN_Stub);

// ---------------------------------------------------------------------------
// 6) discord-rpc.dll (6 functions) — explicit stubs.
// ---------------------------------------------------------------------------
REGISTER_SHIM("discord-rpc", "discord_clearpresence",   (FARPROC)&xwr::Shim_DiscordRpc_ClearPresence);
REGISTER_SHIM("discord-rpc", "discord_initialize",      (FARPROC)&xwr::Shim_DiscordRpc_Initialize);
REGISTER_SHIM("discord-rpc", "discord_respond",         (FARPROC)&xwr::Shim_DiscordRpc_Respond);
REGISTER_SHIM("discord-rpc", "discord_runcallbacks",    (FARPROC)&xwr::Shim_DiscordRpc_RunCallbacks);
REGISTER_SHIM("discord-rpc", "discord_shutdown",        (FARPROC)&xwr::Shim_DiscordRpc_Shutdown);
REGISTER_SHIM("discord-rpc", "discord_updatepresence",  (FARPROC)&xwr::Shim_DiscordRpc_UpdatePresence);

// ---------------------------------------------------------------------------
// 7) steam_api64.dll (6 additional functions beyond SteamShim.cpp).
// ---------------------------------------------------------------------------
REGISTER_SHIM("steam_api64", "SteamAPI_GetHSteamUser",            (FARPROC)&xwr::Shim_SteamAPI_GetHSteamUser);
REGISTER_SHIM("steam_api64", "SteamGameServer_GetHSteamUser",     (FARPROC)&xwr::Shim_SteamGameServer_GetHSteamUser);
REGISTER_SHIM("steam_api64", "SteamGameServer_RunCallbacks",      (FARPROC)&xwr::Shim_SteamGameServer_RunCallbacks);
REGISTER_SHIM("steam_api64", "SteamGameServer_Shutdown",          (FARPROC)&xwr::Shim_SteamGameServer_Shutdown);
REGISTER_SHIM("steam_api64", "SteamInternal_ContextInit",         (FARPROC)&xwr::Shim_SteamInternal_ContextInit);
REGISTER_SHIM("steam_api64", "SteamInternal_GameServer_Init",     (FARPROC)&xwr::Shim_SteamInternal_GameServer_Init);

// ---------------------------------------------------------------------------
// 8) IPHLPAPI.DLL (8 functions) — pass-through to real UWP iphlpapi APIs.
// ---------------------------------------------------------------------------
REGISTER_SHIM("iphlpapi", "GetAdaptersAddresses",           (FARPROC)&xwr::Shim_GetAdaptersAddresses);
REGISTER_SHIM("iphlpapi", "GetAdaptersInfo",                (FARPROC)&xwr::Shim_GetAdaptersInfo);
REGISTER_SHIM("iphlpapi", "GetCurrentThreadCompartmentId",  (FARPROC)&xwr::Shim_GetCurrentThreadCompartmentId);
REGISTER_SHIM("iphlpapi", "IcmpCloseHandle",                (FARPROC)&xwr::Shim_IcmpCloseHandle);
REGISTER_SHIM("iphlpapi", "IcmpCreateFile",                 (FARPROC)&xwr::Shim_IcmpCreateFile);
REGISTER_SHIM("iphlpapi", "IcmpSendEcho",                   (FARPROC)&xwr::Shim_IcmpSendEcho);
REGISTER_SHIM("iphlpapi", "if_nametoindex",                 (FARPROC)&xwr::Shim_if_nametoindex);
REGISTER_SHIM("iphlpapi", "SetCurrentThreadCompartmentId",  (FARPROC)&xwr::Shim_SetCurrentThreadCompartmentId);

// ===========================================================================
// End of GameDllStubs.cpp
// Total REGISTER_SHIM entries: 666
// (477 + 133 + 17 + 10 + 9 + 6 + 6 + 8)
// ===========================================================================
