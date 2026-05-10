---
phase: 09-ba2-dx10-write-new-support
reviewed: 2026-05-09T00:00:00Z
depth: standard
files_reviewed: 17
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/writer.hpp
  - src/formats/ba2/ba2_dx10_writer.cpp
  - src/formats/ba2/ba2_dx10_writer.hpp
  - src/texture/dds_layout.cpp
  - src/texture/dds_layout.hpp
  - src/texture/directxtex_analyzer.cpp
  - src/texture/directxtex_analyzer.hpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp
  - tests/fixtures/generated/source/ba2_dx10_array_bc5_unorm_2slice.dds
  - tests/fixtures/generated/source/ba2_dx10_cubemap_bc1_unorm_6face.dds
  - tests/fixtures/generated/source/ba2_dx10_multi_mip_bc7_unorm.dds
  - tests/fixtures/generated/source/ba2_dx10_writer_sources_manifest.json
  - tests/unit/ba2_dx10_writer_tests.cpp
  - tests/unit/dds_layout_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
findings:
  critical: 1
  warning: 0
  info: 0
  total: 1
status: issues_found
---

# Phase 09: Code Review Report

**Reviewed:** 2026-05-09T00:00:00Z
**Depth:** standard
**Files Reviewed:** 17
**Status:** issues_found

## Summary

Reviewed the BA2 DX10 writer public API, writer implementation, DDS layout/analyzer code, fixture generator, manifest, and unit tests at standard depth. The binary DDS fixtures in scope could not be decoded by the text reader, so their expected semantics were reviewed through the generator and JSON manifest.

The iteration fixed the prior hash/extension serialization issue, but one blocker remains: default test builds still compile an environment-variable-controlled failure hook into the library target.

## Critical Issues

### CR-01: Default builds compile test fault injection into the library

**Classification:** BLOCKER
**File:** `CMakeLists.txt:110-113`, `src/formats/ba2/ba2_dx10_writer.cpp:618-634,762-766`

**Issue:** `LIBBSA_BUILD_TESTS` defaults to `ON`, and the test block adds `LIBBSA_ENABLE_TEST_FAULT_INJECTION` directly to the `libbsa` target. That means the default library build still includes `should_fail_after_backup_for_test`, which reads `LIBBSA_TEST_FAIL_BA2_DX10_PUBLISH_AFTER_BACKUP` from the process environment and deliberately fails after moving an existing output archive aside. Any consumer using a default build can trigger this writer path through inherited environment state; the rollback error is ignored, so this also remains a data-loss risk if rollback fails.

**Fix:** Do not compile test fault injection into the production `libbsa` target. Remove the compile definition from `libbsa` and move this coverage behind a test-only publish abstraction/fake filesystem, or use a separate non-default internal test target that is never the library artifact consumers link.

```cmake
if(LIBBSA_BUILD_TESTS)
  include(CTest)
  enable_testing()
  # Do not add LIBBSA_ENABLE_TEST_FAULT_INJECTION to libbsa.
  # Test rollback via a test-only helper/fake publish layer instead.
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  endif()
endif()
```

---

_Reviewed: 2026-05-09T00:00:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
