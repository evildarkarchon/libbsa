---
phase: 02
slug: binary-i-o-paths-hashes-and-compression-services
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-08
audited: 2026-05-07
---

# Phase 02 — Validation Strategy

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.x with CTest |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `cmake --build --preset windows-msvc-debug-static --config Debug && ctest --preset windows-msvc-debug-static -L unit --output-on-failure` |
| **Full suite command** | `cmake --build --preset windows-msvc-debug-static --config Debug && ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~60 seconds |

## Sampling Rate

- **After every task commit:** Run the plan-specific `ctest -R` command from the PLAN.md task.
- **After every plan wave:** Run the quick run command.
- **Before `/gsd-verify-work`:** Full suite must be green and `git -C TES5Edit status --short` must be empty.
- **Max feedback latency:** 60 seconds for unit filters.

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 02-01-01 | 01 | 1 | BIN-01, BIN-02 | T-02-01-01 | Checked reads/writes reject out-of-range buffers | unit/malformed | `ctest --preset windows-msvc-debug-static -R binary_io --output-on-failure` | ✅ `tests/unit/binary_io_tests.cpp` | ✅ COVERED |
| 02-02-01 | 02 | 2 | BIN-03 | T-02-02-01 | Invalid archive paths and partial sinks fail closed | unit/malformed | `ctest --preset windows-msvc-debug-static -R "(archive_path|payload_stream)" --output-on-failure` | ✅ `tests/unit/archive_path_tests.cpp`, `tests/unit/payload_stream_tests.cpp` | ✅ COVERED |
| 02-03-01 | 03 | 3 | BIN-04, BIN-07 | T-02-03-01 | Deflate corrupt and size-mismatch payloads fail closed | unit/malformed | `ctest --preset windows-msvc-debug-static -R deflate --output-on-failure` | ✅ `tests/unit/deflate_codec_tests.cpp` | ✅ COVERED |
| 02-04-01 | 04 | 4 | BIN-05, BIN-06, BIN-07 | T-02-04-01 | LZ4 frame/block routes are explicit and exact-size checked | unit/malformed | `ctest --preset windows-msvc-debug-static -R "(lz4|compression_router)" --output-on-failure` | ✅ `tests/unit/lz4_codec_tests.cpp`, `tests/unit/compression_router_tests.cpp` | ✅ COVERED |
| 02-05-01 | 05 | 5 | BIN-08 | T-02-05-01 | Hash constants match traced TES5Edit behavior | unit | `ctest --preset windows-msvc-debug-static -R bethesda_hash --output-on-failure` | ✅ `tests/unit/bethesda_hash_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp` | ✅ COVERED |

## Wave 0 Requirements

- Each TDD task creates its Catch2 test file before production implementation.
- Existing infrastructure covers framework installation and discovery.

## Manual-Only Verifications

All phase behaviors have automated verification.

## Validation Audit 2026-05-07

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

All five Phase 2 task groups were cross-referenced against registered Catch2 tests and verified with the Phase 2-focused CTest filter plus the full unit suite.

## Validation Sign-Off

- [x] All tasks have automated verify commands.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 is represented by per-task RED test creation.
- [x] No watch-mode flags.
- [x] Feedback latency target is < 60 seconds per test filter.

**Approval:** automated Nyquist audit passed
