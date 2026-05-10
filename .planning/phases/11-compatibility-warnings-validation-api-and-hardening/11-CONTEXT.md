# Phase 11: compatibility-warnings-validation-api-and-hardening - Context

**Gathered:** 2026-05-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 11 adds a dependency-light public validation API for existing archive files and libbsa-produced writer outputs. Consumers can run validation, receive a summary report with fatal archive diagnostics and typed compatibility warnings, and keep using `archive_reader::open` as the strict open path. Maintainers also get a compatibility evidence catalog, a consolidated malformed hardening matrix, optional skipped-by-default local corpus checks, and an additive sanitizer-oriented validation path.

</domain>

<spec_lock>
## Requirements (locked via SPEC.md)

**8 requirements are locked.** See `11-SPEC.md` for full requirements, boundaries, and acceptance criteria.

Downstream agents MUST read `11-SPEC.md` before planning or implementing. Requirements are not duplicated here.

**In scope (from SPEC.md):**
- Public validation result, error, and typed compatibility-warning surface in dependency-light C++20 headers.
- Validation of existing archive host paths and libbsa-produced writer-output archives.
- Stable warning codes or categories with human-readable diagnostic messages.
- Strict default parsing: malformed archives remain fatal for `archive_reader::open`.
- Validation reporting for malformed archives without producing a partially usable reader.
- Compatibility evidence documentation linking public warnings and non-obvious rules to fixtures, writer-output tests, TES5Edit/BSArchPro reference notes, or optional local corpus checks.
- Required generated legal fixture and writer-output evidence for Phase 11 acceptance.
- Optional local game or BSArchPro-derived comparison tests labeled `requires-game-fixture` and skipped by default.
- Expanded malformed matrix across TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10, compression, and DDS layout behavior.
- Sanitizer-oriented preset or documented CI/test path for malformed parser/decompressor hardening where supported.
- Regression checks proving existing reader, writer, public include boundary, and `TES5Edit/` read-only guarantees remain intact.

**Out of scope (from SPEC.md):**
- Lenient archive recovery mode that opens malformed archives as usable readers - v2 tracks lenient recovery after strict validation is complete.
- Exhaustive detection of every known Bethesda quirk - Phase 11 introduces stable warning infrastructure and representative warnings, not a complete forever-catalog.
- Mandatory local game archive, BSArchPro output, or copyrighted fixture data for default acceptance - generated legal fixtures remain the required evidence.
- Public logging framework integration - consumers receive structured validation results and decide their own logging.
- Public exposure of DirectXTex, DXGI, Windows SDK, libdeflate, lz4, TES5Edit, or private parser/codec types.
- Performance benchmarking, parallel extraction/packing, and concurrency guarantees - Phase 12 owns performance and concurrency.
- Doxygen site generation, full integration examples, and polished target-format guides - Phase 12 owns final documentation polish, though Phase 11 may add validation/evidence docs.
- In-place archive mutation or repair - v1 remains read/write-new plus strict validation.
- Editing, formatting, compiling into, staging, or using `TES5Edit/` as a fixture workspace - the submodule remains read-only reference material.

</spec_lock>

<decisions>
## Implementation Decisions

### Public Validation API Shape
- **D-01:** Expose a public free function as the primary validation entry point: `validate_archive(path, options = {}) -> result<validation_report>`.
- **D-02:** Keep `archive_reader::open` strict. Validation may report malformed archive errors without producing a usable reader.
- **D-03:** Archive-level fatal diagnostics go into `validation_report.errors` when validation can inspect the archive. Reserve `result` failures for call/setup failures such as invalid host path input or unreadable files.
- **D-04:** `validation_report` exposes overall validity, parseable `archive_metadata` when available, fatal validation errors, and compatibility warnings. Do not duplicate entry listing, extraction bytes, or a second reader API in the report.
- **D-05:** `validation_options` stays small: target-family expectation plus optional extraction/decompression-style validation toggles. Do not expose public corpus paths, logging callbacks, repair controls, or broad warning-filter machinery in Phase 11.

### Typed Compatibility Warnings
- **D-06:** Add a public `enum class compatibility_warning_code` for stable machine-readable warning identifiers.
- **D-07:** Warning records carry `code`, `severity`, `message`, and optional `archive_path` when a warning applies to a specific entry.
- **D-08:** Warning severity is a two-value public enum: `advisory` and `risky`. Codes remain the stable branch point; severity supports display, sorting, or escalation.
- **D-09:** Do not expose byte offsets, record indexes, or chunk indexes in public warning records for Phase 11.
- **D-10:** Tests assert warning codes, severity, and optional path presence, not exact diagnostic message text.
- **D-11:** Initial warning coverage should span representative known compatibility quirks across BSA and BA2 families using practical generated fixture or writer-output evidence. The goal is to prove useful infrastructure, not an exhaustive forever-catalog.

### Evidence And Hardening Proof Shape
- **D-12:** Add a committed human-readable Markdown compatibility evidence catalog and a machine check that fails when a public warning code is missing from the catalog.
- **D-13:** Add a single consolidated malformed coverage matrix spanning TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10, compression, oversized arithmetic, unsupported routes, and DDS chunk/layout cases. Existing per-family manifests can remain, but tests/manifests should prove each matrix row.
- **D-14:** Optional local game or BSArchPro-derived checks are smoke/compare only. They must be tagged `requires-game-fixture`, skip by default, and never gate default Phase 11 acceptance.
- **D-15:** Add an additive non-Windows Clang/GCC sanitizer-oriented preset or documented command path for malformed/parser/compression/validation labels. Keep default Windows MSVC static/shared CI unchanged.

### Carry-Forward Decisions
- **D-16:** Preserve dependency-light public headers: no public `std::expected`, libdeflate, lz4, DirectXTex, Windows SDK, TES5Edit, private parser, private writer, private hash, or private compression types.
- **D-17:** Preserve stable public error categories and tests that branch on programmatic identifiers rather than diagnostic strings.
- **D-18:** Generated legal fixtures and writer-produced archives are the mandatory evidence path. Optional local data remains ignored/skipped by default.
- **D-19:** Keep `TES5Edit/` read-only: do not edit, format, stage, compile, or use it as a fixture workspace.

### the agent's Discretion
- Researcher/planner may choose exact header placement, private helper layout, and test file organization if `libbsa/libbsa.hpp` exposes the public validation API and public include-boundary/package-consumer tests stay green.
- Researcher/planner may choose exact warning-code names and the first three representative warning scenarios, provided they span BSA/BA2 families and each code has generated or writer-output evidence plus catalog coverage.
- Researcher/planner may choose exact sanitizer preset naming and label selection if the path is additive, toolchain-gated, documented, and does not break default Windows MSVC CI.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase Scope
- `.planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md` - Locked Phase 11 requirements, boundaries, constraints, and acceptance criteria. MUST read before planning.
- `.planning/ROADMAP.md` - Phase 11 goal, mapped COMP-01 through COMP-06 requirements, success criteria, and Phase 12 boundary.
- `.planning/REQUIREMENTS.md` - Compatibility and validation requirements plus public API, fixture, performance, documentation, and v2 recovery boundaries.

### Project Constraints
- `.planning/PROJECT.md` - Project purpose, dependency policy, public API constraints, streaming goal, error model, key decisions, and TES5Edit boundary.
- `.planning/STATE.md` - Current project state and recent decisions from Phase 8 through Phase 10.
- `AGENTS.md` - Repository instructions, read-only `TES5Edit/` boundary, dependency policy, comments/docs expectations, and validation expectations.
- `docs/PRD.md` - Product goals, supported archive families, write-new scope, hardening expectations, and BSArchPro compatibility direction.

### Prior Phase Decisions
- `.planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-CONTEXT.md` - TES3 writer API, writer-output fixture proof, generated legal evidence, and reader-backed validation decisions.
- `.planning/phases/09-ba2-dx10-write-new-support/09-CONTEXT.md` - BA2 DX10 compressed-only writer rule, DirectXTex-private DDS proof strategy, chunk/dedupe behavior, and generated texture fixture policy.
- `.planning/phases/08-ba2-gnrl-write-new-support/08-CONTEXT.md` - BA2 GNRL writer targets, Starfield field policy, compression routing, safe publish, dedupe, and reader-backed validation patterns.

### Current Codebase State
- `include/libbsa/result.hpp` - Existing public `result<T>`, `error`, and stable `error_code` style to mirror for validation diagnostics.
- `include/libbsa/archive.hpp` - Existing public archive metadata, entry metadata, texture metadata, strict reader facade, and dependency-light header boundary.
- `include/libbsa/libbsa.hpp` - Umbrella public include that must expose the validation API.
- `tests/unit/public_include_boundary_tests.cpp` - Public API compile-boundary harness that should assert validation types/functions without leaking private dependencies.
- `tests/package-consumer/main.cpp` - Installed/package-consumer smoke path that should cover validation API availability.
- `tests/unit/validation_policy_tests.cpp` - Existing policy tests for fixture labels, local fixture behavior, static/shared CI, and TES5Edit read-only guarantees.
- `tests/fixtures/README.md` - Fixture provenance, generated/local split, label taxonomy, and TES5Edit fixture-workspace prohibition to update for compatibility evidence and sanitizer docs.
- `tests/fixtures/generated/validate_fixture_manifests.py` - Existing manifest-shape validator that can inspire or host additional catalog/matrix checks if planner chooses.
- `tests/CMakeLists.txt` - Catch2, generated fixture targets, labels, and package-consumer test wiring for adding validation and catalog checks.
- `CMakePresets.json` - Existing Windows MSVC static/shared presets; Phase 11 sanitizer path must be additive.
- `.github/workflows/ci.yml` - Current Windows static/shared CI and TES5Edit status check; default CI must remain green without local corpus or sanitizer support.

### Reference Material
- `TES5Edit/Core/wbBSArchive.pas` - Read-only behavioral reference for archive structure and compatibility notes; use for tracing only, never modify.
- `TES5Edit/Core/wbBSA.pas` - Read-only behavioral reference for BSA parsing/extraction behavior; use for compatibility research only.
- `TES5Edit/BSArchPro.dpr` - Read-only BSArchPro application reference for compatibility and pack/write option behavior context; do not compile into libbsa.
- `TES5Edit/BSArch/` - Read-only reference directory for BSArchPro-related behavior; do not edit, format, stage, compile, or use as fixture workspace.

### External Diagnostic Guidance
- `https://gcc.gnu.org/onlinedocs/gccint/Guidelines-for-Diagnostics.html` - Diagnostic wording should be actionable, precise, and useful to users deciding whether they care about a warning.
- `https://dev.mysql.com/doc/dev/connector-cpp/latest/group__xapi__diag.html` - Example of exposing warning count/iteration separately from errors in a C/C++ API.
- `https://learn.microsoft.com/en-us/cpp/cpp/errors-and-exception-handling-modern-cpp?view=msvc-170` - Background on public API error handling tradeoffs and when error-code style is appropriate.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `libbsa::result<T>`, `libbsa::error`, and `libbsa::error_code` already encode the stable-code plus human-message split Phase 11 should mirror.
- `archive_reader::open` already documents strict open-time behavior for invalid arguments, I/O failures, unsupported bytes, and malformed supported archives.
- `archive_metadata` and related public value types can be reused in `validation_report` without exposing private parser, compression, or DirectXTex details.
- Per-family generated malformed manifests already cover TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10 cases; Phase 11 can consolidate rather than invent from nothing.
- `validation_policy_tests.cpp` already checks labels, local fixture skip policy, CI preset/workflow expectations, and TES5Edit boundary.

### Established Patterns
- Public API additions belong under `include/libbsa/` and must be exposed through `include/libbsa/libbsa.hpp`.
- Public methods/types added or substantially rewritten need Doxygen-compliant comments.
- Tests assert stable enums and public metadata, not diagnostic text.
- Generated fixtures and manifests must use repository-owned synthetic data outside `TES5Edit/`.
- Optional local corpus tests must be labeled `requires-game-fixture` and skip cleanly when no local fixture path is configured.
- Default CI currently runs Windows MSVC static/shared presets and checks `TES5Edit` status. Sanitizer support must be additive, not a default Windows requirement.

### Integration Points
- Add public validation types/functions and include-boundary/package-consumer coverage.
- Reuse strict parser/open behavior to populate validation errors while keeping `archive_reader::open` unchanged.
- Add validation tests over existing success fixtures, writer-produced outputs, and malformed fixtures across all supported families.
- Add a compatibility evidence catalog and machine check that maps every public warning code to rule description and evidence.
- Add a consolidated malformed matrix and tests/scripts that prove its rows against existing or expanded manifests.
- Add sanitizer preset/docs for malformed/parser/compression/validation labels without changing existing Windows CI behavior.

</code_context>

<specifics>
## Specific Ideas

- The public validation API should feel like a small sibling to `archive_reader::open`, not a second reader class.
- Warning identifiers should be stable enum values; warning messages are for humans and should be actionable but not exact-string test contracts.
- The evidence catalog should make non-obvious Bethesda compatibility facts traceable without forcing downstream agents to reread every prior phase artifact.
- Optional corpus checks are useful as smoke/compare evidence but must never become required to pass the default suite.

</specifics>

<deferred>
## Deferred Ideas

None - discussion stayed within phase scope.

</deferred>

---

*Phase: 11-compatibility-warnings-validation-api-and-hardening*
*Context gathered: 2026-05-10*
