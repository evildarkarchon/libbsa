---
id: T01
parent: S05
milestone: M001-k9wo8b
key_files:
  - tests/package-consumer/main.cpp
key_decisions:
  - Generated a tiny BC1 DDS DXT10 input inline inside the package-consumer executable so BA2 DX10 runtime proof does not depend on source-tree generated fixtures.
  - Validated only the expected top-level archive family through `validation_options::expected_type`, then asserted variant/version from returned metadata so the smoke follows the task's top-level-family validation contract while still proving family-level metadata.
duration: 
verification_result: passed
completed_at: 2026-05-20T03:41:57.948Z
blocker_discovered: false
---

# T01: Added a fixture-free installed package runtime smoke that writes, reopens, validates, and extracts TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives through the umbrella public API.

**Added a fixture-free installed package runtime smoke that writes, reopens, validates, and extracts TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives through the umbrella public API.**

## What Happened

Extended `tests/package-consumer/main.cpp` so the installed-package consumer now does more than compile/link public symbols. The executable recreates `libbsa-package-consumer-smoke-work` under its process working directory, writes a tiny synthetic payload file and an inline-generated BC1 DDS DXT10 file, then uses public writer APIs to produce one TES3 BSA, one Skyrim SE TES4-family BSA, one Starfield v3 BA2 GNRL, and one Starfield v3 BA2 DX10 archive. Each archive is reopened with `archive_reader::open`, checked for top-level type, variant, version, file count, default compression, and Starfield BA2 compression method where applicable. The smoke lists entries, uses `find` and `contains`, extracts through both `extract_bytes` and the existing sink helper, exercises bulk extraction through the package-consumer sink factory, and calls `validate_archive` with `expected_type` plus `validate_entry_extractability = true`. The existing missing archive `io_error` public error-code branch remains covered through both `archive_reader::open` and `validate_archive`. The consumer still includes only `<libbsa/libbsa.hpp>` from libbsa plus standard C++20 headers, and the DDS source is generated from inline standard C++ bytes rather than source-tree fixtures, local corpora, or TES5Edit.

## Verification

Verified the umbrella-only include boundary with a focused source scan, then ran the required configure, library build, and package-consumer CTest smoke on `windows-msvc-debug-static`. `package_consumer_smoke` rebuilt the installed package consumer and passed, proving the runtime writer/open/validation/extraction path through `find_package(libbsa CONFIG REQUIRED)` and `libbsa::libbsa`.

## Verification Evidence

| # | Command | Exit Code | Verdict | Duration |
|---|---------|-----------|---------|----------|
| 1 | `python source scan: verify tests/package-consumer/main.cpp has only <libbsa/libbsa.hpp> and no direct archive/writer/validation/result includes` | 0 | ✅ pass | 83ms |
| 2 | `cmake --preset windows-msvc-debug-static` | 0 | ✅ pass | 794ms |
| 3 | `cmake --build --preset windows-msvc-debug-static --target libbsa` | 0 | ✅ pass | 496ms |
| 4 | `ctest --preset windows-msvc-debug-static -R package_consumer_smoke --output-on-failure` | 0 | ✅ pass | 6531ms |

## Deviations

None.

## Known Issues

None.

## Files Created/Modified

- `tests/package-consumer/main.cpp`
