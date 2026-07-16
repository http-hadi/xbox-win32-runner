#!/usr/bin/env python3
"""
tools/game_simulator.py

Simulates Half Sword's API call sequence inside the UWP runtime simulator.

This script replays the exact API calls that Half Sword (UE5.4.4, 64-bit)
makes during startup and the first few frames of gameplay. The call sequence
is derived from:
  - Half Sword's import table (from coverage_report_v2.md — 1604 imports)
  - UE5 engine startup sequence (CRT init → WinMain → D3D11 → game loop)
  - Standard Win32 game patterns

Each API call is routed through the UWP runtime simulator, which checks:
  1. Does our shim registry have this function? (RESOLVED)
  2. Does UWP AppContainer block it? (BLOCKED)
  3. Is it a native UWP API? (NATIVE)
  4. Is it a stub? (STUB — returns failure)
  5. Is it missing entirely? (MISSING — would crash)

Usage:
  python3 tools/game_simulator.py [--verbose]
  python3 tools/game_simulator.py --game halfsword --verbose
  python3 tools/game_simulator.py --game test_hello
"""
from __future__ import annotations

import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from uwp_simulator import UWPRuntime  # type: ignore
from pe_import_scanner import build_shim_registry  # type: ignore


# ---------------------------------------------------------------------------
# Half Sword startup API call sequence
# ---------------------------------------------------------------------------
# This sequence mirrors what a UE5.4.4 game does from CRT init through the
# first 3 frames of gameplay. Each entry is (dll, funcname, description).
# The function names come directly from Half Sword's import table.

HALF_SWORD_STARTUP_SEQUENCE = [
    # === Phase 1: CRT initialization (before main()) ===
    # VC runtime startup — these are the first ~40 calls any MSVC-compiled exe makes
    ("kernel32", "GetSystemTimeAsFileTime", "CRT: seed RNG"),
    ("kernel32", "GetCurrentProcessId", "CRT: process ID"),
    ("kernel32", "GetCurrentThreadId", "CRT: thread ID"),
    ("kernel32", "QueryPerformanceFrequency", "CRT: timer frequency"),
    ("kernel32", "QueryPerformanceCounter", "CRT: timer baseline"),
    ("kernel32", "GetStartupInfoW", "CRT: startup info"),
    ("kernel32", "GetCommandLineW", "CRT: command line"),
    ("kernel32", "GetCommandLineA", "CRT: command line (ANSI)"),
    ("kernel32", "GetEnvironmentVariableW", "CRT: PATH"),
    ("kernel32", "GetModuleHandleW", "CRT: own module handle"),
    ("kernel32", "GetModuleFileNameW", "CRT: own exe path"),
    ("kernel32", "HeapCreate", "CRT: create heap"),
    ("kernel32", "HeapAlloc", "CRT: allocate"),
    ("kernel32", "HeapFree", "CRT: free"),
    ("kernel32", "HeapSize", "CRT: size query"),
    ("kernel32", "FlsAlloc", "CRT: fiber-local storage slot"),
    ("kernel32", "FlsSetValue", "CRT: set FLS value"),
    ("kernel32", "FlsGetValue", "CRT: get FLS value"),
    ("kernel32", "FlsFree", "CRT: free FLS slot"),
    ("kernel32", "TlsAlloc", "CRT: TLS slot"),
    ("kernel32", "TlsGetValue", "CRT: get TLS value"),
    ("kernel32", "TlsSetValue", "CRT: set TLS value"),
    ("kernel32", "InitializeCriticalSection", "CRT: lock init"),
    ("kernel32", "InitializeCriticalSectionEx", "CRT: lock init ex"),
    ("kernel32", "InitializeSRWLock", "CRT: SRW lock init"),
    ("kernel32", "InitializeConditionVariable", "CRT: condvar init"),
    ("kernel32", "InitializeSListHead", "CRT: lock-free list init"),
    ("kernel32", "SetUnhandledExceptionFilter", "CRT: exception handler"),
    ("kernel32", "RtlCaptureContext", "CRT: context capture"),
    ("kernel32", "RtlLookupFunctionEntry", "CRT: SEH table lookup"),
    ("kernel32", "RtlVirtualUnwind", "CRT: SEH unwind"),
    ("kernel32", "RtlUnwind", "CRT: SEH unwind"),
    ("kernel32", "RtlCaptureStackBackTrace", "CRT: stack trace"),
    ("kernel32", "EncodePointer", "CRT: pointer encoding"),
    ("kernel32", "DecodePointer", "CRT: pointer decoding"),
    ("kernel32", "GetACP", "CRT: ANSI code page"),
    ("kernel32", "GetOEMCP", "CRT: OEM code page"),
    ("kernel32", "GetCPInfo", "CRT: code page info"),
    ("kernel32", "IsValidCodePage", "CRT: validate code page"),
    ("kernel32", "GetUserDefaultLangID", "CRT: default language"),
    ("kernel32", "GetUserDefaultLCID", "CRT: default locale"),
    ("kernel32", "GetSystemDefaultLangID", "CRT: system language"),
    ("kernel32", "GetLocaleInfoW", "CRT: locale info"),
    ("kernel32", "LCMapStringW", "CRT: locale mapping"),
    ("kernel32", "CompareStringW", "CRT: string comparison"),
    ("kernel32", "CompareStringOrdinal", "CRT: ordinal comparison"),
    ("kernel32", "GetStringTypeW", "CRT: character type"),
    ("kernel32", "MultiByteToWideChar", "CRT: MBCS→Unicode"),
    ("kernel32", "WideCharToMultiByte", "CRT: Unicode→MBCS"),
    ("kernel32", "IsProcessorFeaturePresent", "CRT: CPU feature check"),
    ("kernel32", "IsDebuggerPresent", "CRT: debugger check"),
    ("kernel32", "SetLastError", "CRT: clear error"),
    ("kernel32", "GetLastError", "CRT: read error"),
    ("kernel32", "GetCurrentProcess", "CRT: process handle"),
    ("kernel32", "GetCurrentThread", "CRT: thread handle"),
    ("kernel32", "GetVersionExW", "CRT: OS version"),
    ("kernel32", "GetNativeSystemInfo", "CRT: system info"),
    ("kernel32", "GetSystemInfo", "CRT: system info"),
    ("kernel32", "GlobalMemoryStatusEx", "CRT: memory status"),
    ("kernel32", "GetLogicalProcessorInformation", "CRT: CPU topology"),
    ("kernel32", "GetCurrentProcessorNumber", "CRT: CPU core"),
    ("kernel32", "GetCurrentProcessorNumberEx", "CRT: CPU core (NUMA)"),
    ("kernel32", "GetCurrentThreadStackLimits", "CRT: stack bounds"),
    ("kernel32", "SetThreadStackGuarantee", "CRT: stack guard"),
    ("kernel32", "GetThreadPriority", "CRT: thread priority"),
    ("kernel32", "SetThreadPriority", "CRT: set priority"),
    ("kernel32", "GetThreadContext", "CRT: thread context"),
    ("kernel32", "SetThreadAffinityMask", "CRT: CPU affinity"),
    ("kernel32", "GetProcessAffinityMask", "CRT: process affinity"),
    ("kernel32", "GetActiveProcessorCount", "CRT: CPU count"),
    ("kernel32", "GetMaximumProcessorCount", "CRT: max CPUs"),
    ("kernel32", "SwitchToThread", "CRT: yield"),
    ("kernel32", "Sleep", "CRT: sleep"),
    ("kernel32", "SleepEx", "CRT: sleep ex"),
    ("kernel32", "YieldProcessor", "CRT: yield (intrinsic)"),
    ("kernel32", "MemoryBarrier", "CRT: memory barrier"),
    ("kernel32", "_ReadWriteBarrier", "CRT: compiler barrier"),
    # Interlocked ops (used heavily by CRT and std::atomic)
    ("kernel32", "InterlockedIncrement", "CRT: atomic increment"),
    ("kernel32", "InterlockedDecrement", "CRT: atomic decrement"),
    ("kernel32", "InterlockedExchange", "CRT: atomic exchange"),
    ("kernel32", "InterlockedCompareExchange", "CRT: atomic CAS"),
    ("kernel32", "InterlockedFlushSList", "CRT: flush lock-free list"),
    ("kernel32", "InterlockedPopEntrySList", "CRT: pop lock-free"),
    ("kernel32", "InterlockedPushEntrySList", "CRT: push lock-free"),
    # InitOnce (used by std::call_once / static init)
    ("kernel32", "InitOnceBeginInitialize", "CRT: one-time init begin"),
    ("kernel32", "InitOnceComplete", "CRT: one-time init complete"),
    ("kernel32", "InitOnceExecuteOnce", "CRT: one-time init execute"),

    # === Phase 2: WinMain / engine init ===
    ("user32", "RegisterClassExW", "Engine: register window class"),
    ("user32", "CreateWindowExW", "Engine: create main window"),
    ("user32", "ShowWindow", "Engine: show window"),
    ("user32", "UpdateWindow", "Engine: update window"),
    ("user32", "GetClientRect", "Engine: client rect"),
    ("user32", "GetWindowRect", "Engine: window rect"),
    ("user32", "AdjustWindowRectEx", "Engine: adjust rect"),
    ("user32", "GetSystemMetrics", "Engine: screen metrics"),
    ("user32", "GetDC", "Engine: get device context"),
    ("user32", "ReleaseDC", "Engine: release DC"),
    ("user32", "SetCursor", "Engine: set cursor"),
    ("user32", "ShowCursor", "Engine: show/hide cursor"),
    ("user32", "ClipCursor", "Engine: clip cursor to window"),
    ("user32", "GetCursorPos", "Engine: cursor position"),
    ("user32", "SetCursorPos", "Engine: set cursor position"),
    ("user32", "GetAsyncKeyState", "Engine: key state poll"),
    ("user32", "GetKeyState", "Engine: key state"),
    ("user32", "GetKeyboardState", "Engine: full keyboard state"),
    ("user32", "MapVirtualKeyW", "Engine: VK mapping"),
    ("user32", "PeekMessageW", "Engine: message poll"),
    ("user32", "GetMessageW", "Engine: message get"),
    ("user32", "TranslateMessage", "Engine: translate"),
    ("user32", "DispatchMessageW", "Engine: dispatch"),
    ("user32", "PostQuitMessage", "Engine: post quit"),
    ("user32", "PostMessageW", "Engine: post message"),
    ("user32", "SendMessageW", "Engine: send message"),
    ("user32", "DefWindowProcW", "Engine: default proc"),
    ("user32", "GetForegroundWindow", "Engine: foreground check"),
    ("user32", "SetForegroundWindow", "Engine: set foreground"),
    ("user32", "GetFocus", "Engine: focus check"),
    ("user32", "SetFocus", "Engine: set focus"),
    ("user32", "InvalidateRect", "Engine: invalidate"),
    ("user32", "GetMonitorInfoW", "Engine: monitor info"),
    ("user32", "MonitorFromWindow", "Engine: monitor from window"),
    ("user32", "EnumDisplayMonitors", "Engine: enum monitors"),
    ("user32", "EnumDisplaySettingsW", "Engine: display settings"),
    ("user32", "ChangeDisplaySettingsExW", "Engine: change display"),
    ("user32", "GetDpiForWindow", "Engine: DPI"),
    ("user32", "GetDpiForSystem", "Engine: system DPI"),
    ("user32", "SetProcessDPIAware", "Engine: DPI aware"),
    ("user32", "MessageBoxW", "Engine: error dialog"),
    ("user32", "LoadCursorW", "Engine: load cursor"),
    ("user32", "LoadIconW", "Engine: load icon"),
    ("user32", "LoadImageW", "Engine: load image"),

    # === Phase 3: D3D11 initialization ===
    ("d3d11", "D3D11CreateDevice", "D3D11: create device"),
    # The game also calls D3D11CreateDeviceAndSwapChain but we shim D3D11CreateDevice
    # (the pass-through clamps to FL 11_0)
    ("dxgi", "CreateDXGIFactory1", "DXGI: factory"),
    ("dxgi", "CreateDXGIFactory2", "DXGI: factory 2"),
    ("d3d11", "D3D11CreateDeviceAndSwapChain", "D3D11: device+swapchain"),
    ("d3dcompiler_47", "D3DCompile", "D3D: compile shader"),
    ("d3d12", "D3D12CreateDevice", "D3D12: create device (clamped to 11_0)"),

    # === Phase 4: File I/O (loading game assets) ===
    ("kernel32", "CreateFileW", "FS: open asset file"),
    ("kernel32", "ReadFile", "FS: read asset data"),
    ("kernel32", "WriteFile", "FS: write save data"),
    ("kernel32", "CloseHandle", "FS: close file"),
    ("kernel32", "GetFileSizeEx", "FS: get file size"),
    ("kernel32", "GetFileSize", "FS: get file size (legacy)"),
    ("kernel32", "SetFilePointerEx", "FS: seek"),
    ("kernel32", "GetFileAttributesW", "FS: check exists"),
    ("kernel32", "GetFileAttributesExW", "FS: get attributes"),
    ("kernel32", "FindFirstFileW", "FS: find first"),
    ("kernel32", "FindNextFileW", "FS: find next"),
    ("kernel32", "FindClose", "FS: find close"),
    ("kernel32", "CreateDirectoryW", "FS: create dir"),
    ("kernel32", "DeleteFileW", "FS: delete file"),
    ("kernel32", "MoveFileW", "FS: move file"),
    ("kernel32", "CopyFileW", "FS: copy file"),
    ("kernel32", "GetCurrentDirectoryW", "FS: get cwd"),
    ("kernel32", "SetCurrentDirectoryW", "FS: set cwd"),
    ("kernel32", "GetFullPathNameW", "FS: full path"),
    ("kernel32", "GetTempPathW", "FS: temp path"),
    ("kernel32", "GetVolumeInformationW", "FS: volume info"),
    ("kernel32", "GetDiskFreeSpaceExW", "FS: disk space"),
    ("kernel32", "GetDriveTypeW", "FS: drive type"),
    ("kernel32", "FlushFileBuffers", "FS: flush"),
    ("kernel32", "SetEndOfFile", "FS: truncate"),
    ("kernel32", "GetFileTime", "FS: timestamps"),
    ("kernel32", "SetFileTime", "FS: set timestamps"),
    ("kernel32", "GetFileInformationByHandle", "FS: file info"),
    ("kernel32", "GetFileInformationByHandleEx", "FS: file info ex"),
    ("kernel32", "GetFileType", "FS: file type"),
    ("kernel32", "GetFinalPathNameByHandleW", "FS: final path"),
    ("kernel32", "CreateFileMappingW", "FS: memory-mapped file"),
    ("kernel32", "MapViewOfFile", "FS: map view"),
    ("kernel32", "UnmapViewOfFile", "FS: unmap view"),
    ("kernel32", "FlushViewOfFile", "FS: flush mapping"),

    # === Phase 5: Memory management ===
    ("kernel32", "VirtualAlloc", "Mem: allocate"),
    ("kernel32", "VirtualAllocFromApp", "Mem: allocate (UWP)"),
    ("kernel32", "VirtualProtect", "Mem: protect"),
    ("kernel32", "VirtualProtectFromApp", "Mem: protect (UWP)"),
    ("kernel32", "VirtualFree", "Mem: free"),
    ("kernel32", "VirtualQuery", "Mem: query"),
    ("kernel32", "VirtualUnlock", "Mem: unlock"),
    ("kernel32", "GetProcessHeap", "Mem: process heap"),
    ("kernel32", "HeapReAlloc", "Mem: realloc"),
    ("kernel32", "HeapDestroy", "Mem: destroy heap"),
    ("kernel32", "HeapSetInformation", "Mem: heap info"),
    ("kernel32", "HeapCreate", "Mem: create heap"),
    ("kernel32", "GlobalAlloc", "Mem: global alloc"),
    ("kernel32", "GlobalFree", "Mem: global free"),
    ("kernel32", "GlobalLock", "Mem: global lock"),
    ("kernel32", "GlobalUnlock", "Mem: global unlock"),
    ("kernel32", "LocalAlloc", "Mem: local alloc"),
    ("kernel32", "LocalFree", "Mem: local free"),

    # === Phase 6: Threading & synchronization ===
    ("kernel32", "CreateThread", "Thread: create"),
    ("kernel32", "CreateEventW", "Sync: create event"),
    ("kernel32", "CreateMutexW", "Sync: create mutex"),
    ("kernel32", "CreateSemaphoreW", "Sync: create semaphore"),
    ("kernel32", "SetEvent", "Sync: signal event"),
    ("kernel32", "ResetEvent", "Sync: reset event"),
    ("kernel32", "ReleaseMutex", "Sync: release mutex"),
    ("kernel32", "ReleaseSemaphore", "Sync: release semaphore"),
    ("kernel32", "WaitForSingleObject", "Sync: wait"),
    ("kernel32", "WaitForMultipleObjects", "Sync: wait multi"),
    ("kernel32", "WaitForSingleObjectEx", "Sync: wait ex"),
    ("kernel32", "EnterCriticalSection", "Sync: enter CS"),
    ("kernel32", "LeaveCriticalSection", "Sync: leave CS"),
    ("kernel32", "TryEnterCriticalSection", "Sync: try enter CS"),
    ("kernel32", "DeleteCriticalSection", "Sync: delete CS"),
    ("kernel32", "AcquireSRWLockExclusive", "Sync: acquire SRW"),
    ("kernel32", "ReleaseSRWLockExclusive", "Sync: release SRW"),
    ("kernel32", "AcquireSRWLockShared", "Sync: acquire SRW shared"),
    ("kernel32", "ReleaseSRWLockShared", "Sync: release SRW shared"),
    ("kernel32", "SleepConditionVariableCS", "Sync: condvar wait"),
    ("kernel32", "WakeConditionVariable", "Sync: condvar wake"),
    ("kernel32", "WakeAllConditionVariable", "Sync: condvar wake all"),
    ("kernel32", "CreateTimerQueueTimer", "Sync: timer queue"),
    ("kernel32", "CreateTimerQueue", "Sync: create timer queue"),
    ("kernel32", "ChangeTimerQueueTimer", "Sync: change timer"),
    ("kernel32", "DeleteTimerQueueTimer", "Sync: delete timer"),
    ("kernel32", "CreateThreadPoolWait", "Sync: threadpool wait"),
    ("kernel32", "SetThreadPoolWait", "Sync: set threadpool wait"),
    ("kernel32", "CloseThreadPoolWait", "Sync: close threadpool wait"),
    ("kernel32", "WaitForThreadPoolWaitCallbacks", "Sync: wait callbacks"),
    ("kernel32", "QueueUserWorkItem", "Sync: queue work"),
    ("kernel32", "RegisterWaitForSingleObject", "Sync: register wait"),
    ("kernel32", "UnregisterWait", "Sync: unregister wait"),
    ("kernel32", "UnregisterWaitEx", "Sync: unregister wait ex"),
    ("kernel32", "SignalObjectAndWait", "Sync: signal+wait"),
    ("kernel32", "CreateWaitableTimerW", "Sync: waitable timer"),
    ("kernel32", "SetWaitableTimer", "Sync: set timer"),
    ("kernel32", "CancelWaitableTimer", "Sync: cancel timer"),
    ("kernel32", "GetTickCount", "Time: tick count"),
    ("kernel32", "GetTickCount64", "Time: tick count 64"),
    ("kernel32", "GetSystemTime", "Time: system time"),
    ("kernel32", "GetLocalTime", "Time: local time"),
    ("kernel32", "GetSystemTimeAsFileTime", "Time: filetime"),
    ("kernel32", "GetSystemTimePreciseAsFileTime", "Time: precise filetime"),
    ("kernel32", "GetTimeZoneInformation", "Time: timezone"),
    ("kernel32", "GetDynamicTimeZoneInformation", "Time: dynamic TZ"),
    ("kernel32", "SystemTimeToFileTime", "Time: system→file"),
    ("kernel32", "FileTimeToSystemTime", "Time: file→system"),
    ("kernel32", "LocalFileTimeToFileTime", "Time: local→file"),
    ("kernel32", "DosDateTimeToFileTime", "Time: dos→file"),
    ("kernel32", "CompareFileTime", "Time: compare filetimes"),

    # === Phase 7: Input (XInput + keyboard) ===
    ("xinput1_4", "XInputGetState", "Input: poll gamepad"),
    ("xinput1_4", "XInputSetState", "Input: vibration"),
    ("xinput1_4", "XInputGetCapabilities", "Input: caps"),
    ("xinput1_4", "XInputGetBatteryInformation", "Input: battery"),
    ("xinput1_4", "XInputEnable", "Input: enable"),
    ("xinput1_4", "XInputGetKeystroke", "Input: keystroke"),

    # === Phase 8: Audio ===
    ("xaudio2_9", "XAudio2Create", "Audio: create XAudio2"),
    # XAudio2_9Redist is the redist variant
    ("xaudio2_9redist", "XAudio2Create", "Audio: create XAudio2 (redist)"),
    # WinMM audio (FMOD may probe these)
    ("winmm", "waveOutOpen", "Audio: open wave out"),
    ("winmm", "waveOutClose", "Audio: close wave out"),
    ("winmm", "waveOutWrite", "Audio: write buffer"),
    ("winmm", "waveOutGetNumDevs", "Audio: count devices"),
    ("winmm", "waveOutGetDevCapsW", "Audio: device caps"),
    ("winmm", "waveOutGetVolume", "Audio: get volume"),
    ("winmm", "waveOutSetVolume", "Audio: set volume"),
    ("winmm", "timeGetTime", "Audio: time"),
    ("winmm", "timeBeginPeriod", "Audio: timer resolution"),
    ("winmm", "timeEndPeriod", "Audio: timer resolution end"),
    ("winmm", "timeGetDevCaps", "Audio: timer caps"),
    ("winmm", "mmioOpenW", "Audio: open WAV file"),
    ("winmm", "mmioClose", "Audio: close WAV"),
    ("winmm", "mmioRead", "Audio: read WAV"),
    ("winmm", "mmioSeek", "Audio: seek WAV"),
    ("winmm", "mmioDescend", "Audio: descend chunk"),
    ("winmm", "mmioAscend", "Audio: ascend chunk"),
    ("winmm", "mmioStringToFOURCCW", "Audio: fourcc"),
    ("winmm", "midiOutOpen", "Audio: open MIDI"),
    ("winmm", "midiOutClose", "Audio: close MIDI"),
    ("winmm", "midiOutShortMsg", "Audio: MIDI message"),
    ("winmm", "joyGetNumDevs", "Audio: joystick count"),
    ("winmm", "joyGetPosEx", "Audio: joystick pos"),
    ("winmm", "PlaySoundW", "Audio: play sound"),

    # === Phase 9: Network (Steam / multiplayer) ===
    ("ws2_32", "WSAStartup", "Net: init Winsock"),
    ("ws2_32", "socket", "Net: create socket"),
    ("ws2_32", "connect", "Net: connect"),
    ("ws2_32", "send", "Net: send data"),
    ("ws2_32", "recv", "Net: receive data"),
    ("ws2_32", "closesocket", "Net: close socket"),
    ("ws2_32", "bind", "Net: bind"),
    ("ws2_32", "listen", "Net: listen"),
    ("ws2_32", "accept", "Net: accept"),
    ("ws2_32", "setsockopt", "Net: set socket option"),
    ("ws2_32", "getsockopt", "Net: get socket option"),
    ("ws2_32", "ioctlsocket", "Net: ioctl"),
    ("ws2_32", "gethostname", "Net: hostname"),
    ("ws2_32", "gethostbyname", "Net: host by name"),
    ("ws2_32", "inet_addr", "Net: IP address"),
    ("ws2_32", "htons", "Net: host→net short"),
    ("ws2_32", "htonl", "Net: host→net long"),
    ("ws2_32", "ntohs", "Net: net→host short"),
    ("ws2_32", "ntohl", "Net: net→host long"),
    ("ws2_32", "select", "Net: select"),
    ("ws2_32", "WSACleanup", "Net: cleanup"),
    ("ws2_32", "WSAGetLastError", "Net: last error"),
    ("ws2_32", "getaddrinfo", "Net: address info"),
    ("ws2_32", "freeaddrinfo", "Net: free addrinfo"),
    ("ws2_32", "WSASocketW", "Net: WSA socket"),
    ("ws2_32", "WSASend", "Net: WSA send"),
    ("ws2_32", "WSARecv", "Net: WSA recv"),
    ("ws2_32", "WSACloseEvent", "Net: WSA close event"),
    ("ws2_32", "WSACreateEvent", "Net: WSA create event"),
    ("ws2_32", "WSAEventSelect", "Net: WSA event select"),
    ("ws2_32", "WSAWaitForMultipleEvents", "Net: WSA wait"),
    ("ws2_32", "WSAIoctl", "Net: WSA ioctl"),
    ("ws2_32", "WSAPoll", "Net: WSA poll"),
    ("ws2_32", "inet_ntop", "Net: IP to string"),
    ("ws2_32", "inet_pton", "Net: string to IP"),
    ("iphlpapi", "GetAdaptersAddresses", "Net: adapter info"),
    ("iphlpapi", "GetAdaptersInfo", "Net: adapter info (legacy)"),
    ("winhttp", "WinHttpOpen", "Net: HTTP open"),
    ("winhttp", "WinHttpConnect", "Net: HTTP connect"),
    ("winhttp", "WinHttpOpenRequest", "Net: HTTP request"),
    ("winhttp", "WinHttpSendRequest", "Net: HTTP send"),
    ("winhttp", "WinHttpReceiveResponse", "Net: HTTP receive"),

    # === Phase 10: Steam (stubbed — returns offline) ===
    ("steam_api64", "SteamAPI_Init", "Steam: init (stub returns false)"),
    ("steam_api64", "SteamAPI_Shutdown", "Steam: shutdown"),
    ("steam_api64", "SteamAPI_RunCallbacks", "Steam: callbacks"),
    ("steam_api64", "SteamAPI_RestartAppIfNecessary", "Steam: restart check"),
    ("steam_api64", "SteamInternal_ContextInit", "Steam: context init"),
    ("steam_api64", "SteamAPI_GetHSteamUser", "Steam: get HSteamUser"),

    # === Phase 11: Discord (stubbed) ===
    ("discord-rpc", "discord_initialize", "Discord: init (stub)"),
    ("discord-rpc", "discord_updatepresence", "Discord: presence (stub)"),
    ("discord-rpc", "discord_runcallbacks", "Discord: callbacks (stub)"),
    ("discord-rpc", "discord_shutdown", "Discord: shutdown (stub)"),
    ("discord_partner_sdk", "discord_client_connect", "Discord SDK: connect (stub)"),
    ("discord_partner_sdk", "discord_client_disconnect", "Discord SDK: disconnect (stub)"),

    # === Phase 12: Registry ===
    ("advapi32", "RegOpenKeyExW", "Registry: open key"),
    ("advapi32", "RegQueryValueExW", "Registry: query value"),
    ("advapi32", "RegCloseKey", "Registry: close key"),
    ("advapi32", "RegCreateKeyExW", "Registry: create key"),
    ("advapi32", "RegSetValueExW", "Registry: set value"),
    ("advapi32", "RegDeleteKeyW", "Registry: delete key"),
    ("advapi32", "RegDeleteValueW", "Registry: delete value"),
    ("advapi32", "RegEnumKeyExW", "Registry: enum keys"),
    ("advapi32", "RegEnumValueW", "Registry: enum values"),
    ("advapi32", "RegQueryInfoKeyW", "Registry: info"),
    ("advapi32", "RegGetValueW", "Registry: get value"),
    ("advapi32", "RegNotifyChangeKeyValue", "Registry: notify"),

    # === Phase 13: Security / crypto ===
    ("advapi32", "AllocateLocallyUniqueId", "Security: alloc LUID"),
    ("advapi32", "CheckTokenMembership", "Security: token check"),
    ("advapi32", "CreateWellKnownSid", "Security: well-known SID"),
    ("advapi32", "LookupPrivilegeValueW", "Security: privilege lookup"),
    ("advapi32", "SystemFunction036", "Security: RtlGenRandom"),
    ("crypt32", "CertOpenSystemStoreW", "Crypto: open cert store"),
    ("crypt32", "CertCloseStore", "Crypto: close cert store"),
    ("crypt32", "CertFindCertificateInStore", "Crypto: find cert"),
    ("bcrypt", "BCryptOpenAlgorithmProvider", "Crypto: BCrypt open"),
    ("bcrypt", "BCryptGenRandom", "Crypto: BCrypt random"),
    ("bcrypt", "BCryptCloseAlgorithmProvider", "Crypto: BCrypt close"),

    # === Phase 14: Event tracing (ETW) ===
    ("advapi32", "EventRegister", "ETW: register provider"),
    ("advapi32", "EventUnregister", "ETW: unregister"),
    ("advapi32", "EventWriteTransfer", "ETW: write event"),
    ("advapi32", "EventSetInformation", "ETW: set info"),

    # === Phase 15: DLL loading ===
    ("kernel32", "LoadLibraryW", "DLL: load"),
    ("kernel32", "LoadLibraryExW", "DLL: load ex"),
    ("kernel32", "GetProcAddress", "DLL: get proc address"),
    ("kernel32", "FreeLibrary", "DLL: free"),
    ("kernel32", "GetModuleHandleW", "DLL: get handle"),
    ("kernel32", "GetModuleFileNameW", "DLL: get filename"),
    ("kernel32", "DisableThreadLibraryCalls", "DLL: disable thread notifies"),

    # === Phase 16: Console / debug ===
    ("kernel32", "GetStdHandle", "Console: std handle"),
    ("kernel32", "WriteConsoleW", "Console: write"),
    ("kernel32", "GetConsoleMode", "Console: get mode"),
    ("kernel32", "SetConsoleMode", "Console: set mode"),
    ("kernel32", "GetConsoleScreenBufferInfo", "Console: buffer info"),
    ("kernel32", "SetConsoleCtrlHandler", "Console: ctrl handler"),
    ("kernel32", "OutputDebugStringW", "Debug: trace"),
    ("kernel32", "IsDebuggerPresent", "Debug: debugger check"),

    # === Phase 17: Environment / system info ===
    ("kernel32", "GetEnvironmentVariableW", "Env: get var"),
    ("kernel32", "SetEnvironmentVariableW", "Env: set var"),
    ("kernel32", "GetEnvironmentStringsW", "Env: get all"),
    ("kernel32", "FreeEnvironmentStringsW", "Env: free strings"),
    ("kernel32", "GetComputerNameW", "Env: computer name"),
    ("kernel32", "GetUserNameW", "Env: user name"),
    ("kernel32", "GetWindowsDirectoryW", "Env: windows dir"),
    ("kernel32", "GetSystemDirectoryW", "Env: system dir"),
    ("kernel32", "GetDllDirectoryW", "Env: dll dir"),
    ("kernel32", "SetDllDirectoryW", "Env: set dll dir"),
    ("kernel32", "ExpandEnvironmentStringsW", "Env: expand strings"),
    ("kernel32", "GetCurrentDirectoryW", "Env: get cwd"),
    ("kernel32", "SetCurrentDirectoryW", "Env: set cwd"),

    # === Phase 18: Error handling ===
    ("kernel32", "SetErrorMode", "Error: set mode"),
    ("kernel32", "GetThreadErrorMode", "Error: get mode"),
    ("kernel32", "SetThreadErrorMode", "Error: set thread mode"),
    ("kernel32", "RaiseException", "Error: raise exception"),
    ("kernel32", "UnhandledExceptionFilter", "Error: unhandled filter"),
    ("kernel32", "AddVectoredExceptionHandler", "Error: VEH add"),
    ("kernel32", "RemoveVectoredExceptionHandler", "Error: VEH remove"),

    # === Phase 19: Process / thread info ===
    ("kernel32", "GetProcessId", "Proc: process ID"),
    ("kernel32", "GetThreadId", "Proc: thread ID"),
    ("kernel32", "GetProcessTimes", "Proc: process times"),
    ("kernel32", "GetThreadTimes", "Proc: thread times"),
    ("kernel32", "GetProcessHandleCount", "Proc: handle count"),
    ("kernel32", "GetExitCodeProcess", "Proc: exit code"),
    ("kernel32", "GetExitCodeThread", "Proc: thread exit code"),
    ("kernel32", "OpenProcess", "Proc: open (blocked in UWP)"),
    ("kernel32", "OpenThread", "Proc: open thread"),
    ("kernel32", "DuplicateHandle", "Proc: dup handle"),
    ("kernel32", "GetPriorityClass", "Proc: priority"),
    ("kernel32", "QueryFullProcessImageNameW", "Proc: image name"),
    ("kernel32", "ProcessIdToSessionId", "Proc: session ID"),
    ("kernel32", "IsProcessInJob", "Proc: in job check"),
    ("kernel32", "CreateToolhelp32Snapshot", "Proc: snapshot (blocked)"),
    ("kernel32", "Process32FirstW", "Proc: enum first (blocked)"),
    ("kernel32", "Process32NextW", "Proc: enum next (blocked)"),
    ("kernel32", "GetCurrentProcessorNumber", "Proc: CPU number"),

    # === Phase 20: NVIDIA / DLSS (stubbed — game falls back) ===
    ("nvlowlatencyvk", "nvll_vk_initialize", "NVReflex: init (stub)"),
    ("nvlowlatencyvk", "nvll_vk_sleep", "NVReflex: sleep (stub)"),
    ("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_dx11_initialize", "Aftermath: init (stub)"),
    ("gfsdk_aftermath_lib.x64", "gfsdk_aftermath_seteventmarker", "Aftermath: marker (stub)"),
    ("openimagedenoise", "oidnnewdevice", "OIDN: new device (stub)"),
    ("openimagedenoise", "oidncommitdevice", "OIDN: commit (stub)"),

    # === Phase 21: Python (UE5 scripting — stubbed) ===
    ("python311", "Py_Initialize", "Python: init (stub)"),
    ("python311", "Py_DecRef", "Python: decref (stub)"),
    ("python311", "PyImport_ImportModule", "Python: import (stub)"),

    # === Phase 22: GDI (minimal — UE5 uses D3D) ===
    ("gdi32", "CreateCompatibleDC", "GDI: compatible DC"),
    ("gdi32", "SelectObject", "GDI: select object"),
    ("gdi32", "DeleteObject", "GDI: delete object"),
    ("gdi32", "DeleteDC", "GDI: delete DC"),
    ("gdi32", "GetStockObject", "GDI: stock object"),
    ("gdi32", "GetObjectW", "GDI: object info"),
    ("gdi32", "CreateSolidBrush", "GDI: solid brush"),
    ("gdi32", "CreatePen", "GDI: pen"),
    ("gdi32", "CreateFontIndirectW", "GDI: font"),
    ("gdi32", "BitBlt", "GDI: bit blt"),
    ("gdi32", "StretchBlt", "GDI: stretch blt"),
    ("gdi32", "SetTextColor", "GDI: text color"),
    ("gdi32", "SetBkColor", "GDI: bk color"),
    ("gdi32", "SetBkMode", "GDI: bk mode"),
    ("gdi32", "GetDeviceCaps", "GDI: device caps"),
    ("gdi32", "TextOutW", "GDI: text out"),
    ("gdi32", "GetTextExtentPoint32W", "GDI: text extent"),
    ("gdi32", "GetTextMetricsW", "GDI: text metrics"),
    ("gdi32", "Rectangle", "GDI: rectangle"),
    ("gdi32", "FillRect", "GDI: fill rect"),

    # === Phase 23: OLE / COM ===
    ("ole32", "CoInitializeEx", "COM: init"),
    ("ole32", "CoUninitialize", "COM: uninit"),
    ("ole32", "CoCreateInstance", "COM: create instance"),
    ("ole32", "CoTaskMemAlloc", "COM: alloc"),
    ("ole32", "CoTaskMemFree", "COM: free"),
    ("ole32", "CoInitialize", "COM: init (legacy)"),
    ("oleaut32", "SysAllocString", "OLE: alloc BSTR"),
    ("oleaut32", "SysFreeString", "OLE: free BSTR"),
    ("oleaut32", "SysStringLen", "OLE: BSTR length"),
    ("oleaut32", "VariantInit", "OLE: variant init"),
    ("oleaut32", "VariantClear", "OLE: variant clear"),

    # === Phase 24: Shell ===
    ("shell32", "SHGetFolderPathW", "Shell: folder path"),
    ("shell32", "SHGetKnownFolderPath", "Shell: known folder"),
    ("shell32", "ShellExecuteW", "Shell: execute"),
    ("shell32", "SHCreateDirectoryExW", "Shell: create dir"),
    ("shlwapi", "PathAppendW", "Shlwapi: path append"),
    ("shlwapi", "PathCombineW", "Shlwapi: path combine"),
    ("shlwapi", "PathCanonicalizeW", "Shlwapi: canonicalize"),
    ("shlwapi", "PathFindFileNameW", "Shlwapi: find filename"),
    ("shlwapi", "PathFindExtensionW", "Shlwapi: find extension"),
    ("shlwapi", "PathFileExistsW", "Shlwapi: file exists"),
    ("shlwapi", "PathIsDirectoryW", "Shlwapi: is dir"),
    ("shlwapi", "PathRemoveFileSpecW", "Shlwapi: remove filespec"),
    ("shlwapi", "StrCmpIW", "Shlwapi: string compare"),
    ("shlwapi", "StrCmpW", "Shlwapi: string compare"),
    ("shlwapi", "StrStrW", "Shlwapi: string search"),

    # === Phase 25: Version info ===
    ("version", "GetFileVersionInfoW", "Version: get info"),
    ("version", "GetFileVersionInfoSizeW", "Version: get size"),
    ("version", "VerQueryValueW", "Version: query value"),
    ("version", "GetFileVersionInfoExW", "Version: get info ex"),

    # === Phase 26: Misc system DLLs ===
    ("imm32", "ImmGetContext", "IMM: get context"),
    ("imm32", "ImmReleaseContext", "IMM: release context"),
    ("imm32", "ImmGetCompositionStringW", "IMM: composition string"),
    ("dwmapi", "DwmIsCompositionEnabled", "DWM: composition enabled"),
    ("dwmapi", "DwmGetWindowAttribute", "DWM: window attribute"),
    ("ntdll", "NtQueryInformationProcess", "NT: query process"),
    ("ntdll", "NtSetInformationProcess", "NT: set process"),
    ("ntdll", "RtlGetVersion", "NT: get version"),
    ("ntdll", "NtClose", "NT: close handle"),
    ("ntdll", "RtlAllocateHeap", "NT: alloc heap"),
    ("ntdll", "RtlFreeHeap", "NT: free heap"),
    ("dbghelp", "SymInitialize", "DbgHelp: sym init"),
    ("dbghelp", "SymCleanup", "DbgHelp: sym cleanup"),
    ("dbghelp", "MiniDumpWriteDump", "DbgHelp: minidump"),
    ("dbghelp", "StackWalk64", "DbgHelp: stack walk"),
    ("wintrust", "WinVerifyTrust", "WinTrust: verify"),
    ("cfgmgr32", "CM_Get_Device_IDW", "CfgMgr: device ID"),
    ("msi", "MsiOpenDatabaseW", "MSI: open database"),

    # === Phase 27: Shutdown ===
    ("kernel32", "ExitProcess", "Shutdown: exit"),
    ("kernel32", "ExitThread", "Shutdown: exit thread"),
    ("kernel32", "TerminateProcess", "Shutdown: terminate"),
    ("kernel32", "FreeLibraryAndExitThread", "Shutdown: free+exit"),
]


# A minimal test "hello world" exe for comparison
TEST_HELLO_SEQUENCE = [
    ("kernel32", "GetStdHandle", "Get stdout handle"),
    ("kernel32", "GetCommandLineW", "Get command line"),
    ("kernel32", "GetModuleHandleW", "Get own module"),
    ("kernel32", "HeapCreate", "Create heap"),
    ("kernel32", "HeapAlloc", "Allocate"),
    ("kernel32", "WriteFile", "Write hello"),
    ("kernel32", "HeapFree", "Free"),
    ("kernel32", "ExitProcess", "Exit"),
]


def run_simulation(runtime: UWPRuntime, sequence: list, label: str):
    """Run a game simulation sequence through the UWP runtime."""
    print(f"\n{'='*70}")
    print(f"  Simulating: {label}")
    print(f"  Total API calls: {len(sequence)}")
    print(f"{'='*70}\n")

    if runtime.verbose:
        print("  Call-by-call trace:")
        print()

    for dll, func, desc in sequence:
        runtime.resolve_api(dll, func)

    # Print report
    print(runtime.report())


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Simulate Half Sword running in a UWP AppContainer environment"
    )
    parser.add_argument("--game", choices=["halfsword", "test_hello"],
                        default="halfsword", help="Which game to simulate")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Print every API call")
    parser.add_argument("--shims-dir", default=os.path.join(HERE, "..", "shims"),
                        help="Path to shims/ directory")
    args = parser.parse_args()

    # Build the shim registry from our source code
    shims_dir = os.path.abspath(args.shims_dir)
    print(f"Loading shim registry from: {shims_dir}")
    shim_registry = build_shim_registry(shims_dir)
    print(f"  Loaded {len(shim_registry)} shim entries")

    # Create the UWP runtime
    runtime = UWPRuntime(shim_registry)
    runtime.verbose = args.verbose

    # Select the game
    if args.game == "halfsword":
        sequence = HALF_SWORD_STARTUP_SEQUENCE
        label = "Half Sword (UE5.4.4) — full startup + first frame"
    else:
        sequence = TEST_HELLO_SEQUENCE
        label = "test_hello.exe — minimal hello world"

    run_simulation(runtime, sequence, label)

    # Return non-zero if there are missing APIs
    missing = sum(1 for r in runtime.call_log if r.status == "MISSING")
    return 1 if missing > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
