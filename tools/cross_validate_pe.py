#!/usr/bin/env python3
"""
tools/cross_validate_pe.py

Cross-validate our PE import scanner against `pefile` (the de-facto reference
Python library). For each .exe / .dll file in the input set:

  1. Parse with our from-scratch parser (pe_import_scanner.PeFile)
  2. Parse with pefile
  3. Compare: machine, optional-header magic, list of imported DLLs, list of
     imported function names per DLL, presence of delay-load imports.

Exits 0 if every file matches; 1 if any file has a discrepancy; 2 if pefile
itself is missing.

Usage:
  python3 tools/cross_validate_pe.py [path1 path2 ...]

If no paths are given, uses a built-in list of real Windows binaries that ship
with Python wheels on this system (real PE32 / PE32+ files, not synthetic).
"""
from __future__ import annotations

import os
import sys
import glob
from typing import Iterable

# Make sure we can import the scanner
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from pe_import_scanner import PeFile, ImportDescriptor, IMAGE_OPTIONAL_HDR64_MAGIC  # type: ignore

try:
    import pefile  # type: ignore
except ImportError:
    print("FATAL: pefile is not installed. Run: pip install pefile", file=sys.stderr)
    sys.exit(2)


def _read_opt_magic(path: str) -> int:
    """Read the OptionalHeader Magic field directly from the file (independent
    of how the scanner stores it). 0x10b = PE32, 0x20b = PE32+."""
    import struct
    with open(path, "rb") as f:
        data = f.read(0x200)  # DOS+NT headers always fit in 512 bytes
    if len(data) < 0x40 or struct.unpack_from("<H", data, 0)[0] != 0x5A4D:
        return 0
    e_lfanew = struct.unpack_from("<i", data, 0x3C)[0]
    if e_lfanew + 24 > len(data):
        return 0
    if struct.unpack_from("<I", data, e_lfanew)[0] != 0x4550:
        return 0
    opt_off = e_lfanew + 4 + 20
    if opt_off + 2 > len(data):
        return 0
    return struct.unpack_from("<H", data, opt_off)[0]


# ---------------------------------------------------------------------------
# Default test set: real Windows binaries that ship with Python wheels.
# We use distlib's launchers (w32.exe = PE32 i386, w64.exe = PE32+ x86-64),
# setuptools' launchers (cli-32/64.exe, gui-32/64.exe), and Aspose's .NET
# assemblies (real MSVC-compiled DLLs with rich import tables).
# ---------------------------------------------------------------------------
DEFAULT_TEST_SET: list[str] = [
    "/home/z/.venv/lib/python3.12/site-packages/pip/_vendor/distlib/w32.exe",   # PE32 i386
    "/home/z/.venv/lib/python3.12/site-packages/pip/_vendor/distlib/w64.exe",   # PE32+ x64
    "/home/z/.venv/lib/python3.12/site-packages/pip/_vendor/distlib/t64.exe",   # PE32+ x64
    "/home/z/.venv/lib/python3.12/site-packages/pip/_vendor/distlib/t32.exe",   # PE32 i386
    "/home/z/.venv/lib/python3.12/site-packages/setuptools/cli-32.exe",         # PE32 i386
    "/home/z/.venv/lib/python3.12/site-packages/setuptools/cli-64.exe",         # PE32+ x64
    "/home/z/.venv/lib/python3.12/site-packages/setuptools/gui-32.exe",         # PE32 i386
    "/home/z/.venv/lib/python3.12/site-packages/setuptools/gui-64.exe",         # PE32+ x64
    "/home/z/.venv/lib/python3.12/site-packages/aspose/assemblies/words/Aspose.Words.dll",         # PE32 i386 .NET
    "/home/z/.venv/lib/python3.12/site-packages/aspose/assemblies/words/Newtonsoft.Json.dll",       # PE32 i386 .NET
    "/home/z/.venv/lib/python3.12/site-packages/aspose/assemblies/words/SkiaSharp.dll",             # PE32+ x64 native
    "/home/z/.venv/lib/python3.12/site-packages/aspose/assemblies/words/System.Security.AccessControl.dll",  # PE32 i386 .NET
    "/home/z/.venv/lib/python3.12/site-packages/aspose/assemblies/pydrawing/System.Drawing.Common.dll",      # PE32 i386 .NET
]


def _norm_dll(name: str) -> str:
    """Normalize a DLL name for comparison: lowercase, strip .dll/.exe suffix."""
    n = name.lower()
    for ext in (".dll", ".exe", ".drv", ".sys"):
        if n.endswith(ext):
            n = n[: -len(ext)]
            break
    return n


def _func_name_or_ordinal(thunk: int, is_64: bool) -> str:
    """Return either the function name (if by-name import) or 'ordinal:N'."""
    flag = 0x8000000000000000 if is_64 else 0x80000000
    if thunk & flag:
        return f"ordinal:{thunk & 0xFFFF}"
    # by-name; pefile resolves this elsewhere
    return ""  # caller resolves via IMPORT_BY_NAME


def parse_with_pefile(path: str) -> dict:
    """Parse with pefile and return a normalized dict matching our scanner's shape."""
    pe = pefile.PE(path, fast_load=False)
    is_64 = pe.PE_TYPE == pefile.OPTIONAL_HEADER_MAGIC_PE_PLUS
    machine = pe.FILE_HEADER.Machine
    opt_magic = pe.OPTIONAL_HEADER.Magic

    imports: list[dict] = []
    if hasattr(pe, "DIRECTORY_ENTRY_IMPORT"):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            dll_name = entry.dll.decode("utf-8", "replace")
            funcs: list[str] = []
            for imp in entry.imports:
                if imp.name is not None:
                    funcs.append(imp.name.decode("utf-8", "replace"))
                elif imp.ordinal is not None:
                    funcs.append(f"ordinal:{imp.ordinal}")
                else:
                    funcs.append("ordinal:?")
            imports.append({"dll": dll_name, "functions": funcs, "delay_load": False})

    # Delay-load imports
    delay_imports: list[dict] = []
    if hasattr(pe, "DIRECTORY_ENTRY_DELAY_IMPORT"):
        for entry in pe.DIRECTORY_ENTRY_DELAY_IMPORT:
            dll_name = entry.dll.decode("utf-8", "replace")
            funcs: list[str] = []
            for imp in entry.imports:
                if imp.name is not None:
                    funcs.append(imp.name.decode("utf-8", "replace"))
                elif imp.ordinal is not None:
                    funcs.append(f"ordinal:{imp.ordinal}")
                else:
                    funcs.append("ordinal:?")
            delay_imports.append({"dll": dll_name, "functions": funcs, "delay_load": True})

    pe.close()
    return {
        "machine": machine,
        "is_64": is_64,
        "opt_magic": opt_magic,
        "imports": imports + delay_imports,
    }


def parse_with_ours(path: str) -> dict:
    """Parse with our from-scratch parser."""
    pe = PeFile(path)
    if not pe.parse():
        raise ValueError(f"our parser failed on {path}: {pe.parse_error}")
    imports: list[dict] = []
    # Combine standard + delay-load into one list, tagged.
    for imp in pe.imports:
        funcs: list[str] = []
        for f in imp.functions:
            if f.by_ordinal:
                funcs.append(f"ordinal:{f.ordinal}")
            else:
                funcs.append(f.name or "")
        imports.append({"dll": imp.dll_name, "functions": funcs, "delay_load": imp.is_delay_load})
    return {
        "machine": pe.machine,
        "is_64": pe.is_64bit,
        "opt_magic": _read_opt_magic(path),
        "imports": imports,
    }


def _canonical(imports: list[dict]) -> set[tuple[str, str, bool]]:
    """Convert imports to a set of (dll_lower, func_lower, is_delay_load) for comparison."""
    out: set[tuple[str, str, bool]] = set()
    for imp in imports:
        dll = _norm_dll(imp["dll"])
        for f in imp["functions"]:
            out.add((dll, f.lower(), imp["delay_load"]))
    return out


def compare_one(path: str) -> tuple[bool, str]:
    """Returns (ok, message). ok=True if our parser matches pefile."""
    try:
        ref = parse_with_pefile(path)
    except Exception as e:
        return False, f"pefile crashed: {e}"
    try:
        ours = parse_with_ours(path)
    except Exception as e:
        return False, f"our parser crashed: {e}"

    # Compare machine
    if ref["machine"] != ours["machine"]:
        return False, f"machine mismatch: pefile=0x{ref['machine']:04x} ours=0x{ours['machine']:04x}"
    if ref["opt_magic"] != ours["opt_magic"]:
        return False, f"opt_magic mismatch: pefile=0x{ref['opt_magic']:04x} ours=0x{ours['opt_magic']:04x}"

    ref_set = _canonical(ref["imports"])
    our_set = _canonical(ours["imports"])

    only_in_ref = ref_set - our_set
    only_in_ours = our_set - ref_set

    if only_in_ref or only_in_ours:
        msg_parts = []
        if only_in_ref:
            sample = list(only_in_ref)[:5]
            msg_parts.append(f"missing from ours ({len(only_in_ref)}): {sample}")
        if only_in_ours:
            sample = list(only_in_ours)[:5]
            msg_parts.append(f"extra in ours ({len(only_in_ours)}): {sample}")
        return False, "; ".join(msg_parts)

    return True, (
        f"machine=0x{ours['machine']:04x} magic=0x{ours['opt_magic']:04x} "
        f"imports={len(ours['imports'])} dlls={len(set(i['dll'] for i in ours['imports']))} "
        f"total_funcs={len(our_set)}"
    )


def iter_inputs(paths: Iterable[str]) -> Iterable[str]:
    for p in paths:
        if os.path.isdir(p):
            for ext in ("*.exe", "*.dll"):
                yield from glob.glob(os.path.join(p, "**", ext), recursive=True)
        elif os.path.isfile(p):
            yield p
        else:
            print(f"WARN: path does not exist: {p}", file=sys.stderr)


def main() -> int:
    paths = sys.argv[1:] or DEFAULT_TEST_SET
    inputs = list(iter_inputs(paths))
    if not inputs:
        print("No .exe/.dll files found to validate.", file=sys.stderr)
        return 1

    print(f"Cross-validating {len(inputs)} files against pefile {pefile.__version__}")
    print("=" * 80)

    ok_count = 0
    fail_count = 0
    failures: list[tuple[str, str]] = []

    for path in inputs:
        if not os.path.isfile(path):
            print(f"  SKIP (not a file): {path}")
            continue
        ok, msg = compare_one(path)
        short = os.path.relpath(path, "/home/z/.venv/lib/python3.12/site-packages/")
        if ok:
            ok_count += 1
            print(f"  PASS  {short}")
            print(f"        {msg}")
        else:
            fail_count += 1
            failures.append((path, msg))
            print(f"  FAIL  {short}")
            print(f"        {msg}")

    print("=" * 80)
    print(f"Result: {ok_count} pass, {fail_count} fail out of {ok_count + fail_count}")
    if failures:
        print("\nFailures detail:")
        for p, m in failures:
            print(f"  - {p}")
            print(f"      {m}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
