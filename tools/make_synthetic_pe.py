#!/usr/bin/env python3
# =============================================================================
# make_synthetic_pe.py
# =============================================================================
# Helper script that emits a small synthetic 64-bit PE file into the project's
# test/ directory, so the pe_import_scanner can be run end-to-end against a
# known-good fixture.
#
# The fixture imports a mix of:
#   - shimmed functions (kernel32!CreateFileW, user32!MessageBoxW)
#   - unshimmed functions (kernel32!SomeFakeFunc, totally_unknown.dll!Mystery)
#   - a delay-load import (version.dll!GetFileVersionInfoW)
#
# Usage:  python3 tools/make_synthetic_pe.py [output_path]
# =============================================================================
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_pe_scanner import (
    build_minimal_x64_pe,
    build_minimal_x64_pe_with_delay_import,
)


def main(argv=None) -> int:
    argv = argv or sys.argv[1:]
    if argv:
        out_dir = argv[0]
    else:
        # Default: test/SampleGame/ — putting the .exe inside a sub-folder
        # makes it discoverable by host_agent's `--scan-dir test/` (which
        # treats each immediate sub-folder as a game).
        here = os.path.dirname(os.path.abspath(__file__))
        proj_root = os.path.dirname(here)
        out_dir = os.path.join(proj_root, "test", "SampleGame")
    os.makedirs(out_dir, exist_ok=True)

    out_path = os.path.join(out_dir, "SyntheticGame.exe")

    # Two-file "game": a main .exe with mixed imports + a delay-import .dll.
    main_pe = build_minimal_x64_pe(imports=[
        ("kernel32.dll", ["CreateFileW", "CloseHandle", "SomeFakeKernelFunc"]),
        ("user32.dll",   ["MessageBoxW", "GetForegroundWindow"]),
        ("ws2_32.dll",   ["WSAStartup"]),
        ("totally_unknown.dll", ["MysteryFunc"]),
    ])
    with open(out_path, "wb") as fh:
        fh.write(main_pe)
    print(f"wrote {out_path} ({len(main_pe)} bytes)")

    # Companion DLL with a delay-load import.
    companion_path = os.path.join(out_dir, "GameHelper.dll")
    companion_pe = build_minimal_x64_pe_with_delay_import(
        "version.dll", ["GetFileVersionInfoW", "VerQueryValueW"]
    )
    with open(companion_path, "wb") as fh:
        fh.write(companion_pe)
    print(f"wrote {companion_path} ({len(companion_pe)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
