# Codebase Structure

**Analysis Date:** 2026-05-12

## Directory Layout

```text
libbsa/
├── include/libbsa/        # Public C++20 headers exported to consumers
├── src/                   # Library implementation
│   ├── detail/            # Shared internal helpers and Windows-facing plumbing
│   ├── formats/bsa/       # TES3 and TES4-family BSA pipelines
│   ├── formats/ba2/       # BA2 GNRL and DX10 pipelines
│   └── texture/           # DDS metadata and layout helpers
├── tests/                 # Unit tests, fixture generators, consumer smoke tests
├── benchmarks/            # Maintainer benchmark executable
├── docs/                  # Public policy, compatibility, and API docs
├── cmake/                 # Package config templates
├── openspec/              # OpenSpec change/spec workflow artifacts
├── .claude/skills/        # Project-local OpenSpec automation skills
├── TES5Edit/              # Read-only reference submodule
├── CMakeLists.txt         # Root build definition
├── CMakePresets.json      # Supported Windows configure/build/test presets
├── vcpkg.json             # Dependency manifest
└── vcpkg-configuration.json # vcpkg baseline/registry configuration
```

## Directory Purposes

**`include/libbsa/`:**
- Purpose: Stable public library surface.
- Contains: Reader, writer, validation, result, version, and export headers.
- Key files: `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/libbsa.hpp`.

**`src/`:**
- Purpose: All shipped implementation code.
- Contains: Public API implementations, internal helpers, format families, and DDS-specific helpers.
- Key files: `src/archive.cpp`, `src/validation.cpp`, `src/libbsa.cpp`.

**`src/detail/`:**
- Purpose: Shared non-public implementation services.
- Contains: Path normalization, hashing, binary IO, codec routing, worker scheduling, payload streaming, disk-source helpers, and publish safety helpers.
- Key files: `src/detail/archive_path.cpp`, `src/detail/binary_io.cpp`, `src/detail/compression_router.cpp`, `src/detail/parallel_work.cpp`, `src/detail/writer_publish.cpp`.

**`src/formats/bsa/`:**
- Purpose: TES3 and TES4-family BSA-specific rules.
- Contains: Detector, parser, reader, prepare, layout, serialize, writer, constants, and internal headers for BSA families.
- Key files: `src/formats/bsa/bsa_format_detector.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`.

**`src/formats/ba2/`:**
- Purpose: Fallout 4 and Starfield BA2-specific rules.
- Contains: Detector, parser, reader, prepare, layout, serialize, writer, constants, and internal headers for GNRL and DX10 variants.
- Key files: `src/formats/ba2/ba2_format_detector.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`.

**`src/texture/`:**
- Purpose: Keep DDS and DirectXTex concerns out of public headers and non-texture formats.
- Contains: DirectXTex-backed metadata analysis and libbsa-native DDS layout/header logic.
- Key files: `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.cpp`.

**`tests/unit/`:**
- Purpose: Main Catch2 regression suite.
- Contains: API tests, parser tests, writer tests, codec tests, policy tests, and compatibility checks.
- Key files: `tests/unit/archive_reader_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/validation_api_tests.cpp`.

**`tests/fixtures/generated/`:**
- Purpose: Legal synthetic fixture corpus and generator sources.
- Contains: Fixture generator programs, generated archives/manifests, generated DDS sources, and validation scripts.
- Key files: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`, `tests/fixtures/generated/archives/`.

**`tests/package-consumer/`:**
- Purpose: Installed-package smoke test.
- Contains: A standalone CMake consumer project that links `libbsa::libbsa`.
- Key files: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/main.cpp`.

**`benchmarks/`:**
- Purpose: Maintainer-only benchmark/report tooling built from public APIs.
- Contains: Benchmark executable and usage docs.
- Key files: `benchmarks/libbsa_benchmarks.cpp`, `benchmarks/README.md`.

**`docs/`:**
- Purpose: Human-readable project policy and compatibility guidance.
- Contains: PRD, API docs input, format policy, compatibility evidence, and thread-safety guidance.
- Key files: `docs/PRD.md`, `docs/target-format-guide.md`, `docs/thread-safety.md`, `docs/compatibility-evidence.md`.

**`openspec/`:**
- Purpose: OpenSpec workflow state for planned and active changes.
- Contains: Change artifacts and synced specs.
- Key files: `openspec/changes/`, `openspec/specs/`.

**`.claude/skills/`:**
- Purpose: Project-local assistant automation skills.
- Contains: OpenSpec workflow skill indexes.
- Key files: `.claude/skills/openspec-apply-change/SKILL.md`, `.claude/skills/openspec-continue-change/SKILL.md`, `.claude/skills/openspec-explore/SKILL.md`.

**`TES5Edit/`:**
- Purpose: Read-only behavioral reference submodule.
- Contains: Upstream BSArchPro and xEdit reference material.
- Key files: `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`.

## Key File Locations

**Entry Points:**
- `include/libbsa/libbsa.hpp`: Aggregate include for consumers.
- `src/archive.cpp`: Public reader implementation and archive-family dispatch.
- `src/validation.cpp`: Public validation implementation.
- `src/formats/bsa/tes3_bsa_writer.cpp`: TES3 write-new facade.
- `src/formats/bsa/tes4_bsa_writer.cpp`: TES4-family BSA write-new facade.
- `src/formats/ba2/ba2_gnrl_writer.cpp`: BA2 GNRL write-new facade.
- `src/formats/ba2/ba2_dx10_writer.cpp`: BA2 DX10 write-new facade.

**Configuration:**
- `CMakeLists.txt`: Root target graph, install/export rules, benchmark and test toggles.
- `CMakePresets.json`: Supported Windows MSVC static/shared presets.
- `vcpkg.json`: Declared dependencies.
- `vcpkg-configuration.json`: vcpkg baseline/registry pinning.
- `cmake/libbsaConfig.cmake.in`: Installed package-config template.

**Core Logic:**
- `src/detail/archive_path.cpp`: Canonical archive virtual path policy.
- `src/detail/bethesda_hash.cpp`: TES3/TES4/BA2 hash helpers.
- `src/detail/compression_router.cpp`: Shared compression/decompression dispatch.
- `src/detail/payload_stream.cpp`: Shared payload read/write streaming utilities.
- `src/detail/writer_publish.cpp`: Shared writer publication safety layer.
- `src/texture/directxtex_analyzer.cpp`: DDS metadata/source analysis.

**Testing:**
- `tests/CMakeLists.txt`: Test targets and fixture-generation targets.
- `tests/unit/`: Catch2 regression suite.
- `tests/fixtures/README.md`: Fixture policy and provenance rules.
- `tests/package-consumer/`: Install/export smoke test.

## Naming Conventions

**Files:**
- Public headers use concise library nouns: `archive.hpp`, `writer.hpp`, `validation.hpp`.
- Internal implementation files use lowercase snake_case and include the format family in the filename: `tes4_bsa_parser.cpp`, `ba2_dx10_prepare.cpp`, `writer_publish.cpp`.
- Format pipelines use repeated stage suffixes: `*_parser.*`, `*_reader.*`, `*_prepare.*`, `*_layout.*`, `*_serialize.*`, `*_writer.*`.

**Directories:**
- Top-level runtime code is grouped by responsibility: `include/libbsa/`, `src/detail/`, `src/formats/`, `src/texture/`.
- Format families live one level deeper under `src/formats/bsa/` and `src/formats/ba2/`.
- Tests are split by purpose rather than mirroring source one-to-one: `tests/unit/`, `tests/fixtures/`, `tests/package-consumer/`.

## Where to Add New Code

**New public feature:**
- Primary code: add the public surface in `include/libbsa/` and implement it in the matching `src/` facade file such as `src/archive.cpp`, `src/validation.cpp`, or the relevant `src/formats/*/*_writer.cpp`.
- Tests: add Catch2 coverage in `tests/unit/` and use fixture generators under `tests/fixtures/generated/` when bytes-on-disk evidence is needed.

**New archive variant within an existing family:**
- Detection/parsing: add or extend files under `src/formats/bsa/` or `src/formats/ba2/`.
- Shared helpers: only add to `src/detail/` if the helper is truly format-agnostic.
- Public exposure: wire the new variant into `include/libbsa/archive.hpp` or `include/libbsa/writer.hpp` only after the internal pipeline exists.

**New component/module:**
- Implementation: keep format-specific code beside its owning family in `src/formats/bsa/` or `src/formats/ba2/`; keep cross-family services in `src/detail/`; keep DDS-specific code in `src/texture/`.

**Utilities:**
- Shared helpers: `src/detail/`.
- Test-only helpers or fixture generators: `tests/fixtures/generated/` or additional files under `tests/unit/`.

**New documentation or policy:**
- User/developer guidance: `docs/`.
- Planning/change workflow artifacts: `openspec/`.

## Special Directories

**`TES5Edit/`:**
- Purpose: Read-only compatibility reference.
- Generated: No.
- Committed: Yes, as a submodule/reference boundary.

**`tests/fixtures/generated/archives/`:**
- Purpose: Committed synthetic archive fixtures and manifests.
- Generated: Yes.
- Committed: Yes.

**`tests/fixtures/local/`:**
- Purpose: Ignored local game-derived fixture location.
- Generated: Mixed/local-only.
- Committed: No, except placeholder `.gitkeep`.

**`build/`:**
- Purpose: Preset-specific build trees.
- Generated: Yes.
- Committed: No.

**`vcpkg_installed/`:**
- Purpose: Local dependency install tree from vcpkg manifest mode.
- Generated: Yes.
- Committed: No.

**`.planning/codebase/`:**
- Purpose: GSD-generated codebase reference documents for later planning/execution commands.
- Generated: Yes.
- Committed: Yes, when project workflow captures planning artifacts.

**`.claude/skills/`:**
- Purpose: Project-local assistant skill indexes used by OpenSpec workflow helpers.
- Generated: No.
- Committed: Yes.

---

*Structure analysis: 2026-05-12*
