# Codebase Structure

**Analysis Date:** 2026-05-11

## Directory Layout

```text
libbsa/
├── include/libbsa/              # Stable public C++20 API headers
├── src/                         # Private implementation sources
│   ├── detail/                  # Shared private infrastructure and dependency adapters
│   ├── formats/                 # Archive-family implementations
│   │   ├── ba2/                 # BA2 GNRL/DX10 detection, parse, read, write modules
│   │   └── bsa/                 # TES3 and TES4-family BSA detection, parse, read, write modules
│   └── texture/                 # DDS layout and DirectXTex-backed analysis adapters
├── tests/                       # Catch2 tests, generated fixtures, package/export checks
│   ├── unit/                    # Unit, fixture, policy, parser, writer, reader tests
│   ├── fixtures/generated/      # Synthetic archive fixture generators, archives, manifests
│   ├── package-consumer/        # Installed package smoke tests
│   └── export-surface/          # DLL/shared export checks
├── docs/                        # Public docs, policy docs, Doxygen mainpage/config
├── benchmarks/                  # Synthetic benchmark executable and benchmark docs
├── cmake/                       # Installed package config templates
├── openspec/                    # Change/spec workflow artifacts
├── .planning/                   # GSD project planning and codebase map output
├── .claude/skills/              # Project-local OpenSpec skills
├── TES5Edit/                    # Read-only behavioral reference submodule
├── CMakeLists.txt               # Root build target, sources, install/export, tests, benchmarks
├── CMakePresets.json            # Windows MSVC configure/build/test presets
├── vcpkg.json                   # vcpkg manifest dependencies
├── vcpkg-configuration.json     # vcpkg baseline/configuration
├── README.md                    # Project overview and build instructions
├── AGENTS.md                    # Repository agent constraints and project rules
└── CLAUDE.md                    # Assistant-facing project context
```

## Directory Purposes

**`include/libbsa/`:**
- Purpose: Public API and ABI surface for consumers.
- Contains: Header-only public enums, value types, abstract sink/factory interfaces, reader/writer declarations, validation API, result/error model, version and export macros.
- Key files: `include/libbsa/libbsa.hpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`, `include/libbsa/export.hpp`, `include/libbsa/version.hpp`.
- Guidance: Add public API only here; keep headers C++20-compatible and dependency-light.

**`src/`:**
- Purpose: Private implementation for the `libbsa` library target.
- Contains: Public façade implementations (`src/archive.cpp`, `src/validation.cpp`), private format modules, private detail utilities, texture adapters, and minimal translation unit `src/libbsa.cpp`.
- Key files: `src/archive.cpp`, `src/validation.cpp`, `src/libbsa.cpp`.
- Guidance: Put implementation and private helper declarations here; public consumers must not include `src/` headers.

**`src/detail/`:**
- Purpose: Shared internal mechanisms used across archive families.
- Contains: Archive path normalization, binary readers/writers, byte vector allocation helpers, parser primitives, Bethesda hashes, payload stream helpers, compression router/codecs, parallel work scheduler, atomic file operations, writer publish helpers.
- Key files: `src/detail/archive_path.*`, `src/detail/binary_io.*`, `src/detail/parser_primitives.*`, `src/detail/bethesda_hash.*`, `src/detail/compression_router.*`, `src/detail/deflate_codec.*`, `src/detail/lz4_frame_codec.*`, `src/detail/lz4_block_codec.*`, `src/detail/parallel_work.*`, `src/detail/writer_publish.*`, `src/detail/atomic_file_ops.hpp`, `src/detail/payload_stream.*`, `src/detail/byte_vector.hpp`.
- Guidance: Add reusable private helpers here only when they serve multiple format modules or express a cross-cutting internal policy.

**`src/formats/bsa/`:**
- Purpose: BSA-family archive detection, parsing, reading, and writing.
- Contains: Format detector plus TES3 and TES4-family parser/reader/writer modules. Writer modules are split into public façade, prepare, layout, and serialize files.
- Key files: `src/formats/bsa/bsa_format_detector.*`, `src/formats/bsa/tes3_bsa_parser.*`, `src/formats/bsa/tes3_bsa_reader.*`, `src/formats/bsa/tes3_bsa_writer.*`, `src/formats/bsa/tes3_bsa_prepare.*`, `src/formats/bsa/tes3_bsa_layout.*`, `src/formats/bsa/tes3_bsa_serialize.*`, `src/formats/bsa/tes4_bsa_parser.*`, `src/formats/bsa/tes4_bsa_reader.*`, `src/formats/bsa/tes4_bsa_writer.*`, `src/formats/bsa/tes4_bsa_prepare.*`, `src/formats/bsa/tes4_bsa_layout.*`, `src/formats/bsa/tes4_bsa_serialize.*`.
- Guidance: Add new BSA variant behavior under this directory and route it from `src/archive.cpp` or BSA writer façades.

**`src/formats/ba2/`:**
- Purpose: BA2-family archive detection, parsing, reading, and writing.
- Contains: Shared BA2 detector plus GNRL and DX10 parser/reader/writer modules. Writer modules are split into public façade, prepare, layout, and serialize files.
- Key files: `src/formats/ba2/ba2_format_detector.*`, `src/formats/ba2/ba2_gnrl_parser.*`, `src/formats/ba2/ba2_gnrl_reader.*`, `src/formats/ba2/ba2_gnrl_writer.*`, `src/formats/ba2/ba2_gnrl_prepare.*`, `src/formats/ba2/ba2_gnrl_layout.*`, `src/formats/ba2/ba2_gnrl_serialize.*`, `src/formats/ba2/ba2_dx10_parser.*`, `src/formats/ba2/ba2_dx10_reader.*`, `src/formats/ba2/ba2_dx10_writer.*`, `src/formats/ba2/ba2_dx10_prepare.*`, `src/formats/ba2/ba2_dx10_layout.*`, `src/formats/ba2/ba2_dx10_serialize.*`.
- Guidance: Add GNRL-specific functionality in `ba2_gnrl_*`, texture-specific functionality in `ba2_dx10_*`, and cross-subtype detection in `ba2_format_detector.*`.

**`src/texture/`:**
- Purpose: DDS metadata analysis, DDS DXT10 header reconstruction, and BA2 texture chunk planning.
- Contains: DirectXTex adapter and independent DDS layout helper.
- Key files: `src/texture/directxtex_analyzer.*`, `src/texture/dds_layout.*`.
- Guidance: Put DirectXTex calls only in `src/texture/directxtex_analyzer.cpp`; use libbsa-native metadata types at module boundaries.

**`tests/unit/`:**
- Purpose: Catch2 coverage for public API behavior, format parsers/readers/writers, private detail helpers, policy documents, package/export expectations, and compatibility matrix behavior.
- Contains: One or more focused `*_tests.cpp` files per feature area.
- Key files: `tests/unit/archive_reader_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/compression_router_tests.cpp`, `tests/unit/binary_io_tests.cpp`, `tests/unit/parser_primitives_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`, `tests/unit/export_surface_policy_tests.cpp`.
- Guidance: Add tests near related existing test files; use generated fixtures in `tests/fixtures/generated/archives/` and helper manifests when testing archive behavior.

**`tests/fixtures/generated/`:**
- Purpose: Generate legal synthetic fixtures and manifest data for parser, writer, extraction, malformed archive, and compatibility tests.
- Contains: C++ fixture generator tools under `tests/fixtures/generated/*.cpp`, generated archives/manifests under `tests/fixtures/generated/archives/`, and source manifests under `tests/fixtures/generated/source/`.
- Key files: `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`, `tests/fixtures/generated/compatibility_matrix.json`.
- Guidance: Generate or update fixtures through CMake custom targets in `tests/CMakeLists.txt`; do not use `TES5Edit/` as a mutable fixture workspace.

**`tests/package-consumer/`:**
- Purpose: Verify installed package consumption and runtime DLL copy behavior.
- Contains: Consumer CMake project and smoke scripts.
- Key files: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/main.cpp`, `tests/package-consumer/smoke.cmake`, `tests/package-consumer/copy-runtime-dlls.cmake`, `tests/package-consumer/verify-runtime-dll-copy.cmake`.
- Guidance: Add install/export consumption checks here, not in unit tests.

**`tests/export-surface/`:**
- Purpose: Validate shared-library export surface policy.
- Contains: CMake script checks for DLL/export behavior.
- Key files: `tests/export-surface/check-dll-exports.cmake`.
- Guidance: Add ABI/export policy checks here when public symbol rules change.

**`docs/`:**
- Purpose: Consumer and maintainer documentation for API, formats, compatibility, thread safety, integration examples, and Doxygen.
- Contains: Markdown documentation and Doxygen config/mainpage.
- Key files: `docs/thread-safety.md`, `docs/target-format-guide.md`, `docs/compatibility-evidence.md`, `docs/integration-examples.md`, `docs/api-mainpage.md`, `docs/PRD.md`, `docs/Doxyfile.in`.
- Guidance: Keep public behavior docs synchronized with public headers and tests.

**`benchmarks/`:**
- Purpose: Synthetic benchmark executable and benchmark documentation.
- Contains: Benchmark source and README.
- Key files: `benchmarks/libbsa_benchmarks.cpp`, `benchmarks/README.md`.
- Guidance: Keep benchmarks as maintainer tooling; do not make benchmark reports part of runtime behavior.

**`cmake/`:**
- Purpose: CMake package configuration templates for install/export support.
- Contains: Package config template.
- Key files: `cmake/libbsaConfig.cmake.in`.
- Guidance: Update when installed target names, transitive public dependencies, or package config behavior changes.

**`openspec/`:**
- Purpose: OpenSpec workflow state, specs, active changes, and archived changes.
- Contains: `openspec/config.yaml`, active changes under `openspec/changes/`, archive under `openspec/changes/archive/`, canonical specs under `openspec/specs/`.
- Key files: `openspec/specs/archive-writer-layering/spec.md`, `openspec/specs/writer-safe-publish/spec.md`, `openspec/specs/shared-parser-primitives/spec.md`, `openspec/changes/add-explicit-public-export-macro/tasks.md`.
- Guidance: Use OpenSpec artifacts for planned behavior changes; implementation still belongs in `include/`, `src/`, `tests/`, and `docs/`.

**`.planning/`:**
- Purpose: GSD project state, roadmap, research, milestone planning, quick plans, and codebase maps.
- Contains: Planning docs and generated codebase analysis documents.
- Key files: `.planning/PROJECT.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`, `.planning/research/STACK.md`, `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/STRUCTURE.md`.
- Guidance: GSD commands manage this directory; code implementation should not depend on `.planning/` at runtime or build time.

**`.claude/skills/`:**
- Purpose: Project-local OpenSpec skill instructions used by assistants.
- Contains: OpenSpec workflow skill indexes.
- Key files: `.claude/skills/openspec-propose/SKILL.md`, `.claude/skills/openspec-apply-change/SKILL.md`, `.claude/skills/openspec-verify-change/SKILL.md`, `.claude/skills/openspec-archive-change/SKILL.md`.
- Guidance: These skills define workflow conventions, not library runtime architecture.

**`TES5Edit/`:**
- Purpose: Read-only behavioral reference submodule for BSArchPro-compatible archive behavior.
- Contains: Delphi/Pascal reference code such as `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, and `TES5Edit/Core/wbBSA.pas`.
- Key files: `TES5Edit/BSArchPro.dpr`, `TES5Edit/Core/wbBSArchive.pas`, `TES5Edit/Core/wbBSA.pas`.
- Guidance: Never modify, format, stage, compile, or use as writable fixture source.

## Key File Locations

**Entry Points:**
- `include/libbsa/libbsa.hpp`: Public umbrella include.
- `include/libbsa/archive.hpp`: Public archive reader and extraction entry point declarations.
- `include/libbsa/writer.hpp`: Public archive writer entry point declarations.
- `include/libbsa/validation.hpp`: Public validation entry point declaration.
- `src/archive.cpp`: `archive_reader` implementation and read/extract dispatch.
- `src/validation.cpp`: `validate_archive` implementation.
- `CMakeLists.txt`: Main build target, source list, install/export, tests, benchmarks, and docs target.

**Configuration:**
- `CMakeLists.txt`: Root project configuration and library source list.
- `CMakePresets.json`: Windows MSVC debug static/shared presets.
- `tests/CMakeLists.txt`: Test executable, fixture generator targets, and fixture custom targets.
- `vcpkg.json`: vcpkg manifest dependency declaration.
- `vcpkg-configuration.json`: vcpkg baseline/configuration.
- `cmake/libbsaConfig.cmake.in`: Installed package config template.
- `openspec/config.yaml`: OpenSpec configuration.

**Core Logic:**
- `src/formats/bsa/bsa_format_detector.*`: BSA family/version detection.
- `src/formats/bsa/tes3_bsa_parser.*`: TES3 BSA parsing and metadata materialization.
- `src/formats/bsa/tes4_bsa_parser.*`: TES4-family BSA parsing and metadata materialization.
- `src/formats/ba2/ba2_format_detector.*`: BA2 subtype/version detection.
- `src/formats/ba2/ba2_gnrl_parser.*`: BA2 GNRL parsing and metadata materialization.
- `src/formats/ba2/ba2_dx10_parser.*`: BA2 DX10 parsing, chunk metadata, and texture metadata materialization.
- `src/formats/*/*_reader.*`: Listing, lookup, contains, and extraction for each format/subtype.
- `src/formats/*/*_prepare.*`: Writer validation, path normalization, hash/materialization, compression, and target-specific preparation.
- `src/formats/*/*_layout.*`: Writer offset assignment and optional dedupe layout.
- `src/formats/*/*_serialize.*`: Archive byte serialization and payload streaming.
- `src/detail/compression_router.*`: Internal compression/decompression dispatch.
- `src/detail/archive_path.*`: Archive-internal path normalization.
- `src/detail/binary_io.*`: Little-endian binary reader/writer.
- `src/detail/parser_primitives.*`: Safe parser arithmetic and bounded file reads.
- `src/texture/directxtex_analyzer.*`: DirectXTex-backed DDS analysis.
- `src/texture/dds_layout.*`: DDS layout, mip/chunk planning, and header reconstruction.

**Testing:**
- `tests/CMakeLists.txt`: Test build and fixture-generation orchestration.
- `tests/unit/*_tests.cpp`: Catch2 unit and behavior tests.
- `tests/fixtures/generated/*.cpp`: Fixture generator tool sources.
- `tests/fixtures/generated/archives/*.json`: Fixture manifests used by tests.
- `tests/fixtures/generated/archives/*.bsa`: Generated BSA fixtures.
- `tests/fixtures/generated/archives/*.ba2`: Generated BA2 fixtures.
- `tests/package-consumer/`: Install/package consumer checks.
- `tests/export-surface/check-dll-exports.cmake`: Export surface policy check.

**Documentation:**
- `README.md`: Project overview, Windows support, and build commands.
- `docs/thread-safety.md`: Threading/ownership contract for public types.
- `docs/target-format-guide.md`: Supported target formats and compression routing policy.
- `docs/compatibility-evidence.md`: Compatibility warning and behavior evidence.
- `docs/integration-examples.md`: Consumer integration examples.
- `docs/api-mainpage.md`: Doxygen main page content.
- `docs/PRD.md`: Product requirements.
- `AGENTS.md`: Repository implementation constraints.

**Reference:**
- `TES5Edit/BSArchPro.dpr`: BSArchPro reference entry.
- `TES5Edit/BSArch/`: BSArchPro-related reference code.
- `TES5Edit/Core/wbBSArchive.pas`: Archive behavior reference.
- `TES5Edit/Core/wbBSA.pas`: BSA behavior reference.

## Naming Conventions

**Files:**
- Public headers use lowercase nouns under `include/libbsa/`: `archive.hpp`, `writer.hpp`, `validation.hpp`, `result.hpp`.
- Private shared helpers use lowercase snake_case under `src/detail/`: `archive_path.cpp`, `binary_io.hpp`, `compression_router.cpp`, `writer_publish.cpp`.
- Format modules use `<format>_<role>.*` naming: `tes4_bsa_parser.cpp`, `tes4_bsa_reader.hpp`, `tes4_bsa_prepare.cpp`, `tes4_bsa_layout.hpp`, `tes4_bsa_serialize.cpp`, `tes4_bsa_writer.cpp`.
- BA2 subtype modules include subtype in the prefix: `ba2_gnrl_parser.cpp`, `ba2_dx10_reader.cpp`, `ba2_dx10_writer.hpp`.
- Tests use `<subject>_tests.cpp`: `archive_reader_tests.cpp`, `ba2_dx10_writer_tests.cpp`, `compression_router_tests.cpp`.
- Fixture generators use `generate_<fixture_family>_fixtures.cpp`: `generate_tes4_bsa_fixtures.cpp`, `generate_ba2_dx10_fixtures.cpp`.

**Directories:**
- Public include namespace is mirrored by `include/libbsa/`.
- Private implementation namespace groupings map to `src/detail/`, `src/formats/bsa/`, `src/formats/ba2/`, and `src/texture/`.
- Tests are grouped by role: `tests/unit/`, `tests/fixtures/generated/`, `tests/package-consumer/`, and `tests/export-surface/`.
- Documentation lives under `docs/`; planning/workflow state lives under `.planning/` and `openspec/`.

## Where to Add New Code

**New Public Reader Feature:**
- Primary public API: `include/libbsa/archive.hpp`
- Facade implementation: `src/archive.cpp`
- Format-specific parsing/lookup/extraction: `src/formats/bsa/` or `src/formats/ba2/`
- Shared helpers: `src/detail/`
- Tests: `tests/unit/archive_reader_tests.cpp` plus format-specific tests such as `tests/unit/tes4_bsa_reader_tests.cpp` or `tests/unit/ba2_dx10_extraction_tests.cpp`
- Documentation: `docs/target-format-guide.md`, `docs/thread-safety.md`, or `docs/integration-examples.md` as applicable.

**New Public Writer Feature:**
- Primary public API: `include/libbsa/writer.hpp`
- Public writer façade: corresponding `src/formats/*/*_writer.cpp`
- Validation/materialization: corresponding `src/formats/*/*_prepare.*`
- Offset/dedupe layout: corresponding `src/formats/*/*_layout.*`
- Byte serialization: corresponding `src/formats/*/*_serialize.*`
- Publish logic reuse: `src/detail/writer_publish.*`
- Tests: writer-focused files in `tests/unit/` such as `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, or `tests/unit/ba2_dx10_writer_tests.cpp`

**New Archive Format Variant:**
- Detection: `src/formats/bsa/bsa_format_detector.*` or `src/formats/ba2/ba2_format_detector.*`
- Parser: add or extend `src/formats/<family>/<variant>_parser.*`
- Reader: add or extend `src/formats/<family>/<variant>_reader.*`
- Writer: add/extend `*_writer.*`, `*_prepare.*`, `*_layout.*`, and `*_serialize.*` for the relevant family.
- Public target enums/metadata: `include/libbsa/archive.hpp` and/or `include/libbsa/writer.hpp`
- Dispatch: `src/archive.cpp`
- Tests and fixtures: `tests/unit/` and `tests/fixtures/generated/`

**New Shared Utility:**
- Primary code: `src/detail/<utility_name>.hpp` and `src/detail/<utility_name>.cpp`
- CMake source registration: `CMakeLists.txt` in `libbsa_library_sources`
- Tests: `tests/unit/<utility_name>_tests.cpp` and `tests/CMakeLists.txt`
- Use when: The utility serves multiple format modules or enforces an internal architectural policy.

**New Texture/DDS Behavior:**
- DirectXTex-backed metadata/source analysis: `src/texture/directxtex_analyzer.*`
- DDS math/header/chunk planning independent of DirectXTex: `src/texture/dds_layout.*`
- BA2 DX10 integration: `src/formats/ba2/ba2_dx10_*`
- Tests: `tests/unit/dds_layout_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`

**New Validation Rule:**
- Public warning/error shape: `include/libbsa/validation.hpp`
- Implementation: `src/validation.cpp`
- Evidence docs: `docs/compatibility-evidence.md` and `docs/target-format-guide.md`
- Tests: `tests/unit/validation_api_tests.cpp`, `tests/unit/validation_policy_tests.cpp`, `tests/unit/compatibility_warning_tests.cpp`

**New Test Fixture:**
- Generator source: `tests/fixtures/generated/generate_<family>_fixtures.cpp`
- Archive/manifests: `tests/fixtures/generated/archives/`
- CMake custom target and byproducts: `tests/CMakeLists.txt`
- Tests consuming fixture: `tests/unit/<subject>_tests.cpp`

**New Package/Export Behavior:**
- Build config: `CMakeLists.txt` and `cmake/libbsaConfig.cmake.in`
- Consumer smoke test: `tests/package-consumer/`
- Export surface check: `tests/export-surface/check-dll-exports.cmake`
- Public macro/API: `include/libbsa/export.hpp` and public headers requiring `LIBBSA_API`.

**New Documentation:**
- API docs: Doxygen comments in `include/libbsa/*.hpp` plus `docs/api-mainpage.md`.
- Consumer guide: `docs/integration-examples.md`.
- Policy/format docs: `docs/thread-safety.md`, `docs/target-format-guide.md`, `docs/compatibility-evidence.md`.
- Project scope docs: `README.md`, `docs/PRD.md`, `AGENTS.md`.

## Special Directories

**`TES5Edit/`:**
- Purpose: Behavioral reference for BSArchPro-compatible behavior.
- Generated: No.
- Committed: Yes, as a read-only submodule/reference.
- Rule: Never modify, format, stage, compile, or use as a writable test fixture.

**`vcpkg_installed/`:**
- Purpose: Local vcpkg build/install output for dependencies such as DirectXTex.
- Generated: Yes.
- Committed: No for normal source changes; treat as build output.
- Rule: Do not document or depend on generated absolute paths from this directory in source code.

**`build*/` / CMake build trees:**
- Purpose: CMake configure/build/test outputs.
- Generated: Yes.
- Committed: No.
- Rule: Do not place source, fixtures, docs, or planning artifacts under build output directories.

**`tests/fixtures/generated/archives/`:**
- Purpose: Synthetic archive fixtures and JSON manifests for tests.
- Generated: Yes, by fixture generator targets in `tests/CMakeLists.txt`.
- Committed: Yes for stable generated fixtures/manifests that tests consume.
- Rule: Keep fixtures legal and synthetic; update generator source and byproduct lists with fixture changes.

**`openspec/changes/archive/`:**
- Purpose: Archived OpenSpec changes and their completed artifacts.
- Generated: Workflow-managed.
- Committed: Yes when workflow artifacts are part of project history.
- Rule: Do not treat archived change artifacts as implementation source; use `openspec/specs/` and active change artifacts for planning context.

**`.planning/codebase/`:**
- Purpose: Generated codebase map documents for GSD planning/execution.
- Generated: Yes.
- Committed: Workflow-dependent.
- Rule: Documents describe current state and guide future agents; code must not include or depend on them.

**`.claude/skills/`, `.cursor/skills/`, `.codex/skills/`, `.windsurf/skills/`, `.agent/skills/`:**
- Purpose: Assistant workflow skill instructions, primarily OpenSpec in this repository.
- Generated: Tool/workflow-managed.
- Committed: Yes when project-local workflow support is intended.
- Rule: Skill files guide agent behavior only; do not compile or include them in libbsa.

---

*Structure analysis: 2026-05-11*
