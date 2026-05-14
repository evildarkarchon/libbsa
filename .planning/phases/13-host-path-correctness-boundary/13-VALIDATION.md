---
phase: 13
slug: host-path-correctness-boundary
status: ready
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-13
---

# Phase 13 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.14.0 plus CTest preset orchestration |
| **Config file** | `tests/CMakeLists.txt` |
| **Quick run command** | `ctest --preset windows-msvc-debug-static -R "host_file|archive_reader|validation_api|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure` |
| **Phase-close host-path command** | `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary|host_path_correctness_boundary_smoke" --output-on-failure` |
| **Full suite command** | `ctest --preset windows-msvc-debug-static --output-on-failure` |
| **Estimated runtime** | ~20 seconds |

---

## Sampling Rate

- **After every task commit:** Run the narrowest affected selector. The stable focused gates are `ctest --preset windows-msvc-debug-static -R "archive_reader|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10|validation_api" --output-on-failure` for shared runtime changes and `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary|host_path_correctness_boundary_smoke" --output-on-failure` for the dedicated non-ASCII proof.
- **After every plan wave:** Run `ctest --preset windows-msvc-debug-static --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 20 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 13-P3-01 | 13-03 task 2 | 3 | HOST-01 | T-13-03-01 | `archive_reader::open` resolves once and parser entry reads use the shared host-file boundary without narrow-string drift | focused reader/parser regression | `ctest --preset windows-msvc-debug-static -R "archive_reader|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure` | ✅ existing | ✅ green |
| 13-P4-01 | 13-04 task 2 | 4 | HOST-02 | T-13-04-02 | `validate_archive` reuses `archive_reader::open`, keeps the result/report split, and still drives extractability through the public reader path | focused validation/runtime regression | `ctest --preset windows-msvc-debug-static -R "validation_api|archive_reader|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure` | ✅ existing | ✅ green |
| 13-P5-01 | 13-05 task 2 | 5 | HOST-01 / HOST-02 / HOST-03 | T-13-05-01 | Dedicated non-ASCII regression suite proves open, validate, and one canonical extraction per representative archive from public APIs | fixture regression | `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary" --output-on-failure` | ✅ existing | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Coverage Anchors

- `13-P3-01` (`HOST-01`) is anchored by `tests/unit/archive_reader_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, and `tests/unit/host_file_writer_name_tests.cpp`.
- `13-P4-01` (`HOST-02`) is anchored by `tests/unit/validation_api_tests.cpp` plus the shared reader/runtime suites used by `13-P3-01`.
- `13-P5-01` (`HOST-01`, `HOST-02`, `HOST-03`) is anchored by `tests/unit/host_path_correctness_boundary_tests.cpp` and its focused `host_path_correctness_boundary` / `host_path_correctness_boundary_smoke` selectors.

---

## Plan 13-05 Suite Requirements

- [x] `tests/unit/host_path_correctness_boundary_tests.cpp` — dedicated cross-family non-ASCII host-path regression suite
- [x] `tests/CMakeLists.txt` — register the new suite in `libbsa_tests`
- [x] File-local helpers for non-ASCII temp directory/filename setup, archive copy, and manifest-backed expected payload lookup
- [x] Stable tag naming containing `host_path_correctness_boundary` for the targeted quick-run filter
- [x] One stable smoke subset/tag containing `host_path_correctness_boundary_smoke` so task-level feedback stays under the preferred Nyquist latency target

Execution alignment: Plans `13-01` through `13-04` relied on the focused helper, reader, and validation gates while the runtime boundary was being built. Plan `13-05` added the dedicated non-ASCII public-proof suite and smoke subset as the phase-close regression surface.

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or an earlier existing focused regression gate
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] The dedicated non-ASCII suite landed in Plan 13-05 and closes the remaining phase-specific proof obligations
- [x] No watch-mode flags
- [x] Feedback latency < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-05-13

---

## Validation Audit 2026-05-13

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved | 0 |
| Escalated | 0 |

Focused audit evidence:

- `ctest --preset windows-msvc-debug-static -N -R "host_path_correctness_boundary|host_path_correctness_boundary_smoke"` discovered 5 focused tests.
- `ctest --preset windows-msvc-debug-static -R "archive_reader|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure` passed 38/38.
- `ctest --preset windows-msvc-debug-static -R "validation_api|archive_reader|tes3_bsa_reader|tes4_bsa_reader|ba2_gnrl_reader|ba2_dx10" --output-on-failure` passed 44/44.
- `ctest --preset windows-msvc-debug-static -R "host_path_correctness_boundary|host_path_correctness_boundary_smoke" --output-on-failure` passed 5/5.
