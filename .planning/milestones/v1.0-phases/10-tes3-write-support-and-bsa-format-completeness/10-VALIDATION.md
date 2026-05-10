---
phase: 10
slug: tes3-write-support-and-bsa-format-completeness
status: passed
nyquist_compliant: true
wave_0_complete: true
created: 2026-05-09
updated: 2026-05-10
audited: 2026-05-10
---

# Phase 10 - Validation Strategy

> Nyquist validation contract for Phase 10 after execution and verification.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 via vcpkg manifest with CTest discovery |
| **Config file** | `tests/CMakeLists.txt` plus root `CMakeLists.txt` |
| **Configured build dir** | `build/windows-msvc-debug-static` |
| **Build command** | `cmake --build "build/windows-msvc-debug-static" --target libbsa_tests --config Debug` |
| **Focused run command** | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "archive_path\|tes3_bsa_writer\|tes3_bsa_reader\|TES4 BSA writer\|public_include_boundary" --output-on-failure` |
| **Full suite command** | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug --output-on-failure` |
| **Reference-boundary command** | `git -C "TES5Edit" status --short` |
| **Estimated focused runtime** | About 1 second on the current Windows/MSVC Debug build |
| **Estimated full runtime** | About 11 seconds on the current Windows/MSVC Debug build |

---

## Sampling Rate

- **After every task commit:** Run the focused test matching the changed surface, usually `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "tes3_bsa_writer|archive_path|public_include_boundary" --output-on-failure`.
- **After every plan wave:** Run the Phase 10 focused gate: `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "archive_path|tes3_bsa_writer|tes3_bsa_reader|TES4 BSA writer|public_include_boundary" --output-on-failure`.
- **Before `/gsd-verify-work`:** Run the full CTest suite and verify `git -C "TES5Edit" status --short` is empty.
- **Max feedback latency:** Focused Phase 10 gate is sub-second to about 1 second; full suite is about 11 seconds in the current configured build.

---

## Requirement-to-Task Map

All Phase 10 plans map to `WBSA-04`: consumers can create new TES3/Morrowind BSA archives from disk files or memory buffers. Task IDs below use plan-level execution groups because each plan summary records multiple atomic TDD commits under the same requirement.

| Task ID | Plan | Requirement | Behavior Covered | Test Type | Automated Command | Test Evidence | Status |
|---------|------|-------------|------------------|-----------|-------------------|---------------|--------|
| 10-01 | Public writer contract | WBSA-04 | Public TES3 writer API compiles from public headers, remains dedicated/raw-only, and exposes no private dependency types | compile/unit | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R public_include_boundary --output-on-failure` | `tests/unit/public_include_boundary_tests.cpp` | COVERED |
| 10-02 | Source ownership, validation, and safe publish | WBSA-04 | Disk and memory entries are owned safely; duplicate, invalid, missing-source, overwrite, and temp sibling behavior returns structured results | unit/integration | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R tes3_bsa_writer --output-on-failure` | `tests/unit/tes3_bsa_writer_tests.cpp` | COVERED |
| 10-03 | Byte-accurate serializer | WBSA-04 | Header, records, name offsets, hashes, data-section-relative raw offsets, and payload bytes match TES3 layout constraints | unit/byte-level | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R tes3_bsa_writer --output-on-failure` | `tests/unit/tes3_bsa_writer_tests.cpp` | COVERED |
| 10-04 | Reader-backed round-trip | WBSA-04 | Writer output reopens through `archive_reader`, supports lookup variants, and extracts disk, memory, and zero-byte payloads byte-for-byte | integration/roundtrip | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "tes3_bsa_writer|tes3_bsa_reader" --output-on-failure` | `tests/unit/tes3_bsa_writer_tests.cpp`; `tests/unit/tes3_bsa_reader_tests.cpp` | COVERED |
| 10-05 | Fixture evidence and BSA regression | WBSA-04 | Canonical fixture evidence is generated through the public writer API and validated through manifest facts, direct table parsing, and reader extraction | fixture/regression | `cmake --build "build/windows-msvc-debug-static" --target generate_tes3_bsa_writer_fixtures --config Debug` and focused/full CTest gates | `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp`; `tests/fixtures/generated/archives/tes3_writer_canonical.bsa`; `tests/fixtures/generated/archives/tes3_writer_canonical_manifest.json` | COVERED |
| 10-06 | Verification gap closure | WBSA-04 | Embedded-NUL archive paths fail before serialization and `overwrite_existing=false` uses no-replace final publication | regression | `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "archive_path|tes3_bsa_writer" --output-on-failure` | `tests/unit/archive_path_tests.cpp`; `tests/unit/tes3_bsa_writer_tests.cpp` | COVERED |
| 10-final | Phase regression and boundary guard | WBSA-04 | TES3 writer, TES3 reader, TES4 writer, public boundary, and read-only reference boundary remain green together | regression/reference-boundary | focused CTest gate, full CTest suite, and `git -C "TES5Edit" status --short` | Current audit: focused 44/44 passed; full 245/245 passed with two expected opt-in skips; TES5Edit status empty | COVERED |

---

## Gap Analysis

| Requirement | Coverage Status | Evidence |
|-------------|-----------------|----------|
| WBSA-04 | COVERED | Public API, serializer, reader-backed round-trip, fixture evidence, embedded-NUL rejection, and no-replace publish semantics all have automated tests. |

No missing or partial Phase 10 validation gaps were found during the 2026-05-10 audit. No new test files were generated.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| None | WBSA-04 | Phase 10 acceptance criteria are covered by automated CTest and repository cleanliness checks | N/A |

Broader game/tool compatibility validation remains scheduled for Phase 11 and is not a Phase 10 manual-only gap.

---

## Validation Audit 2026-05-10

| Metric | Count |
|--------|-------|
| Gaps found | 0 |
| Resolved by new tests | 0 |
| Escalated/manual-only | 0 |
| Existing automated coverage confirmed | 7 task groups |

| Command | Result |
|---------|--------|
| `cmake --build "build/windows-msvc-debug-static" --target libbsa_tests --config Debug` | PASS |
| `ctest --test-dir "build/windows-msvc-debug-static" -C Debug -R "archive_path\|tes3_bsa_writer\|tes3_bsa_reader\|TES4 BSA writer\|public_include_boundary" --output-on-failure` | PASS, 44/44 |
| `ctest --test-dir "build/windows-msvc-debug-static" -C Debug --output-on-failure` | PASS, 245/245; expected opt-in fixture tests skipped |
| `git -C "TES5Edit" status --short` | PASS, empty output |

---

## Validation Sign-Off

- [x] All task groups have automated verify commands.
- [x] Sampling continuity: no 3 consecutive task groups lack automated verify.
- [x] Phase 10 gap-closure work is covered by automated regression tests.
- [x] No watch-mode flags.
- [x] Feedback latency measured for focused and full suites.
- [x] `nyquist_compliant: true` set in frontmatter after mapping every Phase 10 behavior to tests.

**Approval:** automated audit complete
