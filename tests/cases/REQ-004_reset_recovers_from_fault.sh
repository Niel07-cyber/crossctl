#!/usr/bin/env bash
# REQ-004: FAULT shall be exited only by an explicit RESET.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start --timeout 2 || exit 1

out="$(ctl_session TRAIN_DETECTED TRAIN_DETECTED TICK TICK \
                   BARRIER_DOWN TRAIN_CLEARED RESET)"

assert_contains "faulted"                "STATUS 4 FAULT LOWER DANGER" "$out"
assert_contains "ignores BARRIER_DOWN"   "STATUS 5 FAULT LOWER DANGER" "$out"
assert_contains "ignores TRAIN_CLEARED"  "STATUS 6 FAULT LOWER DANGER" "$out"
assert_contains "RESET returns to idle"  "STATUS 7 IDLE RAISE CLEAR"   "$out"

case_summary
exit $?
