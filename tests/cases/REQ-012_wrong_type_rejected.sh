#!/usr/bin/env bash
# REQ-012: Frames whose type is not EVENT shall be rejected.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session '@0x20:TRAIN_DETECTED' TRAIN_DETECTED)"

assert_contains "status-typed frame rejected" "ERROR 1 unexpected type"  "$out"
assert_contains "state unaffected"            "STATUS 2 APPROACHING LOWER DANGER" "$out"

case_summary
exit $?
