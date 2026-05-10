# CLAUDE.md

## Project Scope

libbsa is a Windows-only C++20 library for reading, writing, validating, and extracting Bethesda archive formats.

Review and implementation work should assume Windows with MSVC, CMake, CTest, and vcpkg. Do not request Linux, macOS, POSIX, or general cross-platform portability changes unless the user explicitly reopens platform support.

## Supported Presets

Use the Windows presets in `CMakePresets.json`:

- `windows-msvc-debug-static`
- `windows-msvc-debug-shared`

Linux presets are intentionally absent and should not be reintroduced.

## TES5Edit Boundary

`TES5Edit/` is read-only reference material. Do not edit, format, stage, commit, compile, or generate files inside it.
