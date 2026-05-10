# Milestones

## v1.0 Complete Library (Shipped: 2026-05-10)

**Status:** Shipped  
**Scope:** Phases 1-12  
**Plans:** 75/75 complete  
**Tasks:** 185 counted from plan summaries  
**Requirements:** 81/81 v1 requirements complete  
**Audit:** Passed on 2026-05-10  
**Deferred items at close:** 0 open artifact-audit items

### Delivered

- Reusable C++20 libbsa package with CMake/vcpkg static and shared builds, exported package metadata, Catch2/CTest validation, and CI coverage.
- Read, list, query, validate, and extract support for TES3 BSA, TES4-family BSA v103/v104/v105, BA2 GNRL, and BA2 DX10/DDS archive families.
- Write-new support for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 archives, proven by reopening generated archives and comparing extracted payloads or DDS metadata.
- Internal compression and texture boundaries for libdeflate, official lz4 frame/raw-block routes, and DirectXTex DDS analysis without leaking those types through public headers.
- Structured validation reports and compatibility warnings, malformed-fixture coverage, sanitizer-oriented hardening presets, and machine-checked compatibility evidence.
- Bounded-memory extraction and writer finalization, opt-in parallel worker execution, benchmark reporting, thread-safety guidance, Doxygen configuration, and compile-checked consumer examples.

### Archives

- [v1.0 roadmap archive](milestones/v1.0-ROADMAP.md)
- [v1.0 requirements archive](milestones/v1.0-REQUIREMENTS.md)
- [v1.0 audit archive](milestones/v1.0-MILESTONE-AUDIT.md)
- [v1.0 phase artifacts](milestones/v1.0-phases/)

### Validation Snapshot

- `cmake --build --preset windows-msvc-debug-static` passed.
- `ctest --preset windows-msvc-debug-static --output-on-failure` passed: 245/245 runnable tests, 2 expected opt-in fixture skips.
- `cmake --build --preset windows-msvc-debug-static --target libbsa_benchmark_report` passed with 8 correctness-checked benchmark rows.
- `git -C TES5Edit status --short` produced no output.

### Known Follow-Up

- Fresh requirements for the next milestone should start with `$gsd-new-milestone`.
- v2 candidates remain unpromoted until discussed: sample CLI, public fuzzing harnesses, lenient corrupt-archive recovery, and binary ABI policy.
