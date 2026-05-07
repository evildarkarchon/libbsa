---
phase: 09-bsa-writers
fixed_at: 2026-05-07T07:30:05.1096687Z
review_path: .planning/phases/09-bsa-writers/09-REVIEW.md
iteration: 1
findings_in_scope: 4
fixed: 4
skipped: 0
status: all_fixed
---

# Phase 09: Code Review Fix Report

**Fixed at:** 2026-05-07T07:30:05.1096687Z
**Source review:** .planning/phases/09-bsa-writers/09-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 4
- Fixed: 4
- Skipped: 0

## Fixed Issues

### CR-01: BLOCKER - TES4-family writer stores the wrong header value for total folder name length

**Files modified:** `src/bsa_writer.cpp`, `tests/bsa_writer_tests.cpp`
**Commit:** b0b3726
**Applied fix:** Tracked serialized TES4 folder-name bytes separately from folder block bytes, wrote the folder-name total to header offset 24, and asserted the generated header value in the TES4-family writer round-trip test.

### CR-02: BLOCKER - TES4 reader ignores the declared file-name table length after checking it

**Files modified:** `src/bsa_reader.cpp`, `tests/bsa_reader_tests.cpp`
**Commit:** c9695a0
**Applied fix:** Bounded TES4 filename parsing by the header-declared filename table end and added a malformed archive test proving names after a zero-length declared table are rejected.

### WR-01: Bound compressed BSA metadata reads to declared stored payloads

**Files modified:** `src/bsa_reader.cpp`, `tests/bsa_reader_tests.cpp`
**Commit:** 6743cf4
**Applied fix:** `open_bsa` now validates compressed metadata against the entry's declared stored payload span before reading embedded-name prefixes or the uncompressed-size header. Added a malformed fixture proving compressed metadata located outside the declared stored payload is rejected even when bytes still exist later in the archive.

### WR-02: Validate TES4 native header field widths before serialization

**Files modified:** `src/bsa_writer.cpp`
**Commit:** dccffca
**Applied fix:** TES4-family planning now checks folder count, total folder-name length, total file-name length, and file-count increments before serializing 32-bit native header fields, preventing silent truncation. A dedicated oversized fixture was not added because reaching these native header limits through the public in-memory API would require impractically large (>4 GiB or billions of entries) test data; the focused writer suite was rerun to verify existing behavior.

## Verification

- `cmake --preset windows-msvc-vcpkg` failed because the preset requires Visual Studio 17 2022, which is not installed in this environment.
- `cmake -S . -B build/windows-vs2026-vcpkg -G "Visual Studio 18 2026" -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" -DBUILD_TESTING=ON`
- `cmake --build build/windows-vs2026-vcpkg --config Debug --target libbsa_bsa_reader_tests libbsa_bsa_writer_tests`
- `ctest --test-dir build/windows-vs2026-vcpkg -C Debug -R "libbsa_bsa_(reader|writer)_tests" --output-on-failure` — 49/49 tests passed.
- `cmake -S . -B build/reviewfix-vs2026-vcpkg -G "Visual Studio 18 2026" -A x64 -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DBUILD_TESTING=ON`
- `cmake --build build/reviewfix-vs2026-vcpkg --config Debug --target libbsa_bsa_reader_tests`
- `ctest --test-dir build/reviewfix-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_reader_tests` — 24/24 tests passed.
- `cmake --build build/reviewfix-vs2026-vcpkg --config Debug --target libbsa_bsa_writer_tests`
- `ctest --test-dir build/reviewfix-vs2026-vcpkg --output-on-failure -C Debug -R libbsa_bsa_writer_tests` — 26/26 tests passed.
- `ctest --test-dir build/reviewfix-vs2026-vcpkg --output-on-failure -C Debug -R "libbsa_bsa_(reader|writer)_tests"` — 50/50 tests passed.

---

_Fixed: 2026-05-07T07:30:05.1096687Z_
_Fixer: the agent (gsd-code-fixer)_
_Iteration: 1_
