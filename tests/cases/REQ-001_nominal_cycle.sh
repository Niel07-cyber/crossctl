#!/usr/bin/env bash
# REQ-001: A full crossing cycle returns the barrier to the raised position.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session TRAIN_DETECTED TRAIN_DETECTED BARRIER_DOWN TRAIN_CLEARED BARRIER_UP)"

assert_contains "approach reported"  "STATUS 1 APPROACHING LOWER DANGER" "$out"
assert_contains "closing reported"   "STATUS 2 CLOSING LOWER DANGER"     "$out"
assert_contains "closed reported"    "STATUS 3 CLOSED HOLD DANGER"       "$out"
assert_contains "opening reported"   "STATUS 4 OPENING RAISE DANGER"     "$out"
assert_contains "returns to idle"    "STATUS 5 IDLE RAISE CLEAR"         "$out"

case_summary
