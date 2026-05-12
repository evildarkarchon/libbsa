## Why

Archive headers currently control record, folder, file, and texture chunk counts before the parsers have a single policy gate for metadata work. A malformed archive can therefore force large vector reservations, duplicate-detection hash tables, sort inputs, or chunk-list processing before the archive is rejected.

## What Changes

- Add explicit parser-side limits for archive-declared metadata counts in TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 open/list metadata paths.
- Reject archives whose declared file, folder, per-folder file, or aggregate DX10 chunk counts exceed those limits with `libbsa::error_code::format_error` before reserving metadata containers or doing sort/hash-set work.
- Translate allocation failures from metadata containers, not just byte buffers and strings, into `format_error` results so public parser paths do not leak `std::bad_alloc` or `std::length_error`.
- Keep the initial policy internal and deterministic; do not add a public caller-configurable options API in this change.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `archive-record-metadata-validation`: Add requirements for rejecting excessive archive-declared metadata counts before metadata allocations or CPU-heavy validation work.
- `allocation-error-translation`: Extend allocation-failure translation to parser metadata containers such as record vectors, entry vectors, path/hash sets, payload-span vectors, and DX10 chunk vectors.

## Impact

- Affected parser files: `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, and `src/formats/ba2/ba2_dx10_parser.cpp`.
- Affected shared helpers: likely `src/detail/parser_primitives.hpp`, `src/detail/parser_primitives.cpp`, or a small internal helper for metadata count validation and result-based container reservation.
- Affected tests: malformed archive/count-limit tests for each parser family plus allocation-translation coverage where feasible.
- Public API impact: none intended; archives exceeding libbsa's safety policy will fail open/validation with `format_error`.
- Dependencies: none.
