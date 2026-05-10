---
phase: 03-format-detection-and-tes4-family-bsa-read-extract
plan: 05
subsystem: archive-format-parser
tags: [cpp20, tes4-bsa, metadata, lookup, tdd, catch2]

requires:
  - phase: 03-format-detection-and-tes4-family-bsa-read-extract
    provides: TES4-family detection, generated fixtures, malformed fixture manifests, and archive-level metadata open state
provides:
  - Full TES4-family BSA table/name parser for deterministic public entry metadata
  - Duplicate canonical path validation during open
  - Normalized entries/find/contains lookup behavior over canonical archive paths
  - Embedded-name prefix metadata materialized before listing or lookup
affects: [phase-03-extraction, tes4-bsa-reader, archive-reader-public-api]

tech-stack:
  added: []
  patterns: [private TES4 parser result, canonical sorted metadata vector, normalized lookup delegation]

key-files:
  created:
    - src/formats/bsa/tes4_bsa_reader.hpp
    - src/formats/bsa/tes4_bsa_reader.cpp
  modified:
    - CMakeLists.txt
    - src/archive.cpp
    - src/formats/bsa/tes4_bsa_parser.hpp
    - src/formats/bsa/tes4_bsa_parser.cpp
    - tests/unit/tes4_bsa_reader_tests.cpp

key-decisions:
  - "TES4-family open now fails archives without usable folder/file names as unsupported rather than exposing hash-only entries."
  - "Public original_path values preserve archive spelling but normalize separators to `/` for archive-internal semantics."

patterns-established:
  - "Parse record order privately, then expose value metadata sorted by canonical path."
  - "Use detail::normalize_archive_path for every public lookup input and duplicate canonical key validation."

requirements-completed: [FMT-03, FMT-04, FMT-05, BSA-05, BSA-06]

duration: 8min
completed: 2026-05-08
---

# Phase 03 Plan 05: TES4-Family BSA Metadata and Lookup Summary

**TES4-family BSA table parsing with deterministic entry metadata, embedded-name prefix materialization, duplicate-path rejection, and normalized public lookup.**

## Performance

- **Duration:** 8 min
- **Started:** 2026-05-08T07:48:00Z
- **Completed:** 2026-05-08T07:56:09Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Parsed TES4-family folder records, folder names, file records, and file name blocks into `entry_metadata` values with checked bounds and span validation.
- Materialized `path`, `original_path`, sizes, payload offsets, TES4 hashes, compression mode, record flags, and embedded-name prefix sizes during open.
- Added a private TES4 reader helper layer for deterministic `entries()`, normalized `find()`, and `contains()` behavior.
- Rejected duplicate canonical paths during open with `format_error`, and rejected structurally named-disabled archives as `unsupported`.

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Parse tables, paths, hashes, and embedded-name metadata tests** - `8a498fc` (test)
2. **Task 1 GREEN: Parse tables, paths, hashes, and embedded-name metadata** - `c5c96d5` (feat)
3. **Task 2: Deterministic entries, find, and contains coverage** - `a6c9c7e` (test)

_Note: TDD tasks may have multiple commits (test → feat → refactor)._

## Files Created/Modified

- `src/formats/bsa/tes4_bsa_reader.hpp` - Private TES4-family reader helper declarations for listing and lookup.
- `src/formats/bsa/tes4_bsa_reader.cpp` - Sorted entry copies plus normalized find/contains helper implementation.
- `src/formats/bsa/tes4_bsa_parser.hpp` - Added parsed archive result carrying metadata and entries.
- `src/formats/bsa/tes4_bsa_parser.cpp` - Added checked table/name parsing, duplicate canonical validation, compression derivation, and embedded prefix inspection.
- `src/archive.cpp` - Stores parsed entries in reader state and delegates public listing/lookup methods.
- `CMakeLists.txt` - Registers the private TES4 reader implementation source.
- `tests/unit/tes4_bsa_reader_tests.cpp` - Adds fixture-backed metadata, embedded-name, duplicate-path, lookup, and hash metadata assertions.

## Decisions Made

- TES4-family `original_path` preserves archive spelling while using `/` separators to avoid host filesystem path semantics.
- Lookup remains canonical-path based even though parser preserves TES4 hash metadata for compatibility proof.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Added private lookup delegation while wiring parser state**
- **Found during:** Task 1 (Parse tables, paths, hashes, and embedded-name metadata)
- **Issue:** Parsed entries would remain unreachable from public `entries()`/`find()`/`contains()` without reader state delegation, preventing Task 1 metadata verification from observing materialized values.
- **Fix:** Added `tes4_bsa_reader.*`, registered it in CMake, and stored sorted entries in `archive_reader::state`.
- **Files modified:** `CMakeLists.txt`, `src/archive.cpp`, `src/formats/bsa/tes4_bsa_reader.hpp`, `src/formats/bsa/tes4_bsa_reader.cpp`
- **Verification:** `ctest --preset windows-msvc-debug-static -R "tes4_bsa_metadata|tes4_bsa_entry_metadata|tes4_bsa_listing|tes4_bsa_embedded_name|tes4_bsa_malformed_open|tes4_bsa_lookup|tes4_bsa_hash_lookup|public_include_boundary" --output-on-failure`
- **Committed in:** `c5c96d5`

---

**Total deviations:** 1 auto-fixed (Rule 2 missing critical)
**Impact on plan:** Necessary to make parsed metadata observable and keep lookup semantics consistent; no scope beyond Plan 03-05.

## Issues Encountered

- Task 2 lookup tests passed immediately because lookup delegation had been added during Task 1 to expose parsed entries. The behavior was retained and documented under TDD compliance.

## TDD Gate Compliance

- RED gate exists: `8a498fc` added failing metadata and duplicate canonical path tests before parser implementation.
- GREEN gate exists: `c5c96d5` implemented parser/reader behavior after the failing tests.
- Warning: Task 2 lookup tests in `a6c9c7e` passed immediately because the Task 1 GREEN commit already included the required public lookup delegation as critical wiring.

## Known Stubs

None.

## Self-Check: PASSED

- Verified all created/modified files exist.
- Verified task commits exist: `8a498fc`, `c5c96d5`, `a6c9c7e`.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03-06 can use `payload_offset`, `stored_size`, `raw_size`, `compression`, and `embedded_name_prefix_size` to implement extraction without reparsing names.
- Malformed compression payload fixtures remain deferred to extraction verification, as intended by the phase plan.

---
*Phase: 03-format-detection-and-tes4-family-bsa-read-extract*
*Completed: 2026-05-08*
