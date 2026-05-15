# Milestones

## v1.1 Hardening (Shipped: 2026-05-15)

**Phases completed:** 6 phases, 20 plans, 55 tasks
**Status:** Shipped  
**Scope:** Phases 13-17.1  
**Requirements:** 14/14 v1.1 requirements complete  
**Audit:** Tech-debt route on 2026-05-15, with no requirement gaps, no integration gaps, and no broken E2E flows  
**Deferred items at close:** 8 acknowledged open artifact-audit items; see `STATE.md` Deferred Items

**Key accomplishments:**

- Neutral host_file helper names now back the existing writer disk-read flows with focused rename regression coverage.
- Shared `host_file_path` state now keeps original UTF-8 diagnostics text beside one resolved Windows path while migrated writer reads reopen only through the resolved boundary.
- archive_reader now stores one resolved host-file path per open while TES3/TES4/BA2 parser entry seams reopen metadata only through the shared host_file boundary.
- Stored resolved host-file paths now drive follow-on TES3/TES4/BA2 extraction reopens and validation setup stays unified behind archive_reader::open.
- A dedicated Catch2 suite now proves non-ASCII Windows host-path open, validation, and canonical extraction across TES4 and representative BA2 archives from the public API.
- Runnable Release static/shared preset triads plus a real MSVC ASan hardening lane backed by policy-checked CTest package proof
- Role-aware Windows CI matrix plus a separate MSVC AddressSanitizer hardening job that both run the checked-in preset contract
- Quick-path Windows docs plus independently checked planning summaries for the supported debug, Release package-proof, and MSVC AddressSanitizer verification matrix
- archive_reader now chooses one backend seam at open time, then reuses it across lookup and extraction while dedicated runtime and policy suites lock the unchanged reader contract.
- TES4-family BSA parsing now routes checked raw table parsing and payload descriptor rules through private seams with direct Catch2 regression coverage.
- BA2 DX10 write preparation now separates add-time DDS snapshot ownership from planned chunk assembly and compression routing behind private seams with focused regression coverage.
- Dedicated source-policy guardrails now lock TES4 parser and BA2 DX10 preparer seam separation, with focused, full-debug, and MSVC ASan validation evidence for Phase 16 closure.
- TES4-family BSA writer dedupe now narrows candidates with a format-local keyed bucket while preserving exact final stored-byte equality before shared offsets.
- BA2 GNRL writer dedupe now uses explicit final-stored fingerprint evidence for candidate narrowing while preserving exact stored-byte equality and disk-source change rejection.
- BA2 DX10 writer snapshot directories are now cleaned during ordinary success and failure paths, and write attempts consume the writer to prevent stale snapshot reuse.
- BA2 DX10 writer lifecycle docs and policy tests now truthfully guard consumed write attempts, best-effort snapshot cleanup, residual abnormal-termination risk, and public API stability for DX10-02.
- Focused Debug, MSVC AddressSanitizer, and Release package proof evidence now closes Phase 17 and v1.1 with planning state aligned to verified writer-hotspot requirements.
- Manifest-backed reader matrix now proves copied non-ASCII Windows host paths exercise `entries`, `find`, `contains`, and `extract_entries` through the public API.
- The host-file and reader state contracts now keep only operational resolved-path and selected-backend-table data, with source-policy tests guarding against stale diagnostics or duplicate backend identity state returning.
- Writer host-path audit debt is closed with a source-policy inventory matrix plus focused Debug and MSVC AddressSanitizer verification evidence across all required Phase 17.1 requirements and decisions.

### Archives

- [v1.1 roadmap archive](milestones/v1.1-ROADMAP.md)
- [v1.1 requirements archive](milestones/v1.1-REQUIREMENTS.md)
- [v1.1 audit archive](milestones/v1.1-MILESTONE-AUDIT.md)
- [v1.1 phase artifacts](milestones/v1.1-phases/)

---

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
