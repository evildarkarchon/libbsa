# CLAUDE.md

@AGENTS.md

## Project Scope

libbsa is a Windows-only C++20 library for reading, writing, validating, and extracting Bethesda archive formats.

Review and implementation work should assume Windows with MSVC, CMake, CTest, and vcpkg. Do not request Linux, macOS, POSIX, or general cross-platform portability changes unless the user explicitly reopens platform support.

## Supported Presets

Use the Windows-only presets in `CMakePresets.json`:

- **Debug quick path**: `windows-msvc-debug-static`.
- **Debug inner-loop lane**: `windows-msvc-debug-shared`.
- **Release package-proof lanes**: `windows-msvc-release-static` and
  `windows-msvc-release-shared`.
- **MSVC AddressSanitizer hardening lane**: `windows-msvc-asan-static`.

Linux presets are intentionally absent and should not be reintroduced.

## TES5Edit Boundary

`TES5Edit/` is read-only reference material. Do not edit, format, stage, commit, compile, or generate files inside it.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
