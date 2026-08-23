#!/usr/bin/env bash
# REQ-014: A frame failing validation shall not be acted upon.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session '!TRAIN_DETECTED' TRAIN_DETECTED)"

assert_contains "corrupt frame rejected" "ERROR 0 crc mismatch"            "$out"
assert_contains "still starts from idle" "STATUS 2 APPROACHING LOWER DANGER" "$out"

case_summary
exit $?
