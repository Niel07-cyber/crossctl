# crossctl

[![CI](https://github.com/Niel07-cyber/crossctl/actions/workflows/ci.yml/badge.svg)](https://github.com/Niel07-cyber/crossctl/actions/workflows/ci.yml)

A level-crossing controller and its automated integration test bench.

The controller is deliberately small; the tooling around it is the point. This is a
demonstration of the integration and validation workflow — building for host and
target, automating the suite, and shipping a reproducible artifact — not a vital
product, and it carries no safety certification.

## Software integration & validation

- C11 telegram layer (`clib/`) and C++17 control logic (`src/`) linked into one binary
- 39 unit checks via `--selftest`, covering CRC vectors and every frame rejection path
- Bash integration suite driving the built binary over TCP, one fresh server process
  per case so no test depends on another's state
- Each case is tagged with a requirement ID and discovered automatically — adding a
  test is dropping a file in `tests/cases/`

## Platform expertise

- Runs on macOS/clang/arm64, Linux/gcc/arm64 and Linux/gcc/amd64
- Host toolchain differs from the target, so the build is containerised for
  reproducibility (`docker/Dockerfile`)
- CI executes the same suite across all three

## Build profiles

| Command | Profile |
|---|---|
| `make` | release, `-O2` |
| `make debug` | `-O0 -g3` |
| `make test-asan` | AddressSanitizer |
| `make test-ubsan` | UndefinedBehaviorSanitizer, no recover |
| `make check` | all profiles, as CI runs them |
| `make docker-test` | suite inside Debian container |
| `make docker-test-amd64` | same, x86_64 under emulation |
| `make release` | versioned artifact, gated on the suite passing |

## Protocol

Fixed 8-byte header, big-endian, CRC-16/CCITT-FALSE:

Malformed length, bad magic, wrong version, truncated frames and CRC failures are
all rejected and reported rather than acted upon.

## Design notes

**FAULT is fail-safe.** Barrier lowered, signal at danger. A crossing that fails with
the barrier up is dangerous; one that fails with it down only causes congestion.

**`Signal::Clear` is reachable from exactly one state.** Every other path, including
the fallthrough, returns danger. Permissive outputs should be hard to reach by accident.

**Signals use `sigaction` without `SA_RESTART`.** Plain `signal()` has restart
semantics on BSD, so `accept()` resumed instead of returning `EINTR` and the shutdown
flag was never observed — the server hung on SIGTERM. Found by the harness blocking.

## Build

```sh
make check          # everything
make test CASE=REQ-010   # one case
./build/release/crossctl --serve --port 8042
```

## Release

`make release` builds, runs the selftest and the full integration suite, then
packages a versioned tarball containing the binary, release notes recording
commit and toolchain, user and production manuals, and a SHA-256 manifest.
Packaging aborts if any test fails.

## Status

13 requirement-tagged integration cases, 39 unit checks, clean under
AddressSanitizer, UndefinedBehaviorSanitizer, cppcheck and shellcheck.
