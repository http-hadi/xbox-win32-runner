# Architecture

This document describes the internal design of the Xbox Win32 Runner. The
audience is a C++/UWP developer who has already read `COMPILE.md`, has a
working build of the project, and now wants to understand how the pieces
fit together before modifying them.

The document is organized top-down: first the big picture, then the threat
model that constrains the design, then a per-component walk-through, and
finally a discussion of limitations.

---

## Table of Contents

1.  [Executive Summary](#1-executive-summary)
2.  [Threat Model](#2-threat-model)
3.  [Component Overview](#3-component-overview)
4.  [PE Loader Pipeline](#4-pe-loader-pipeline)
5.  [IAT Rewriting](#5-iat-rewriting)
6.  [Path Translation](#6-path-translation)
7.  [D3D11 Bridge](#7-d3d11-bridge)
8.  [I/O Bridges](#8-io-bridges)
9.  [Shim Coverage Strategy](#9-shim-coverage-strategy)
10. [Target Game: Half Sword](#10-target-game-half-sword)
11. [Limitations and Future Work](#11-limitations-and-future-work)

---

## 1. Executive Summary

The Xbox Win32 Runner is a UWP application that loads and executes native
Windows x64 PE binaries inside the Xbox Series X|S Developer Mode sandbox.
The Xbox, despite being an x86-64 PC with a desktop-class GPU, runs UWP
applications inside an AppContainer that restricts the Win32 surface area
to a small, sandboxed subset. Native Win32 games cannot run unmodified
because:

1. The UWP AppContainer does not load arbitrary `.exe` files via
   `CreateProcess` — there is no general-purpose loader for desktop PE
   files.
2. Many Win32 imports (registry, services, raw device I/O, GDI on HDCs)
   are either absent or restricted inside the AppContainer.
3. Direct3D 11 is available but capped at feature level 11_0; Direct3D 12
   requires UWP-specific swap-chain construction.

The runner solves these problems by:

1. Implementing its own in-process PE loader (`PeLoader`) that maps a
   game's `.exe` into memory using the AppContainer-friendly
   `VirtualAllocFromApp` / `VirtualProtectFromApp` primitives, applies
   relocations, walks the import and delay-import tables, runs TLS
   callbacks, and registers exception tables.
2. Replacing the game's expected Win32 imports with a registry of "shim"
   functions (`ShimRegistry` + 23 shim modules). Each shim either
   forwards to the corresponding UWP API, translates the call to an
   AppContainer-friendly equivalent, or returns a safe failure value.
3. Owning the real D3D11 device and swap chain (`D3D11Bridge`) and
   handing the same `ID3D11Device*` / `ID3D11DeviceContext*` /
   `IDXGISwapChain*` out to the game's IAT, so the game renders into the
   same back buffer the UWP host presents to the Xbox display.
4. Bridging input and audio between the Xbox's UWP-native input stack
   (Windows.Gaming.Input, CoreWindow) and the game's expected Win32
   surface (XInput, GetAsyncKeyState, waveOut).

The reference target game is Half Sword — an Unreal Engine 5.4.4 title
that ships with a D3D11 RHI, no kernel-mode anti-cheat, and only the
standard Steam stub. The runner is designed to make this game playable on
the Xbox; broader Win32-game compatibility is a future-work item, not a
design goal.

---

## 2. Threat Model

UWP applications on Xbox run inside an AppContainer — the same sandbox
that desktop UWP apps use, with additional Xbox-specific restrictions.
The runner's design is constrained by what the AppContainer allows.

### 2.1 What the AppContainer allows

| Capability                                       | Allowed? | Notes                                          |
|--------------------------------------------------|----------|------------------------------------------------|
| `VirtualAllocFromApp` with `PAGE_EXECUTE_READWRITE` | Yes   | Requires the `codeGeneration` restricted capability, declared in `Package.appxmanifest`. |
| `VirtualProtectFromApp` to flip pages between RW / RX | Yes | Same `codeGeneration` capability.             |
| `RtlAddFunctionTable` to register exception handlers | Yes | Used by the PE loader for SEH table registration. |
| `CreateFile2` / `CreateFileW` inside LocalState    | Yes    | All file I/O must go through the UWP local folder. |
| `D3D11CreateDevice` with feature level 11_0       | Yes     | 11_1 and higher are not exposed to UWP on Xbox. |
| `Windows.Gaming.Input::Gamepad`                   | Yes     | Up to 8 controllers, exposed via WinRT.       |
| `IXAudio2` (XAudio 2.9 for UWP)                   | Yes     | Render client available through the XAudio2 UWP API. |
| `WinSock` (ws2_32)                                | Yes     | With capability `internetClient` / `internetClientServer`. |
| Raw keyboard / mouse via `GetAsyncKeyState`        | No      | Replaced by CoreWindow events → KeyboardBridge. |
| `HKLM` / `HKCU` registry writes                   | No      | Advapi32 shim simulates an in-memory registry. |
| Loading unsigned drivers                          | No      | No driver surface at all.                     |
| Spawning arbitrary child processes                | No      | Only `FullTrustProcessLauncher` for declared desktop extensions. |
| `LoadLibrary` on system DLLs (kernel32, user32)   | Partial | The DLLs exist but their exports are ACL'd to AppContainer callers; many return `ACCESS_DENIED`. |

### 2.2 What we do not defend against

The runner assumes the user has physical access to a developer Xbox and
has activated Developer Mode. It is **not** a security boundary and
**not** a DRM bypass:

- We do not load games the user does not already own. The user must push
  the game's `.exe` and supporting files to the Xbox's local folder via
  the Device Portal.
- We do not strip DRM. A game that requires Steam to be running will see
  the Steam shim return "not running" and degrade gracefully (or refuse
  to launch — that is the game's choice).
- We do not bypass anti-cheat. A game that uses Easy Anti-Cheat or
  BattlEye will fail to launch because those systems require kernel-mode
  drivers, which the AppContainer cannot load.

### 2.3 The `codeGeneration` capability

The runner's `Package.appxmanifest` declares the `codeGeneration`
restricted capability:

```xml
<Capabilities>
  <rescap:Capability Name="codeGeneration" />
  ...
</Capabilities>
```

Without this, `VirtualAllocFromApp` with `PAGE_EXECUTE_READWRITE` fails
with `E_ACCESSDENIED`, and the PE loader cannot map a game's `.text`
section. The capability is granted automatically in Developer Mode and
during sideloading; it is **not** granted for Microsoft Store
submissions. This is the single most important reason the runner is a
sideload-only project.

### 2.4 Trust boundaries

```
+-------------------------------------------------------------+
| Xbox OS kernel (Xbox-specific hypervisor + secure kernel)  |
+-------------------------------------------------------------+
             |
             | AppContainer boundary
             v
+-------------------------------------------------------------+
| XboxWin32Runner.exe (UWP host process)                     |
|                                                             |
|   +-------------------+    +-----------------------------+  |
|   |  UWP shell        |    |   PE loader                 |  |
|   |  (CoreWindow,     |    |   (maps game .exe into      |  |
|   |   message pump,   |    |    RWX memory in this       |  |
|   |   D3D11 swap      |    |    process)                 |  |
|   |   chain)          |    +-----------------------------+  |
|   +-------------------+                |                    |
|             |                          v                    |
|             |              +-----------------------------+  |
|             |              |  Game .exe's .text section  |  |
|             |              |  (executed in this process, |  |
|             |              |   AppContainer identity)    |  |
|             |              +-----------------------------+  |
|             |                          |                    |
|             v                          v                    |
|   +-----------------------------------------------------+   |
|   |  Shim layer (in-process)                            |   |
|   |  kernel32 / user32 / gdi32 / d3d11 / xinput / ...   |   |
|   +-----------------------------------------------------+   |
+-------------------------------------------------------------+
             |
             v
+-------------------------------------------------------------+
| Xbox kernel-mode services (D3D11 GPU scheduler, XAudio2    |
| render endpoint, WinSock, file system)                     |
+-------------------------------------------------------------+
```

Everything above the lower AppContainer boundary runs with the runner's
UWP identity. The game's code executes **inside the runner's process**,
so any security-relevant operation the game attempts is bounded by the
runner's capabilities — which is exactly what we want.

---

## 3. Component Overview

The runner is a single UWP executable that links 34 `.cpp` files (at the
time of writing) into a single binary. The components are organized by
responsibility, not by DLL boundary: every component is statically linked
into the runner's process.

### 3.1 High-level block diagram

```
                +---------------------------------+
                |  uwp-shell/main.cpp             |
                |  (WinMain entry point)          |
                +---------------------------------+
                              |
                              v
                +---------------------------------+
                |  uwp-shell/App.{h,cpp}          |
                |  - LoadConfig(run.config)       |
                |  - Initialize (CoreWindow,     |
                |    D3D11Bridge, bridges)        |
                |  - Run (PE loader + msg pump)   |
                +---------------------------------+
                              |
        +---------------------+---------------------+---------------------+
        |                     |                     |                     |
        v                     v                     v                     v
+-----------------+   +-----------------+   +-----------------+   +-----------------+
| PeLoader        |   | ShimRegistry    |   | D3D11Bridge     |   | PathTranslator  |
| pe-loader/      |   | shims/          |   | d3d11-bridge/   |   | shims/kernel32/ |
| PeLoader.{h,cpp}|   | ShimRegistry.*  |   | D3D11Bridge.*   |   | PathTranslator.*|
+-----------------+   +-----------------+   +-----------------+   +-----------------+
        |                     |
        | queries              | queried by
        | ResolveExport()      | PeLoader.ProcessImports
        v                     ^
+-----------------------------------------------------------------------+
| Shim modules (23 files, ~843 REGISTER_SHIM entries)                   |
| shims/kernel32/  shims/user32/  shims/gdi32/  shims/advapi32/         |
| shims/shell32/   shims/shlwapi/ shims/ole32/  shims/winmm/            |
| shims/version/   shims/pass-through/  shims/stubs/  shims/legacy/     |
+-----------------------------------------------------------------------+
        |                     |                     |                     |
        v                     v                     v                     v
+-----------------+   +-----------------+   +-----------------+   +-----------------+
| XInputBridge    |   | KeyboardBridge  |   | AudioBridge     |   | GdiRenderer     |
| bridges/        |   | bridges/        |   | bridges/        |   | bridges/        |
| XInputBridge.*  |   | KeyboardBridge.*|   | AudioBridge.*   |   | GdiRenderer.*   |
+-----------------+   +-----------------+   +-----------------+   +-----------------+
        |
        v
+-----------------------------------------------------------------------+
| Xbox kernel-mode services                                              |
| D3D11 GPU scheduler | XAudio2 render | WinSock | NT file system       |
+-----------------------------------------------------------------------+
```

### 3.2 Component responsibilities

| Component              | Header                       | Responsibility                                                              |
|------------------------|------------------------------|-----------------------------------------------------------------------------|
| `App`                  | `uwp-shell/App.h`            | Application lifecycle: load config, initialize, run, teardown.              |
| `PeLoader`             | `pe-loader/PeLoader.h`       | Map a game's `.exe` into process memory; resolve imports; run entry point.  |
| `ShimRegistry`         | `shims/ShimRegistry.h`       | O(1) lookup of `(dll, func) → FARPROC`. Populated by `REGISTER_SHIM` macros. |
| Shim modules           | `shims/<module>/*.cpp`       | Implement the Win32 surface the game expects; delegate to UWP where possible. |
| `D3D11Bridge`          | `d3d11-bridge/D3D11Bridge.h` | Owns the real `ID3D11Device` / swap chain; hands raw pointers to shims.     |
| `XInputBridge`         | `bridges/XInputBridge.h`     | Polls `Windows.Gaming.Input::Gamepad` and feeds `XINPUT_STATE` to the shim. |
| `KeyboardBridge`       | `bridges/KeyboardBridge.h`   | Receives CoreWindow `KeyDown`/`KeyUp` events; feeds `GetAsyncKeyState`.     |
| `AudioBridge`          | `bridges/AudioBridge.h`      | Circular PCM buffer + render thread; submits to `IXAudio2SourceVoice`.      |
| `GdiRenderer`          | `bridges/GdiRenderer.h`      | Direct2D / DirectWrite backing for the gdi32 shim's HDC operations.         |
| `PathTranslator`       | `shims/kernel32/PathTranslator.h` | Maps virtual `C:\Game\` to UWP local folder; handles relative paths.   |

### 3.3 Data flow at startup

```
WinMain
  |
  v
App::Main()
  |
  +-- LoadConfig(run.config)         -> m_exePath, m_workingDir, m_cmdLine, m_dllSearchPath
  |
  +-- Initialize()
  |     |
  |     +-- ShimRegistry::Instance()    -> singleton constructed; all REGISTER_SHIM
  |     |                                  static initializers fire
  |     +-- PathTranslator::Instance().SetPhysicalRoot(LocalFolder)
  |     +-- PathTranslator::Instance().SetVirtualCwd(m_workingDir)
  |     +-- CoreWindow::GetForCurrentThread() / Activate
  |     +-- m_hwnd = WindowInterop::GetWindowHandle(coreWnd)    [PENDING - see HONEST_STATUS]
  |     +-- D3D11Bridge::Instance().Initialize(m_hwnd)
  |     +-- XInputBridge::Instance()  (touch)
  |     +-- KeyboardBridge::Instance().SetTargetWindow(m_hwnd)
  |     +-- AudioBridge::Instance()   (touch)
  |     +-- GdiRenderer::Instance()   (touch)
  |
  +-- Run()
        |
        +-- PeLoader.SetShimRegistry(&ShimRegistry::Instance())
        +-- PeLoader.AddDllSearchDir(...)  for each entry in m_dllSearchPath
        +-- CreateThread(GameThreadProc)
        |     |
        |     +-- PeLoader.LoadModuleFromPath(m_exePath)
        |     |     10-step pipeline (see Section 4)
        |     +-- PeLoader.RunExe(exe)
        |     |     -> calls exe's entry point
        |     |     -> game runs, calls into shims via its IAT
        |     +-- ctx.finished = true
        |
        +-- GetMessageW / TranslateMessage / DispatchMessageW pump (main thread)
        |     per-iteration:
        |       XInputBridge::Instance().Poll()
        |       D3D11Bridge::Instance().Present(1, 0)
        |       if (ctx.finished) PostQuitMessage(exitCode)
        |
        +-- WaitForSingleObject(gameThread, 5000ms)
        +-- AudioBridge.Shutdown()
        +-- D3D11Bridge.Shutdown()
        +-- return exitCode
```

---

## 4. PE Loader Pipeline

The PE loader is the heart of the runner. It performs the same job as the
Windows kernel's `ntoskrnl!LdrpMapDllNtFileName` path, but entirely in
user mode and using only AppContainer-safe primitives.

The loader is implemented in `pe-loader/PeLoader.{h,cpp}`. The public
entry points are `LoadModule(name)` (searches the DLL search path),
`LoadModuleFromPath(path)` (absolute path), and `RunExe(exe)` (calls the
entry point of a previously loaded EXE module).

### 4.1 The 11-step pipeline

Each step is a private method on `PeLoader`. The pipeline runs linearly;
if any step fails, the loader sets `LastError()` and returns `nullptr`.

```
LoadModuleFromPath(path)
  |
  1. MapFile(path)                -- ReadFileCallback reads the .exe bytes
  |
  2. ParseHeaders                 -- DOS / NT / Optional / Section headers
  |
  3. AllocateImage                -- VirtualAllocFromApp(PAGE_EXECUTE_READWRITE,
  |                                                    SizeOfImage)
  |
  4. CopyHeaders                  -- copy DOS + NT + Section headers to image base
  |
  5. CopySections                 -- copy each section's raw bytes to its VA
  |
  6. ApplyRelocations             -- IMAGE_REL_BASED_DIR64 (and HIGHLOW for x86)
  |
  7. ProcessImports               -- walk IMAGE_DIRECTORY_ENTRY_IMPORT,
  |                                  rewrite IAT with shim addresses
  |
  8. ProcessDelayImports          -- walk IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,
  |                                  rewrite delay-IAT with shim addresses
  |
  9. ProcessTls                   -- run TLS callbacks (DLL_PROCESS_ATTACH)
  |
 10. RegisterExceptionTables      -- RtlAddFunctionTable(.pdata)
  |
 11. CallDllMain(DLL_PROCESS_ATTACH)  -- if module is a DLL
     -- OR --
     RunExe(exe)                      -- if module is an EXE
```

### 4.2 Step 1: MapFile

`MapFile` reads the entire `.exe` or `.dll` into a `std::vector<uint8_t>`
in memory. The runner does **not** use memory-mapped files because the
AppContainer restricts which paths can be opened via `CreateFileMapping`,
and a plain `ReadFile` is simpler to reason about.

The file-IO callback (`SetReadFileCallback`) is overridable for testing
— the unit tests inject a callback that returns synthesized PE bytes
without touching the disk.

### 4.3 Step 2: ParseHeaders

`ParseHeaders` validates the PE structure and extracts:

- DOS header (`IMAGE_DOS_HEADER`): magic `0x5A4D` ("MZ"), `e_lfanew`.
- NT signature (`0x00004550` = "PE\0\0").
- `IMAGE_FILE_HEADER`: `Machine` (must be `0x8664` for x64; `0x014C` for
  x86 — the runner accepts both but only runs x64).
- `IMAGE_OPTIONAL_HEADER`: `Magic` (`0x20B` for PE32+, `0x10B` for PE32),
  `ImageBase`, `SizeOfImage`, `SizeOfHeaders`, `AddressOfEntryPoint`,
  `DataDirectory[16]`.
- `IMAGE_SECTION_HEADER[]`: per-section `VirtualAddress`, `VirtualSize`,
  `PointerToRawData`, `SizeOfRawData`, characteristics.

The parser stores the section-header array pointer (`outSectionHeaders`)
for use by `RvaToFileOffset` and `FindSection` in later steps.

### 4.4 Step 3: AllocateImage

`AllocateImage` calls `VirtualAllocFromApp` with:

- `dwSize = SizeOfImage`
- `flAllocationType = MEM_RESERVE | MEM_COMMIT`
- `flProtect = PAGE_EXECUTE_READWRITE`

The `codeGeneration` capability (declared in
`Package.appxmanifest`) is required for the `PAGE_EXECUTE_READWRITE`
flag. Without it, `VirtualAllocFromApp` returns `nullptr` and the loader
fails.

We use a single `PAGE_EXECUTE_READWRITE` allocation for the entire image
rather than per-section permissions (`.text` RX, `.data` RW, etc.) for
two reasons:

1. Per-section permissions would require multiple `VirtualProtectFromApp`
   calls and careful page-alignment of section boundaries — significant
   added complexity for marginal security benefit (the game's code is
   already running in our address space).
2. Some games use self-modifying code or JIT (e.g. Lua interpreters
   embedded in the engine), which require `W^X` to be relaxed.

The image base returned by `VirtualAllocFromApp` is **not** the
preferred base from the PE header. Step 6 (`ApplyRelocations`) adjusts
absolute references to point at the new base.

### 4.5 Step 4: CopyHeaders

`CopyHeaders` copies the first `SizeOfHeaders` bytes (DOS header + NT
headers + section headers + DOS stub) verbatim from the source file to
the image base. The DOS stub is not used at runtime but is preserved for
compatibility with tools that scan the in-memory image.

### 4.6 Step 5: CopySections

For each `IMAGE_SECTION_HEADER`:

- Compute `destVA = imageBase + section.VirtualAddress`.
- Compute `srcOffset = section.PointerToRawData`.
- Compute `copySize = min(section.SizeOfRawData, section.VirtualSize)`.
- `memcpy(destVA, src + srcOffset, copySize)`.
- Zero-fill the remainder (`section.VirtualSize - copySize` bytes) —
  this is the `.bss` area, which has no raw bytes in the file.

Sections with no `SizeOfRawData` (pure `.bss`) are zeroed entirely.

### 4.7 Step 6: ApplyRelocations

`ApplyRelocations` walks the base relocation table
(`IMAGE_DIRECTORY_ENTRY_BASERELOC`, DataDirectory index 5). Each block
contains a `IMAGE_BASE_RELOCATION` header (`VirtualAddress` + `SizeOfBlock`)
followed by an array of 16-bit `relocation entries`:

- High 4 bits: relocation type.
- Low 12 bits: offset within the page.

For x64, the runner handles:

| Type                          | Value | Action                                                |
|-------------------------------|-------|-------------------------------------------------------|
| `IMAGE_REL_BASED_ABSOLUTE`    | 0     | Skip (padding).                                       |
| `IMAGE_REL_BASED_HIGHLOW`     | 3     | 32-bit fixup: add `delta` to a `DWORD`.               |
| `IMAGE_REL_BASED_DIR64`       | 10    | 64-bit fixup: add `delta` to a `ULONGLONG`.           |

(`delta = actualBase - preferredBase`.)

Any other type is reported in `LastError()` and aborts the load. In
practice, modern x64 PEs only emit ABSOLUTE / HIGHLOW / DIR64.

### 4.8 Step 7: ProcessImports

`ProcessImports` walks `IMAGE_DIRECTORY_ENTRY_IMPORT` (DataDirectory
index 1), which is an array of `IMAGE_IMPORT_DESCRIPTOR` (20 bytes each):

```
OriginalFirstThunk  (RVA to INT — Import Name Table)
TimeDateStamp       (0 if not bound)
ForwarderChain      (0 if no forwarders)
Name                (RVA to DLL name, null-terminated ASCII)
FirstThunk          (RVA to IAT — Import Address Table)
```

For each descriptor:

1. Read the DLL name from `Name`.
2. Walk the thunk array at `OriginalFirstThunk` (fall back to
   `FirstThunk` if `OriginalFirstThunk == 0`).
3. For each 8-byte thunk value:
   - If `(thunk & IMAGE_ORDINAL_FLAG64) != 0`: import by ordinal; the
     ordinal is `(thunk & 0xFFFF)`. Resolve via
     `ShimRegistry::ResolveExport(dllName, ordinal)` — or, if no shim is
     registered, via `LoadModule(dllName) + GetExportByOrdinal`.
   - Otherwise: the low 31 bits are an RVA to an
     `IMAGE_IMPORT_BY_NAME` (a 2-byte hint + null-terminated ASCII
     name). Resolve via `ShimRegistry::ResolveExport(dllName, name)`.
4. Write the resolved function pointer into the IAT slot at
   `FirstThunk + i * 8`.

If neither a shim nor a loaded module provides the function, the IAT
slot is left as `0`. The game will crash on first call, but the loader
succeeds — this is intentional, so the runner can produce a coverage
report listing missing imports (via `tools/pe_import_scanner.py`).

### 4.9 Step 8: ProcessDelayImports

`ProcessDelayImports` walks `IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT`
(DataDirectory index 13). Each `IMAGE_DELAYLOAD_DESCRIPTOR` is 32 bytes:

```
Attributes          (bit 0 = RVA-based)
DllNameRVA          (RVA to DLL name)
ModuleHandleRVA     (RVA to a HMODULE slot — filled in at load time)
ImportAddressTableRVA (RVA to IAT)
ImportNameTableRVA  (RVA to INT)
BoundImportAddressTableRVA  (0 if not bound)
UnloadDelayInformationTableRVA (0 if not unloadable)
TimeDateStamp       (0 if not bound)
```

The processing mirrors Step 7, but writes into the delay-IAT instead of
the standard IAT. Delay-load thunks in the game's `.text` section call
through this table; by pre-filling it, we short-circuit the
delay-load helper that would otherwise call `LoadLibrary` (which is
ACL'd to AppContainer callers and often fails).

### 4.10 Step 9: ProcessTls

`ProcessTls` walks `IMAGE_DIRECTORY_ENTRY_TLS` (DataDirectory index 9).
For 64-bit PEs, the TLS directory is a 40-byte `IMAGE_TLS_DIRECTORY64`:

```
StartAddressOfRawData
EndAddressOfRawData
AddressOfIndex          (RVA to a DWORD that holds the per-thread TLS slot index)
AddressOfCallBacks      (RVA to a null-terminated array of PIMAGE_TLS_CALLBACK)
SizeOfZeroFill
Characteristics
```

For each callback in the `AddressOfCallBacks` array (until a `nullptr`
entry), the loader invokes:

```
callback(imageBase, DLL_PROCESS_ATTACH, nullptr);
```

TLS callbacks run before the entry point. C++ static initializers in
the game (and its statically-linked CRT) rely on this for ordering.

The loader assigns each loaded module a unique TLS index from
`m_tlsIndexCounter`. On subsequent `NotifyThreadAttach` / `NotifyThreadDetach`
calls (driven by `DllMain(DLL_THREAD_ATTACH/DETACH)` notifications), each
loaded module's TLS callbacks are re-invoked.

### 4.11 Step 10: RegisterExceptionTables

`RegisterExceptionTables` walks `IMAGE_DIRECTORY_ENTRY_EXCEPTION`
(DataDirectory index 3). This directory contains an array of
`IMAGE_RUNTIME_FUNCTION_ENTRY` (8 bytes for x64):

```
BeginAddress       (RVA of function start)
EndAddress         (RVA of function end, or UnwindData for x64)
UnwindData         (RVA to UNWIND_INFO, or EndAddress for x64)
```

The loader passes the array to `RtlAddFunctionTable`, which registers it
with the OS's stack-unwinder. This is required for C++ exceptions,
`__try`/`__except`, and stack traces to work correctly.

### 4.12 Step 11: CallDllMain / RunExe

- For a **DLL**: the loader calls `CallDllMain(mod, DLL_PROCESS_ATTACH)`,
  which invokes the module's entry point (typically `_DllMainCRTStartup`
  → `DllMain`) with `DLL_PROCESS_ATTACH` and `lpvReserved = 0`.
- For an **EXE**: `RunExe` invokes the entry point with the CRT-style
  signature (`(void*)hinst, (void*)prev, (char*)cmdline, int showCmd`).
  On x64 the exact calling convention is `__cdecl` with the four
  arguments in `RCX, RDX, R8, R9`. The loader reads the EXE's
  `AddressOfEntryPoint` (added to the image base) and calls it as a
  function pointer.

The game's main thread runs synchronously inside `RunExe` until the
game exits. The runner's `App::Run` method spawns this on a background
thread (see Section 3.3) so the main thread can drive the UWP message
pump.

---

## 5. IAT Rewriting

The IAT (Import Address Table) is the single most important data
structure the PE loader rewrites. Without it, the game's `.text` section
contains references to `kernel32!CreateFileW` at the *preferred* base
address of `kernel32.dll` — which is not where our shim lives.

### 5.1 How the IAT works

When the OS loader (or, in our case, `PeLoader`) processes an import
descriptor, it:

1. Reads the function name (or ordinal) from the INT.
2. Resolves it to a function pointer (`FARPROC`).
3. Writes that pointer into the IAT slot.

The game's compiled code calls through the IAT:

```asm
; game code calling CreateFileW
lea     rax, [__imp_CreateFileW]    ; __imp_CreateFileW is in the IAT
call    qword ptr [rax]             ; indirect call through IAT slot
```

By controlling what the loader writes into the IAT, we control which
function actually runs when the game calls `CreateFileW`.

### 5.2 The shim registry as a fake DLL export table

`ShimRegistry` is a flat `unordered_map<(dllLower, funcLower), FARPROC>`.
The key is case-insensitive on both halves (DLL name and function name),
matching the Windows loader's behavior.

Each shim module registers its functions at namespace scope via the
`REGISTER_SHIM` macro:

```cpp
REGISTER_SHIM("kernel32", "CreateFileW", (FARPROC)&Shim_CreateFileW);
```

The macro expands to a static initializer that calls
`ShimRegistry::Instance().Register(...)` before `main` runs. By the time
`App::Initialize` returns, all 800+ shims are registered.

### 5.3 The resolution path

`PeLoader::ResolveImport(dllName, funcName)`:

1. Normalize `dllName` to lowercase, strip `.dll`/`.exe`/`.sys` etc.
2. Normalize `funcName` to lowercase.
3. Query `ShimRegistry::ResolveExport(dllName, funcName)`.
4. If found, return the FARPROC.
5. If not found, check if a game-shipped DLL with this name is already
   loaded (`m_byName`). If so, query `GetExport(mod, funcName)`.
6. If not loaded, attempt `EnsureDependencyLoaded(dllName)` — search
   `m_searchDirs` for `<dllName>.dll`, recursively `LoadModule` it.
7. If still not found, return 0 (the IAT slot is left null).

For ordinal imports (`ResolveImportByOrdinal`), the same path is taken
but with `GetExportByOrdinal` instead of `GetExport`. Most Win32 system
DLLs do not export by ordinal, so this path is rare; the few that do
(e.g. some `ws2_32` ordinals) are handled by the
`shims/pass-through/Ws2_32Shim.cpp` module.

### 5.4 Why we don't use real DLLs

Why not just `LoadLibrary("kernel32.dll")` and let the OS loader handle
imports? Three reasons:

1. **AppContainer ACLs**: System DLLs like `kernel32.dll` export the
   same set of functions to AppContainer processes as to desktop
   processes, but **some** functions (e.g. `CreateFileW` on paths
   outside the package) silently fail or return `ACCESS_DENIED`. The
   shim layer wraps these calls and routes them through
   `PathTranslator` so they succeed inside the AppContainer.
2. **Missing exports**: Some Win32 functions (e.g. `RegOpenKeyExA` on
   `HKLM`) are not exported to AppContainer processes at all. The shim
   provides a reimplementation that returns `ERROR_FILE_NOT_FOUND`
   instead of failing to link.
3. **Behavior translation**: Some functions need behavior translation
   rather than just path translation. For example,
   `GetLogicalDrives()` in the real kernel32 returns a bitmask
   including `C:` and `D:`; in our shim, it returns only `C:` (the
   virtual drive). `GetDiskFreeSpaceEx` translates the path through
   `PathTranslator` before delegating.

### 5.5 The shim signature requirement

Every shim function must have the **exact same signature** as the real
Win32 function it replaces, including calling convention (`__stdcall`
for most Win32, `__cdecl` for CRT). On x64, calling conventions are
simpler (everything is the unified x64 calling convention), but
variadic functions and inline-assembly call sites still require careful
matching.

The shim modules enforce this by declaring each shim as
`extern "C" RET __stdcall Shim_FuncName(args)` — the `extern "C"`
prevents name mangling, and `__stdcall` matches Win32's convention.

---

## 6. Path Translation

The `PathTranslator` (`shims/kernel32/PathTranslator.{h,cpp}`) maps
virtual Win32 paths to real UWP-local-folder paths so the game's file
I/O works transparently inside the AppContainer.

### 6.1 The virtual C: drive

The runner presents the game with a single virtual drive: `C:\`. The
root of `C:\` maps to the UWP local folder:

```
Virtual:        C:\
                |
                +-- Game\
                |     +-- Binaries\Win64\HalfSword.exe
                |     +-- Engine\Binaries\Win64\*.dll
                |     +-- ...
                |
                +-- Users\
                |     +-- <user>\AppData\Local\...
                |
                +-- Windows\
                      +-- System32\   (mapped to a read-only list of fake DLLs)

Real:           %LOCALAPPDATA%\Packages\Zai.XboxWin32Runner_...\LocalState\
                |
                +-- Game\
                +-- Users\
                +-- Windows\
```

The mapping is set at startup by `App::Initialize`:

```cpp
pathTx.SetPhysicalRoot(L"C:\\Users\\...\\LocalState\\");
pathTx.SetVirtualCwd(m_workingDir);   // typically "C:\\Game\\"
```

### 6.2 Translation rules

`PathTranslator::TranslateToReal(virtualPath)` applies the following
rules in order:

1. **UNC paths** (`\\server\share\...`): translated to
   `\\server\share\...` inside the physical root's `Network\` subfolder.
   Games rarely use UNC, so this is mostly a safety net.
2. **Drive-letter paths** (`C:\foo`, `D:\bar`): the drive letter is
   stripped, the remainder is appended to the physical root. `D:\` and
   later letters map to subfolders `D\`, `E\`, etc. inside the physical
   root (the runner does not support multiple real drives).
3. **Relative paths** (`foo\bar`, `.\foo`, `..\bar`): resolved against
   the virtual cwd, then translated. `..` is supported but clamped at
   the virtual root (you cannot escape `C:\`).
4. **Bare filenames** (`save.dat`): resolved as a relative path against
   the virtual cwd.

### 6.3 Example

```
Virtual cwd:           C:\Game\
Path opened by game:   ..\Engine\Config\DefaultEngine.ini

Step 1: Resolve to absolute virtual path
        C:\Game\..\Engine\Config\DefaultEngine.ini
        → C:\Engine\Config\DefaultEngine.ini

Step 2: Strip drive letter
        \Engine\Config\DefaultEngine.ini

Step 3: Append to physical root
        C:\Users\...\LocalState\Engine\Config\DefaultEngine.ini
```

### 6.4 Who calls TranslateToReal?

Every shim that opens a file system object calls `TranslateToReal`
before delegating to the real UWP API. Examples:

- `Shim_CreateFileW(lpFileName, ...)` →
  `CreateFileW(PathTranslator::Instance().TranslateToReal(lpFileName), ...)`
- `Shim_GetFileAttributesW(path)` →
  `GetFileAttributesW(TranslateToReal(path))`
- `Shim_DeleteFileW(path)` → `DeleteFileW(TranslateToReal(path))`
- `Shim_MoveFileW(a, b)` → translates both, then `MoveFileW`
- `Shim_FindFirstFileW(pattern)` → translates pattern, then
  `FindFirstFileW`

The `kernel32` shim alone has 30+ functions that route through
`PathTranslator`. The `shell32` and `shlwapi` shims additionally
translate `CSIDL_*` and known-folder GUIDs through the same mechanism
(`SHGetFolderPath` returns the virtual path; the game then opens it
through the kernel32 shim, which translates a second time).

### 6.5 The reverse direction

`TranslateToVirtual(realPath)` is the inverse — used only for
diagnostic output (e.g. logging "game opened save.dat (real:
C:\Users\...\LocalState\Game\save.dat)"). It is not on any hot path.

---

## 7. D3D11 Bridge

The D3D11 bridge (`d3d11-bridge/D3D11Bridge.{h,cpp}`) owns the **single
real** D3D11 device, immediate context, and swap chain for the entire
runner process. The `shims/pass-through/D3D11Shim.cpp` module hands out
raw pointers from the bridge to the game's IAT — so the game sees the
same device the runner uses to `Present()`.

### 7.1 Why a single shared device?

The Xbox GPU scheduler enforces a single D3D11 device per process for
UWP apps. Creating a second device fails with `DXGI_ERROR_DEVICE_HUNG`
or `E_ACCESSDENIED`. The runner therefore creates one device at startup
and shares it with the game.

### 7.2 Initialization

`D3D11Bridge::Initialize(HWND hwnd)`:

1. Build a feature-level list with a single entry:
   `D3D_FEATURE_LEVEL_11_0`. (UWP on Xbox caps at 11_0; higher levels
   return `E_INVALIDARG`.)
2. Set `D3D11_CREATE_DEVICE_BGRA_SUPPORT` (required for interop with
   Direct2D, which the GdiRenderer uses).
3. Build a `DXGI_SWAP_CHAIN_DESC`:
   - `BufferCount = 2` (double-buffered).
   - `BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM`.
   - `BufferDesc.RefreshRate = { 60, 1 }`.
   - `BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT`.
   - `SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD`.
   - `SampleDesc.Count = 1` (no MSAA — the game controls its own
     MSAA via `OMSetRenderTargets`).
   - `Windowed = TRUE` (UWP is always windowed; the OS manages the
     full-screen transition).
   - `OutputWindow = hwnd`.
4. Call `D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
   nullptr, createFlags, featureLevels, 1, D3D11_SDK_VERSION,
   &swapChainDesc, &m_swapChain, &m_device, nullptr, &m_context)`.

### 7.3 Feature-level clamping

The D3D11 shim's `D3D11CreateDevice` and `D3D11CreateDeviceAndSwapChain`
exports intercept the game's requested feature levels and clamp them to
`11_0` before delegating to the bridge. If the game requests
`D3D_FEATURE_LEVEL_11_1` or `12_0`, the shim silently downgrades.

UE5's D3D11 RHI probes for `11_1` and `12_0` at startup; without
clamping, the probe would fail and the RHI would fall back to a
software device. With clamping, the probe succeeds at `11_0` and the
RHI initializes normally.

### 7.4 The present loop

The runner's main thread calls `D3D11Bridge::Present(1, 0)` once per
message-pump iteration. The arguments are:

- `SyncInterval = 1`: VSync on (one refresh per present).
- `Flags = 0`: no DXGI_PRESENT_TEST, no DXGI_PRESENT_DO_NOT_SEQUENCE.

The game's own `Present()` calls (made through the shim) become no-ops:
the shim checks if the swap chain is already managed by the bridge, and
if so, returns `S_OK` without calling the real `Present` (which would
double-flip and cause stuttering).

### 7.5 Limitations

- **No multi-swap-chain support**: The bridge owns exactly one swap
  chain. Games that create their own swap chain (e.g. for an off-screen
  render target) will receive the bridge's swap chain instead, which is
  incorrect. UE5 does not do this in its D3D11 RHI; other engines might.
- **No `DXGI_PRESENT_TEST`**: The bridge's `Present` ignores the test
  flag. Games that probe the present status before flipping will see
  the bridge flip unconditionally.
- **No `IDXGIFactory::CreateSwapChainForComposition`**: The bridge only
  supports the `CreateSwapChainForHwnd` (windowed) path. UWP
  composition swap chains are not exposed.

---

## 8. I/O Bridges

The I/O bridges translate between the Xbox's UWP-native input/audio
surface and the game's expected Win32 surface. There are four bridges:

### 8.1 XInputBridge

| Aspect           | Detail                                                  |
|------------------|---------------------------------------------------------|
| Header           | `bridges/XInputBridge.h`                                |
| Polling cadence  | Once per message-pump iteration (target: 250 Hz).       |
| Source           | `Windows.Gaming.Input::Gamepad::Gamepads()` enumeration.|
| Output           | `XINPUT_STATE[4]` cached on the singleton.              |
| Vibration        | `Gamepad::Vibration` (left/right motor, 0..1 scaling).  |

The bridge enumerates up to 4 connected gamepads per `Poll()` call. For
each gamepad, it calls `GetCurrentReading()` and translates the
`GamepadReading` struct into the Win32 `XINPUT_GAMEPAD` layout:

```
GamepadReading.Buttons (bitmask)  →  XINPUT_GAMEPAD.wButtons (bitmask)
GamepadReading.LeftThumbstickX    →  XINPUT_GAMEPAD.sThumbLX  (SHORT, ±32767)
GamepadReading.LeftThumbstickY    →  XINPUT_GAMEPAD.sThumbLY  (SHORT, ±32767)
GamepadReading.RightThumbstickX   →  XINPUT_GAMEPAD.sThumbRX  (SHORT, ±32767)
GamepadReading.RightThumbstickY   →  XINPUT_GAMEPAD.sThumbRY  (SHORT, ±32767)
GamepadReading.LeftTrigger        →  XINPUT_GAMEPAD.bLeftTrigger  (BYTE, 0..255)
GamepadReading.RightTrigger       →  XINPUT_GAMEPAD.bRightTrigger (BYTE, 0..255)
```

The `xwr::XInputShim::XInputGetState` function reads from the bridge's
cache rather than calling `XInputGetState` directly. This decouples the
game's polling rate from the WinRT enumeration rate.

### 8.2 KeyboardBridge

| Aspect           | Detail                                                  |
|------------------|---------------------------------------------------------|
| Header           | `bridges/KeyboardBridge.h`                              |
| Source           | `CoreWindow::KeyDown` / `KeyUp` / `CharacterReceived` events. |
| Output           | 256-byte `m_keyState[]` (bit 0x80 = pressed).           |
| Per-key events   | Posts `WM_KEYDOWN` / `WM_KEYUP` / `WM_CHAR` to the target HWND. |

The UWP host wires `CoreWindow::KeyDown` →
`KeyboardBridge::OnKeyDown(vkey, scanCode)` and `KeyUp` →
`OnKeyUp(...)`. Each handler updates the 256-byte state array and posts
a `WM_KEYDOWN`/`WM_KEYUP` message to the target HWND (set via
`SetTargetWindow`).

`lParam` packing follows the Win32 convention:

```
bits  0..15   repeat count (always 1)
bits 16..23   scan code
bit  24       extended key flag
bits 25..28   reserved
bit  29       context code (always 0 — Alt not pressed)
bit  30       previous state (1 if key was already down)
bit  31       transition state (0 = KEYDOWN, 1 = KEYUP)
```

The shim `shim_GetAsyncKeyState(vkey)` returns `0x8000` if the key is
pressed (bit 0x8000 = "currently pressed"). `shim_GetKeyState(vkey)`
adds the toggle bit (CapsLock/NumLock — currently always 0, see
Limitations). `shim_GetKeyboardState(buf)` `memcpy`s the 256-byte
table into the caller's buffer.

### 8.3 AudioBridge

| Aspect             | Detail                                                |
|--------------------|-------------------------------------------------------|
| Header             | `bridges/AudioBridge.h`                               |
| Buffer size        | 2 seconds of audio (frame-aligned).                   |
| Push API           | `PushSamples(const void* pcm, size_t bytes)` (thread-safe). |
| Render thread      | Pulls ~10 ms per iteration; waits 10 ms on shutdown event. |
| Endpoint           | `IXAudio2SourceVoice` (created lazily by the xaudio2 shim). |

The audio bridge is a lock-free single-producer, single-consumer ring
buffer. The producer side (the game's audio thread, via the winmm
`waveOutWrite` shim) calls `PushSamples` under a `CRITICAL_SECTION`. The
consumer side (the bridge's render thread) pulls ~10 ms of audio per
iteration and submits it to an `IXAudio2SourceVoice` owned by the
xaudio2 shim.

Overflow handling: on buffer-full, `PushSamples` drops the new samples.
This is intentional — dropping new audio is less audible than stuttering
on old audio (which the human ear perceives as a glitch).

Underflow handling: on buffer-empty, the render thread sleeps 10 ms and
retries. The XAudio2 source voice will play silence during the gap.

### 8.4 GdiRenderer

| Aspect             | Detail                                                |
|--------------------|-------------------------------------------------------|
| Header             | `bridges/GdiRenderer.h`                               |
| Backing            | `ID2D1Factory*` + `IDWriteFactory*` (lazy-created).   |
| Per-HDC state      | Render target, pen position, text color, bk color.    |
| Supported ops      | `FillRect`, `Rectangle`, `LineTo`, `MoveToEx`, `TextOutW`, `BitBlt`. |

The GdiRenderer translates GDI operations into Direct2D / DirectWrite
calls. Each `HDC` (the gdi32 shim allocates fake HDC values from a
counter) has its own `ID2D1DCRenderTarget` bound to the HDC's clip box.

In practice, few modern games use GDI for rendering — most use D3D
directly. The GdiRenderer exists primarily so that the game's splash
screen, debug overlays, or copy-protection dialogs do not crash.

---

## 9. Shim Coverage Strategy

The 23 shim modules are organized into four categories, based on what
they do with each call:

### 9.1 Categories

| Category        | Count | Behavior                                                                                  |
|-----------------|-------|-------------------------------------------------------------------------------------------|
| **real**        | 9     | Reimplement the function on top of UWP-safe APIs. Path translation, virtual handle tables, in-memory state. |
| **pass-through**| 6     | Delegate directly to the real UWP API, with minor argument massaging (e.g. clamp feature levels). |
| **stub**        | 5     | Return failure (false / 0 / nullptr / `E_NOTIMPL`) so the game degrades gracefully.       |
| **legacy**      | 4     | Return fake COM pointers and success HRESULTs so probe-style init code does not crash.    |

### 9.2 Why each category exists

- **Real** shims exist for functions that the UWP surface does not
  expose at all (e.g. registry access, `SetWindowsHookEx`, raw GDI on
  HDCs). The shim either reimplements the function (e.g. an in-memory
  registry) or returns a sensible failure.

- **Pass-through** shims exist for functions that the UWP surface
  exposes with the same signature (e.g. D3D11, XInput, XAudio2, WinSock).
  The shim exists only to ensure the game's IAT can be rewritten — the
  actual call goes straight through to the real UWP API.

- **Stub** shims exist for functions that the game calls but that have
  no UWP equivalent (e.g. Steam API, Discord RPC, DirectSound,
  MediaFoundation). Returning failure lets the game detect "feature not
  present" and proceed (e.g. disable achievements, run without Steam
  integration).

- **Legacy** shims exist for old graphics APIs (D3D9, D3D8, DirectDraw,
  OpenGL) that some games probe at startup before falling back to
  D3D11. The shim returns a fake COM object pointer and `S_OK`, so the
  probe succeeds, the game's init code creates a "device", then the
  game's frame loop tries to render — at which point the game's own
  fallback-to-D3D11 path kicks in. (This is fragile; see Limitations.)

### 9.3 Per-DLL details

See `docs/SHIM_COVERAGE.md` for a per-DLL table of registered functions,
categories, and known limitations.

---

## 10. Target Game: Half Sword

Half Sword is the runner's reference target. Its characteristics drove
several design decisions.

### 10.1 Game profile

| Property             | Value                                                    |
|----------------------|----------------------------------------------------------|
| Engine               | Unreal Engine 5.4.4                                      |
| Architecture         | 64-bit (x64)                                             |
| Rendering API        | D3D11 (RHI selectable via `-dx11` command-line flag)     |
| D3D12 RHI            | Available but not used (UWP caps at FL 11_0; D3D12 path is unmaintained). |
| Audio                | FMOD + XAudio2 backend                                   |
| Input                | XInput (controllers) + Win32 keyboard/mouse              |
| Anti-cheat           | None                                                     |
| DRM                  | Steam stub (wrapped SteamAPI calls)                      |
| Multiplayer          | Single-player only                                       |
| Save location        | `%USERPROFILE%\Saved Games\HalfSword\`                   |
| Config location      | `%LOCALAPPDATA%\HalfSword\Saved\Config\Windows\`         |

### 10.2 Why Half Sword is a good fit

- **D3D11**: The runner's D3D11 bridge is the maintained rendering path.
  Half Sword ships with a D3D11 RHI that initializes cleanly at FL 11_0.
- **No anti-cheat**: The runner cannot load kernel-mode drivers, so any
  anti-cheat would block the game. Half Sword has none.
- **Steam stub only**: The Steam stub calls `SteamAPI_RestartAppIfNecessary`,
  which the runner's Steam shim returns `0` (false) from — telling the
  game "Steam is not required, continue". The game then runs without
  Steam integration (no achievements, no Steam overlay, no multiplayer
  matchmaking).
- **FMOD + XAudio2**: The audio path goes through the AudioBridge →
  XAudio2. FMOD's XAudio2 backend writes PCM frames via `IXAudio2SourceVoice::SubmitSourceBuffer`,
  which the runner intercepts at the xaudio2 shim level and routes into
  the bridge's ring buffer.

### 10.3 Known compatibility gaps

- **`SHGetKnownFolderPath`**: Half Sword's save path uses
  `FOLDERID_SavedGames`. The shell32 shim translates this to
  `C:\Users\<user>\Saved Games\HalfSword\` inside the virtual drive —
  the runner does not yet auto-create this directory, so the first save
  may fail.
- **`GetUserNameW`**: Half Sword reads the current user's name for the
  save path. The advapi32 shim returns a hardcoded `XboxUser`. The
  resulting path (`C:\Users\XboxUser\Saved Games\HalfSword\`) is
  correct, but the user must create it manually before launch (see
  `COMPILE.md` Section 11.3).
- **FMOD bank loading**: FMOD loads `.bank` files via standard file I/O
  (works through the kernel32 shim). However, FMOD's streaming system
  uses `CreateFileMappingW` for memory-mapped access, which the UWP
  surface restricts. The shim falls back to `ReadFile` on failure, but
  this may cause audio stutter on streaming sounds.

---

## 11. Limitations and Future Work

This section catalogues what the runner does **not** do, with brief
explanations. For an up-to-date "what works today" status, see
`docs/HONEST_STATUS.md`.

### 11.1 What we don't attempt

- **CPU emulation**: The Xbox Series X|S CPU is x86-64, so no emulation
  is needed. The runner would not work on ARM-based Xbox variants (there
  are none currently).
- **32-bit x86 games**: The PE loader accepts both PE32 and PE32+ files,
  but the shim signatures are tuned for x64. 32-bit support would
  require separate shim files with `__stdcall` calling conventions and
  32-bit pointer types.
- **Direct3D 12**: UWP on Xbox caps D3D11 at FL 11_0; D3D12 is available
  but requires UWP-specific swap-chain construction that the runner does
  not yet implement. Games that require D3D12 (e.g. UE5 Nanite demos
  with the D3D12 RHI) will not run.
- **Multi-process games**: AppContainer does not allow
  `CreateProcess` for arbitrary child processes. Games that spawn
  worker processes (e.g. some MMO launchers) cannot run.
- **Kernel-mode anti-cheat**: EAC, BattlEye, etc. require kernel-mode
  drivers, which AppContainer cannot load. Games using these systems
  cannot run.
- **DRM circumvention**: The runner does not strip DRM. A game that
  requires Steam to be running will see the Steam shim return "not
  running" and may refuse to launch.

### 11.2 Known limitations

- **UWP shell does not yet create a CoreWindow**: The `App::Initialize`
  method has a marked TODO where the CoreWindow + WindowInterop HWND
  interop should be wired. Until this is done, the D3D11 bridge's
  `Initialize(0)` returns false, and the swap chain is not created.
  (See `HONEST_STATUS.md` Phase 1.)
- **Keyboard input is not yet wired**: The CoreWindow `KeyDown`/`KeyUp`
  event handlers are not yet registered. The KeyboardBridge exists and
  compiles, but no events reach it.
- **Audio render thread is stubbed**: The AudioBridge's render thread
  drains the ring buffer but does not submit to `IXAudio2SourceVoice`.
  Audio is silent.
- **GDI drawing is best-effort**: The GdiRenderer implements the common
  operations (`FillRect`, `TextOut`, `BitBlt`) but not the full GDI
  surface. Games that use exotic ROPs or region clipping may render
  incorrectly.
- **No real test against Half Sword binaries**: The project does not
  ship Half Sword's `.exe` (for legal reasons), so the PE loader has
  only been tested against synthetic PEs and small test programs.
- **PathTranslator's physical root is lazy-populated**: In real builds,
  the physical root is not set until the first `TranslateToReal` call
  queries `ApplicationData::Current().LocalFolder`. If a shim is called
  before this lazy init completes, the path translation fails. The fix
  is to call `SetPhysicalRoot` explicitly inside `App::Initialize`.

### 11.3 Future work

Items roughly in priority order:

1. Wire CoreWindow + WindowInterop HWND into `App::Initialize`.
2. Wire CoreWindow `KeyDown`/`KeyUp`/`CharacterReceived` into
   `KeyboardBridge`.
3. Wire a 250 Hz timer (or per-message-pump-iteration call) into
   `XInputBridge::Poll()`.
4. Implement the AudioBridge render thread's XAudio2 source-voice
   submission.
5. Run `pe_import_scanner.py` against Half Sword's actual `.exe`,
   audit the coverage report, fill in missing shim functions.
6. Implement D3D12 → D3D11 fallback (for games that ship a D3D12-only
   RHI).
7. Implement `CreateFileMappingW` / `MapViewOfFile` shim that routes
   through `VirtualAllocFromApp` + `ReadFile` (for FMOD streaming).
8. Add per-section permissions (`.text` RX, `.data` RW) for defense in
   depth.
9. Implement the Xbox push endpoint in `host_agent.py` (currently a
   stub).
10. Add a static HTML dashboard to the host agent for browsing games +
    coverage.

---

## Appendix A: Source file inventory

```
pe-loader/
    PeLoader.h                - Public API
    PeLoader.cpp              - 11-step pipeline implementation
shims/
    CommonPre.h               - Forced-include header
    Win32Types.h              - Supplemental constants
    ShimRegistry.h            - (dll, func) -> FARPROC registry
    ShimRegistry.cpp
    kernel32/
        Kernel32Shim.cpp      - 149 REGISTER_SHIM entries
        PathTranslator.h
        PathTranslator.cpp
    user32/User32Shim.cpp     - 137
    gdi32/Gdi32Shim.cpp       - 124
    advapi32/Advapi32Shim.cpp - 41
    shell32/Shell32Shim.cpp   - 19
    shlwapi/ShlwapiShim.cpp   - 51
    ole32/Ole32Shim.cpp       - 36
    winmm/WinmmShim.cpp       - 41
    version/VersionShim.cpp   - 12
    pass-through/
        D3D11Shim.cpp         - 4
        D3D12Shim.cpp         - 5
        XInputShim.cpp        - 6 (x 4 DLL names = 24 effective)
        XAudio2Shim.cpp       - 2
        Ws2_32Shim.cpp        - 17
        MiscPassthroughShim.cpp - 11
    stubs/
        SteamShim.cpp         - 38 (x 2 DLL names = 76 effective)
        DiscordShim.cpp       - 9
        DirectSoundShim.cpp   - 10
        MfplatShim.cpp        - 14
        MiscStubShim.cpp      - 42
    legacy/
        D3D9Shim.cpp          - 9
        D3D8Shim.cpp          - 2
        DDrawShim.cpp         - 6
        Opengl32Shim.cpp      - 153
d3d11-bridge/
    D3D11Bridge.h
    D3D11Bridge.cpp
bridges/
    XInputBridge.h / .cpp
    KeyboardBridge.h / .cpp
    AudioBridge.h / .cpp
    GdiRenderer.h / .cpp
uwp-shell/
    main.cpp
    App.h / App.cpp
    pch.h
    run.config
    Package.appxmanifest
    XboxWin32Runner.vcxproj
    XboxWin32Runner.vcxproj.filters
    packages.config
    Assets/*.png
winheaders/
    Windows.h                 - Stub Windows SDK for Linux syntax-check
tools/
    pe_import_scanner.py      - PE import + shim coverage scanner
    test_pe_scanner.py        - Unit tests for the scanner
    make_synthetic_pe.py      - Generates test/SampleGame/*.exe
host-agent/
    host_agent.py             - REST broker (Steam/Epic/GOG scan)
    README.md
    requirements.txt
scripts/
    check_all.sh              - g++ -fsyntax-only harness
test/
    SampleGame/
        SyntheticGame.exe     - 1 KB synthetic x64 PE
        GameHelper.dll        - 1 KB synthetic x64 PE with delay import
```

## Appendix B: Reference: the REGISTER_SHIM macro

The shim registration system is built around a single macro:

```cpp
// shims/ShimRegistry.h
#define REGISTER_SHIM(dll, name, ptr)                                          \
    static int XWR_REGID(__COUNTER__) = []() {                                 \
        ::xwr::ShimRegistry::Instance().Register(                              \
            L##dll, L##name, reinterpret_cast<FARPROC>(ptr));                  \
        return 0;                                                              \
    }()
```

The macro:

1. Generates a unique variable name using `__COUNTER__` (so multiple
   `REGISTER_SHIM` calls in the same translation unit don't clash).
2. Initializes it with a lambda that calls
   `ShimRegistry::Instance().Register(...)` at static-init time.
3. The lambda returns `0` so the variable is a valid `int` initializer.

Because the registration happens at static-init time, all 800+ shims are
registered before `main` runs. By the time `App::Initialize` calls
`ShimRegistry::Instance()` (just to construct the singleton), the
registrations have already fired against the singleton's `m_table`.

### Macro-pattern variants

Some shim modules use a `#define` block to register the same set of
functions under multiple DLL names. For example,
`shims/pass-through/XInputShim.cpp`:

```cpp
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
```

This registers 6 functions × 4 DLL names = 24 effective entries from 6
raw `REGISTER_SHIM` macro calls. The same pattern is used by
`SteamShim.cpp` (38 functions × 2 DLL names = 76 effective entries).
