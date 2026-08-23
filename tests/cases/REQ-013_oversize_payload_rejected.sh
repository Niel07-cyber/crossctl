#!/usr/bin/env bash
# REQ-013: A declared payload length beyond the maximum shall be rejected.
# shellcheck source=tests/lib/harness.sh
source "$(dirname "$0")/../lib/harness.sh"

ctl_start || exit 1

big="$(python3 -c 'print("A" * 300)')"
out="$(ctl_session "$big")"

assert_contains "oversize length rejected" "ERROR 0 bad length" "$out"
assert_contains "logged by server"         "reject: bad length" "$(ctl_log)"

case_summary
exit $?
