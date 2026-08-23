#!/usr/bin/env bash
# Discover and run every case in tests/cases.
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${BIN:-$ROOT/build/crossctl}"

if [ ! -x "$BIN" ]; then
  echo "error: $BIN not built. Run 'make' first." >&2
  exit 1
fi

filter="${1:-}"
total=0; failed=0

echo "crossctl integration suite"
echo "binary: $BIN"
echo

for case_file in "$ROOT"/tests/cases/*.sh; do
  name="$(basename "$case_file" .sh)"
  [ -n "$filter" ] && [[ "$name" != *"$filter"* ]] && continue

  req="$(grep -m1 '^# REQ-' "$case_file" | sed 's/^# //')"
  total=$((total + 1))
  echo "[$name]"
  [ -n "$req" ] && echo "  $req"

  if bash "$case_file"; then :; else failed=$((failed + 1)); fi
  echo
done

echo "=========================================="
if [ "$failed" -eq 0 ]; then
  echo "ALL $total CASE(S) PASSED"
else
  echo "$failed of $total CASE(S) FAILED"
fi
exit $((failed > 0))
