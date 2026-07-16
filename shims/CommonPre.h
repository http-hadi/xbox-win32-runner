// shims/CommonPre.h
// Forced-include header that runs BEFORE every .cpp file's own includes.
//
// On MSVC (real Windows/Xbox builds): pulls in ALL real Windows SDK headers
//   via UwpSdkIncludes.h — <windows.h>, <xinput.h>, <winsock2.h>, <d3d11.h>,
//   <d2d1.h>, <dwrite.h>, <wincodec.h>, <xaudio2.h>, <mmsystem.h>, etc.
//   Plus #pragma comment(lib, ...) for all the .lib dependencies.
//
// On Linux (syntax-check builds): includes our stub winheaders/Windows.h
//   which defines just enough types for g++ -fsyntax-only to pass.
//
// This file is referenced by the vcxproj via <ForcedIncludeFiles>CommonPre.h
// so every .cpp gets it automatically.

#ifndef XWR_COMMON_PRE_H
#define XWR_COMMON_PRE_H

// Pull in the full SDK header set (real on MSVC, stub on Linux)
#include "UwpSdkIncludes.h"

#endif  // XWR_COMMON_PRE_H
