#!/usr/bin/env bash
# REQ-020: Several frames arriving in one read shall each be processed.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session --burst TRAIN_DETECTED TRAIN_DETECTED BARRIER_DOWN)"

replies="$(printf '%s\n' "$out" | grep -c 'STATUS' || true)"
assert_eq "three frames, three replies" "3" "$replies"
assert_contains "final state closed" "STATUS 3 CLOSED HOLD DANGER" "$out"

case_summary
exit $?
