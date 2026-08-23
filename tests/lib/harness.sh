# shellcheck shell=bash
# crossctl integration test harness.

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${BIN:-$ROOT/build/crossctl}"

PASS=0
FAIL=0
CTL_PID=""
CTL_PORT=""
CTL_LOG=""

free_port() {
  python3 -c 'import socket;s=socket.socket();s.bind(("127.0.0.1",0));print(s.getsockname()[1]);s.close()'
}

# ctl_start [--timeout N]  — extra args are forwarded to the binary.
# shellcheck disable=SC2120  # optional args; bare calls are valid
ctl_start() {
  CTL_PORT="$(free_port)"
  CTL_LOG="$(mktemp -t crossctl.XXXXXX)"
  "$BIN" --serve --port "$CTL_PORT" "$@" >"$CTL_LOG" 2>&1 &
  CTL_PID=$!

  # Poll for the readiness line rather than sleeping.
  local waited=0
  while ! grep -q "listening on" "$CTL_LOG" 2>/dev/null; do
    if ! kill -0 "$CTL_PID" 2>/dev/null; then
      echo "  server died on startup:"; sed 's/^/    /' "$CTL_LOG"; return 1
    fi
    sleep 0.05
    waited=$((waited + 1))
    if [ "$waited" -gt 100 ]; then
      echo "  server never became ready"; return 1
    fi
  done
  return 0
}

ctl_stop() {
  if [ -n "${CTL_PID:-}" ] && [ "$CTL_PID" -gt 1 ] 2>/dev/null; then
    kill -TERM "$CTL_PID" 2>/dev/null || true
    # Bounded wait, then escalate. Never block the suite on a stuck server.
    local n=0
    while kill -0 "$CTL_PID" 2>/dev/null; do
      sleep 0.05
      n=$((n + 1))
      if [ "$n" -ge 20 ]; then
        kill -KILL "$CTL_PID" 2>/dev/null || true
        break
      fi
    done
    wait "$CTL_PID" 2>/dev/null || true
  fi
  CTL_PID=""
}

ctl_log() { cat "$CTL_LOG"; }

# ctl_session EVENT[:SEQ] ... -> decoded reply lines on stdout
# Prefix an event with '!' to corrupt its CRC.
ctl_session() {
  python3 "$ROOT/tests/lib/session.py" "$CTL_PORT" "$@"
}

assert_eq() {
  local what="$1" want="$2" got="$3"
  if [ "$want" = "$got" ]; then
    printf '  %-44s PASS\n' "$what"; PASS=$((PASS + 1))
  else
    printf '  %-44s FAIL\n' "$what"
    printf '      expected: %s\n      actual:   %s\n' "$want" "$got"
    FAIL=$((FAIL + 1))
  fi
}

assert_contains() {
  local what="$1" needle="$2" hay="$3"
  if printf '%s' "$hay" | grep -qF -- "$needle"; then
    printf '  %-44s PASS\n' "$what"; PASS=$((PASS + 1))
  else
    printf '  %-44s FAIL\n' "$what"
    printf '      expected to contain: %s\n      actual:\n' "$needle"
    printf '%s\n' "$hay" | sed 's/^/        /'
    FAIL=$((FAIL + 1))
  fi
}

case_summary() {
  echo "  --- $PASS passed, $FAIL failed"
  [ "$FAIL" -eq 0 ]
}

trap ctl_stop EXIT
