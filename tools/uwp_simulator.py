#!/usr/bin/env python3
"""
tools/uwp_simulator.py

A Windows 10 IoT Core UWP runtime simulator that emulates the AppContainer
environment our Xbox Win32 Runner targets.

This simulator:
  1. Loads our shim registry (extracted from shims/*.cpp REGISTER_SHIM calls)
  2. Emulates the UWP AppContainer sandbox:
     - Virtual filesystem (C:\Game\ → LocalState\Game\)
     - Virtual registry (in-memory)
     - Virtual D3D11 device (returns success, no rendering)
     - Virtual XInput (4 controller slots)
     - AppContainer restrictions (no CreateProcess, no low ports, no drivers)
  3. Runs a "game simulator" that replays Half Sword's API call sequence
  4. Reports which APIs succeed/fail in the UWP environment

Usage:
  python3 tools/uwp_simulator.py [--verbose] [--game halfsword|test_hello]

The game simulator calls every API Half Sword imports (based on the coverage
report) in the order a UE5 game would call them during startup. For each call:
  - If our shim registry has the function → "RESOLVED" (would work on Xbox)
  - If UWP blocks the API → "BLOCKED" (AppContainer restriction)
  - If no shim exists → "MISSING" (would crash on Xbox)

This is the closest we can get to runtime testing without Xbox hardware.
"""
from __future__ import annotations

import argparse
import os
import sys
import json
import time
from collections import defaultdict, OrderedDict
from dataclasses import dataclass, field
from typing import Any, Callable, Dict, List, Optional, Set, Tuple

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from pe_import_scanner import build_shim_registry, normalize_dll_key  # type: ignore

# ---------------------------------------------------------------------------
# UWP AppContainer restrictions
# ---------------------------------------------------------------------------
# These APIs are BLOCKED in UWP AppContainer mode. If the game calls them,
# the real UWP runtime returns ERROR_ACCESS_DENIED or similar.
BLOCKED_APIS: Set[Tuple[str, str]] = {
    # Process creation — AppContainer can't launch external processes
    ("kernel32", "createprocessa"),
    ("kernel32", "createprocessw"),
    ("kernel32", "createremotethread"),
    # Job objects — blocked
    ("kernel32", "createjobobjecta"),
    ("kernel32", "createjobobjectw"),
    ("kernel32", "assignprocesstojobobject"),
    ("kernel32", "setinformationjobobject"),
    # Driver loading — blocked
    ("kernel32", "loadlibrarya"),  # Actually allowed but only for system DLLs
    ("setupapi", "setupdigetclassdevsw"),
    # Low port binding — blocked (< 1024)
    ("ws2_32", "bind"),  # Allowed but restricted
    # Registry — HKLM write blocked
    ("advapi32", "regcreatekeyexw"),  # Allowed for HKCU only
    # Console — limited
    ("kernel32", "allocconsole"),
    # Service control — blocked
    ("advapi32", "openscmanagerw"),
    ("advapi32", "createservicew"),
    ("advapi32", "startservicew"),
}

# APIs that UWP supports natively (pass-through to real OS)
NATIVE_UWP_APIS: Set[str] = {
    "d3d11", "d3d12", "dxgi", "d2d1", "dwrite", "windowscodecs",
    "xinput1_4", "xinput1_3", "xinput1_2", "xinput9_1_0",
    "xaudio2_9", "xaudio2_8", "xaudio2_7",
    "ws2_32", "crypt32", "bcrypt", "ncrypt",
    "ucrtbase", "msvcrt", "msvcp140", "vcruntime140", "vcruntime140_1",
    "msvcp140_atomic_wait",
}

# ---------------------------------------------------------------------------
# UWP Runtime State
# ---------------------------------------------------------------------------
@dataclass
class VirtualFile:
    path: str
    data: bytes = b""
    position: int = 0
    handle: int = 0

@dataclass
class VirtualRegistryKey:
    path: str
    values: Dict[str, Tuple[int, bytes]] = field(default_factory=dict)  # name → (type, data)
    subkeys: Dict[str, 'VirtualRegistryKey'] = field(default_factory=dict)

@dataclass
class VirtualD3D11Device:
    feature_level: int = 0xB000  # D3D_FEATURE_LEVEL_11_0
    created: bool = False
    swap_chain: bool = False

@dataclass
class VirtualXInputState:
    connected: List[bool] = field(default_factory=lambda: [False, False, False, False])
    states: List[Dict] = field(default_factory=lambda: [{} for _ in range(4)])

@dataclass
class CallResult:
    api: str           # e.g. "kernel32!CreateFileW"
    status: str        # "RESOLVED", "BLOCKED", "MISSING", "STUB"
    detail: str = ""
    timestamp: float = 0.0

class UWPRuntime:
    """Simulates the Windows 10 IoT Core UWP AppContainer environment."""

    def __init__(self, shim_registry: Set[Tuple[str, str]]):
        self.shim_registry = shim_registry
        self.files: Dict[int, VirtualFile] = {}
        self.next_handle = 0x1000
        self.registry_root = VirtualRegistryKey(path="")
        self.d3d11 = VirtualD3D11Device()
        self.xinput = VirtualXInputState()
        self.call_log: List[CallResult] = []
        self.verbose = False

        # Initialize virtual registry with common keys games read
        self._init_registry()

    def _init_registry(self):
        """Populate the virtual registry with common game-read keys."""
        # HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion
        hklm = self.registry_root.subkeys.setdefault("hklm", VirtualRegistryKey("hklm"))
        winnt = hklm.subkeys.setdefault("software\\microsoft\\windows nt\\currentversion",
                                         VirtualRegistryKey("software\\microsoft\\windows nt\\currentversion"))
        winnt.values["ProductName"] = (1, b"Windows 10 IoT Core\x00".decode("utf-8").encode("utf-16-le"))
        winnt.values["CurrentBuild"] = (1, b"22621\x00".decode("utf-8").encode("utf-16-le"))
        winnt.values["CurrentVersion"] = (1, b"6.3\x00".decode("utf-8").encode("utf-16-le"))

        # HKCU\SOFTWARE\Valve\Steam (Steam stub returns offline)
        hkcu = self.registry_root.subkeys.setdefault("hkcu", VirtualRegistryKey("hkcu"))
        steam = hkcu.subkeys.setdefault("software\\valve\\steam",
                                         VirtualRegistryKey("software\\valve\\steam"))
        steam.values["SteamPath"] = (1, b"C:\\Program Files (x86)\\Steam\x00".decode("utf-8").encode("utf-16-le"))

    def log_call(self, dll: str, func: str, status: str, detail: str = ""):
        """Record an API call result."""
        api = f"{dll}!{func}"
        result = CallResult(
            api=api,
            status=status,
            detail=detail,
            timestamp=time.time(),
        )
        self.call_log.append(result)
        if self.verbose:
            marker = {"RESOLVED": "✓", "BLOCKED": "✗", "MISSING": "?", "STUB": "○", "NATIVE": "★"}
            print(f"  {marker.get(status, '?')} {api:50s} {status:8s} {detail}")
        return result

    def resolve_api(self, dll: str, func: str) -> CallResult:
        """Check if an API call would succeed in the UWP environment.

        Returns a CallResult with status:
          RESOLVED — our shim handles it (would work on Xbox)
          BLOCKED  — UWP AppContainer blocks this API
          NATIVE   — UWP supports it natively (pass-through)
          STUB     — our shim stubs it (returns failure, game degrades)
          MISSING  — no shim exists (would crash on Xbox)
        """
        dll_key = normalize_dll_key(dll)
        func_lower = func.lower()

        # 1) Check if AppContainer blocks this API
        if (dll_key, func_lower) in BLOCKED_APIS:
            return self.log_call(dll, func, "BLOCKED", "AppContainer restriction")

        # 2) Check if our shim registry has it
        if (dll_key, func_lower) in self.shim_registry:
            # Check if it's a native UWP DLL (pass-through)
            if dll_key in NATIVE_UWP_APIS:
                return self.log_call(dll, func, "NATIVE", "UWP pass-through")
            # Check if it's a stub DLL (returns failure)
            stub_dlls = {"steam_api64", "steam_api", "discord_game_sdk",
                        "discord_partner_sdk", "dsound", "mfplat", "mfreadwrite",
                        "mf", "avrt", "setupapi", "netapi32", "qwave", "ktmw32",
                        "uiautomationcore", "dxcore", "python311",
                        "gfsdk_aftermath_lib.x64", "nvlowlatentvk", "openimagedenoise",
                        "discord-rpc", "d3d9", "d3d8", "ddraw", "opengl32",
                        "cabinet", "msi", "dbghelp", "mmdevapi", "msdmo",
                        "propsys", "uxtheme", "wininet", "wintrust", "imm32"}
            if dll_key in stub_dlls:
                return self.log_call(dll, func, "STUB", "stub returns failure")
            return self.log_call(dll, func, "RESOLVED", "shim handles it")

        # 3) Not in shim registry — would crash
        return self.log_call(dll, func, "MISSING", "no shim registered")

    # -----------------------------------------------------------------------
    # Virtual filesystem operations
    # -----------------------------------------------------------------------
    def virtual_create_file(self, path: str, access: int) -> int:
        """Simulate CreateFileW with path translation."""
        # Translate C:\Game\... → LocalState\Game\...
        translated = path.replace("C:\\Game\\", "LocalState\\Game\\")
        handle = self.next_handle
        self.next_handle += 1
        self.files[handle] = VirtualFile(path=translated)
        return handle

    def virtual_read_file(self, handle: int, size: int) -> bytes:
        """Simulate ReadFile."""
        f = self.files.get(handle)
        if not f:
            return b""
        data = f.data[f.position:f.position + size]
        f.position += len(data)
        return data

    def virtual_close_handle(self, handle: int) -> bool:
        """Simulate CloseHandle."""
        if handle in self.files:
            del self.files[handle]
            return True
        return False

    # -----------------------------------------------------------------------
    # Summary report
    # -----------------------------------------------------------------------
    def report(self) -> str:
        """Generate a summary report of all API calls."""
        by_status = defaultdict(int)
        by_dll = defaultdict(lambda: defaultdict(int))
        for r in self.call_log:
            by_status[r.status] += 1
            dll = r.api.split("!")[0].lower()
            by_dll[dll][r.status] += 1

        total = len(self.call_log)
        lines = []
        lines.append("=" * 70)
        lines.append("  UWP RUNTIME SIMULATION REPORT")
        lines.append("=" * 70)
        lines.append(f"  Total API calls simulated: {total}")
        lines.append(f"  RESOLVED (shim works):     {by_status['RESOLVED']:6d} ({100*by_status['RESOLVED']/max(1,total):.1f}%)")
        lines.append(f"  NATIVE (UWP pass-through): {by_status['NATIVE']:6d} ({100*by_status['NATIVE']/max(1,total):.1f}%)")
        lines.append(f"  STUB (returns failure):    {by_status['STUB']:6d} ({100*by_status['STUB']/max(1,total):.1f}%)")
        lines.append(f"  BLOCKED (AppContainer):    {by_status['BLOCKED']:6d} ({100*by_status['BLOCKED']/max(1,total):.1f}%)")
        lines.append(f"  MISSING (would crash):     {by_status['MISSING']:6d} ({100*by_status['MISSING']/max(1,total):.1f}%)")
        lines.append("")
        lines.append("  Per-DLL breakdown:")
        lines.append(f"    {'DLL':<30s} {'RESOLVED':>8s} {'NATIVE':>7s} {'STUB':>5s} {'BLOCKED':>7s} {'MISSING':>7s}")
        lines.append(f"    {'-'*30} {'-'*8} {'-'*7} {'-'*5} {'-'*7} {'-'*7}")
        for dll in sorted(by_dll.keys()):
            s = by_dll[dll]
            total_dll = sum(s.values())
            lines.append(f"    {dll:<30s} {s.get('RESOLVED',0):>8d} {s.get('NATIVE',0):>7d} {s.get('STUB',0):>5d} {s.get('BLOCKED',0):>7d} {s.get('MISSING',0):>7d}")
        lines.append("=" * 70)

        # List missing APIs (would crash)
        missing = [r for r in self.call_log if r.status == "MISSING"]
        if missing:
            lines.append(f"\n  CRITICAL: {len(missing)} APIs have no shim (game would crash):")
            for r in missing[:20]:
                lines.append(f"    {r.api}")
            if len(missing) > 20:
                lines.append(f"    ... and {len(missing)-20} more")

        # List blocked APIs
        blocked = [r for r in self.call_log if r.status == "BLOCKED"]
        if blocked:
            lines.append(f"\n  WARNING: {len(blocked)} APIs blocked by AppContainer (game must handle gracefully):")
            for r in blocked[:10]:
                lines.append(f"    {r.api} — {r.detail}")

        return "\n".join(lines)
