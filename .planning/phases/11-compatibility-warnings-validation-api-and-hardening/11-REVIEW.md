---
phase: 11-compatibility-warnings-validation-api-and-hardening
reviewed: 2026-05-10T05:06:41Z
depth: standard
files_reviewed: 28
files_reviewed_list:
  - CMakeLists.txt
  - CMakePresets.json
  - docs/compatibility-evidence.md
  - include/libbsa/libbsa.hpp
  - include/libbsa/validation.hpp
  - src/archive.cpp
  - src/detail/byte_vector.hpp
  - src/detail/deflate_codec.cpp
  - src/detail/lz4_block_codec.cpp
  - src/detail/lz4_frame_codec.cpp
  - src/formats/ba2/ba2_dx10_reader.cpp
  - src/formats/ba2/ba2_gnrl_reader.cpp
  - src/formats/bsa/tes4_bsa_reader.cpp
  - src/formats/bsa/tes4_bsa_reader.hpp
  - src/validation.cpp
  - tests/CMakeLists.txt
  - tests/fixtures/README.md
  - tests/fixtures/generated/compatibility_matrix.json
  - tests/fixtures/generated/validate_fixture_manifests.py
  - tests/package-consumer/main.cpp
  - tests/unit/ba2_gnrl_reader_tests.cpp
  - tests/unit/compatibility_matrix_tests.cpp
  - tests/unit/compatibility_warning_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
  - tests/unit/tes3_bsa_reader_tests.cpp
  - tests/unit/tes4_bsa_reader_tests.cpp
  - tests/unit/validation_api_tests.cpp
  - tests/unit/validation_policy_tests.cpp
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
status: clean
---

# Phase 11: Code Review Report

**Reviewed:** 2026-05-10T05:06:41Z
**Depth:** standard
**Files Reviewed:** 28
**Status:** clean

## Summary

Reviewed all non-planning files changed from `6d9ad74..HEAD` after commit `7ed2bd3` while excluding `TES5Edit/`.

The prior warning is fixed. `src/archive.cpp` no longer contains the obsolete `read_stored_payload` helper, the span-based `extract_tes4_bsa_payload` declaration and definition were removed from the TES4 reader, and `archive_reader::extract` now keeps TES4 extraction on `extract_tes4_bsa_payload_from_file`.

All reviewed files meet quality standards. No issues found.

Verification performed:

```text
rg -n "read_stored_payload|extract_tes4_bsa_payload\b|extract_tes4_bsa_payload_from_file|tes4_bsa_payload" src\archive.cpp src\formats\bsa\tes4_bsa_reader.cpp src\formats\bsa\tes4_bsa_reader.hpp include tests
cmake --build --preset windows-msvc-debug-static --target libbsa_tests
python tests\fixtures\generated\validate_fixture_manifests.py
ctest --preset windows-msvc-debug-static -R "validation_api|validation_policy|validate_fixture_manifests|compatibility_matrix|compatibility_warning|tes4_bsa" --output-on-failure
ctest --preset windows-msvc-debug-static --output-on-failure
```

Focused CTest passed 32/32. Full CTest passed 188/188, with `local game fixtures are opt-in` skipped as expected.

---

_Reviewed: 2026-05-10T05:06:41Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
