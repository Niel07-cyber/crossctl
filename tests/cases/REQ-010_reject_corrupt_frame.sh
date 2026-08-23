#!/usr/bin/env bash
# REQ-010: Frames failing CRC validation shall be rejected, not acted upon.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session '!TRAIN_DETECTED')"
assert_contains "corrupt frame rejected" "ERROR 0 crc mismatch" "$out"
assert_contains "logged by server"       "reject: crc mismatch" "$(ctl_log)"

case_summary
