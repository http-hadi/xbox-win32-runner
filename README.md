# Xbox Win32 Runner

Run native Windows x64 executables on Xbox Series X/S Developer Mode — **without** CPU
emulation, **without** streaming. The Xbox CPU is x86-64; we load the game's PE
file into the UWP process's address space using `VirtualAllocFromApp`, rewrite
its Import Address Table to point at our Win32 API shims, and let the game's
machine code execute natively on the Xbox CPU.

## What's in the box

| Path | Description |
|------|-------------|
| `pe-loader/` | PE loader: parses DOS/NT headers, maps sections, applies relocations, processes imports + delay-load imports + TLS callbacks + exception tables, calls DllMain / exe entrypoint |
| `shims/` | Win32 API compatibility layer (23 modules, ~800 REGISTER_SHIM entries) |
| `shims/CommonPre.h` | Forced-include header (undefs WIN32_LEAN_AND_MEAN, defines NOMINMAX) |
| `shims/Win32Types.h` | Supplemental constants/types not in the UWP SDK |
| `shims/ShimRegistry.h` | Central `dll.func` → FARPROC lookup table |
| `shims/kernel32/PathTranslator.{h,cpp}` | Translates `C:\Game\save.dat` → UWP local folder |
| `d3d11-bridge/` | Creates the real Xbox D3D11 device; game's D3D11 calls go directly to the GPU |
| `bridges/` | Real I/O bridges: XInput (via Windows.Gaming.Input), Keyboard (CoreWindow), Audio (WASAPI/XAudio2), GdiRenderer (Direct2D) |
| `uwp-shell/` | UWP application entry point + vcxproj + Package.appxmanifest + run.config |
| `tools/pe_import_scanner.py` | Scans game .exe, cross-references against the shim registry, reports missing APIs |
| `tools/test_pe_scanner.py` | Unit tests for the scanner (9 tests, all pass) |
| `host-agent/host_agent.py` | HTTP server that scans Steam/Epic/GOG, serves game list, pushes to Xbox |
| `winheaders/Windows.h` | Stub Win32 SDK header for `g++ -fsyntax-only` syntax checking on Linux |
| `scripts/check_all.sh` | Runs `g++ -fsyntax-only` on every .cpp file |
| `docs/COMPILE.md` | Build & deploy instructions (VS 2022 → .appx → Xbox Device Portal) |
| `docs/ARCHITECTURE.md` | Technical design: PE loader pipeline, IAT rewriting, D3D11 bridge, threat model |
| `docs/SHIM_COVERAGE.md` | Per-DLL API count and coverage table |
| `docs/HONEST_STATUS.md` | What works, what doesn't, roadmap to Half Sword playable |

## Quick start

### Build

```bash
# On Linux: verify syntax first (no compilation needed)
cd xbox-win32-runner
bash scripts/check_all.sh    # expects: "34 ok, 0 failed"
```

```powershell
# On Windows: open the UWP project in Visual Studio 2022
# (requires "Universal Windows Platform development" workload + v143 C++ tools)
devenv uwp-shell\XboxWin32Runner.vcxproj
# Pick Release|Xbox → Build → produces XboxWin32Runner.appx
```

### Test the PE scanner

```bash
python3 tools/pe_import_scanner.py --input /path/to/installed/game --format markdown
```

### Run the host agent

```bash
python3 host-agent/host_agent.py --port 8000 --xbox 192.168.1.100:11443
# Then browse to http://localhost:8000/games
```

## Build status

- **Syntax check (g++ -std=c++17 -fsyntax-only)**: 34/34 files pass
- **PE scanner unit tests**: 9/9 pass
- **Host agent smoke test**: `/health` and `/games` return JSON correctly
- **UWP build (VS 2022)**: vcxproj is configured (compile results not verified on real Xbox hardware)
- **Half Sword playable**: NOT YET — see `docs/HONEST_STATUS.md` for the roadmap

## License

Open-source for educational purposes. Sideloading UWP apps on Xbox Developer
Mode is officially supported by Microsoft. Running retail games on hardware you
own is generally legal (varies by jurisdiction). This project does NOT
circumvent DRM — it only loads games you already own.

## See also

- `docs/COMPILE.md` — full build instructions
- `docs/ARCHITECTURE.md` — technical design
- `docs/HONEST_STATUS.md` — what works and what doesn't
