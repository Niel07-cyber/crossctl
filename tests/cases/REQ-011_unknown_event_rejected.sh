#!/usr/bin/env bash
# REQ-011: Unrecognised event payloads shall be rejected without changing state.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session LAUNCH_ROCKET TRAIN_DETECTED)"

assert_contains "unknown event rejected" "ERROR 1 unknown event"        "$out"
assert_contains "state unaffected"       "STATUS 2 APPROACHING LOWER DANGER" "$out"

case_summary
exit $?
