#!/usr/bin/env bash
# REQ-005: Only barrier movements have a deadline; stable states shall not fault.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start --timeout 2 || exit 1

out="$(ctl_session TICK TICK TICK TICK TICK TICK)"
fault_count="$(printf '%s\n' "$out" | grep -c 'FAULT' || true)"
assert_eq "IDLE survives repeated ticks" "0" "$fault_count"

out2="$(ctl_session TRAIN_DETECTED TRAIN_DETECTED BARRIER_DOWN \
                    TICK TICK TICK TICK)"
assert_contains "CLOSED survives ticks" "STATUS 7 CLOSED HOLD DANGER" "$out2"

case_summary
exit $?
