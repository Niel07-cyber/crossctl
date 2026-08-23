#!/usr/bin/env bash
# REQ-003: A barrier movement that does not confirm within the deadline shall fault.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start --timeout 3 || exit 1

out="$(ctl_session TRAIN_DETECTED TRAIN_DETECTED TICK TICK TICK)"

assert_contains "closing entered"      "STATUS 2 CLOSING LOWER DANGER" "$out"
assert_contains "holds before deadline" "STATUS 4 CLOSING LOWER DANGER" "$out"
assert_contains "faults on deadline"   "STATUS 5 FAULT LOWER DANGER"   "$out"

case_summary
exit $?
