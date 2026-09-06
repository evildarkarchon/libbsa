# AGENTS.md

## Project Purpose

libbsa is a reusable Windows-only C++20 library for reading, writing, validating, and extracting Bethesda Game Studios archive formats, including BSA and BA2 variants from Morrowind through Starfield. It targets modding tools, asset pipelines, and game utilities that need archive access without UI coupling or Delphi/BSArchPro implementation details leaking into the public API.

**Core value:** libbsa must read, write, and extract every supported Bethesda archive format with byte-level compatibility against official tools and BSArchPro.

The behavioral reference is the BSArchPro code from the `TES5Edit` git submodule. Treat that code as prior art and compatibility guidance while designing a clean C++ library surface for this project.

Useful reference areas currently include:

- `TES5Edit/BSArchPro.dpr`
- `TES5Edit/BSArch/`
- `TES5Edit/Core/wbBSArchive.pas`
- `TES5Edit/Core/wbBSA.pas`

## Platform Support

libbsa is a Windows-only library. Development, review, CI, packaging, and dependency validation target Windows with MSVC and vcpkg. Do not raise portability findings or add implementation work solely to support Linux, macOS, POSIX, or general cross-platform behavior unless the user explicitly reopens platform support.

## Hard Boundary: TES5Edit Is Read-Only

`TES5Edit/` is a read-only reference submodule. Do not modify it for any reason.

This includes:

- Do not edit files under `TES5Edit/`.
- Do not format files under `TES5Edit/`.
- Do not apply generated changes under `TES5Edit/`.
- Do not update the submodule pointer.
- Do not stage or commit changes inside `TES5Edit/`.
- Do not treat the submodule as vendored source to be compiled into this project.

All implementation work belongs outside `TES5Edit/`.

## Language and Implementation Direction

- The implementation language is C++20. Do not expose C++23-only library types in public headers; in particular, use `libbsa::result<T>` rather than `std::expected`.
- Error model: prefer `libbsa::result<T>` or an explicit error-code style for I/O and format failures. Reserve exceptions for programmer precondition violations.
- Keep the library reusable and independent of application-specific UI or tooling.
- Keep public headers minimal. They must not leak platform, compression, or DirectXTex/DXGI implementation details.
- Prefer clear, idiomatic Windows-oriented C++ interfaces over direct transliteration of Delphi/Pascal structure.
- Preserve archive-format behavior discovered from BSArchPro unless there is a documented reason to diverge.
- When porting behavior, trace the reference code first and record non-obvious compatibility constraints near the new implementation.
- No global mutable state and no singleton-based behavior. Thread safety comes from isolated objects and explicit ownership.
- Large archives must be handled with streaming I/O and bounded scratch buffers, not whole-archive memory loading.
- Do not use `std::filesystem::path` for archive-internal paths. Bethesda virtual paths are normalized archive keys, and host separator, case, and encoding rules can corrupt lookups and hashes. Use filesystem paths only at host I/O boundaries.
- Favor the open/read/write-new archive flow. In-place archive mutation is deliberately deferred because shifting tables, compression, DDS chunks, and deduplication make it hard to make safe.

Domain vocabulary for archive concepts lives in `CONTEXT.md`; architectural decisions live under `docs/adr/`.

## Dependencies

Dependencies are consumed through vcpkg in manifest mode. `vcpkg.json` and `vcpkg-configuration.json` are committed, and the registry baseline in `vcpkg-configuration.json` is what pins versions — do not duplicate version pins in documentation.

Library dependencies:

- `libdeflate` (`compression`, `decompression`, and `zlib` features) for the DEFLATE-family payloads. The `zlib` feature is required: retail archives store RFC1950 zlib-wrapped streams, never bare RFC1951 deflate, and the whole corpus proves it (issue #42). Do not enable the `gzip` feature unless fixtures prove a gzip-wrapped need.
- `lz4`, the official library linked as `lz4::lz4`, for both the frame and raw block paths. Do not depend on the lz4 CLI or treat its GPL terms as applying to the library.
- `DirectXTex` for texture analysis, used only behind an internal adapter.

Tooling and test dependencies:

- `argparse` is used by the CLI tool target only.
- `catch2` and `nlohmann-json` are used by the test targets only.

Rules:

- Keep dependencies scoped to the target that needs them.
- When adding a dependency, document the need, the alternatives considered, and the expected project impact.
- Avoid by default: zlib, miniz, and zlib-ng (libdeflate covers the required DEFLATE payloads); Boost, libarchive, and ZIP/7z libraries (they do not implement BSA/BA2 semantics); and external logging or formatting libraries such as `spdlog` and `fmt` (return structured errors and let consumers format them).

## Compression and Texture Routing by Archive Variant

Encode the compression method as explicit metadata. Never infer it solely from a file extension.

- **TES3 BSA**: no compression. Use standard C++ binary I/O and explicit little-endian reads, and keep data-section-relative offset math and TES3 hash/path utilities isolated in TES3 code.
- **TES4 / FO3 / FNV / Skyrim LE BSA**: libdeflate's *zlib* APIs (`libdeflate_zlib_*`). Keep embedded-name handling, archive flags, file flags, and hash ordering in format-specific code.
- **Skyrim SE/AE BSA**: official LZ4 *frame* APIs (`LZ4F_*`) only.
- **Fallout 4 BA2, Starfield BA2 v2, and Starfield BA2 v3 when not LZ4**: libdeflate's *zlib* APIs (`libdeflate_zlib_*`).
- **Starfield BA2 v3 with `CompressionMethod == 3`**: official raw *block* APIs (`LZ4_*safe*`).

Every DEFLATE-family payload is RFC1950 zlib-wrapped, never bare RFC1951. The reference only ever selects `ctZlib`, `ctLZ4Frame`, or `ctLZ4Block` (`wbBSArchive.pas:32`, `1790-1813`), and `ctZlib` runs through Delphi's `ZCompressStream`/`DecompressToUserBuf`, which produce and consume wrapped streams. libbsa therefore has no raw-deflate route at all; do not add one. Reading raw deflate is what issue #42 fixed, and writing it produces archives the games cannot read.

LZ4 frame and raw block formats are different, and selecting the wrong API can fail or silently corrupt output. Keep the frame and block wrappers separate.

Decompression helpers must be exact-size: fail if the decompressed byte count does not match the size recorded in archive metadata. The zlib route carries two narrow, corpus-proven exceptions that still honour the exact size, documented on `decompress_zlib_exact`: an empty payload decoding to zero bytes, and a complete DEFLATE stream whose Adler-32 trailer is missing or short. The second mirrors the reference's deliberate `Buffer error` tolerance for vanilla `Fallout - Misc.bsa` (`wbBSArchive.pas:1800-1810`). Do not widen either one.

BA2 DX10/DDS work goes through the DirectXTex adapter for dimensions, DXGI format, mip count, array/cubemap metadata, and mip chunk planning. Persist libbsa-native metadata, never DirectXTex objects.

## Comments and Documentation

- Never delete an accurate comment as cleanup. Remove or rewrite a comment only when the code it describes is deleted or has changed enough to make the comment wrong.
- If a comment is removed or rewritten, mention it in the final reply.
- Add comments for non-obvious why: format compatibility constraints, ownership/lifetime decisions, error-handling edge cases, threading behavior, cancellation behavior, and deliberate deviations from the reference implementation.
- Add Doxygen-compliant C++ doc comments (/// or /** ... */) for public APIs and for methods that are added or substantially rewritten.
- Trivial private helpers may omit doc comments when their purpose is obvious.
- Any change that may affect format compatibility must be verified against the reference implementation and documented in the commit message.

## Build and Validation

Builds use CMake (minimum 4.0) with CTest, driven by the Windows-only presets in `CMakePresets.json`:

- `windows-msvc-debug-static` — Debug quick path.
- `windows-msvc-debug-shared` — Debug inner-loop lane.
- `windows-msvc-release-static` and `windows-msvc-release-shared` — Release package-proof lanes.
- `windows-msvc-asan-static` — MSVC AddressSanitizer hardening lane.

Linux presets are intentionally absent and should not be reintroduced. `.github/workflows/ci.yml` runs every preset on `windows-latest` against a pinned CMake version and fails the build if the `TES5Edit/` submodule changed.

Test expectations:

- Add focused tests for archive parsing, writing, round-tripping, and compatibility behavior as those surfaces are implemented.
- Prefer fixture-based tests that prove byte-level or metadata-level compatibility with known archive behavior.
- Label tests for CTest filtering, following the labels already in `tests/CMakeLists.txt` (`unit`, `fixture`, `roundtrip`, `malformed`, `cli`, `export_surface`, `concurrency`, and similar).
- Archive parsers consume untrusted binary data, so exercise malformed headers, oversized sizes, truncated payloads, and decompression failures — especially under the ASan lane.
- Never keep production or library code around exclusively for test compatibility. When an API or behavior changes, migrate affected tests to the current API or remove obsolete tests; test-only compatibility shims in product code are not allowed. This is mandatory.
- Do not use the `TES5Edit/` submodule as a mutable test fixture.

### Running two test binaries at once

`libbsa_tests` refuses to start when another instance is already running, and exits non-zero with a
diagnostic naming the condition. If you see that message, it is the guard, not a defect in the code
under test — check for a second `ctest` run or a stray test process from an earlier session.

Set `LIBBSA_TEST_ALLOW_CONCURRENT` to any non-empty value, in the environment of every instance that
may overlap, to run two on purpose. Doing so is safe: each process owns a private temp root and
cannot see another process's temp state. The guard is a legibility aid, not the isolation mechanism.
ADR-0004 records why it exists anyway, and ADR-0003 records the isolation.

The same guard means `ctest -j` (or `CTEST_PARALLEL_LEVEL`) refuses most of the suite, since every
test case is its own process. Run CTest serially, as every preset does. Parallel runs were already
out of scope before the guard — tests inside one run still share fixed temp-derived names — so the
opt-out will let a parallel run *start* but will not make it correct.

The `concurrent_test_instances` CTest case is the proof that all of this holds. It starts two test
binaries at the same time, filtered to the BA2 DX10 snapshot cleanup cases and both carrying the
opt-out, requires both to exit zero, and then requires an instance started *without* the opt-out to
be refused. It carries the `concurrency` label, so `ctest -L concurrency` runs it alone and
`ctest -LE concurrency` leaves it out; it costs well under a second. Its driver lives in
`tests/concurrency/`. It also holds a CTest `RESOURCE_LOCK`, which is what would keep a future
second concurrency case from being scheduled inside it — that property only bites under `ctest -j`,
which this repository does not use, so today it is a statement of intent for whoever adds the second
one rather than a constraint doing active work.

### Local game archive corpus

`tests/fixtures/local/` holds an uncommitted corpus of vanilla retail archives copied from the
supported games. The directory is gitignored, so its contents are invisible from a clean checkout and
cannot be discovered by reading the repository.

Agents are authorized to read these archives and to run the `[requires-game-fixture]` tests against
them without asking first. This is standing authorization.

- Treat the corpus as strictly read-only reference data. Never modify, move, rename, delete, or
  rewrite an archive in it, and never commit one.
- Never copy archive bytes into committed fixtures, test sources, issues, or commit messages. Retail
  archives are not redistributable; report offsets, versions, and counts instead of content.
- The corpus is resolved by `LIBBSA_GAME_FIXTURES` when set, otherwise from `tests/fixtures/local/`
  directly. Tests skip when neither is available, so a clean checkout still passes.
- Prefer this corpus over synthesized fixtures whenever a claim concerns real archive layout.
  Generated fixtures are produced by libbsa's own writers, so they can only prove libbsa agrees with
  itself. Compatibility claims about physical layout must be checked against retail archives.

## Agent skills

### Issue tracker

Track work and PRDs as GitHub Issues in `evildarkarchon/libbsa`; external pull requests are not a triage request surface. See `docs/agents/issue-tracker.md`.

### Triage labels

Use the canonical triage-state labels: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, and `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

Use the single-context layout rooted at `CONTEXT.md`, with architectural decisions under `docs/adr/`. See `docs/agents/domain.md`.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

When the user types `/graphify`, use the installed graphify skill or instructions before doing anything else.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- Dirty graphify-out/ files are expected after hooks or incremental updates; dirty graph files are not a reason to skip graphify. Only skip graphify if the task is about stale or incorrect graph output, or the user explicitly says not to use it.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
