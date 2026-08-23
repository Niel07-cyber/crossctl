#!/usr/bin/env bash
# REQ-022: Each reply shall carry the sequence number of the frame it answers.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

out="$(ctl_session TRAIN_DETECTED TRAIN_DETECTED BARRIER_DOWN TRAIN_CLEARED)"
seqs="$(printf '%s\n' "$out" | awk '{print $2}' | tr '\n' ' ' | sed 's/ $//')"

assert_eq "sequence numbers echoed in order" "1 2 3 4" "$seqs"

case_summary
exit $?
