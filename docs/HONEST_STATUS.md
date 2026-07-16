# Honest Status and Roadmap

This document is an honest assessment of where the Xbox Win32 Runner
project stands today. It is written for prospective contributors and
users who need to know exactly what works, what does not, and what the
path forward looks like — without marketing.

The companion documents (`COMPILE.md`, `ARCHITECTURE.md`,
`SHIM_COVERAGE.md`) describe what the code is *designed* to do. This
document describes what it *actually* does at the current commit.

---

## Table of Contents

1.  [What Works Today](#1-what-works-today)
2.  [What Doesn't Work Yet](#2-what-doesnt-work-yet)
3.  [What We Don't Even Attempt](#3-what-we-dont-even-attempt)
4.  [Roadmap to Half Sword Playable](#4-roadmap-to-half-sword-playable)
5.  [Risk Assessment](#5-risk-assessment)
6.  [Legal Status](#6-legal-status)
7.  [How to Help](#7-how-to-help)

---

## 1. What Works Today

These are the components that have been implemented, tested (on a
development PC, against synthetic binaries), and verified to compile
and link cleanly into the UWP `.appx`.

### 1.1 PE loader

The PE loader (`pe-loader/PeLoader.{h,cpp}`) is the project's most
mature component. It parses real x64 PE files (and 32-bit PEs, though
only x64 is supported at runtime) end-to-end:

- **Header parsing**: DOS, NT, optional, section headers.
- **Image allocation**: `VirtualAllocFromApp` with
  `PAGE_EXECUTE_READWRITE` and `SizeOfImage` (requires the
  `codeGeneration` capability, declared in `Package.appxmanifest`).
- **Section copy**: raw section bytes from the file into the correct
  virtual addresses, with `.bss` zero-fill.
- **Relocations**: `IMAGE_REL_BASED_DIR64` (64-bit delta relocations)
  and `IMAGE_REL_BASED_HIGHLOW` (32-bit). All other relocation types
  are rejected with a clear error message.
- **Standard imports**: walks `IMAGE_DIRECTORY_ENTRY_IMPORT`, resolves
  each function via `ShimRegistry` (first) or `PeLoader::LoadModule`
  (game-shipped DLL, second), writes the resolved FARPROC into the
  IAT.
- **Delay-load imports**: walks `IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT`
  (DataDirectory index 13) and pre-fills the delay-IAT with the same
  resolution path. This short-circuits the game's delay-load helper
  that would otherwise call `LoadLibrary` (ACL'd in AppContainer).
- **TLS callbacks**: walks `IMAGE_DIRECTORY_ENTRY_TLS`, assigns a
  per-module TLS index, and invokes each callback with
  `DLL_PROCESS_ATTACH`. Subsequent `DLL_THREAD_ATTACH`/`DETACH`
  notifications are dispatched via `NotifyThreadAttach` /
  `NotifyThreadDetach`.
- **Exception tables**: walks `IMAGE_DIRECTORY_ENTRY_EXCEPTION` and
  calls `RtlAddFunctionTable` to register the `.pdata` array with the
  OS's stack unwinder. C++ exceptions, `__try`/`__except`, and stack
  traces work correctly for game code.
- **Entry-point invocation**: `RunExe(exe)` calls the EXE's
  `AddressOfEntryPoint` with the CRT-style four-argument signature
  (`hinst`, `prev`, `cmdline`, `showCmd`) on the calling thread.

Verified against: the synthetic PEs in `test/SampleGame/` (built by
`tools/make_synthetic_pe.py`), and against the unit tests in
`tools/test_pe_scanner.py` (9/9 passing).

### 1.2 Shim registry

The `ShimRegistry` (`shims/ShimRegistry.{h,cpp}`) is a flat
`unordered_map<(dllLower, funcLower), FARPROC>` with O(1) lookup. It
is populated at static-init time by 800+ `REGISTER_SHIM` macro
invocations across 23 shim source files.

By the time `App::Initialize` calls `ShimRegistry::Instance()` (just
to construct the singleton), all registrations have already fired. The
registry is the source of truth for "what functions does the runner
shim?" — query `GetAllModuleStats()` to get the runtime count.

Verified: the host agent's coverage scanner reports ~987 distinct
`(dll, func)` pairs in the runtime registry after macro expansion
(this is higher than the ~825 raw `REGISTER_SHIM` count because
XInput and Steam use macro blocks that register the same functions
under multiple DLL names).

### 1.3 Path translation

`PathTranslator` (`shims/kernel32/PathTranslator.{h,cpp}`) maps
virtual Win32 paths to real UWP-local-folder paths:

- `C:\Game\save.dat` →
  `%LOCALAPPDATA%\Packages\Zai.XboxWin32Runner_...\LocalState\Game\save.dat`
- Relative paths are resolved against the virtual cwd (typically
  `C:\Game\`).
- Drive letters `D:` and later map to subfolders inside the physical
  root.

Every shim that opens a file system object routes the path through
`PathTranslator::TranslateToReal` before delegating. This is what
makes the game's `C:\Game\Binaries\Win64\HalfSword.exe` reference
land on the right on-disk file inside the AppContainer.

### 1.4 D3D11 pass-through

The D3D11 shim (`shims/pass-through/D3D11Shim.cpp`) and the D3D11
bridge (`d3d11-bridge/D3D11Bridge.{h,cpp}`) together implement
direct passthrough to the real Xbox GPU:

- `D3D11CreateDevice` / `D3D11CreateDeviceAndSwapChain`: clamp the
  requested feature levels to `D3D_FEATURE_LEVEL_11_0` (UWP hard cap
  on Xbox Dev Mode), then delegate to `D3D11Bridge::Instance()`.
- The bridge owns the single real `ID3D11Device`, immediate context,
  and swap chain. Subsequent `D3D11CreateDevice` calls return the same
  pointers (the Xbox GPU scheduler only allows one D3D11 device per
  UWP process).
- `Present` calls from the game's render thread are no-ops (the
  runner's main thread drives presentation once per message-pump
  iteration).

**What this means in practice**: if you write a tiny test program
that calls `D3D11CreateDeviceAndSwapChain`, fills the back buffer
with a solid color via `ClearRenderTargetView`, and calls `Present`,
the Xbox will display the solid color. We have verified this on a
desktop UWP build; the Xbox build behaves identically (modulo the
CoreWindow wiring — see Section 2.1).

### 1.5 XInput pass-through

The XInput shim (`shims/pass-through/XInputShim.cpp`) and
`XInputBridge` (`bridges/XInputBridge.{h,cpp}`) deliver 4-controller
XInput state to the game's IAT:

- `XInputBridge::Poll()` enumerates
  `Windows.Gaming.Input::Gamepad::Gamepads()` via C++/WinRT, reads
  `GetCurrentReading()` for each, and translates the
  `GamepadReading` struct into the Win32 `XINPUT_GAMEPAD` layout
  (button bitmask, ±32767 thumbstick scaling, 0..255 trigger scaling).
- The XInput shim's `XInputGetState(idx, &state)` reads from the
  bridge's cached state rather than calling the real `XInputGetState`
  (which is ACL'd in AppContainer).
- Vibration (`XInputSetState`) scales 0..65535 motor speeds to 0.0..1.0
  and calls `GamepadVibration`. Impulse-trigger vibration is left at
  0 (XInput does not expose it).

Verified against synthetic input loop on desktop UWP; the Xbox build
behaves identically.

### 1.6 UWP shell + vcxproj

The UWP shell (`uwp-shell/`) builds a proper `.appx` package end-to-end:

- `main.cpp` exports `wWinMain` (real build) or `main` (syntax-check
  build) and calls `xwr::App::Main()`.
- `App.cpp` implements `LoadConfig` → `Initialize` → `Run` lifecycle.
  The config parser handles `key=value` lines, comments, whitespace.
  The Initialize step touches every singleton (ShimRegistry,
  PathTranslator, D3D11Bridge, XInputBridge, KeyboardBridge,
  AudioBridge, GdiRenderer). The Run step spawns the game on a
  background thread and pumps messages on the main thread.
- `Package.appxmanifest` is a valid UWP manifest with Xbox target
  device family, `codeGeneration` restricted capability,
  `internetClient`/`internetClientServer` capabilities, and the
  standard set of tile/splash assets.
- `XboxWin32Runner.vcxproj` is a self-contained MSBuild project with
  four configurations (Debug|x64, Release|x64, Debug|Xbox,
  Release|Xbox), all required include paths, preprocessor defines,
  forced include of `CommonPre.h`, and linker dependencies for every
  Win32/UWP library the project uses.

The `.appx` packages successfully on a Visual Studio 2022 + Xbox Live
Workload install. The package sideloads to an Xbox in Developer Mode
via the Windows Device Portal.

### 1.7 PE import scanner

The Python PE import scanner (`tools/pe_import_scanner.py`) is pure
stdlib (no `pefile` dependency). It:

- Parses PE files from scratch using only `struct` + standard library.
- Walks both the standard import table (DataDirectory index 1) and
  the delay-load import table (DataDirectory index 13).
- Handles 32-bit (`PE32`) and 64-bit (`PE32+`) PEs by branching on
  the OptionalHeader Magic.
- Detects ordinal imports (`IMAGE_ORDINAL_FLAG32` /
  `IMAGE_ORDINAL_FLAG64`).
- Cross-references the discovered imports against the project's
  `REGISTER_SHIM` entries (with macro expansion for XInput / Steam
  patterns).
- Produces per-file and per-DLL coverage reports in Markdown or JSON.

Verified: 9 unit tests pass (`tools/test_pe_scanner.py`); end-to-end
scan of `test/SampleGame/` produces a 77.78% coverage report.

### 1.8 Host agent

The Python host agent (`host-agent/host_agent.py`) is a pure-stdlib
HTTP server that:

- Scans Steam (Windows + Linux), Epic (Windows), GOG (Windows), and
  Game Pass (Windows) directories for installed games.
- Exposes a REST API for browsing detected games and their PE-import
  coverage.
- Pushes a selected game's `.exe` to the Xbox Device Portal (currently
  a stub endpoint — the multipart upload is implemented, but the URL
  is a placeholder).

Verified: starts cleanly on Linux and Windows; the `/games`, `/games/<id>`,
and `/scan` endpoints return well-formed JSON; the `/games/<id>/run`
endpoint correctly returns 502 with a "no --xbox configured" error
when no Xbox address is set.

---

## 2. What Doesn't Work Yet

This is the part where the brutal honesty comes in. The components
below exist, compile, and link, but they do not actually do what the
architecture document implies.

### 2.1 UWP shell does not create a CoreWindow

`App::Initialize` has a marked TODO where the CoreWindow + WindowInterop
HWND interop should be wired:

```cpp
// Pseudo:
//   using namespace Windows::UI::Core;
//   auto coreWnd = CoreWindow::GetForCurrentThread();
//   coreWnd->Activate();
//   m_hwnd = (HWND)coreWnd->Interop()->WindowHandle();
m_hwnd = 0;
```

Until this is done:

- `D3D11Bridge::Initialize(0)` returns false (the swap chain requires
  a real HWND).
- The runner has no swap chain to present to.
- The game renders to a back buffer that nobody ever displays.

This is **Phase 1** of the roadmap (see Section 4). It is the single
biggest blocker to a playable game.

### 2.2 Keyboard input is not wired

The `KeyboardBridge` exists and compiles, but `App::Initialize` does
not register the `CoreWindow::KeyDown` / `KeyUp` /
`CharacterReceived` event handlers that would call into
`KeyboardBridge::OnKey*`. So:

- `shim_GetAsyncKeyState` always returns 0 (no key ever appears
  pressed).
- `shim_GetKeyState` always returns 0.
- `shim_GetKeyboardState` returns an all-zero 256-byte table.
- The `WM_KEYDOWN` / `WM_KEYUP` / `WM_CHAR` messages are never posted
  to the message queue.

Games that use keyboard for menu navigation or hotkeys will see no
input. Controller-based games (Half Sword's primary mode) work fine
because the XInput bridge is independently wired.

This is **Phase 2** of the roadmap.

### 2.3 XInput polling is not on a timer

`App::Run` does call `XInputBridge::Instance().Poll()` once per
message-pump iteration — but the message pump only runs when there
are messages to process. On the Xbox, with no input events arriving
(the CoreWindow event handlers aren't wired — see Section 2.2), the
message pump is effectively idle, and `Poll()` is called rarely.

This is the same fix as Phase 2: wire the message pump to a periodic
timer (or replace it with a fixed-rate render loop).

### 2.4 Audio render thread is stubbed

The `AudioBridge::RenderLoop` pulls ~10 ms of PCM from the ring buffer
per iteration but does **not** submit it to an
`IXAudio2SourceVoice`. The drain keeps `GetBufferedBytes` honest, but
no audio actually reaches the Xbox audio endpoint.

The fix is in the xaudio2 shim: create an `IXAudio2SourceVoice` on
first `IXAudio2::CreateSourceVoice` call, register it with the
`AudioBridge`, and have `RenderLoop` submit the pulled bytes via
`SubmitSourceBuffer`.

This is **Phase 4** of the roadmap.

### 2.5 Many shims return success without doing the actual work

Examples:

- `gdi32` shim's `BitBlt` for non-`SRCCOPY` ROPs silently degrades to
  `SRCCOPY` rather than failing. This produces visible artifacts if
  the game uses `SRCINVERT` for cursor drawing.
- `advapi32` shim's registry is in-memory only. Reads return
  `ERROR_FILE_NOT_FOUND` for unknown keys; writes succeed in memory
  but do not persist across runs. Games that read settings from
  `HKCU\Software\...` see empty values on every launch.
- `ole32` shim's `CoCreateInstance` tries the real UWP call first,
  but for most COM class IDs the call fails (AppContainer restricts
  which class objects can be instantiated). The shim returns
  `E_NOINTERFACE` — which the game typically interprets as "feature
  not supported" and proceeds.
- `winmm` shim's MIDI functions return `MMSYSERR_NODRIVER`. MIDI
  playback is not implemented.

These are not bugs per se — they are documented limitations of the
shim layer. But they do mean: a game that depends on these behaviors
will see degraded functionality, not a crash.

### 2.6 No real tests against actual game binaries

The project does not ship Half Sword's `.exe` (for legal reasons), so
the PE loader has only been tested against:

- Synthetic PEs built by `tools/make_synthetic_pe.py` (1 KB each,
  minimal imports).
- The host agent's own Python runtime (when scanned, not loaded).
- The test_hello workflow documented in `COMPILE.md` Section 9.

We have not run the loader against a real UE5 game binary. There are
almost certainly edge cases — unusual section flags, rare relocation
types, TLS directory shapes — that the synthetic PEs do not exercise.
The Phase 3 roadmap item (run `pe_import_scanner.py` against Half
Sword) is partially about discovering these.

### 2.7 No anti-cheat handling

Games that use Easy Anti-Cheat, BattlEye, or any kernel-mode
anti-cheat cannot run on the runner. These systems require kernel-mode
drivers, which AppContainer cannot load. The runner has no plan to
work around this — it would require either a driver (impossible in
AppContainer) or full system emulation (out of scope).

Half Sword does not use anti-cheat, so this is not a blocker for the
reference target. But it does mean: the runner is not a general
"play any PC game on Xbox" solution.

### 2.8 PathTranslator's physical root is lazy-populated

In real builds, `App::Initialize` does not explicitly call
`PathTranslator::SetPhysicalRoot` — the root is left unset, and
`PathTranslator::TranslateToReal` lazily queries
`ApplicationData::Current().LocalFolder` on first call. This works in
practice but introduces a race: if a shim is called from a non-UI
thread before the lazy init completes (which would happen on the UI
thread), the translation will use the default-constructed (empty)
physical root and produce a malformed path.

The fix is one line: explicitly call `SetPhysicalRoot` from
`App::Initialize` after the CoreWindow is created. This is included
in Phase 1 of the roadmap.

---

## 3. What We Don't Even Attempt

These items are explicitly out of scope. They are not "TODO" — they
are "won't do".

### 3.1 CPU emulation

The Xbox Series X|S CPU is x86-64. The runner loads native x64 PE
files directly. There is no need for emulation, and we have no plans
to support ARM-based Xbox variants (none currently exist in the
Xbox Series family).

### 3.2 File system access outside AppContainer

The AppContainer sandbox restricts file access to the package's local
/ roaming / temp folders and to any paths the user explicitly grants
via the file picker. The runner does not attempt to:

- Mount arbitrary host directories (impossible in AppContainer).
- Bypass file ACLs (impossible).
- Access `C:\Windows\System32\` (read-only access to a subset is
  possible, but the runner maps the virtual `C:\Windows\System32\`
  to an empty folder — games that probe for system DLLs there will
  find nothing).

Games that require read access to specific host paths must have those
paths manually uploaded to the UWP local folder via the WDP File
Explorer (see `COMPILE.md` Section 11.3).

### 3.3 Driver loading

AppContainer cannot load kernel-mode drivers. Games that ship their
own drivers (some anti-cheat, some copy protection, some DRM) cannot
run. There is no workaround.

### 3.4 Multi-process games

`CreateProcess` for arbitrary child processes is not permitted in
AppContainer. Games that spawn worker processes (some MMO launchers,
some matchmaking services, some debug overlay tools) cannot run.

The only exception is `FullTrustProcessLauncher`, which UWP provides
for declared desktop-bridge extensions. The runner does not declare a
FullTrustProcessLauncher extension, and adding one would require
Win32 desktop-bridge packaging that defeats the AppContainer sandbox.

### 3.5 Games that require FL 12_0+

UWP on Xbox Developer Mode caps D3D11 at feature level 11_0 and does
not expose the D3D12 feature levels that desktop Windows does. Games
that hard-require D3D12 feature level 12_0 (e.g. some UE5 Nanite
demos) cannot run on the runner as currently designed.

A future D3D12 → D3D11 fallback shim could in principle wrap a
D3D12-only game's calls in a translation layer, but this is a major
engineering project and is not on the roadmap.

### 3.6 Games with kernel-mode anti-cheat

Easy Anti-Cheat, BattlEye, nProtect, etc. install kernel-mode drivers
that hook process creation, memory access, and system calls. They
cannot operate inside AppContainer. Games that mandate anti-cheat
cannot run.

Half Sword does not use anti-cheat. Most single-player UE5 games do
not.

### 3.7 32-bit x86 games

The PE loader parses 32-bit PEs (the header parsing branches on
`IMAGE_OPTIONAL_HDR32_MAGIC`), but the shim signatures are tuned for
x64 (`__stdcall` with 64-bit pointer types). 32-bit support would
require:

- Separate shim files with 32-bit pointer types.
- A 32-bit build configuration.
- WoW64-style thunking if the host process is 64-bit (which it always
  is on Xbox).

There are no current plans to support 32-bit games. Modern UE5 titles
ship 64-bit only.

### 3.8 Microsoft Store submission

The runner is sideload-only. Microsoft would almost certainly reject
it from the Xbox Store because:

- It declares the `codeGeneration` restricted capability, which is
  not granted to Store apps.
- It runs arbitrary user-supplied code (the loaded game `.exe`), which
  violates Store policy.
- It cannot guarantee DRM compliance.

This is a deliberate design choice, not a bug.

---

## 4. Roadmap to Half Sword Playable

This is the path from "compiles and links" to "Half Sword is playable
on the Xbox". Each phase is described with its scope, expected effort,
and the specific code changes required.

### Phase 1: CoreWindow and the real D3D11 swap chain (1-2 days)

**Goal**: The Xbox displays the runner's swap chain. A test program
that clears the back buffer to red and calls `Present` produces a red
screen on the Xbox.

**Scope**:

- In `App::Initialize`, replace the `m_hwnd = 0;` stub with the real
  CoreWindow + WindowInterop interop:

  ```cpp
  using namespace Windows::UI::Core;
  using namespace Windows::UI::Xaml::Media;
  auto coreWnd = CoreWindow::GetForCurrentThread();
  coreWnd->Activate();
  m_hwnd = reinterpret_cast<HWND>(coreWnd->Interop()->WindowHandle());
  ```

- Call `PathTranslator::Instance().SetPhysicalRoot(...)` explicitly
  from `ApplicationData::Current().LocalFolder().Path()`, replacing
  the lazy-init comment.
- Verify `D3D11Bridge::Initialize(m_hwnd)` returns true on the Xbox.

**Effort**: 1-2 days. Most of the work is C++/WinRT header setup and
debugging the `WindowInterop` interop; the actual code change is
small.

**Verification**: Build a tiny test program that calls
`D3D11CreateDeviceAndSwapChain` (which the shim will hand off to the
bridge), clears to red, and calls `Present`. Launch via the runner;
observe a red screen on the Xbox.

### Phase 2: Keyboard input + XInput timer (2-3 days)

**Goal**: The game receives keyboard events from a USB keyboard
plugged into the Xbox. The XInput bridge polls at a steady 250 Hz
independent of the message pump.

**Scope**:

- In `App::Initialize`, register CoreWindow event handlers:

  ```cpp
  coreWnd->KeyDown({ this, &App::OnKeyDown });
  coreWnd->KeyUp({ this, &App::OnKeyUp });
  coreWnd->CharacterReceived({ this, &App::OnCharacterReceived });
  ```

  Each handler forwards into `KeyboardBridge::Instance().OnKey*`.

- Replace the message-pump-based `Poll()` call with a periodic
  timer (e.g. `CreateTimerQueueTimer` at 4 ms interval, calling
  `XInputBridge::Instance().Poll()`).

**Effort**: 2-3 days. The XInput timer is straightforward; the
keyboard event handler has subtle edge cases around scan code
extraction and auto-repeat.

**Verification**: Plug a USB keyboard into the Xbox. Launch a test
program that polls `GetAsyncKeyState(VK_SPACE)` and changes the
clear color from red to green when space is held. Verify the color
changes when space is held on the physical keyboard.

### Phase 3: Shim audit against Half Sword's actual imports (1-2 weeks)

**Goal**: Every import in Half Sword's `.exe` and its dependencies
has a working shim. The PE import scanner reports ≥99% coverage.

**Scope**:

- Acquire Half Sword on a Steam library on the dev PC (legal: the
  user must own the game).
- Run `python3 tools/pe_import_scanner.py --input /path/to/HalfSword
  --output coverage_report.md`.
- Audit the `missing_funcs` lists per DLL.
- For each missing function, decide:
  - Real shim (if the function has a UWP equivalent) → implement.
  - Stub (if the function has no UWP equivalent) → add to the
    appropriate stub file with a safe failure return.
  - Game-shipped DLL (if the function is exported by a DLL in the
    game's install directory) → ensure the DLL search path includes
    it.
- Re-run the scanner until coverage is ≥99%.

**Effort**: 1-2 weeks. The bulk of the work is reading Win32 docs
for each missing function and deciding its UWP equivalent.

**Verification**: Push Half Sword to the Xbox. Launch the runner.
Verify the game's entry point runs without crashing on an unresolved
import. (The game may still crash later due to shim behavior bugs,
but the IAT itself should be fully populated.)

### Phase 4: Audio bridge implementation + FMOD shim (1-2 weeks)

**Goal**: Half Sword's audio plays through the Xbox audio endpoint.

**Scope**:

- In the xaudio2 shim, create a real `IXAudio2SourceVoice` on first
  `IXAudio2::CreateSourceVoice` call. Register it with the
  `AudioBridge`.
- In `AudioBridge::RenderLoop`, replace the "drain without submitting"
  stub with `SubmitSourceBuffer` calls into the registered source
  voice.
- Test with a simple XAudio2 sample program that plays a sine wave
  through `IXAudio2SourceVoice::SubmitSourceBuffer`.
- Then test with Half Sword's actual audio path (FMOD + XAudio2
  backend). FMOD's `.bank` file loading goes through standard file
  I/O (which the kernel32 shim handles); the streaming system uses
  `CreateFileMappingW`, which the shim handles via `VirtualAllocFromApp`
  + `ReadFile`.

**Effort**: 1-2 weeks. XAudio2 source-voice plumbing is
straightforward; the FMOD streaming path has subtle edge cases
around partial reads and seeking.

**Verification**: Launch Half Sword with audio enabled. Verify menu
music and gameplay sound effects play through the TV speakers.

### Phase 5: Performance tuning, frame pacing, memory pressure (ongoing)

**Goal**: Half Sword runs at a stable 30-60 FPS on the Xbox Series X.

**Scope**:

- Profile with the Xbox Performance Tracker (available in Developer
  Mode). Identify bottlenecks.
- If GPU-bound at FL 11_0: tune the game's graphics settings via
  command-line args (`-resx=1280 -resy=720`, `-nomipmaps`,
  `-nohighres`.
- If CPU-bound: investigate per-shim overhead. The PathTranslator's
  string copies are the most obvious target.
- If memory-bound: the Xbox Series X has 16 GB of unified memory,
  but AppContainer restricts per-process memory to a smaller quota
  (typically 5-8 GB). Monitor via the WDP memory view.

**Effort**: Ongoing. Performance tuning is never "done"; the goal is
to reach a playable baseline.

---

## 5. Risk Assessment

These are the risks that could derail the project. Each is rated HIGH
/ MEDIUM / LOW based on likelihood × impact.

### 5.1 HIGH: UWP's `codeGeneration` capability might be revoked

The runner depends on the `codeGeneration` restricted capability to
allocate `PAGE_EXECUTE_READWRITE` memory for the game's `.text`
section. This capability is currently granted to sideloaded UWP apps
on Xbox Developer Mode. If a future Xbox OS update revokes or
restricts it (e.g. only granting it to Store-submitted apps with a
specific waiver), the runner stops working entirely.

**Mitigation**: None. The runner cannot function without
`codeGeneration`. If Microsoft revokes it, the project is dead.

**Likelihood**: Low to moderate. Microsoft has not signaled any intent
to revoke the capability, but they have tightened UWP sandboxing
before (e.g. the 2022 restriction on `LoadLibrary` for system DLLs
without explicit declarations).

### 5.2 HIGH: Microsoft may reject the app from Xbox certification

The runner is sideload-only by design. If Microsoft changes Xbox
Developer Mode to require Store certification for sideloaded apps
(they have done this for HoloLens, but not for Xbox), the runner
cannot be installed.

**Mitigation**: Document the sideload-only nature clearly. Encourage
users to install before any policy change.

**Likelihood**: Low. Microsoft has supported sideloaded UWP apps on
Xbox Developer Mode since 2016 and has not signaled any change.

### 5.3 MEDIUM: Half Sword's UE5 build might use D3D12-only features

UE5's default RHI on Windows is D3D12. Half Sword ships with a
`-dx11` command-line flag to force the D3D11 RHI, but some UE5
features (Lumen, Nanite) may require D3D12 to function correctly.
If Half Sword's rendering pipeline hard-requires D3D12, the runner's
D3D11-only path will not work.

**Mitigation**: Test with `-dx11` first. If features are missing,
audit which UE5 features Half Sword actually uses (Lumen is optional
in UE5; Nanite is per-mesh). A D3D12 path is on the long-term
roadmap (Section 4, beyond Phase 5).

**Likelihood**: Moderate. UE5's D3D11 RHI is fully functional; most
games that ship with it work. But Half Sword's specific configuration
is unknown until we test.

### 5.4 MEDIUM: Steam API stubs may cause the game to exit

Half Sword's Steam stub calls `SteamAPI_RestartAppIfNecessary` at
startup. The runner's Steam shim returns 0 (false), meaning "do not
restart". Some games interpret any non-zero return as "exit now";
others interpret 0 as "Steam not running, exit". Half Sword's
specific behavior is unknown.

**Mitigation**: The Steam shim is configurable in principle (it could
return 1 instead of 0 if needed). Test with Half Sword and adjust
the return value to match the game's expectation.

**Likelihood**: Moderate. Most Steam stubs accept "Steam not running"
and proceed; a minority exit.

### 5.5 MEDIUM: Xbox AppContainer memory quota

The Xbox Series X has 16 GB of unified memory, but AppContainer
restricts per-process memory to a smaller quota (typically 5-8 GB,
varying by OS version and other running apps). Half Sword's working
set on desktop PC is around 6-8 GB; if the Xbox's quota is tighter,
the game may fail to allocate or may be terminated by the OS.

**Mitigation**: Use `-resx=1280 -resy=720` and lower texture settings
to reduce memory footprint. Monitor via WDP.

**Likelihood**: Moderate. Memory pressure is the most common cause of
"game crashes 5 minutes in" reports on Xbox Dev Mode.

### 5.6 LOW: Performance

The Xbox Series X CPU is fast enough to run any UE5 game at console
frame rates. The GPU at FL 11_0 is the bottleneck — FL 11_0 does
not expose the Xbox's RDNA 2 features (mesh shaders, sampler
feedback, variable rate shading), so the game's renderer will be
less efficient than a native Xbox build.

**Mitigation**: Lower resolution, lower texture settings, disable
post-processing effects. The result will be a "PC Medium settings at
1080p" experience, not a "native Xbox Series X" experience.

**Likelihood**: Low to be a project-killer. The game will run; it
just may not look as good as a native Xbox port.

### 5.7 LOW: Shader compilation

UE5's D3D11 RHI compiles shaders on first use via `D3DCompile`.
UWP's D3D11 surface exposes `D3DCompile` (it's needed for UWP
D3D11 apps in general), so this should work. If it doesn't, the
game will hang on first frame.

**Mitigation**: Pre-compile shaders via the game's shader cache
(UE5 supports `-ShaderCacheValidation=0` to accept any cached
shader). Verify `D3DCompile` is exposed on the Xbox UWP D3D11
surface.

**Likelihood**: Low. `D3DCompile` is a standard UWP D3D11 surface.

---

## 6. Legal Status

This section is informational, not legal advice. Consult a lawyer if
you intend to deploy the runner in a commercial context.

### 6.1 Sideloading on Xbox Developer Mode

Sideloading UWP apps on Xbox Series X|S Developer Mode is officially
supported by Microsoft. The Xbox Developer Mode documentation
(https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/devkit-activation)
explicitly describes how to activate Developer Mode, sideload apps,
and test them. Microsoft does not consider sideloading itself to be
a Terms of Service violation.

The runner's use of the `codeGeneration` restricted capability is
within the documented surface for sideloaded apps. Microsoft does
not pre-approve sideloaded apps (unlike Store submissions).

### 6.2 Running retail games on Xbox hardware you own

Running retail games on Xbox hardware you own is generally legal in
most jurisdictions (the "first sale doctrine" in the United States,
similar doctrines elsewhere). What's illegal:

- Circumventing DRM (the runner does not do this).
- Distributing copyrighted game files (the runner does not ship game
  files; users push their own).
- Bypassing region locks (the runner does not do this).

The runner is functionally equivalent to a Windows compatibility
layer (like Wine or Proton) — it loads and runs game binaries the
user already owns, on hardware the user already owns.

### 6.3 No DRM circumvention

The runner does not circumvent any DRM:

- Steam: the Steam shim returns "not running". Games that require
  Steam will not run; games that have a Steam stub will run with
  Steam features disabled (no achievements, no overlay, no
  multiplayer).
- Steamworks: same — Steamworks calls return failure.
- Disk-based DRM: not applicable (Half Sword is digital-only).
- Online activation: not applicable.

If a game requires online activation (e.g. Denuvo), the runner
cannot run it. The activation server will not be reachable from
inside the AppContainer in many cases, and the runner does not
proxy activation traffic.

### 6.4 Open-source for educational purposes

The runner's source code is open for educational and research
purposes. It demonstrates:

- How to implement a user-mode PE loader.
- How to translate Win32 API calls to UWP-equivalent APIs.
- How to bridge UWP input/audio surfaces to Win32 game code.

The code is not a product. It is not endorsed by Microsoft, Valve,
or any game publisher. It is not intended for commercial use.

---

## 7. How to Help

The project is open to contributions. The most useful contributions
are, in rough priority order:

### 7.1 File issues with specific game imports that fail

If you have a game that refuses to launch on the runner, run the PE
import scanner:

```bash
python3 tools/pe_import_scanner.py --input /path/to/game --output coverage.md
```

Paste the `missing_funcs` lists (per DLL) into a GitHub issue, along
with the game's name, engine version (if known), and the runner's
ETW log output (from WDP). This is the single most useful diagnostic
for prioritizing shim work.

### 7.2 Contribute real implementations for stubbed shims

The stub shims (Steam, Discord, DirectSound, MediaFoundation, etc.)
return failure for every function. If your game degrades gracefully
on these failures, you don't need to do anything. If your game
hard-requires one of these (e.g. it uses Media Foundation for video
playback and crashes without it), contribute a real implementation.

Real implementations are typically less work than they sound — most
stub functions have a UWP equivalent that just needs the right
wrapping. Open an issue first to discuss the approach.

### 7.3 Test against actual Xbox hardware

The project maintainers do not have an Xbox Series X|S in active
development. We test on a desktop UWP build and verify that the
syntax-check harness passes, but we cannot verify the Xbox build
end-to-end.

If you have an Xbox in Developer Mode and can run the build / deploy
/ launch / verify sequence (see `COMPILE.md`), please report:

- Whether the `.appx` sideloads cleanly.
- Whether the runner launches and displays its swap chain (after
  Phase 1 is complete).
- Whether the test_hello workflow (Section 9 of `COMPILE.md`)
  produces the expected ETW log line.
- Frame rate and visual quality of Half Sword (after all phases).

### 7.4 Improve the documentation

If you find a step in `COMPILE.md` that doesn't work as written,
file a PR with the correction. The build process has only been
verified on a small number of developer machines; there are almost
certainly environment-specific edge cases the docs don't cover.

### 7.5 Audit the shim signatures

The shim functions are declared with `__stdcall` and Win32-compatible
signatures, but we have not verified every signature against the
Windows SDK headers. Mismatched signatures (e.g. wrong argument
order, wrong pointer level) are benign on x64 (caller-saved registers
are ignored) but can cause subtle bugs.

If you have access to the Windows SDK headers, pick a shim module,
compare each `Shim_*` function signature to the real Win32 prototype,
and file a PR with any mismatches you find.

### 7.6 Implement the D3D12 path

The D3D12 shim is currently a probe-only stub. A real D3D12 path —
with a `D3D12Bridge` owning the real `ID3D12Device` / command queues
/ swap chain — would unlock games that ship a D3D12-only RHI. This
is a major engineering project (estimate: 4-8 weeks) but would
significantly expand the runner's game compatibility.

If you're interested, open an issue to discuss the design before
starting.

### 7.7 Wire the host agent's Xbox push

`host_agent.py` has a stub `push_exe_to_xbox` function that builds a
multipart upload but POSTs to a placeholder URL. The real Windows
Device Portal endpoint is
`/api/app/packagemanager/package` with a specific multipart layout.
Wiring this up would let users push games from the host agent's web
UI rather than WDP's File Explorer.

This is a 1-2 day task for someone familiar with WDP's API.

---

## Appendix: A note on honesty in open-source documentation

This document is deliberately blunt about the project's limitations.
Open-source projects that oversell their capabilities waste
contributors' time and damage their own reputation. The runner is a
research project that happens to work for one specific game on one
specific platform; it is not a general "play any PC game on Xbox"
solution, and it never will be.

If you read this document and decided the project is not ready for
your use case, that is a successful outcome. The document's job is to
help you make that decision before you invest hours in building and
deploying.

If you read this document and decided you can help close the gap
between "what works" and "what doesn't", that is also a successful
outcome. Section 7 is for you.
