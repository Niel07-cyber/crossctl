#!/usr/bin/env bash
# REQ-021: A frame split across TCP reads shall be reassembled, not rejected.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session --split TRAIN_DETECTED TRAIN_DETECTED)"

errors="$(printf '%s\n' "$out" | grep -c 'ERROR' || true)"
assert_eq "no spurious errors"    "0" "$errors"
assert_contains "both processed"  "STATUS 2 CLOSING LOWER DANGER" "$out"

case_summary
exit $?
