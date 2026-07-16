// shims/UwpSdkIncludes.h
// Master include file for the Xbox Win32 Runner.
//
// On MSVC: includes <winsock2.h> BEFORE <windows.h> (required), then
// <windows.h> with ALL exclusion macros undef'd so the FULL header set
// is included. Then explicitly includes the few extra headers that
// <windows.h> doesn't pull in by default (xinput, d3d11, d2d1, etc.).
//
// On Linux: includes our stub winheaders/Windows.h for syntax checking.
//
// CRITICAL: Do NOT manually include dozens of SDK sub-headers (winuser.h,
// wingdi.h, winsvc.h, etc.) — <windows.h> already includes them all when
// WIN32_LEAN_AND_MEAN and the NO* macros are undef'd. Manually including
// them causes ordering conflicts (ntdef.h redefinition errors).

#pragma once

// ===========================================================================
// STEP 1: KILL ALL EXCLUSION MACROS
// ===========================================================================
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif
#ifdef NOGDI
#undef NOGDI
#endif
#ifdef NOUSER
#undef NOUSER
#endif
#ifdef NOSERVICE
#undef NOSERVICE
#endif
#ifdef NOSOUND
#undef NOSOUND
#endif
#ifdef NOCOMM
#undef NOCOMM
#endif
#ifdef NOCRYPT
#undef NOCRYPT
#endif
#ifdef NOHELP
#undef NOHELP
#endif
#ifdef NOGDI
#undef NOGDI
#endif
#ifdef NOMCX
#undef NOMCX
#endif
#ifdef NOICONS
#undef NOICONS
#endif
#ifdef NOMEMMGR
#undef NOMEMMGR
#endif
#ifdef NOMETAFILE
#undef NOMETAFILE
#endif
#ifdef NOOPENFILE
#undef NOOPENFILE
#endif
#ifdef NOSCROLL
#undef NOSCROLL
#endif
#ifdef NOSHOWWINDOW
#undef NOSHOWWINDOW
#endif
#ifdef NOSYSCOMMANDS
#undef NOSYSCOMMANDS
#endif
#ifdef NOSYSMETRICS
#undef NOSYSMETRICS
#endif
#ifdef NOTEXTMETRIC
#undef NOTEXTMETRIC
#endif
#ifdef NOVIRTUALKEYCODES
#undef NOVIRTUALKEYCODES
#endif
#ifdef NOWH
#undef NOWH
#endif
#ifdef NOWINMESSAGES
#undef NOWINMESSAGES
#endif
#ifdef NOWINOFFSETS
#undef NOWINOFFSETS
#endif
#ifdef NOWINSTYLES
#undef NOWINSTYLES
#endif
#ifdef NORESOURCE
#undef NORESOURCE
#endif
#ifdef NOATOM
#undef NOATOM
#endif
#ifdef NOCLIPBOARD
#undef NOCLIPBOARD
#endif
#ifdef NOCOLOR
#undef NOCOLOR
#endif
#ifdef NOCTLMGR
#undef NOCTLMGR
#endif
#ifdef NODEFERWINDOWPOS
#undef NODEFERWINDOWPOS
#endif
#ifdef NODRAWTEXT
#undef NODRAWTEXT
#endif
#ifdef NOKEYSTATES
#undef NOKEYSTATES
#endif
#ifdef NOMB
#undef NOMB
#endif
#ifdef NOMENUS
#undef NOMENUS
#endif
#ifdef NOMSG
#undef NOMSG
#endif
#ifdef NOPROFILER
#undef NOPROFILER
#endif
#ifdef NOAPIENTRY
#undef NOAPIENTRY
#endif
#ifdef NOKANJI
#undef NOKANJI
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

// ===========================================================================
// STEP 2: Version macros
// ===========================================================================
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#endif
#ifndef WINDOWS_IGNORE_PACKING_MISMATCH
#define WINDOWS_IGNORE_PACKING_MISMATCH
#endif

// ===========================================================================
// STEP 3: Include headers
// ===========================================================================

#if defined(_MSC_VER)
  // =====================================================================
  // REAL WINDOWS / XBOX BUILD
  // =====================================================================

  // Winsock2 MUST be included BEFORE windows.h (otherwise winsock.h
  // gets included instead, causing redefinition conflicts).
  #include <winsock2.h>
  #include <ws2tcpip.h>

  // <windows.h> with all NO* macros undef'd includes the FULL header set:
  //   winuser.h, wingdi.h, winreg.h, winsvc.h, wincrypt.h, shellapi.h,
  //   shlobj.h, shlwapi.h, mmsystem.h (via mmsyscom), winver.h, commdlg.h,
  //   objbase.h, oleauto.h, oaidl.h, dbghelp.h, imm.h, dwmapi.h,
  //   setupapi.h, iphlpapi.h, winhttp.h, msi.h, uiautomationcore.h,
  //   propsys.h, mmdeviceapi.h, mfapi.h, ncrypt.h, cfgmgr32.h, rpc.h,
  //   avrt.h, evntprov.h, aclapi.h, accctrl.h, sal.h, intrin.h, etc.
  //
  // DO NOT manually include these sub-headers — <windows.h> handles the
  // ordering. Manual includes cause ntdef.h redefinition errors.
  #include <windows.h>

  // Extra headers NOT included by <windows.h>:
  #include <xinput.h>
  #include <xaudio2.h>
  #include <d3d11.h>
  #include <d3d12.h>
  #include <dxgi.h>
  #include <dxgi1_2.h>
  #include <dxgi1_3.h>
  #include <dxgi1_4.h>
  #include <d3dcompiler.h>
  #include <d2d1.h>
  #include <dwrite.h>
  #include <wincodec.h>
  #include <icmpapi.h>
  #include <fci.h>
  #include <inspectable.h>
  #include <directxmath.h>
  #include <tlhelp32.h>
  #include <psapi.h>
  #include <winternl.h>
  #include <evntprov.h>

  // Linker directives
  #pragma comment(lib, "kernel32.lib")
  #pragma comment(lib, "user32.lib")
  #pragma comment(lib, "gdi32.lib")
  #pragma comment(lib, "advapi32.lib")
  #pragma comment(lib, "shell32.lib")
  #pragma comment(lib, "shlwapi.lib")
  #pragma comment(lib, "ole32.lib")
  #pragma comment(lib, "oleaut32.lib")
  #pragma comment(lib, "uuid.lib")
  #pragma comment(lib, "winmm.lib")
  #pragma comment(lib, "version.lib")
  #pragma comment(lib, "d3d11.lib")
  #pragma comment(lib, "d3d12.lib")
  #pragma comment(lib, "dxgi.lib")
  #pragma comment(lib, "d3dcompiler.lib")
  #pragma comment(lib, "d2d1.lib")
  #pragma comment(lib, "dwrite.lib")
  #pragma comment(lib, "windowscodecs.lib")
  #pragma comment(lib, "xinput.lib")
  #pragma comment(lib, "xaudio2.lib")
  #pragma comment(lib, "ws2_32.lib")
  #pragma comment(lib, "crypt32.lib")
  #pragma comment(lib, "bcrypt.lib")
  #pragma comment(lib, "dbghelp.lib")
  #pragma comment(lib, "imm32.lib")
  #pragma comment(lib, "dwmapi.lib")
  #pragma comment(lib, "setupapi.lib")
  #pragma comment(lib, "iphlpapi.lib")
  #pragma comment(lib, "winhttp.lib")
  #pragma comment(lib, "msi.lib")
  #pragma comment(lib, "uiautomationcore.lib")
  #pragma comment(lib, "propsys.lib")
  #pragma comment(lib, "mmdevapi.lib")
  #pragma comment(lib, "mfplat.lib")
  #pragma comment(lib, "mfuuid.lib")
  #pragma comment(lib, "ncrypt.lib")
  #pragma comment(lib, "cfgmgr32.lib")
  #pragma comment(lib, "rpcrt4.lib")
  #pragma comment(lib, "avrt.lib")
  #pragma comment(lib, "wintrust.lib")
  #pragma comment(lib, "fci.lib")
  #pragma comment(lib, "comdlg32.lib")
  #pragma comment(lib, "psapi.lib")

#else
  // =====================================================================
  // LINUX SYNTAX-CHECK BUILD
  // =====================================================================
  #ifndef __stdcall
  #define __stdcall
  #endif
  #ifndef __cdecl
  #define __cdecl
  #endif
  #ifndef __fastcall
  #define __fastcall
  #endif
  #ifndef __vectorcall
  #define __vectorcall
  #endif
  #include <Windows.h>
#endif

// ===========================================================================
// STEP 4: Supplemental types/constants (safe on both platforms — uses
// #ifndef guards so it only adds what's missing)
// ===========================================================================
#include "Win32Types.h"
