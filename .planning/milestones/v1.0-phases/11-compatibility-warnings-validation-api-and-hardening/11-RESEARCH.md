# Phase 11: Compatibility Warnings, Validation API, and Hardening - Research

**Researched:** 2026-05-10
**Domain:** C++20 public validation API, structured diagnostics, compatibility evidence, malformed archive hardening, CMake/Catch2 validation
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

The following constraints are copied from `.planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md`. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

### Locked Decisions

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

### Deferred Ideas (OUT OF SCOPE)

None - discussion stayed within phase scope.
</user_constraints>

## Summary

Phase 11 should add a small public validation layer beside the existing strict `archive_reader::open` path, not a second archive reader. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] The current public API already uses `libbsa::result<T>`, stable `error_code` values, and public metadata value types, so validation can return `result<validation_report>` while keeping fatal archive diagnostics in `validation_report.errors` when the archive can be inspected. [VERIFIED: include/libbsa/result.hpp; VERIFIED: include/libbsa/archive.hpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

The highest-risk planning areas are the public/private API boundary, the report-error split, warning evidence coverage, and the malformed matrix. [VERIFIED: tests/unit/public_include_boundary_tests.cpp; VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp] Existing tests already cover many malformed TES3, TES4-family, BA2 GNRL, and BA2 DX10 cases, but several manifest helper functions silently map unknown `expected_error` strings to `invalid_argument`, which conflicts with Phase 11 acceptance. [VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]

**Primary recommendation:** Implement `include/libbsa/validation.hpp` plus `src/validation.cpp` as a strict-open-backed validation facade, add warning policy helpers privately, add a machine-checked Markdown evidence catalog, consolidate malformed matrix coverage, and add an additive Clang/GCC sanitizer preset or documented CTest path without changing the default Windows MSVC static/shared CI. [VERIFIED: include/libbsa/libbsa.hpp; VERIFIED: src/archive.cpp; VERIFIED: CMakePresets.json; VERIFIED: .github/workflows/ci.yml; CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html]

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| COMP-01 | Maintainer can compare extracted fixture output against BSArchPro-derived expected bytes/metadata. | Keep generated legal fixtures mandatory, add an evidence catalog entry for each warning/rule, and keep optional local BSArchPro/game comparisons behind `requires-game-fixture` and skip-by-default behavior. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: tests/fixtures/README.md; VERIFIED: tests/unit/validation_policy_tests.cpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| COMP-02 | Maintainer can verify libbsa-produced archives load or validate as target-compatible. | Run the public validation API over existing writer-output paths for TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10 instead of adding a second reader surface. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: include/libbsa/writer.hpp; VERIFIED: tests/unit/tes3_bsa_writer_tests.cpp; VERIFIED: tests/unit/tes4_bsa_writer_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_writer_tests.cpp; VERIFIED: tests/unit/ba2_dx10_writer_tests.cpp] |
| COMP-03 | Consumer can receive structured compatibility warnings without adopting a logging framework. | Use public `compatibility_warning_code`, two-value severity enum, message text, and optional archive path in `validation_report.warnings`; do not add callbacks or logging integration. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| COMP-04 | Parser can gracefully reject malformed, truncated, oversized, or internally inconsistent archives. | Preserve strict `archive_reader::open` behavior and have validation report fatal diagnostics without producing a reader for malformed archives. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: include/libbsa/archive.hpp; VERIFIED: src/archive.cpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |
| COMP-05 | Malformed parser/decompressor hardening is backed by sanitizer tests. | Add an additive Clang/GCC sanitizer preset or documented command path selecting malformed, validation, parser, and compression labels; keep existing Windows presets unchanged. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: CMakePresets.json; VERIFIED: .github/workflows/ci.yml; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html; CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html] |
| COMP-06 | Maintainer can trace each non-obvious compatibility rule to evidence. | Add a Markdown compatibility evidence catalog plus a machine check that fails if a public warning code is undocumented. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
</phase_requirements>

## Project Constraints (from AGENTS.md)

- `TES5Edit/` is a read-only reference submodule and must not be edited, formatted, staged, compiled into libbsa, used as vendored source, or used as a mutable fixture workspace. [VERIFIED: AGENTS.md]
- Implementation belongs outside `TES5Edit/` and should preserve BSArchPro-compatible behavior when traced, with non-obvious compatibility constraints recorded near new implementation. [VERIFIED: AGENTS.md]
- Public implementation language is C++20, and public interfaces should be reusable, portable, and avoid Delphi/Pascal transliteration. [VERIFIED: AGENTS.md]
- Required dependencies are libdeflate, official lz4, DirectXTex, and vcpkg; new external dependencies require documented need, alternatives, and project impact. [VERIFIED: AGENTS.md]
- Public headers must remain minimal and should not leak platform, compression, DirectXTex, or private implementation details. [VERIFIED: AGENTS.md; VERIFIED: tests/unit/public_include_boundary_tests.cpp]
- Comments that are accurate must not be removed as cleanup; public APIs and substantially rewritten methods need Doxygen-compliant C++ doc comments. [VERIFIED: AGENTS.md]
- Validation should use focused tests for parsing, writing, round-tripping, compatibility behavior, byte-level fixture evidence, and metadata-level fixture evidence. [VERIFIED: AGENTS.md]

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| Public validation API | Public library API | Private validation implementation | Consumers need a dependency-light `validate_archive` function in installed headers, while parsing and policy logic should remain in `src/`. [VERIFIED: include/libbsa/libbsa.hpp; VERIFIED: tests/package-consumer/main.cpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| Fatal validation diagnostics | Private parser/reader layer | Public report model | Existing strict parsers own archive correctness checks; validation should translate parse failures into report diagnostics when inspection is possible. [VERIFIED: src/archive.cpp; VERIFIED: include/libbsa/archive.hpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| Compatibility warnings | Private policy layer | Public warning enums/records | Stable public codes are the consumer contract; rule detection and archive-family quirks should remain private and fixture-backed. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md; VERIFIED: tests/unit/public_include_boundary_tests.cpp] |
| Writer-output validation | Public writer tests | Reader/validation facade | Writer products should be validated using the same consumer-facing API that external users call after packing. [VERIFIED: include/libbsa/writer.hpp; VERIFIED: tests/unit/*writer_tests.cpp; VERIFIED: .planning/phases/10-tes3-write-support-and-bsa-format-completeness/10-VERIFICATION.md] |
| Compatibility evidence catalog | Documentation/test infrastructure | Warning policy tests | The catalog is a maintainer-facing artifact, but tests must enforce public warning-code coverage. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| Malformed hardening matrix | Test infrastructure | Parser/compression/DDS internals | Existing manifests are family-specific; Phase 11 needs a consolidated matrix that maps cases to parsers and expected public errors. [VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |
| Sanitizer path | Build/test infrastructure | Parser/compression tests | Sanitizers are a CMake/CTest execution mode, not part of the public runtime API. [VERIFIED: CMakePresets.json; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html; CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html] |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++ | C++20 | Public API and implementation language | The project requires C++20 and already uses a local `libbsa::result<T>` instead of public C++23 `std::expected`. [VERIFIED: AGENTS.md; VERIFIED: include/libbsa/result.hpp] |
| CMake | Minimum 3.24; local 4.3.2 available | Build, presets, install/export, CTest orchestration | `CMakePresets.json` declares schema version 6 and CMake minimum 3.24; local CMake reports 4.3.2. [VERIFIED: CMakePresets.json; VERIFIED: cmake --version] |
| vcpkg | Local executable `2026-04-08-e0612b42...` via `$env:VCPKG_ROOT\vcpkg.exe` | Manifest dependency acquisition | The repo uses committed `vcpkg.json` and `vcpkg-configuration.json`; Microsoft recommends manifest mode for most vcpkg users and project-scoped installs. [VERIFIED: vcpkg.json; VERIFIED: vcpkg-configuration.json; VERIFIED: $env:VCPKG_ROOT\vcpkg.exe x-package-info; CITED: https://learn.microsoft.com/vcpkg/concepts/manifest-mode] |
| Catch2 | 3.14.0 | Unit, fixture, compatibility, malformed tests | The repo already uses Catch2 through CMake and `catch_discover_tests`; Context7 documents `ADD_TAGS_AS_LABELS` and `DISCOVERY_MODE PRE_TEST`, matching current test wiring. [VERIFIED: vcpkg x-package-info catch2; VERIFIED: tests/CMakeLists.txt; CITED: Context7 /catchorg/catch2] |
| CTest | Local 4.3.2 | Test selection and CI execution | CTest supports `-L` label regex selection, and the repo uses Catch2 tags as CTest labels. [VERIFIED: ctest --version; VERIFIED: tests/CMakeLists.txt; CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html] |
| nlohmann-json | 3.12.0#2 | Test manifest and catalog validation | The repo already depends on `nlohmann-json` for tests and uses JSON manifests for generated fixture contracts. [VERIFIED: vcpkg x-package-info nlohmann-json; VERIFIED: tests/CMakeLists.txt; VERIFIED: tests/fixtures/generated/validate_fixture_manifests.py] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| libdeflate | 1.25 | Private deflate codec dependency | Use only through existing private compression paths; validation should exercise outputs, not expose libdeflate in public headers. [VERIFIED: vcpkg x-package-info libdeflate; VERIFIED: tests/unit/public_include_boundary_tests.cpp; VERIFIED: AGENTS.md] |
| lz4 | 1.10.0 | Private LZ4 frame/raw block codec dependency | Use only through existing private compression paths; do not depend on the GPL-licensed `tools` feature or CLI. [VERIFIED: vcpkg x-package-info lz4; VERIFIED: AGENTS.md] |
| DirectXTex | 2026-03-31 | Private DDS metadata/reconstruction validation dependency | Keep DirectXTex behind the existing internal texture/DDS boundary; never expose DirectXTex/DXGI types in validation headers. [VERIFIED: vcpkg x-package-info directxtex; VERIFIED: tests/unit/public_include_boundary_tests.cpp; VERIFIED: AGENTS.md] |
| Python | 3.14.4 local | Manifest/catalog validation scripts | Extend or add scripts for evidence catalog and malformed matrix checks. [VERIFIED: python --version; VERIFIED: tests/fixtures/generated/validate_fixture_manifests.py] |
| Clang | 22.1.5 local | Sanitizer-oriented local path | Use for additive sanitizer configure/test path when GCC is unavailable locally. [VERIFIED: clang++ --version; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html] |
| Git | 2.54.0.windows.1 local | TES5Edit cleanliness and CI checks | Keep `git status --short TES5Edit` empty as a phase gate. [VERIFIED: git --version; VERIFIED: .github/workflows/ci.yml; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `validate_archive` free function | `archive_validator` class | A class suggests retained state or a second reader; the locked decision requires a free function primary entry. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| Structured report vectors | Logging callbacks | Callbacks would add policy and threading/lifetime surface; Phase 11 explicitly excludes public logging integration. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| Existing strict parser reuse | Separate lenient parser | A separate parser risks divergence and contradicts the strict `archive_reader::open` default. [VERIFIED: src/archive.cpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |
| Generated/writer-output fixtures | Required local game archives | Required local archives would introduce copyrighted fixture dependency and violate default acceptance. [VERIFIED: tests/fixtures/README.md; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |
| Additive Clang/GCC sanitizer path | Default Windows MSVC sanitizer CI | Phase 11 locks default Windows static/shared CI unchanged; MSVC ASan exists but has option compatibility limits and should not become the default gate here. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md; CITED: https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170] |

**Installation:**

```powershell
$env:VCPKG_ROOT = "C:\vcpkg"
cmake --preset windows-msvc-debug-static
cmake --build --preset windows-msvc-debug-static
ctest --preset windows-msvc-debug-static --output-on-failure
```

**Version verification:**

```powershell
& $env:VCPKG_ROOT\vcpkg.exe x-package-info libdeflate --x-json
& $env:VCPKG_ROOT\vcpkg.exe x-package-info lz4 --x-json
& $env:VCPKG_ROOT\vcpkg.exe x-package-info directxtex --x-json
& $env:VCPKG_ROOT\vcpkg.exe x-package-info catch2 --x-json
& $env:VCPKG_ROOT\vcpkg.exe x-package-info nlohmann-json --x-json
```

The commands above emitted usable package JSON but returned exit code 1 because vcpkg warned that `vcpkg.json` sets `builtin-baseline` while `vcpkg-configuration.json` overrides the default registry; plan around the warning instead of treating package metadata as unavailable. [VERIFIED: vcpkg x-package-info command output; VERIFIED: vcpkg.json; VERIFIED: vcpkg-configuration.json]

## Architecture Patterns

### System Architecture Diagram

```text
Consumer / writer test / fixture test
        |
        v
validate_archive(host_path, validation_options)
        |
        +--> Host-path preflight
        |        |
        |        +--> unreadable / invalid host path
        |                |
        |                v
        |        result<validation_report> error
        |
        v
archive_reader::open(host_path)  [strict parser path]
        |
        +--> success
        |        |
        |        v
        |   metadata + entries
        |        |
        |        +--> target-family policy checks
        |        +--> compatibility warning collectors
        |        +--> optional extraction/decompression validation pass
        |        |
        |        v
        |   validation_report{valid=true, metadata, warnings, errors=[]}
        |
        +--> supported archive malformed / unsupported route / decode failure
                 |
                 v
          validation_report{valid=false, errors=[fatal diagnostic], warnings=[]}

Compatibility evidence catalog + malformed matrix checks
        |
        v
CTest/Catch2/Python verification over generated fixtures and writer outputs
```

This design lets callers trace the primary path from host path to strict parser, metadata extraction, policy checks, optional decode validation, and report output. [VERIFIED: src/archive.cpp; VERIFIED: include/libbsa/archive.hpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

### Recommended Project Structure

```text
include/libbsa/
├── validation.hpp        # Public validation enums, records, options, report, validate_archive
└── libbsa.hpp            # Umbrella include adds validation.hpp

src/
├── validation.cpp        # validate_archive implementation and report assembly
└── detail/
    └── validation_policy.*  # Private warning/rule helpers if needed

docs/
└── compatibility-evidence.md  # Warning/rule evidence catalog

tests/
├── unit/
│   ├── validation_api_tests.cpp
│   ├── compatibility_warning_tests.cpp
│   ├── compatibility_evidence_tests.cpp
│   └── malformed_matrix_tests.cpp
├── fixtures/
│   ├── README.md
│   └── generated/
│       ├── compatibility_matrix.json
│       └── validate_fixture_manifests.py
└── package-consumer/main.cpp
```

Header placement is at planner discretion, but `libbsa/libbsa.hpp` must expose the public validation API and public include-boundary/package-consumer tests must stay green. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md; VERIFIED: include/libbsa/libbsa.hpp; VERIFIED: tests/unit/public_include_boundary_tests.cpp; VERIFIED: tests/package-consumer/main.cpp]

### Pattern 1: Public Report Mirrors Existing Result/Error Style

**What:** Define public validation types with stable enums for branching and human text for display. [VERIFIED: include/libbsa/result.hpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**When to use:** Use this for all public validation output; tests should branch on enum values and path presence, not full strings. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**Example:**

```cpp
// Source: local API pattern in include/libbsa/result.hpp and Phase 11 CONTEXT.md.
namespace libbsa {

enum class compatibility_warning_severity {
  advisory,
  risky,
};

enum class compatibility_warning_code {
  compressed_sound_payload,
  embedded_name_target_risk,
  ba2_target_family_mismatch,
};

struct compatibility_warning {
  compatibility_warning_code code;
  compatibility_warning_severity severity;
  std::string message;
  std::optional<std::string> archive_path;
};

struct validation_diagnostic {
  error_code code;
  std::string message;
  std::optional<std::string> archive_path;
};

struct validation_options {
  std::optional<archive_variant> expected_variant;
  bool validate_extractability{false};
};

struct validation_report {
  bool valid{false};
  std::optional<archive_metadata> metadata;
  std::vector<validation_diagnostic> errors;
  std::vector<compatibility_warning> warnings;
};

[[nodiscard]] result<validation_report> validate_archive(std::string_view path,
                                                         validation_options options = {});

} // namespace libbsa
```

The exact enum names are discretionary, but the public shape above satisfies the locked decisions and keeps third-party/private types out of headers. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md; VERIFIED: tests/unit/public_include_boundary_tests.cpp]

### Pattern 2: Strict-Open-Backed Validation

**What:** Implement validation by using existing reader dispatch first; on success, assemble metadata and warnings; on failure, convert inspectable archive failures into `validation_report.errors`. [VERIFIED: src/archive.cpp; VERIFIED: include/libbsa/archive.hpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**When to use:** Use for all existing archive and writer-output validation unless a future phase explicitly adds lenient recovery. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]

**Example:**

```cpp
// Source: local result/error style in include/libbsa/result.hpp.
auto result = libbsa::validate_archive(path, options);
if (!result) {
  // Invalid host path or unreadable file: caller/setup failure.
  return result.error().code;
}

const auto& report = result.value();
if (!report.valid) {
  for (const auto& diagnostic : report.errors) {
    // Branch on diagnostic.code, not diagnostic.message.
  }
}

for (const auto& warning : report.warnings) {
  // Branch on warning.code; display warning.message if desired.
}
```

### Pattern 3: Evidence Catalog Coverage Check

**What:** Store compatibility rules and public warning codes in a committed Markdown catalog, then add a test/script that fails when a public `compatibility_warning_code` is missing from the catalog. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**When to use:** Use every time a warning code or non-obvious compatibility rule is added. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]

**Example:**

```markdown
<!-- Source: Phase 11 CONTEXT.md D-12 evidence catalog requirement. -->
| Warning Code | Severity | Rule | Evidence |
|--------------|----------|------|----------|
| compressed_sound_payload | risky | Compressed sound-like payloads are target-risky. | generated fixture: ...; test: ... |
```

The planner should decide whether the machine check is C++ test code, Python, or both; the check must make missing codes fail deterministically. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

### Pattern 4: Consolidated Malformed Matrix Over Existing Manifests

**What:** Keep existing family manifests, but add a single Phase 11 matrix mapping each required malformed category to at least one manifest case, test phase, and expected public `error_code`. [VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**When to use:** Use for COMP-04 and COMP-05 planning so family-specific gaps are visible before implementation tasks are split. [VERIFIED: .planning/REQUIREMENTS.md]

**Example:**

```json
{
  "id": "ba2_dx10_decoded_size_mismatch",
  "family": "ba2_dx10",
  "category": "decompression_size_mismatch",
  "manifest": "ba2_dx10_malformed_manifest.json",
  "expected_error": "format_error",
  "phase": "extract"
}
```

### Anti-Patterns to Avoid

- **Warning-as-open-success recovery:** Do not downgrade malformed parser failures into warnings or return a usable reader for malformed archives. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]
- **Second public reader in `validation_report`:** Do not duplicate entry listing, extraction bytes, or a second reader API in the report. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]
- **Public implementation leakage:** Do not expose `std::expected`, libdeflate, lz4, DirectXTex, DXGI, Windows SDK, TES5Edit, private parser, private writer, private hash, or private compression types in validation headers. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md; VERIFIED: tests/unit/public_include_boundary_tests.cpp]
- **Message-string tests:** Do not compare full diagnostic messages in tests; compare stable enum values, severity, and optional path presence. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]
- **Required local corpus:** Do not make game archives, BSArchPro outputs, or copyrighted bytes required for default acceptance. [VERIFIED: tests/fixtures/README.md; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Error transport | Custom exception hierarchy for archive data failures | Existing `libbsa::result<T>` and `error_code` | The public API already uses result/error values for I/O and format failures. [VERIFIED: include/libbsa/result.hpp; VERIFIED: AGENTS.md] |
| Logging integration | Logger interface, callback tree, or sink ownership model | `validation_report.errors` and `validation_report.warnings` | Logging is explicitly out of scope; consumers can log structured records themselves. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| Archive parsing | A separate validation parser | Existing strict `archive_reader::open` and private parsers | Parser duplication risks divergent compatibility behavior. [VERIFIED: src/archive.cpp; VERIFIED: include/libbsa/archive.hpp] |
| JSON parsing in tests | Ad hoc string parsing | Existing nlohmann-json test dependency and Python JSON scripts | The repo already uses structured JSON fixture manifests. [VERIFIED: tests/CMakeLists.txt; VERIFIED: tests/fixtures/generated/validate_fixture_manifests.py] |
| DDS validation | Hand-written public DDS/DXGI parser | Existing private DirectXTex-backed DDS path | DirectXTex is required but must remain private. [VERIFIED: AGENTS.md; VERIFIED: tests/unit/public_include_boundary_tests.cpp] |
| Sanitizer orchestration | Custom test runner | CMake presets and CTest label selection | CMake/CTest already owns builds/tests; CTest supports label regex selection. [VERIFIED: CMakePresets.json; CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html] |
| Compatibility corpus | Required committed game archives | Generated fixtures plus skipped local `requires-game-fixture` checks | Default acceptance must not rely on copyrighted local data. [VERIFIED: tests/fixtures/README.md; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |

**Key insight:** The hard part is not parsing one more archive shape; it is keeping public validation stable while proving each warning and fatal diagnostic through legal fixtures, writer outputs, and sanitizer-friendly malformed coverage. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md; VERIFIED: tests/fixtures/README.md; VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json]

## Common Pitfalls

### Pitfall 1: Confusing `result` Failure With Reported Archive Failure

**What goes wrong:** Validation returns `result` failure for malformed archive bytes that could have been inspected. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**Why it happens:** Existing `archive_reader::open` returns `result<archive_reader>` errors directly, so a thin wrapper can accidentally preserve the old single-error shape. [VERIFIED: include/libbsa/archive.hpp; VERIFIED: src/archive.cpp]

**How to avoid:** Reserve `result` failure for invalid host path or unreadable file setup failures; put inspectable archive fatal diagnostics in `validation_report.errors`. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**Warning signs:** Tests only assert `!validate_archive(...)` for malformed fixtures instead of asserting `validate_archive(...).value().errors`. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]

### Pitfall 2: Unknown Manifest Errors Silently Passing

**What goes wrong:** A typo in `expected_error` silently maps to `invalid_argument`. [VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp]

**Why it happens:** Several test-local `error_code_from_manifest` helpers return `invalid_argument` in the default branch. [VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp]

**How to avoid:** Replace default mapping with a hard test failure or shared strict parser before adding the consolidated matrix. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]

**Warning signs:** New malformed matrix rows pass after intentionally misspelling `format_error`. [VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp]

### Pitfall 3: Public Header Dependency Leakage

**What goes wrong:** `validation.hpp` includes or names private implementation types, platform types, compression libraries, DirectXTex/DXGI types, or `std::expected`. [VERIFIED: tests/unit/public_include_boundary_tests.cpp]

**Why it happens:** Validation needs to summarize parser and DDS behavior, which tempts direct reuse of internal structs. [VERIFIED: include/libbsa/archive.hpp; VERIFIED: include/libbsa/writer.hpp]

**How to avoid:** Use `archive_metadata`, `error_code`, strings, vectors, optional values, and libbsa-owned enums only; extend public include-boundary tests for validation types. [VERIFIED: include/libbsa/archive.hpp; VERIFIED: include/libbsa/result.hpp; VERIFIED: tests/unit/public_include_boundary_tests.cpp]

**Warning signs:** Public token scan finds `DirectX`, `DXGI`, `libdeflate`, `lz4`, `std::expected`, `TES5Edit`, or private helper names in installed headers. [VERIFIED: tests/unit/public_include_boundary_tests.cpp]

### Pitfall 4: Evidence Catalog Drifts From Public Enum

**What goes wrong:** New `compatibility_warning_code` values are added without documentation or fixture/reference evidence. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**Why it happens:** C++ enums are not automatically enumerable without an explicit list or source scan. [ASSUMED]

**How to avoid:** Add a test-local or private authoritative list, or a conservative source parser, and compare it against the Markdown catalog. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]

**Warning signs:** Catalog test only checks that the file exists. [ASSUMED]

### Pitfall 5: Sanitizer Path Breaks Default Windows CI

**What goes wrong:** Sanitizer flags are added to existing Windows MSVC static/shared presets or required workflow matrix entries. [VERIFIED: CMakePresets.json; VERIFIED: .github/workflows/ci.yml]

**Why it happens:** Sanitizers are useful enough that implementers may make them a default build option. [ASSUMED]

**How to avoid:** Add a separate Clang/GCC preset or documented command path and select labels with CTest `-L`; keep `windows-msvc-debug-static` and `windows-msvc-debug-shared` unchanged. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md; CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html]

**Warning signs:** `.github/workflows/ci.yml` changes its existing static/shared matrix to require sanitizer support. [VERIFIED: .github/workflows/ci.yml]

### Pitfall 6: Stale Local Build Tree

**What goes wrong:** CTest uses stale include paths from `J:/libbsa-gsd/build/...` instead of the current `J:/libbsa` checkout. [VERIFIED: ctest --test-dir build/windows-msvc-debug-static -N output]

**Why it happens:** The existing `build/windows-msvc-debug-static` directory contains generated CTest include paths from another checkout. [VERIFIED: ctest --test-dir build/windows-msvc-debug-static -N output]

**How to avoid:** Start Phase 11 validation by reconfiguring the preset before trusting local build/test output. [VERIFIED: cmake --preset windows-msvc-debug-static; VERIFIED: CMakePresets.json]

**Warning signs:** CTest reports it cannot find `J:/libbsa-gsd/build/windows-msvc-debug-static/tests/libbsa_tests-..._include.cmake`. [VERIFIED: ctest --test-dir build/windows-msvc-debug-static -N output]

## Code Examples

Verified patterns from official and local sources:

### Consumer Validation Flow

```cpp
// Source: include/libbsa/result.hpp and Phase 11 CONTEXT.md.
const auto checked = libbsa::validate_archive("example.bsa");
if (!checked) {
  // Invalid host path or unreadable file.
  return checked.error().code;
}

const libbsa::validation_report& report = checked.value();
for (const auto& diagnostic : report.errors) {
  switch (diagnostic.code) {
    case libbsa::error_code::format_error:
    case libbsa::error_code::unsupported:
      break;
    default:
      break;
  }
}

for (const auto& warning : report.warnings) {
  switch (warning.code) {
    case libbsa::compatibility_warning_code::compressed_sound_payload:
      break;
    default:
      break;
  }
}
```

### CTest Label Selection

```powershell
# Source: CMake ctest(1) manual, CTest -L label regex behavior.
ctest --preset windows-msvc-debug-static -L "malformed|validation|compat" --output-on-failure
```

CTest `-L` runs tests whose labels match a regular expression; multiple `-L` options are an AND relationship. [CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html]

### Additive Sanitizer Preset Shape

```json
{
  "name": "linux-clang-debug-sanitize",
  "displayName": "Linux Clang Debug Sanitizers",
  "generator": "Ninja",
  "binaryDir": "${sourceDir}/build/${presetName}",
  "condition": {
    "type": "equals",
    "lhs": "${hostSystemName}",
    "rhs": "Linux"
  },
  "cacheVariables": {
    "CMAKE_BUILD_TYPE": "Debug",
    "CMAKE_CXX_STANDARD": "20",
    "CMAKE_CXX_STANDARD_REQUIRED": "ON",
    "CMAKE_CXX_COMPILER": "clang++",
    "CMAKE_CXX_FLAGS_DEBUG": "-fsanitize=address,undefined -fno-omit-frame-pointer",
    "CMAKE_EXE_LINKER_FLAGS_DEBUG": "-fsanitize=address,undefined",
    "LIBBSA_BUILD_TESTS": "ON"
  }
}
```

CMake presets support `cacheVariables`, and GCC documents AddressSanitizer, UndefinedBehaviorSanitizer, debug info, and frame-pointer guidance for sanitizer diagnostics. [CITED: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html]

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `archive_reader::open` is the only consumer-facing validity check. | Add `validate_archive` that can return a report with errors and warnings while preserving strict open behavior. | Phase 11 decision. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] | Consumers can validate and inspect warnings without opening malformed archives. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |
| Compatibility evidence is spread across specs, tests, manifests, and comments. | Add a committed Markdown evidence catalog plus machine coverage check. | Phase 11 decision. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] | Maintainers can trace warning rules to generated fixtures, writer output, reference notes, or optional local checks. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |
| Malformed tests are family-specific and helper behavior differs by file. | Add one consolidated matrix and reject unknown `expected_error` values. | Phase 11 acceptance. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] | Coverage gaps and manifest typos become visible. [VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp] |
| Default CI is Windows MSVC static/shared only. | Keep default CI unchanged and add a separate sanitizer-oriented path. | Phase 11 decision. [VERIFIED: CMakePresets.json; VERIFIED: .github/workflows/ci.yml; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] | Hardening can run where supported without breaking default developer machines. [CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html; CITED: https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170] |

**Deprecated/outdated:**

- Public `std::expected` is not acceptable for this C++20 public API. [VERIFIED: AGENTS.md; VERIFIED: include/libbsa/result.hpp; VERIFIED: tests/unit/public_include_boundary_tests.cpp]
- Public logging callbacks are out of scope for Phase 11. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]
- Required local game/BSArchPro corpus data is out of scope for default acceptance. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md; VERIFIED: tests/fixtures/README.md]
- In-place mutation or repair is out of scope. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | C++ enum catalog drift needs an explicit list or source scan because C++20 does not provide reflection-style enum iteration. | Common Pitfalls | A weaker catalog check could miss undocumented warning codes. |
| A2 | Implementers may accidentally make sanitizer flags default because sanitizer hardening is valuable. | Common Pitfalls | Default Windows CI or local MSVC builds could break. |
| A3 | A catalog check that only verifies file existence is insufficient. | Common Pitfalls | Warning coverage could appear complete while public warning codes remain undocumented. |
| A4 | The consolidated malformed matrix format was resolved during planning as JSON with manifest-backed and test-backed row types. | Resolved Open Questions | If implementers drift from the resolved schema, machine validation could become weak or hard to maintain. |
| A5 | Research freshness estimate is 30 days for repo-local architecture and 7 days for dependency/tool versions. | Metadata | Planner could rely on stale tool/package versions if implementation starts later. |

## Open Questions (All RESOLVED)

1. **RESOLVED 2026-05-10: Which three warning scenarios should be first?**
   - What we know: The phase requires at least three warning-producing valid archive scenarios spanning BSA and BA2 families, and project docs mention sound compression risk, embedded-name target risk, and target-family compatibility checks as relevant compatibility areas. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md; VERIFIED: docs/PRD.md; VERIFIED: include/libbsa/writer.hpp]
   - Resolution: Plans 11-01 and 11-03 lock the initial public warning codes to `compressed_sound_payload`, `bsa_embedded_name_compatibility_risk`, and `target_family_mismatch`. Warning tests use generated or writer-output evidence, assert code/severity/path presence only, and Plan 11-04 catalogs the warning evidence after Plan 11-03 creates `tests/unit/compatibility_warning_tests.cpp`. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-01-PLAN.md; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-03-PLAN.md; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-04-PLAN.md]

2. **RESOLVED 2026-05-10: Should the sanitizer path be preset-only or also a CI lane?**
   - What we know: Phase 11 requires a sanitizer-oriented preset or documented command path and locks default Windows MSVC CI unchanged. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]
   - Resolution: Plan 11-07 adds configure/build/test presets named `linux-clang-asan-ubsan`, documents `ctest --preset linux-clang-asan-ubsan -L "malformed|validation|compression" --output-on-failure`, and explicitly does not add the sanitizer path to `.github/workflows/ci.yml`. Default Windows MSVC static/shared CI remains unchanged. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-07-PLAN.md; VERIFIED: CMakePresets.json; VERIFIED: .github/workflows/ci.yml]

3. **RESOLVED 2026-05-10: Where should malformed matrix data live?**
   - What we know: Existing manifests live under `tests/fixtures/generated/archives`, and `validate_fixture_manifests.py` already validates some manifest schema. [VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; VERIFIED: tests/fixtures/generated/validate_fixture_manifests.py]
   - Resolution: Plan 11-06 stores the machine-readable matrix at `tests/fixtures/generated/compatibility_matrix.json`, validates it from `tests/fixtures/generated/validate_fixture_manifests.py` and `tests/unit/compatibility_matrix_tests.cpp`, and supports two explicit row shapes: `evidence_type: "manifest"` for generated archive/manifest/case rows and `evidence_type: "test"` for in-repo test-backed rows. The oversized TES4-family arithmetic row uses the test-backed path against `tests/unit/tes4_bsa_reader_tests.cpp`; all other malformed archive rows remain generated/legal manifest-backed evidence outside `TES5Edit/`. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-06-PLAN.md; VERIFIED: tests/fixtures/generated/validate_fixture_manifests.py; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp]

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| CMake | Configure/build/presets | Yes | 4.3.2 | None needed. [VERIFIED: cmake --version] |
| CTest | Test execution and labels | Yes | 4.3.2 | None needed. [VERIFIED: ctest --version] |
| vcpkg executable | Dependency metadata/install | Yes via `$env:VCPKG_ROOT\vcpkg.exe`; not on PATH as `vcpkg` | 2026-04-08 build | Invoke through `$env:VCPKG_ROOT\vcpkg.exe`. [VERIFIED: $env:VCPKG_ROOT\vcpkg.exe x-package-info] |
| Python | JSON/catalog validators | Yes | 3.14.4 | C++/nlohmann-json test if script execution is unavailable. [VERIFIED: python --version; VERIFIED: tests/CMakeLists.txt] |
| Clang++ | Sanitizer path | Yes | 22.1.5 | Use GCC on Linux CI if available. [VERIFIED: clang++ --version; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html] |
| GCC/G++ | Alternative sanitizer path | No local `g++` found | - | Use Clang++ locally; planner can target GCC in Linux CI if desired. [VERIFIED: command availability probe] |
| MSVC `cl` in current shell | Windows local build | No in current PowerShell PATH | - | Run from a Visual Studio Developer Prompt or rely on CI; do not block research. [VERIFIED: command availability probe] |
| Git | TES5Edit cleanliness | Yes | 2.54.0.windows.1 | None needed. [VERIFIED: git --version] |
| Existing `build/windows-msvc-debug-static` | Local test reuse | Present but stale | CTest points to `J:/libbsa-gsd` include path | Reconfigure before testing. [VERIFIED: ctest --test-dir build/windows-msvc-debug-static -N output] |

**Missing dependencies with no fallback:**

- None for planning; local Windows build execution may require a Visual Studio Developer Prompt because `cl` is not on this PowerShell PATH. [VERIFIED: command availability probe]

**Missing dependencies with fallback:**

- `vcpkg` is not on PATH, but `$env:VCPKG_ROOT\vcpkg.exe` works. [VERIFIED: $env:VCPKG_ROOT\vcpkg.exe x-package-info]
- `g++` is not on PATH, but local `clang++` exists for a sanitizer-oriented path. [VERIFIED: command availability probe; VERIFIED: clang++ --version]
- Existing build output is stale and should be regenerated before local validation. [VERIFIED: ctest --test-dir build/windows-msvc-debug-static -N output]

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.14.0 plus CTest 4.3.2. [VERIFIED: vcpkg x-package-info catch2; VERIFIED: ctest --version] |
| Config file | `tests/CMakeLists.txt`, `CMakePresets.json`. [VERIFIED: tests/CMakeLists.txt; VERIFIED: CMakePresets.json] |
| Quick run command | `cmake --preset windows-msvc-debug-static && cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static -R "validation|compatibility|malformed|public_include_boundary|package_consumer" --output-on-failure` [VERIFIED: CMakePresets.json; VERIFIED: tests/CMakeLists.txt] |
| Full suite command | `cmake --preset windows-msvc-debug-static && cmake --build --preset windows-msvc-debug-static && ctest --preset windows-msvc-debug-static --output-on-failure` and repeat for `windows-msvc-debug-shared`. [VERIFIED: CMakePresets.json; VERIFIED: .github/workflows/ci.yml] |

### Phase Requirements To Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| COMP-01 | Fixture extraction bytes/metadata can be compared against generated or optional BSArchPro-derived expectations. | fixture/compat | `ctest --preset windows-msvc-debug-static -L "fixture|compat" --output-on-failure` | Partial; existing fixture tests exist, Phase 11 compatibility fixture tests/catalog missing. [VERIFIED: tests/unit/*reader_tests.cpp; VERIFIED: tests/fixtures/README.md] |
| COMP-02 | Validation succeeds for generated archives and writer-produced archives across TES3, TES4-family BSA, BA2 GNRL, BA2 DX10. | unit/roundtrip/compat | `ctest --preset windows-msvc-debug-static -R "validation|writer" --output-on-failure` | Missing `validation_api_tests.cpp`; writer tests exist. [VERIFIED: tests/unit/*writer_tests.cpp] |
| COMP-03 | Valid warning scenarios produce stable code/severity/path records without exact message assertions. | unit/compat | `ctest --preset windows-msvc-debug-static -R "compatibility_warning" --output-on-failure` | Missing. [VERIFIED: rg --files tests/unit] |
| COMP-04 | Malformed archives fail strict open and validation reports fatal errors without usable reader. | malformed/unit | `ctest --preset windows-msvc-debug-static -L malformed --output-on-failure` | Partial; existing malformed tests exist, consolidated matrix missing. [VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; VERIFIED: tests/unit/*malformed_tests.cpp] |
| COMP-05 | Sanitizer-backed malformed/parser/compression/validation path exists and selects hardening labels. | build/test | `ctest --preset <sanitize-preset> -L "malformed|validation|compression" --output-on-failure` | Missing. [VERIFIED: CMakePresets.json; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html] |
| COMP-06 | Every public warning code is documented with rule and evidence reference. | docs/unit/script | `ctest --preset windows-msvc-debug-static -R "compatibility_evidence" --output-on-failure` | Missing. [VERIFIED: rg --files docs tests] |

### Sampling Rate

- **Per task commit:** Run the targeted validation/warning/malformed CTest subset for touched behavior. [VERIFIED: tests/CMakeLists.txt; CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html]
- **Per wave merge:** Run full `windows-msvc-debug-static` suite, including public include boundary and package consumer tests. [VERIFIED: CMakePresets.json; VERIFIED: tests/unit/public_include_boundary_tests.cpp; VERIFIED: tests/package-consumer/main.cpp]
- **Phase gate:** Run static and shared full suites, optional sanitizer path where supported, `git status --short TES5Edit`, and catalog/matrix machine checks. [VERIFIED: .github/workflows/ci.yml; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]

### Wave 0 Gaps

- [ ] `include/libbsa/validation.hpp` - public validation API types and Doxygen comments. [VERIFIED: include/libbsa/libbsa.hpp; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]
- [ ] `src/validation.cpp` - strict-open-backed report assembly. [VERIFIED: src/archive.cpp]
- [ ] `tests/unit/validation_api_tests.cpp` - public API and writer-output validation. [VERIFIED: tests/unit/*writer_tests.cpp]
- [ ] `tests/unit/compatibility_warning_tests.cpp` - three warning scenarios and no exact message assertions. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]
- [ ] `docs/compatibility-evidence.md` plus `tests/unit/compatibility_evidence_tests.cpp` or script - warning-code catalog coverage. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]
- [ ] `tests/fixtures/generated/compatibility_matrix.json` or equivalent - consolidated malformed matrix. [VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json]
- [ ] Strict manifest expected-error mapping - replace silent default `invalid_argument` branches. [VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp]
- [ ] Sanitizer preset or documented command path - additive only. [VERIFIED: CMakePresets.json; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]
- [ ] Reconfigure local build tree before validation because the current build directory contains stale `J:/libbsa-gsd` CTest references. [VERIFIED: ctest --test-dir build/windows-msvc-debug-static -N output]

## Security Domain

Security enforcement is enabled by default because `.planning/config.json` does not explicitly set `security_enforcement` to `false`. [VERIFIED: .planning/config.json]

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | No | libbsa is a local archive library with no authentication surface. [VERIFIED: .planning/PROJECT.md] |
| V3 Session Management | No | libbsa has no session state or web session boundary. [VERIFIED: .planning/PROJECT.md] |
| V4 Access Control | No | This phase validates local archive data and does not define authorization policy. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |
| V5 Input Validation | Yes | Treat archive bytes, header counts, paths, compression metadata, and DDS chunks as untrusted input; enforce strict parser errors, bounded allocation, exact-size decompression, and malformed matrix tests. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json] |
| V6 Cryptography | No | Phase 11 has no cryptographic operation or key material. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |

OWASP ASVS is a web-application verification standard, so only input-validation guidance maps directly to this C++ archive-library phase. [CITED: https://owasp.org/www-project-application-security-verification-standard/; VERIFIED: .planning/PROJECT.md]

### Known Threat Patterns for C++ Archive Validation

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed counts or sizes causing oversized allocation or integer overflow | Denial of Service / Tampering | Check archive-controlled counts and size arithmetic before allocation; add malformed matrix rows and sanitizer path. [VERIFIED: .planning/REQUIREMENTS.md; VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html] |
| Truncated payload or table data causing out-of-bounds reads | Denial of Service | Preserve strict parser bounds checks and validate fatal diagnostics under `malformed` tests. [VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md] |
| Decompression size mismatch or corrupt payload | Denial of Service / Tampering | Use exact expected-size decompression checks through existing private codecs and report stable `format_error` diagnostics. [VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; VERIFIED: include/libbsa/result.hpp] |
| Duplicate canonical paths or path normalization confusion | Tampering | Keep archive-internal paths normalized as virtual paths and reject duplicate canonical entries. [VERIFIED: tests/fixtures/generated/archives/*_malformed_manifest.json; VERIFIED: docs/PRD.md] |
| Public diagnostics leaking private offsets/chunk indexes as stable API | Information Disclosure / API Lock-in | Do not expose byte offsets, record indexes, or chunk indexes in public warnings. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md] |
| Copyrighted local game data accidentally becoming a required test artifact | Compliance / Supply Chain | Keep local corpus ignored, labeled `requires-game-fixture`, and skipped by default. [VERIFIED: tests/fixtures/README.md; VERIFIED: tests/unit/validation_policy_tests.cpp] |

## Sources

### Primary (HIGH confidence)

- `.planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md` - locked Phase 11 decisions, scope, canonical refs, and discretion areas. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md]
- `.planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md` - locked requirements, boundaries, acceptance criteria. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-SPEC.md]
- `.planning/REQUIREMENTS.md`, `.planning/STATE.md`, `.planning/ROADMAP.md`, `.planning/PROJECT.md`, `docs/PRD.md`, `AGENTS.md` - project and phase constraints. [VERIFIED: repo file reads]
- `include/libbsa/result.hpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/libbsa.hpp`, `src/archive.cpp` - current public API and strict reader behavior. [VERIFIED: repo file reads]
- `tests/CMakeLists.txt`, `tests/unit/public_include_boundary_tests.cpp`, `tests/package-consumer/main.cpp`, `tests/unit/validation_policy_tests.cpp`, `tests/fixtures/README.md`, `tests/fixtures/generated/validate_fixture_manifests.py`, `tests/fixtures/generated/archives/*_malformed_manifest.json` - current validation/test infrastructure. [VERIFIED: repo file reads]
- `CMakePresets.json`, `.github/workflows/ci.yml`, `vcpkg.json`, `vcpkg-configuration.json` - current build, CI, and dependency wiring. [VERIFIED: repo file reads]
- Context7 `/catchorg/catch2` - Catch2/CMake discovery, `ADD_TAGS_AS_LABELS`, `DISCOVERY_MODE PRE_TEST`. [CITED: Context7 /catchorg/catch2]
- Context7 `/kitware/cmake` - CTest label selection and CMake test preset/cache variable behavior. [CITED: Context7 /kitware/cmake]
- Microsoft Learn vcpkg manifest mode - manifest mode and project-scoped dependency installs. [CITED: https://learn.microsoft.com/vcpkg/concepts/manifest-mode]
- Microsoft Learn C++ error handling - public API error-code tradeoffs. [CITED: https://learn.microsoft.com/en-us/cpp/cpp/errors-and-exception-handling-modern-cpp?view=msvc-170]
- Official CMake docs - CTest `-L`, test presets, and preset `cacheVariables`. [CITED: https://cmake.org/cmake/help/latest/manual/ctest.1.html; CITED: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html]
- GCC instrumentation docs - AddressSanitizer and UndefinedBehaviorSanitizer compiler options. [CITED: https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html]
- Microsoft Learn AddressSanitizer - MSVC `/fsanitize=address` support and limitations. [CITED: https://learn.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-170]
- OWASP ASVS project page - ASVS scope as web application security verification standard. [CITED: https://owasp.org/www-project-application-security-verification-standard/]

### Secondary (MEDIUM confidence)

- Local command probes for CMake, CTest, Python, Git, Clang, vcpkg package info, and command availability. [VERIFIED: shell command output]

### Tertiary (LOW confidence)

- None used for recommendations; assumptions are listed in the Assumptions Log. [VERIFIED: this research]

## Metadata

**Confidence breakdown:**

- Standard stack: HIGH - package versions and tools were verified through repo files, vcpkg metadata, local command probes, and official docs. [VERIFIED: vcpkg x-package-info; VERIFIED: CMakePresets.json; CITED: https://learn.microsoft.com/vcpkg/concepts/manifest-mode]
- Architecture: HIGH - Phase 11 locked the public API shape and strict reader relationship, and current code already exposes the result/error/metadata patterns needed. [VERIFIED: .planning/phases/11-compatibility-warnings-validation-api-and-hardening/11-CONTEXT.md; VERIFIED: include/libbsa/result.hpp; VERIFIED: include/libbsa/archive.hpp; VERIFIED: src/archive.cpp]
- Pitfalls: HIGH for codebase-specific pitfalls and MEDIUM for implementation-process pitfalls - manifest mapping and stale build path were verified locally; enum-catalog and sanitizer-default drift are assumptions flagged for planner attention. [VERIFIED: tests/unit/tes3_bsa_reader_tests.cpp; VERIFIED: tests/unit/tes4_bsa_reader_tests.cpp; VERIFIED: tests/unit/ba2_gnrl_reader_tests.cpp; VERIFIED: ctest --test-dir build/windows-msvc-debug-static -N output; ASSUMED]

**Research date:** 2026-05-10
**Valid until:** 2026-06-09 for repo-local architecture; 2026-05-17 for dependency/tool version freshness. [ASSUMED]
