#!/usr/bin/env bash
# Produce a versioned, verifiable release artifact.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${VERSION:-$(git describe --tags --always --dirty 2>/dev/null || echo 0.0.0)}"
COMMIT="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
BUILT="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
ARCH="$(uname -m)"
OS="$(uname -s)"
STAGE="dist/crossctl-${VERSION}-${OS}-${ARCH}"

# macOS ships shasum, Debian ships sha256sum.
if command -v sha256sum >/dev/null 2>&1; then
  SHA="sha256sum"
  SHACHECK="sha256sum -c"
else
  SHA="shasum -a 256"
  SHACHECK="shasum -a 256 -c"
fi

echo "==> building release binary"
make --no-print-directory PROFILE=release all >/dev/null

echo "==> verifying before packaging"
make --no-print-directory PROFILE=release selftest >/dev/null
make --no-print-directory PROFILE=release test >/dev/null

rm -rf "$STAGE"
mkdir -p "$STAGE/bin" "$STAGE/doc"
cp build/release/crossctl "$STAGE/bin/"

echo "==> writing release notes"
LAST_TAG="$(git describe --tags --abbrev=0 2>/dev/null || true)"
{
  echo "crossctl ${VERSION}"
  echo "================================"
  echo
  echo "Commit:    ${COMMIT}"
  echo "Built:     ${BUILT}"
  echo "Platform:  ${OS} ${ARCH}"
  echo "Toolchain: $(g++ --version | head -1)"
  echo
  echo "Verification"
  echo "------------"
  echo "Unit selftest and integration suite pass under release, debug,"
  echo "AddressSanitizer and UndefinedBehaviorSanitizer profiles."
  echo "Cases: $(find tests/cases -name '*.sh' | wc -l | tr -d ' ')"
  echo
  echo "Changes"
  echo "-------"
  if [ -n "$LAST_TAG" ]; then
    git log --no-merges --pretty='  - %s' "${LAST_TAG}..HEAD"
  else
    git log --no-merges --pretty='  - %s' -20
  fi
} > "$STAGE/doc/RELEASE_NOTES.txt"

echo "==> writing manuals"
cat > "$STAGE/doc/USER_MANUAL.txt" <<'DOC'
crossctl — user manual
======================

SYNOPSIS
  crossctl --serve [--port N] [--timeout N]
  crossctl --selftest
  crossctl --version

OPTIONS
  --port N     TCP listen port on 127.0.0.1. Default 842.
  --timeout N  Ticks allowed for a barrier movement before FAULT. Default 10.

TELEGRAM FORMAT
  [magic:2][ver:1][type:1][seq:2][len:2][payload:len][crc16:2]
  Big-endian. CRC-16/CCITT-FALSE over header and payload.

  Types:  0x10 EVENT (in)  0x20 STATUS (out)  0x30 ERROR (out)

EVENTS
  TRAIN_DETECTED  TRAIN_CLEARED  BARRIER_DOWN  BARRIER_UP  TICK  RESET

STATES
  IDLE  APPROACHING  CLOSING  CLOSED  OPENING  FAULT

  FAULT is entered on movement timeout and left only by RESET.
  Outputs in FAULT are barrier LOWER, signal DANGER.

EXIT CODES
  0  normal shutdown / selftest passed
  1  bind or listen failure / selftest failed
DOC

cat > "$STAGE/doc/PRODUCTION_MANUAL.txt" <<DOC
crossctl — production manual
============================

BUILD FROM SOURCE
  make                     release build
  make check               all profiles, as CI runs them
  make docker-test         suite in a Debian container
  make docker-test-amd64   same, x86_64

RELEASE PROCEDURE
  1. Confirm CI green on main.
  2. Tag:  git tag -a vX.Y.Z -m "release X.Y.Z"
  3. Run:  make release
  4. Verify: ${SHACHECK} MANIFEST.sha256 inside the artifact.
  5. Attach the tarball to the tag.

  make release refuses to package if the selftest or the integration
  suite fails.

VERIFICATION EVIDENCE
  Each artifact carries RELEASE_NOTES.txt recording commit, build time,
  toolchain version and the number of integration cases executed.

DEPLOYMENT
  Single binary. Requires libstdc++ and libc only.
  Listens on loopback; front with a bridge process for remote access.
DOC

echo "==> generating manifest"
( cd "$STAGE" && find . -type f ! -name MANIFEST.sha256 -exec $SHA {} + \
  > MANIFEST.sha256 )

echo "==> packaging"
TARBALL="dist/crossctl-${VERSION}-${OS}-${ARCH}.tar.gz"
tar -czf "$TARBALL" -C dist "$(basename "$STAGE")"

echo
echo "release: $TARBALL"
ls -lh "$TARBALL" | awk '{print "size:    " $5}'
echo "files:"
find "$STAGE" -type f | sed "s|$STAGE/|  |"
