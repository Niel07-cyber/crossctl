#!/usr/bin/env bash
# REQ-002: The signal shall never show CLEAR while the crossing is not idle.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session TRAIN_DETECTED TRAIN_DETECTED BARRIER_DOWN)"

clear_count="$(printf '%s\n' "$out" | grep -c 'CLEAR' || true)"
assert_eq "no CLEAR aspect while occupied" "0" "$clear_count"

case_summary
