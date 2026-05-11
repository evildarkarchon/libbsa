# Codebase Structure

**Analysis Date:** 2026-05-11

## Directory Layout

```text
J:\libbsa/
├── AGENTS.md                         # Project constraints and repo-specific agent rules
├── CMakeLists.txt                    # Main library, docs, benchmark, install/export, and test entry point
├── CMakePresets.json                 # Supported Windows MSVC static/shared debug presets
├── README.md                         # Project overview, Windows support, build commands, TES5Edit boundary
├── vcpkg.json                        # Manifest-mode dependency list
├── vcpkg-configuration.json          # vcpkg registry baseline
├── cmake/
│   └── libbsaConfig.cmake.in         # Installed package config template
├── include/
│   └── libbsa/                       # Public C++20 headers
├── src/
│   ├── archive.cpp                   # Public archive_reader implementation and reader dispatch
│   ├── validation.cpp                # Public validate_archive implementation
│   ├── libbsa.cpp                    # Minimal translation unit for package/library target
│   ├── detail/                       # Private shared helpers
│   ├── formats/
│   │   ├── bsa/                      # TES3 and TES4-family BSA internals
│   │   └── ba2/                      # BA2 GNRL and BA2 DX10 internals
│   └── texture/                      # DDS/DirectXTex private boundary
├── tests/
│   ├── CMakeLists.txt                # Test executable, fixture generators, CTest registration
│   ├── unit/                         # Catch2 unit, fixture, policy, and round-trip tests
│   ├── fixtures/                     # Generated and local fixture corpus
│   └── package-consumer/             # Installed-package smoke project
├── benchmarks/
│   ├── README.md                     # Benchmark policy and report target instructions
│   └── libbsa_benchmarks.cpp         # Synthetic benchmark report generator
├── docs/                             # Public docs, policy docs, API/Doxygen files
├── openspec/                         # OpenSpec workflow metadata and active/archive changes
├── .planning/                        # GSD planning, milestone, research, and codebase maps
├── .codex/skills/                    # Project-local OpenSpec skill indexes for Codex
└── TES5Edit/                         # Read-only reference submodule; never edit, build, stage, or vendor
```

## Directory Purposes

**Root:**
- Purpose: Holds build entry points, dependency manifests, project rules, and top-level documentation.
- Contains: `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `vcpkg-configuration.json`, `README.md`, `AGENTS.md`.
- Key files: `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `AGENTS.md`.

**`include/libbsa/`:**
- Purpose: Stable public C++20 API surface for consumers.
- Contains: Public reader, writer, validation, result, umbrella, and version headers.
- Key files: `include/libbsa/libbsa.hpp`, `include/libbsa/archive.hpp`, `include/libbsa/writer.hpp`, `include/libbsa/validation.hpp`, `include/libbsa/result.hpp`, `include/libbsa/version.hpp`.

**`src/`:**
- Purpose: Library implementation and private source tree.
- Contains: Public method implementations, reader dispatch, validation implementation, format modules, shared detail helpers, texture adapters.
- Key files: `src/archive.cpp`, `src/validation.cpp`, `src/libbsa.cpp`.

**`src/detail/`:**
- Purpose: Private reusable implementation helpers with no public API exposure.
- Contains: Binary little-endian I/O, archive-path normalization, Bethesda hashing, codec wrappers, compression routing, byte-vector allocation guards, payload streaming, worker scheduling, atomic publish helpers.
- Key files: `src/detail/binary_io.hpp`, `src/detail/binary_io.cpp`, `src/detail/archive_path.hpp`, `src/detail/archive_path.cpp`, `src/detail/bethesda_hash.hpp`, `src/detail/bethesda_hash.cpp`, `src/detail/compression_router.hpp`, `src/detail/compression_router.cpp`, `src/detail/deflate_codec.cpp`, `src/detail/lz4_frame_codec.cpp`, `src/detail/lz4_block_codec.cpp`, `src/detail/parallel_work.cpp`, `src/detail/payload_stream.cpp`, `src/detail/atomic_file_ops.hpp`.

**`src/formats/bsa/`:**
- Purpose: Private BSA-family implementation modules.
- Contains: BSA detector, TES3 parser/reader/writer, TES4-family parser/reader/writer.
- Key files: `src/formats/bsa/bsa_format_detector.hpp`, `src/formats/bsa/bsa_format_detector.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/bsa/tes3_bsa_reader.cpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/bsa/tes4_bsa_reader.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`.

**`src/formats/ba2/`:**
- Purpose: Private BA2-family implementation modules.
- Contains: BA2 detector, GNRL parser/reader/writer, DX10 parser/reader/writer, publish rollback helpers.
- Key files: `src/formats/ba2/ba2_format_detector.cpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`, `src/formats/ba2/ba2_publish.hpp`.

**`src/texture/`:**
- Purpose: Private DirectXTex and DDS layout boundary for BA2 DX10 support.
- Contains: DDS metadata/source analysis, DXT10 header generation, mip sizing, chunk planning, chunk order validation.
- Key files: `src/texture/directxtex_analyzer.hpp`, `src/texture/directxtex_analyzer.cpp`, `src/texture/dds_layout.hpp`, `src/texture/dds_layout.cpp`.

**`tests/unit/`:**
- Purpose: Catch2 tests for public APIs, private helpers, format parsers/readers/writers, fixtures, policies, and package boundaries.
- Contains: One `*_tests.cpp` file per behavior area.
- Key files: `tests/unit/archive_reader_tests.cpp`, `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/validation_api_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.

**`tests/fixtures/`:**
- Purpose: Legal fixture corpus and fixture generation tools.
- Contains: Generated `.bsa`, `.ba2`, `.json`, and DDS source fixtures; local uncommitted fixture placeholder; fixture documentation and manifest validator.
- Key files: `tests/fixtures/README.md`, `tests/fixtures/generated/validate_fixture_manifests.py`, `tests/fixtures/generated/compatibility_matrix.json`, `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.

**`tests/package-consumer/`:**
- Purpose: Verify installed CMake package and runtime DLL copy behavior from an external consumer project.
- Contains: Consumer `CMakeLists.txt`, smoke script, runtime DLL copy helper, and verification script.
- Key files: `tests/package-consumer/CMakeLists.txt`, `tests/package-consumer/main.cpp`, `tests/package-consumer/smoke.cmake`, `tests/package-consumer/copy-runtime-dlls.cmake`, `tests/package-consumer/verify-runtime-dll-copy.cmake`.

**`docs/`:**
- Purpose: Project and public API documentation.
- Contains: PRD, Doxygen configuration, API mainpage, thread-safety policy, target-format guide, compatibility evidence, integration examples.
- Key files: `docs/PRD.md`, `docs/Doxyfile.in`, `docs/api-mainpage.md`, `docs/thread-safety.md`, `docs/target-format-guide.md`, `docs/compatibility-evidence.md`, `docs/integration-examples.md`.

**`benchmarks/`:**
- Purpose: Maintainer benchmark tooling with report-only performance data and correctness verification through public APIs.
- Contains: Benchmark runner and usage/policy documentation.
- Key files: `benchmarks/libbsa_benchmarks.cpp`, `benchmarks/README.md`.

**`cmake/`:**
- Purpose: CMake package configuration templates.
- Contains: Installed config template.
- Key files: `cmake/libbsaConfig.cmake.in`.

**`openspec/`:**
- Purpose: OpenSpec workflow configuration and change artifacts.
- Contains: `openspec/config.yaml` and change/spec directories.
- Key files: `openspec/config.yaml`.

**`.planning/`:**
- Purpose: GSD planning state, milestone artifacts, research, quick notes, and generated codebase maps.
- Contains: `codebase/`, `milestones/`, `phases/`, `quick/`, `research/`.
- Key files: `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/STRUCTURE.md`, `.planning/codebase/STACK.md`, `.planning/codebase/TESTING.md`.

**`.codex/skills/`:**
- Purpose: Project-local OpenSpec workflow skill indexes for Codex.
- Contains: OpenSpec skill directories with `SKILL.md` files.
- Key files: `.codex/skills/openspec-apply-change/SKILL.md`, `.codex/skills/openspec-verify-change/SKILL.md`, `.codex/skills/openspec-archive-change/SKILL.md`.

**`TES5Edit/`:**
- Purpose: Read-only reference submodule for BSArchPro-compatible behavior.
- Contains: Delphi/Pascal reference implementation and related project files.
- Key files: `TES5Edit/BSArchPro.dpr`, `TES5Edit/BSArch/`, `TES5Edit/Core/wbBSArchive.pas`, `TES5Edit/Core/wbBSA.pas`.

## Key File Locations

**Entry Points:**
- `include/libbsa/libbsa.hpp`: Umbrella public include for consumers.
- `include/libbsa/archive.hpp`: Public archive reader, metadata, extraction sink, and bulk extraction API.
- `include/libbsa/writer.hpp`: Public write-new APIs for TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10.
- `include/libbsa/validation.hpp`: Public validation report and warning API.
- `src/archive.cpp`: `archive_reader` implementation and format dispatch.
- `src/validation.cpp`: `validate_archive` implementation.
- `CMakeLists.txt`: Build-system entry point for library, docs, benchmarks, install/export, and tests.

**Configuration:**
- `CMakePresets.json`: Windows MSVC Debug static/shared configure, build, and test presets.
- `vcpkg.json`: Manifest-mode dependencies: `libdeflate`, `lz4`, `directxtex`, `catch2`, `nlohmann-json`.
- `vcpkg-configuration.json`: vcpkg registry baseline.
- `cmake/libbsaConfig.cmake.in`: Installed package config template.
- `docs/Doxyfile.in`: Doxygen config template for public API docs.
- `.clangd`: clangd configuration.

**Core Logic:**
- `src/formats/bsa/bsa_format_detector.cpp`: BSA byte classifier.
- `src/formats/bsa/tes3_bsa_parser.cpp`: TES3 BSA metadata parser.
- `src/formats/bsa/tes3_bsa_reader.cpp`: TES3 BSA lookup/extraction.
- `src/formats/bsa/tes3_bsa_writer.cpp`: TES3 BSA write-new implementation.
- `src/formats/bsa/tes4_bsa_parser.cpp`: TES4-family BSA metadata parser.
- `src/formats/bsa/tes4_bsa_reader.cpp`: TES4-family BSA lookup/extraction.
- `src/formats/bsa/tes4_bsa_writer.cpp`: TES4-family BSA write-new implementation.
- `src/formats/ba2/ba2_format_detector.cpp`: BA2 byte classifier.
- `src/formats/ba2/ba2_gnrl_parser.cpp`: BA2 GNRL metadata parser.
- `src/formats/ba2/ba2_gnrl_reader.cpp`: BA2 GNRL lookup/extraction.
- `src/formats/ba2/ba2_gnrl_writer.cpp`: BA2 GNRL write-new implementation.
- `src/formats/ba2/ba2_dx10_parser.cpp`: BA2 DX10 texture metadata parser.
- `src/formats/ba2/ba2_dx10_reader.cpp`: BA2 DX10 reconstructed DDS extraction.
- `src/formats/ba2/ba2_dx10_writer.cpp`: BA2 DX10 write-new implementation.
- `src/detail/compression_router.cpp`: Explicit codec dispatch.
- `src/texture/directxtex_analyzer.cpp`: DirectXTex adapter.
- `src/texture/dds_layout.cpp`: DXT10 header/chunk layout rules.

**Testing:**
- `tests/CMakeLists.txt`: Test target, fixture generators, CTest tests, package-consumer tests.
- `tests/unit/*_tests.cpp`: Catch2 tests.
- `tests/fixtures/generated/*.cpp`: Fixture generator tools.
- `tests/fixtures/generated/archives/`: Generated `.bsa`, `.ba2`, and manifest files.
- `tests/fixtures/generated/source/`: Generated DDS source fixtures.
- `tests/fixtures/local/`: Local-only fixture placeholder.
- `tests/package-consumer/`: External consumer/package smoke tests.

**Documentation:**
- `README.md`: Build and boundary summary.
- `AGENTS.md`: Authoritative repo rules for agents.
- `docs/thread-safety.md`: Public thread-safety policy.
- `docs/target-format-guide.md`: Supported target behavior guide.
- `docs/compatibility-evidence.md`: Compatibility evidence documentation.
- `docs/integration-examples.md`: Consumer examples.
- `docs/api-mainpage.md`: Doxygen mainpage content.

**Workflow/Planning:**
- `openspec/config.yaml`: OpenSpec configuration.
- `.planning/codebase/`: Generated codebase maps.
- `.planning/milestones/`, `.planning/phases/`: GSD milestone and phase artifacts.
- `.codex/skills/`: Project-local OpenSpec skill indexes.

## Naming Conventions

**Files:**
- Public headers use concise lowercase nouns: `include/libbsa/archive.hpp`, `include/libbsa/result.hpp`, `include/libbsa/writer.hpp`.
- Format implementation files use `<format>_<role>.hpp/.cpp`: `src/formats/bsa/tes4_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`.
- Format detector files use `<family>_format_detector.hpp/.cpp`: `src/formats/bsa/bsa_format_detector.cpp`, `src/formats/ba2/ba2_format_detector.cpp`.
- Internal helper files use lowercase snake_case nouns: `src/detail/binary_io.cpp`, `src/detail/archive_path.cpp`, `src/detail/payload_stream.cpp`.
- Texture helper files use lowercase snake_case: `src/texture/dds_layout.cpp`, `src/texture/directxtex_analyzer.cpp`.
- Tests use `*_tests.cpp`: `tests/unit/ba2_dx10_writer_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp`.
- Fixture generators use `generate_*_fixtures.cpp`: `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`.
- Generated archive manifests use target/variant names plus `_manifest.json`: `tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json`.

**Directories:**
- Public include namespace path mirrors the library namespace: `include/libbsa/`.
- Private format directories group by archive family: `src/formats/bsa/`, `src/formats/ba2/`.
- Private utility directories group by concern: `src/detail/`, `src/texture/`.
- Test directories group by role: `tests/unit/`, `tests/fixtures/`, `tests/package-consumer/`.
- Planning/workflow directories stay under `.planning/`, `openspec/`, and tool-specific skill directories such as `.codex/skills/`.

**C++ Symbols:**
- Public types and functions use lowercase snake_case: `archive_reader`, `entry_metadata`, `validate_archive`, `write_execution_options`.
- Public enum classes use lowercase snake_case enumerators: `archive_type::bsa`, `entry_compression::lz4_block`, `error_code::format_error`.
- Private helper functions use lowercase snake_case: `normalize_archive_path`, `detect_ba2_format`, `parse_tes4_bsa_archive_file`.
- Private raw record structs use lowercase snake_case names: `detected_bsa_format`, `ba2_dx10_archive`, `tes4_writer_entry`.

## Where to Add New Code

**New Public Reader Metadata or API:**
- Primary code: `include/libbsa/archive.hpp`
- Implementation: `src/archive.cpp` or the relevant `src/formats/*/*reader.cpp`
- Tests: `tests/unit/archive_reader_tests.cpp` plus format-specific tests under `tests/unit/`
- Build registration: Add new `.cpp` files to `target_sources(libbsa PRIVATE ...)` in `CMakeLists.txt`.

**New Public Writer Capability:**
- Primary code: `include/libbsa/writer.hpp`
- BSA implementation: `src/formats/bsa/`
- BA2 implementation: `src/formats/ba2/`
- Tests: `tests/unit/*writer_tests.cpp`, `tests/unit/*writer_execution_tests.cpp`
- Package/API boundary tests: `tests/unit/public_include_boundary_tests.cpp`, `tests/package-consumer/main.cpp` when public API shape changes.

**New BSA Variant or Rule:**
- Detector changes: `src/formats/bsa/bsa_format_detector.cpp`
- Parser changes: `src/formats/bsa/*_parser.cpp`
- Extraction changes: `src/formats/bsa/*_reader.cpp`
- Writer changes: `src/formats/bsa/*_writer.cpp`
- Fixtures: `tests/fixtures/generated/generate_*_bsa_fixtures.cpp`, `tests/fixtures/generated/archives/`
- Tests: `tests/unit/tes3_bsa_reader_tests.cpp`, `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/tes3_bsa_writer_tests.cpp`, `tests/unit/tes4_bsa_writer_tests.cpp`.

**New BA2 Variant or Rule:**
- Detector changes: `src/formats/ba2/ba2_format_detector.cpp`
- GNRL code: `src/formats/ba2/ba2_gnrl_parser.cpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`
- DX10 code: `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`, `src/formats/ba2/ba2_dx10_writer.cpp`
- Publish helpers: `src/formats/ba2/ba2_publish.hpp`
- Fixtures: `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`, `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`
- Tests: `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`.

**New Compression Behavior:**
- Primary code: `src/detail/compression_router.hpp`, `src/detail/compression_router.cpp`
- Codec-specific helper: Add under `src/detail/`
- Format selection: Relevant parser/writer under `src/formats/`
- Tests: `tests/unit/compression_router_tests.cpp`, codec-specific tests under `tests/unit/`, and fixture round-trip tests.
- Public headers: Keep codec library types out of `include/libbsa/`.

**New Archive Path or Hash Behavior:**
- Path normalization: `src/detail/archive_path.cpp`
- Hashing: `src/detail/bethesda_hash.cpp`
- Tests: `tests/unit/archive_path_tests.cpp`, `tests/unit/bethesda_hash_tests.cpp`, and affected format fixture tests.

**New DDS or Texture Behavior:**
- DirectXTex interaction: `src/texture/directxtex_analyzer.cpp`
- DDS header/chunk layout: `src/texture/dds_layout.cpp`
- BA2 DX10 parser/reader/writer: `src/formats/ba2/ba2_dx10_*.cpp`
- Tests: `tests/unit/dds_layout_tests.cpp`, `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`.

**New Validation Rule:**
- Public types: `include/libbsa/validation.hpp`
- Implementation: `src/validation.cpp`
- Tests: `tests/unit/validation_api_tests.cpp`, `tests/unit/validation_policy_tests.cpp`, `tests/unit/compatibility_warning_tests.cpp`.
- Documentation: `docs/target-format-guide.md`, `docs/compatibility-evidence.md`, or `docs/integration-examples.md` when public behavior changes.

**New Fixture Generator:**
- Implementation: `tests/fixtures/generated/generate_*_fixtures.cpp`
- CMake target: `tests/CMakeLists.txt`
- Generated files: `tests/fixtures/generated/archives/` or `tests/fixtures/generated/source/`
- Manifest validation: `tests/fixtures/generated/validate_fixture_manifests.py`
- Documentation: `tests/fixtures/README.md`.

**New Package/Install Behavior:**
- Build config: `CMakeLists.txt`, `cmake/libbsaConfig.cmake.in`
- Consumer smoke: `tests/package-consumer/`
- Runtime DLL behavior: `tests/package-consumer/copy-runtime-dlls.cmake`, `tests/package-consumer/verify-runtime-dll-copy.cmake`
- Tests: `tests/unit/public_include_boundary_tests.cpp`.

**New Documentation:**
- Public behavior docs: `docs/`
- Build and usage summary: `README.md`
- Agent/project constraints: `AGENTS.md`
- API docs: Public header Doxygen comments plus `docs/api-mainpage.md`.

**Utilities:**
- Shared private helper: `src/detail/`
- Format-family private helper: Relevant `src/formats/bsa/` or `src/formats/ba2/`
- Texture-specific helper: `src/texture/`
- Tests: Place helper tests in `tests/unit/<helper>_tests.cpp`.

## Special Directories

**`TES5Edit/`:**
- Purpose: Read-only BSArchPro/TES5Edit behavior reference.
- Generated: No
- Committed: Submodule reference only
- Constraint: Do not edit, format, stage, compile, vendor, or use as mutable test fixture.

**`build/`:**
- Purpose: CMake build output for presets such as `build/windows-msvc-debug-static`.
- Generated: Yes
- Committed: No
- Constraint: Do not use generated build output as source of truth for library code or docs.

**`tests/fixtures/generated/`:**
- Purpose: Committed legal generated fixture corpus and generator source.
- Generated: Yes for archives/source fixtures; generator source is hand-maintained.
- Committed: Yes for current fixture corpus and generators.
- Constraint: Update generator source and manifests together; do not use `TES5Edit/` as a mutable fixture workspace.

**`tests/fixtures/local/`:**
- Purpose: Placeholder for local game archives or machine-specific fixtures.
- Generated: No
- Committed: Only `.gitkeep`
- Constraint: Keep real local/game fixture data uncommitted.

**`.planning/`:**
- Purpose: GSD planning state, codebase maps, phase artifacts, and research.
- Generated: Yes
- Committed: Project-dependent planning artifacts are present in the repo.
- Constraint: Codebase mappers refresh only assigned documents under `.planning/codebase/`.

**`openspec/`:**
- Purpose: OpenSpec change workflow configuration and artifacts.
- Generated: Partly
- Committed: Yes
- Constraint: Use named OpenSpec changes exactly when executing OpenSpec workflows.

**`.codex/skills/`, `.agent/skills/`, `.claude/skills/`, `.cursor/skills/`, `.gemini/skills/`, `.kilocode/skills/`, `.opencode/skills/`:**
- Purpose: Tool-specific OpenSpec skill indexes and commands.
- Generated: Yes
- Committed: Present in the working tree.
- Constraint: Read `SKILL.md` indexes when project skills are relevant; do not treat skill directories as library implementation.

**`.github/workflows/`:**
- Purpose: GitHub Actions CI workflow definitions.
- Generated: No
- Committed: Yes
- Constraint: Keep CI aligned with Windows, MSVC, CMake 4.0+, and vcpkg expectations.

---

*Structure analysis: 2026-05-11*
