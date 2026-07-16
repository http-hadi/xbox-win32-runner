#!/usr/bin/env bash
# scripts/run_all_tests.sh
# Run every layer of the test suite that doesn't require Xbox hardware.
#
# Layer 1: C++ syntax check     (g++ -std=c++17 -fsyntax-only)
# Layer 2: PE parser unit tests (tools/test_pe_scanner.py)
# Layer 3: PE parser cross-val  (tools/cross_validate_pe.py vs pefile)
# Layer 4: IAT resolution test  (tools/runtime_pe_loader_test.py)
# Layer 5: Host agent smoke     (start server, curl endpoints, kill)
#
# Exits 0 if every layer passes; 1 if any layer fails.

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PASS=0
FAIL=0

run_layer() {
    local name="$1"
    local cmd="$2"
    echo "============================================================"
    echo "  LAYER $name"
    echo "============================================================"
    if eval "$cmd"; then
        echo "  → PASS"
        PASS=$((PASS+1))
    else
        echo "  → FAIL"
        FAIL=$((FAIL+1))
    fi
    echo
}

# Layer 1
run_layer "1: C++ syntax check" "bash scripts/check_all.sh | tail -3 | grep -q '0 failed'"

# Layer 2
run_layer "2: PE parser unit tests" "python3 tools/test_pe_scanner.py 2>&1 | tail -3 | grep -q '^OK'"

# Layer 3
run_layer "3: PE parser cross-validation vs pefile" \
    "python3 tools/cross_validate_pe.py 2>&1 | tail -2 | grep -q '0 fail'"

# Layer 4
run_layer "4: IAT resolution test (synthetic game)" \
    "python3 tools/runtime_pe_loader_test.py 2>&1 | grep -q 'Grand total'"

# Layer 5
run_layer "5: Host agent smoke test" '
    python3 host-agent/host_agent.py --port 8790 --scan-dir test/ >/tmp/ha_test.log 2>&1 &
    HA_PID=$!
    sleep 2
    H=$(curl -s http://localhost:8790/health)
    G=$(curl -s http://localhost:8790/games)
    S=$(curl -s -X POST http://localhost:8790/scan)
    kill $HA_PID 2>/dev/null
    wait 2>/dev/null
    echo "$H" | grep -q "\"status\": \"ok\"" && \
    echo "$G" | grep -q "SampleGame" && \
    echo "$S" | grep -q "\"ok\": true"
'

# Layer 6: App wiring test — verifies CoreWindow/HWND interop, keyboard event
# wiring, and XInput polling frequency using the WinRT stubs.
run_layer "6: App wiring test (CoreWindow/HWND/XInput)" '
    g++ -std=c++17 \
        -I winheaders -I pe-loader -I shims -I shims/kernel32 \
        -I d3d11-bridge -I bridges -I uwp-shell \
        -include shims/CommonPre.h \
        -D_WIN32 -D_WIN64 -DUNICODE -D_UNICODE -DXWR_SYNTAX_CHECK \
        test/test_app_wiring.cpp test/stub_win32_impl.cpp \
        shims/ShimRegistry.cpp shims/kernel32/PathTranslator.cpp \
        -o /tmp/test_app_wiring -lpthread 2>&1 && \
    /tmp/test_app_wiring 2>&1 | tail -3 | grep -q "0 fail"
'

# Layer 7: UWP runtime simulation — simulates Half Sword's 488 API calls
# in a virtual AppContainer environment and verifies 0 missing shims.
run_layer "7: UWP runtime simulation (Half Sword startup)" '
    python3 tools/game_simulator.py --game halfsword 2>&1 | \
        grep "MISSING" | grep -q "0 (0.0%)"
'

# Layer 8: Win32 → UWP translation verifier — executes the real translation
# code paths and verifies CreateFileW→CreateFile2, VirtualAlloc→VirtualAllocFromApp,
# path translation, identity stubs, D3D11 FL clamping, CoreWindow HWND interop.
run_layer "8: Win32→UWP translation verifier" '
    g++ -std=c++17 \
        -I winheaders -I pe-loader -I shims -I shims/kernel32 \
        -I d3d11-bridge -I bridges -I uwp-shell \
        -include shims/CommonPre.h \
        -D_WIN32 -D_WIN64 -DUNICODE -D_UNICODE -DXWR_SYNTAX_CHECK \
        test/test_win32_to_uwp_translation.cpp test/stub_win32_impl.cpp \
        shims/kernel32/Win32ToUwpTranslator.cpp \
        shims/kernel32/PathTranslator.cpp \
        shims/ShimRegistry.cpp \
        -o /tmp/test_translation -lpthread 2>&1 && \
    /tmp/test_translation 2>&1 | tail -3 | grep -q "0 fail"
'

echo "============================================================"
echo "  SUMMARY:  $PASS layers passed, $FAIL failed"
echo "============================================================"
exit $FAIL
