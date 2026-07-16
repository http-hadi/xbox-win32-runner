#!/usr/bin/env bash
# scripts/check_all.sh
# Run g++ -fsyntax-only against every .cpp in the project, using the stub
# winheaders/Windows.h and forced-include CommonPre.h.
#
# Usage: scripts/check_all.sh [pattern]
#   pattern  optional grep -E filter for file paths (e.g. "pe-loader|shims")

set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WINHEADERS="$ROOT/winheaders"
COMMONPRE="$ROOT/shims/CommonPre.h"
PATTERN="${1:-.}"

if [[ ! -f "$WINHEADERS/Windows.h" ]]; then
    echo "FATAL: $WINHEADERS/Windows.h not found" >&2
    exit 2
fi
if [[ ! -f "$COMMONPRE" ]]; then
    echo "FATAL: $COMMONPRE not found" >&2
    exit 2
fi

# Collect every .cpp file under the project, excluding tools/host-agent (Python).
mapfile -t FILES < <(find "$ROOT" \
    -type d \( -name 'host-agent' -o -name 'tools' \) -prune -o \
    -type f -name '*.cpp' -print | grep -E "$PATTERN" | sort)

if [[ "${#FILES[@]}" -eq 0 ]]; then
    echo "No .cpp files matched pattern '$PATTERN'." >&2
    exit 0
fi

PASS=0
FAIL=0
FAILED_FILES=()

for F in "${FILES[@]}"; do
    OUT=$(g++ -std=c++17 -fsyntax-only -fno-exceptions -fno-rtti \
        -I"$WINHEADERS" \
        -I"$ROOT" \
        -I"$ROOT/pe-loader" \
        -I"$ROOT/shims" \
        -I"$ROOT/shims/kernel32" \
        -I"$ROOT/shims/user32" \
        -I"$ROOT/shims/gdi32" \
        -I"$ROOT/shims/advapi32" \
        -I"$ROOT/shims/shell32" \
        -I"$ROOT/shims/shlwapi" \
        -I"$ROOT/shims/ole32" \
        -I"$ROOT/shims/winmm" \
        -I"$ROOT/shims/version" \
        -I"$ROOT/shims/pass-through" \
        -I"$ROOT/shims/stubs" \
        -I"$ROOT/shims/legacy" \
        -I"$ROOT/d3d11-bridge" \
        -I"$ROOT/bridges" \
        -I"$ROOT/uwp-shell" \
        -include "$COMMONPRE" \
        -D_WIN32 -D_WIN64 -DUNICODE -D_UNICODE -DXWR_SYNTAX_CHECK \
        "$F" 2>&1)
    if [[ -z "$OUT" ]]; then
        printf '%-90s OK\n' "$F"
        PASS=$((PASS+1))
    else
        printf '%-90s FAIL\n' "$F"
        printf '%s\n' "$OUT" | head -n 40 | sed 's|^|    |'
        FAIL=$((FAIL+1))
        FAILED_FILES+=("$F")
    fi
done

echo
echo "============================================================"
echo "  syntax check summary:  $PASS ok, $FAIL failed"
if [[ "$FAIL" -gt 0 ]]; then
    echo "  Failed files:"
    for F in "${FAILED_FILES[@]}"; do
        echo "    - $F"
    done
    exit 1
fi
exit 0
