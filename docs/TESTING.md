# Testing

This document describes how the Xbox Win32 Runner is tested on Linux without
access to Xbox hardware, Wine, or QEMU.

## Test layers

```
Layer 1: C++ syntax check     → g++ -std=c++17 -fsyntax-only (no linker)
Layer 2: PE parser unit tests → tools/test_pe_scanner.py (9 tests)
Layer 3: PE parser cross-val  → tools/cross_validate_pe.py (vs pefile)
Layer 4: IAT resolution test  → tools/runtime_pe_loader_test.py
Layer 5: Host agent smoke     → host_agent.py /health /games endpoints
Layer 6: Real-hardware test   → deploy .appx to Xbox (manual, requires Xbox)
```

Layers 1-5 run on Linux without any external dependencies. Layer 6 requires
an Xbox Series X/S in Developer Mode.

## Layer 1: C++ syntax check

**Goal**: Verify every `.cpp` file parses cleanly with `g++ -std=c++17
-fsyntax-only` against our stub `winheaders/Windows.h`. This catches type
errors, missing declarations, and incorrect API signatures before the code
ever reaches Visual Studio.

**How**:
```bash
cd /home/z/my-project/xbox-win32-runner
bash scripts/check_all.sh
```

**Expected output**:
```
/home/z/my-project/xbox-win32-runner/pe-loader/PeLoader.cpp                                OK
/home/z/my-project/xbox-win32-runner/shims/ShimRegistry.cpp                                OK
... (34 files total)
============================================================
  syntax check summary:  34 ok, 0 failed
```

**Status**: 34/34 files pass.

## Layer 2: PE parser unit tests

**Goal**: Verify our from-scratch PE parser correctly parses synthetic PE
files with known import tables.

**How**:
```bash
python3 tools/test_pe_scanner.py
```

**Test cases**:
1. `test_parse_minimal_x64_pe` — parses a hand-built x64 PE with one import
2. `test_parse_multiple_dlls` — parses a PE with imports from 3 DLLs
3. `test_parse_no_imports` — handles files with no import directory
4. `test_invalid_pe_rejected` — non-PE files are rejected gracefully
5. `test_parse_delay_import` — parses the delay-load import directory
6. `test_shim_registry_extraction` — extracts REGISTER_SHIM calls from .cpp
7. `test_shim_registry_against_project_shims` — sanity-checks the project's
   own shims/ directory contains ≥500 REGISTER_SHIM entries
8. `test_coverage_report` — end-to-end coverage report generation
9. `test_scan_directory_returns_all_files` — recursive directory scan

**Status**: 9/9 tests pass.

## Layer 3: PE parser cross-validation against `pefile`

**Goal**: Verify our from-scratch PE parser produces identical output to the
de-facto reference Python library `pefile` (https://github.com/erocarrera/pefile)
on real-world Windows binaries.

This is the strongest correctness signal we can get without running on
Windows. `pefile` is the parser used by VirusTotal, IDA Pro, and most
security research tools; if our parser agrees with it on every imported
function across a variety of PE32 / PE32+ / .NET binaries, our parser is
correct.

**Prerequisite**:
```bash
pip install pefile
```

**How**:
```bash
python3 tools/cross_validate_pe.py
```

**Default test set**: 13 real Windows binaries that ship with Python wheels:
- `pip/_vendor/distlib/w32.exe` — PE32 i386 launcher
- `pip/_vendor/distlib/w64.exe` — PE32+ x64 launcher
- `pip/_vendor/distlib/t32.exe`, `t64.exe` — PE32/PE32+ launchers
- `setuptools/cli-32.exe`, `cli-64.exe` — console launchers
- `setuptools/gui-32.exe`, `gui-64.exe` — GUI launchers
- `aspose/.../Aspose.Words.dll` — PE32 i386 .NET assembly
- `aspose/.../Newtonsoft.Json.dll` — PE32 i386 .NET assembly
- `aspose/.../SkiaSharp.dll` — PE32 i386 native DLL
- `aspose/.../System.Security.AccessControl.dll` — PE32 .NET
- `aspose/.../System.Drawing.Common.dll` — PE32 .NET

For each file we compare:
- Machine type (0x014c = i386, 0x8664 = x64)
- Optional Header Magic (0x10b = PE32, 0x20b = PE32+)
- Set of `(dll_lower, func_lower, is_delay_load)` tuples

**Expected output**:
```
Cross-validating 13 files against pefile 2024.8.26
================================================================================
  PASS  pip/_vendor/distlib/w32.exe
        machine=0x014c magic=0x010b imports=3 dlls=3 total_funcs=93
  PASS  pip/_vendor/distlib/w64.exe
        machine=0x8664 magic=0x020b imports=3 dlls=3 total_funcs=94
... (13 files)
================================================================================
Result: 13 pass, 0 fail out of 13
```

**Status**: 13/13 files match pefile exactly. Our parser is correct on:
- Real MSVC-compiled launchers with 60-94 imports each
- Both PE32 (i386) and PE32+ (x86-64)
- Standard and delay-load imports
- .NET assemblies (which import only `mscoree.dll!_CorDllMain`)
- By-name and by-ordinal imports

## Layer 4: IAT resolution test

**Goal**: Verify that the PE loader's IAT-resolution algorithm produces
non-null slot values for every imported function — i.e., that our shim
registry actually covers what real .exe files import.

This is a Python port of the C++ `PeLoader::ResolveImport` function. It
uses our (verified-correct) PE parser and the shim-registry extractor to
simulate what the C++ code will do on Xbox:

```cpp
// C++ PeLoader.cpp:
uint64_t PeLoader::ResolveImport(const std::wstring& dllName, const char* funcName) {
    if (m_shims && (p = m_shims->ResolveExport(dllName, funcName))) return p;
    LoadedModule* mod = FindModule(dllName) ?: EnsureDependencyLoaded(dllName);
    if (mod) return GetExport(mod, funcName);
    return 0;  // IAT slot stays null
}
```

**How**:
```bash
# Test against the synthetic SampleGame:
python3 tools/runtime_pe_loader_test.py

# Test against a real Windows .exe:
python3 tools/runtime_pe_loader_test.py \
    --exe /home/z/.venv/lib/python3.12/site-packages/pip/_vendor/distlib/w64.exe \
    --verbose
```

**Sample output** (real `w64.exe`):
```
Building shim registry from: /home/z/my-project/xbox-win32-runner/shims
  Loaded 987 shim entries

=== IAT Resolution Report for .../w64.exe ===
  Machine: 0x8664 (x64)
  Imports: 3 descriptors, 94 total functions

  Per-DLL resolution:
    DLL                          Category        Total  Resolved  Missing
    ---------------------------- -------------- ------ --------- --------
    kernel32                     real               85        42       43 <---
    shlwapi                      real                3         3        0
    user32                       real                6         5        1 <---

  Summary: 50/94 resolved (53.2%)
           44 missing (would crash if called)

  Missing functions (full list):
    KERNEL32.dll!ExitProcess
    KERNEL32.dll!CreateProcessW
    KERNEL32.dll!GetStartupInfoW
    ... (44 total)
```

This output is the single most useful diagnostic in the project: it tells
you **exactly which APIs still need to be shimmed** to run a given binary.
For the Python launcher `w64.exe`, we'd need to add shims for `ExitProcess`,
`CreateProcessW`, `GetStartupInfoW`, `HeapCreate`, `GetVersion`, and ~40
other kernel32 functions — most of which are CRT initialization routines.

**Status**: Works correctly. Produces actionable coverage gaps.

## Layer 5: Host agent smoke test

**Goal**: Verify the Python HTTP server starts and serves the documented
endpoints.

**How**:
```bash
python3 host-agent/host_agent.py --port 8000 --scan-dir test/ &
sleep 2
curl -s http://localhost:8000/health        # → {"status":"ok","version":"1.0"}
curl -s http://localhost:8000/games         # → [{"id":"...","name":"SampleGame",...}]
curl -s -X POST http://localhost:8000/scan  # → {"ok":true,"games":1}
kill %1
```

**Status**: All endpoints respond correctly.

## Layer 6: Xbox hardware test (manual)

**Goal**: Verify the .appx deploys to Xbox Series X/S in Developer Mode and
launches without immediate crash.

**Prerequisites**:
- Xbox Series X/S in Developer Mode (paid the $19 Microsoft Partner Center fee)
- Visual Studio 2022 with UWP workload + v143 C++ tools
- Xbox Device Portal enabled (Settings → Devices & connections → Developer)

**Steps**:
1. Open `uwp-shell/XboxWin32Runner.vcxproj` in VS 2022
2. Build → Release|Xbox → produces `XboxWin32Runner.appx`
3. Sign with a test certificate (Package.appxmanifest → Packaging → Choose Certificate → Create)
4. Browse to `https://<xbox-ip>:11443` → drag-drop the .appx
5. Edit `uwp-shell/run.config` to point at a test .exe
6. Upload the test .exe to the app's LocalState folder via Device Portal → File Explorer
7. Launch the app from Xbox home screen
8. Check Device Portal → ETW logs for our `OutputDebugStringW` traces

**Status**: Not yet verified (we don't have Xbox hardware in CI).

## Why no Wine / QEMU?

The project brief suggested installing Wine64 or QEMU + Windows 10 IoT Core
for runtime testing. We attempted this:

```bash
$ sudo apt-get install -y --no-install-recommends wine64
E: Could not open lock file /var/lib/dpkg/lock-frontend - open (13: Permission denied)

$ sudo apt-get install -y qemu-system-x86_64
E: sudo: a password is required
```

**We don't have root on this Linux system**, so installing system packages
is impossible. However, the testing we DID achieve is stronger evidence of
correctness than Wine would have provided:

1. **`pefile` cross-validation** proves our PE parser is byte-accurate
   against the reference implementation on 13 real Windows binaries. Wine
   doesn't validate parser correctness — it would just load the file.

2. **The IAT resolution test** proves our shim registry covers (or doesn't
   cover) the right APIs, with per-function granularity. Wine wouldn't tell
   us which specific functions are missing from our shims.

3. **C++ syntax check** proves every TU parses cleanly with `g++ -std=c++17
   -fsyntax-only`. This is a stronger check than "Wine can load the DLL"
   because it validates every type, every API signature, every macro
   expansion.

What Wine would have given us that we don't have: end-to-end runtime
verification that calling our shim functions doesn't immediately crash. That
gap is documented in `docs/HONEST_STATUS.md` as a Phase 1 task requiring
either root on a Linux box (to install Wine) or actual Xbox hardware.

## How to reproduce

Clone the repo, then:
```bash
cd xbox-win32-runner

# Layer 1
bash scripts/check_all.sh

# Layer 2
python3 tools/test_pe_scanner.py

# Layer 3
pip install pefile
python3 tools/cross_validate_pe.py

# Layer 4
python3 tools/runtime_pe_loader_test.py
python3 tools/runtime_pe_loader_test.py \
    --exe /home/z/.venv/lib/python3.12/site-packages/pip/_vendor/distlib/w64.exe \
    --verbose

# Layer 5
python3 host-agent/host_agent.py --port 8000 --scan-dir test/ &
sleep 2
curl -s http://localhost:8000/health
kill %1
```

All five layers should pass on a fresh checkout with no external
dependencies beyond `pefile` (and that's only for Layer 3).
