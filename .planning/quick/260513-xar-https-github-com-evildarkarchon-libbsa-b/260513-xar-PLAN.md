---
status: ready
created: 2026-05-14
quick_id: 260513-xar
slug: decode-public-host-paths-as-utf8-on-windows
type: execute
wave: 1
depends_on: []
files_modified:
  - src/detail/host_file_path.cpp
  - tests/unit/host_file_path_tests.cpp
  - tests/CMakeLists.txt
autonomous: true
requirements:
  - quick-260513-xar
must_haves:
  truths:
    - "Windows callers can pass documented UTF-8 host paths with non-ASCII characters and archive open/validate/reopen flows resolve the real file."
    - "Invalid UTF-8 host-path bytes fail at the shared resolution boundary instead of silently becoming an ANSI-code-page path."
  artifacts:
    - path: "src/detail/host_file_path.cpp"
      provides: "Strict UTF-8-to-native-path conversion at the shared host-path seam"
    - path: "tests/unit/host_file_path_tests.cpp"
      provides: "Regression coverage for non-ASCII UTF-8 resolution and malformed UTF-8 rejection"
    - path: "tests/CMakeLists.txt"
      provides: "Compilation of the new focused host_file_path test suite"
  key_links:
    - from: "src/detail/host_file_path.cpp"
      to: "src/archive.cpp"
      via: "detail::resolve_host_file_path(host_path)"
      pattern: "resolve_host_file_path"
    - from: "tests/unit/host_file_path_tests.cpp"
      to: "src/detail/host_file_path.cpp"
      via: "direct seam-level regression checks"
      pattern: "resolve_host_file_path"
---

# Quick Task 260513-xar: Decode public host paths as UTF-8 on Windows

<objective>
Fix the Windows/MSVC host-path boundary so public UTF-8 strings are converted to a native wide filesystem path once, then reused by archive open, validation, parser reopen, and later host-file consumers.

Purpose: Preserve the documented UTF-8 public API contract on the project’s supported Windows target instead of letting `std::filesystem::path{std::string}` reinterpret bytes through the active ANSI code page.
Output: One seam-level implementation change plus focused regression coverage.
</objective>

<context>
@.planning/STATE.md
@.planning/quick/260513-xar-https-github-com-evildarkarchon-libbsa-b/260513-xar-RESEARCH.md
@src/detail/host_file_path.hpp
@src/detail/host_file_path.cpp
@src/archive.cpp
@tests/CMakeLists.txt
@tests/unit/host_path_correctness_boundary_tests.cpp

<interfaces>
From src/detail/host_file_path.hpp:
- `struct host_file_path { std::string original_utf8; std::filesystem::path resolved; };`
- `result<host_file_path> resolve_host_file_path(std::string_view host_path);`

From src/archive.cpp:
- `archive_reader::open(std::string_view host_path)` already fails on empty input, then calls `detail::resolve_host_file_path(host_path)` once and reuses `host_file_path::resolved` for detection, size probes, parsing, and reopens.

Research lock:
- Use `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ...)` at the `resolve_host_file_path()` seam.
- Keep `original_utf8` unchanged for diagnostics.
- Do not add per-reopen conversions or ANSI-code-page fallbacks.
</interfaces>
</context>

<tasks>

<task type="auto" tdd="true">
  <name>Task 1: Add seam-level UTF-8 host path regression coverage</name>
  <files>tests/unit/host_file_path_tests.cpp, tests/CMakeLists.txt</files>
  <behavior>
    - Test 1: A native Windows path containing `Ångström-日本語` round-trips through `path.u8string()` into `resolve_host_file_path()` and the returned `resolved` path equals the original native path.
    - Test 2: The returned `original_utf8` field preserves the caller bytes unchanged for diagnostics.
    - Test 3: Malformed UTF-8 bytes are rejected with a libbsa error instead of producing a best-effort path.
  </behavior>
  <action>Create a new focused internal test suite per D-53 through D-61 that exercises `detail::resolve_host_file_path()` directly rather than expanding the public black-box boundary suite. Register the file in `tests/CMakeLists.txt`, create native non-ASCII paths with `std::filesystem::path`/wide text, convert to explicit UTF-8 with `u8string()`, and lock the malformed-byte case so the later implementation must fail before archive I/O begins. Keep comments/doc intent aligned with the existing host-path seam rationale.</action>
  <verify>
    <automated>cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex host_file_path</automated>
  </verify>
  <done>A dedicated test target compiles, the new cases fail before the implementation change, and the suite proves both non-ASCII success and malformed UTF-8 rejection expectations.</done>
</task>

<task type="auto" tdd="true">
  <name>Task 2: Resolve public UTF-8 host text to a native Windows path</name>
  <files>src/detail/host_file_path.cpp</files>
  <behavior>
    - Test 1: Valid UTF-8 host text yields `host_file_path{original_utf8, resolved}` where `resolved` matches the original native path.
    - Test 2: Invalid UTF-8 returns an error compatible with the existing invalid-argument style boundary failures.
  </behavior>
  <action>Replace `std::filesystem::path{std::string{host_path}}` with an explicit Windows UTF-8 decode path per the research lock: copy the caller bytes once, return an empty resolved path for empty text if reached, call `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ...)` twice to size and fill a `std::wstring`, and build `std::filesystem::path` from that wide string. Add a short why-comment documenting that MSVC narrow `path` construction follows the active ANSI code page, so the public UTF-8 contract must be decoded here. On conversion failure, return a libbsa argument error instead of falling back to ACP or lossy replacement.</action>
  <verify>
    <automated>cmake --build --preset windows-msvc-debug-static --target libbsa_tests && ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "host_file_path|host_path_correctness_boundary"</automated>
  </verify>
  <done>`resolve_host_file_path()` is the single UTF-8-to-native conversion seam, non-ASCII Windows paths resolve correctly, malformed UTF-8 is rejected, and the existing public host-path boundary suite still passes through the unchanged API.</done>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| caller UTF-8 text -> host path resolver | Untrusted path bytes cross from public API input into native filesystem access |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-260513-xar-01 | T | `resolve_host_file_path()` | mitigate | Reject malformed UTF-8 with `MB_ERR_INVALID_CHARS` so path bytes cannot be silently rewritten through ACP fallback semantics. |
| T-260513-xar-02 | D | archive open/validate/reopen path seam | mitigate | Keep conversion centralized at `resolve_host_file_path()` so all downstream host-file I/O reuses the same validated native path object. |
</threat_model>

<verification>
- `cmake --build --preset windows-msvc-debug-static --target libbsa_tests`
- `ctest --preset windows-msvc-debug-static --output-on-failure --tests-regex "host_file_path|host_path_correctness_boundary"`
</verification>

<success_criteria>
- UTF-8 host paths with non-ASCII characters open and validate successfully on Windows/MSVC through the existing public API.
- The shared host-path seam preserves caller UTF-8 text for diagnostics while storing a native wide `std::filesystem::path` for I/O.
- Malformed UTF-8 cannot silently resolve through the system ANSI code page.
</success_criteria>

<output>
After completion, update this quick-task directory with the execution summary or completion notes expected by the quick-task workflow.
</output>
