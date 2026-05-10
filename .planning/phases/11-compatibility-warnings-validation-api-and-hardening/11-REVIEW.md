---
phase: 11-compatibility-warnings-validation-api-and-hardening
reviewed: 2026-05-10T04:42:10Z
depth: standard
files_reviewed: 19
files_reviewed_list:
  - CMakeLists.txt
  - CMakePresets.json
  - docs/compatibility-evidence.md
  - include/libbsa/libbsa.hpp
  - include/libbsa/validation.hpp
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
  critical: 1
  warning: 3
  info: 0
  total: 4
status: issues_found
---

# Phase 11: Code Review Report

**Reviewed:** 2026-05-10T04:42:10Z
**Depth:** standard
**Files Reviewed:** 19
**Status:** issues_found

## Summary

Reviewed Phase 11 validation API, warning policy, malformed matrix, sanitizer preset, fixture policy, and public API/test contract changes. The main blocker is that opt-in extractability validation routes through `extract_bytes`, which can allocate archive-controlled payload sizes and let `std::bad_alloc` escape the public `result` error model. The remaining issues are test-gate and portability defects.

## Critical Issues

### CR-01: BLOCKER - Extractability validation materializes archive-controlled payloads

**File:** `src/validation.cpp:120`

**Issue:** `validate_extractability` calls `reader.extract_bytes(entry.path)` for every entry. `extract_bytes` creates a vector-backed sink sized from parser-derived `entry.raw_size`, so `validation_options::validate_entry_extractability = true` can force large allocations for valid large/sparse archives or malicious archive metadata. That violates the public `result`-based error contract because allocation failure can throw instead of returning `validation_report.errors`, and it makes validation a memory DoS path. This is especially risky because validation is meant to harden untrusted archives.

**Fix:**
```cpp
class discard_sink final : public payload_sink {
 public:
  result<std::size_t> write(std::span<const std::byte> bytes) override {
    return bytes.size();
  }
};

void validate_extractability(const archive_reader& reader,
                             const std::vector<entry_metadata>& entries,
                             validation_report& report) {
  for (const auto& entry : entries) {
    discard_sink sink;
    auto extracted = reader.extract(entry.path, sink);
    if (!extracted) {
      append_fatal(report, extracted.error().code);
    }
  }
}
```
Add a regression using a sparse large raw payload with `validate_entry_extractability = true` to prove validation streams into the discard sink and does not allocate the entry bytes.

## Warnings

### WR-01: WARNING - Matrix validator is not part of the default test gate

**File:** `tests/CMakeLists.txt:228`

**Issue:** `tests/fixtures/generated/validate_fixture_manifests.py` now contains the strongest matrix consistency checks, including archive/manifest/case `phase` and `expected_error` matching, but `tests/CMakeLists.txt` never registers that script as a CTest test. The GitHub workflow only runs `ctest`, so these checks are manual-only and can silently drift from default CI.

**Fix:**
```cmake
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
  NAME validate_fixture_manifests
  COMMAND Python3::Interpreter
          ${PROJECT_SOURCE_DIR}/tests/fixtures/generated/validate_fixture_manifests.py
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
set_tests_properties(validate_fixture_manifests PROPERTIES
  LABELS "unit;fixture;malformed;compatibility_matrix"
)
```

### WR-02: WARNING - Catalog coverage test duplicates the warning-code list instead of deriving it

**File:** `tests/unit/validation_policy_tests.cpp:65`

**Issue:** The catalog test hard-codes the three Phase 11 warning-code strings. If `include/libbsa/validation.hpp` adds another `compatibility_warning_code`, the test can still pass while the new public warning is missing from `docs/compatibility-evidence.md`. That weakens the intended "every public warning code is cataloged" contract.

**Fix:** Read `include/libbsa/validation.hpp`, extract the enumerators from `enum class compatibility_warning_code`, and require a catalog heading for every extracted name. Keep the existing rule/evidence checks after the dynamically collected list is built.

### WR-03: WARNING - `std::move` is used without including `<utility>`

**File:** `src/validation.cpp:53`

**Issue:** `append_warning` uses `std::move`, but `src/validation.cpp` does not include `<utility>`. The current build succeeds through transitive standard-library includes, but that is not guaranteed and can break on another STL/toolchain.

**Fix:**
```cpp
#include <utility>
```

---

_Reviewed: 2026-05-10T04:42:10Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
