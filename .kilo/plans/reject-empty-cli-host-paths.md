# Reject empty pack/unpack filesystem paths (P2)

## Problem

`path_from_utf8` in `tools/cli/main.cpp:428` returns a default
`std::filesystem::path{}` when handed an empty UTF-8 string
(`tools/cli/main.cpp:432-434`):

```cpp
if (utf8_path.empty()) {
    return std::filesystem::path{};
}
```

The pack/unpack host-path positionals are decoded through this helper and then
resolved with `std::filesystem::absolute(...)`, which treats an empty path as the
current working directory (CWD):

- `run_pack` decodes `<input-dir>` and `<output-archive>`
  (`tools/cli/main.cpp:1007`, `1012`), then `collect_input_files` calls
  `std::filesystem::absolute(input_dir, ...)` (`tools/cli/main.cpp:788`).
- `run_unpack` decodes `<output-dir>` (`tools/cli/main.cpp:1515`), then
  `prepare_output_root` calls `std::filesystem::absolute(output_dir, ...)`
  (`tools/cli/main.cpp:1454`).

As a result, when a quoted variable expands to an empty argument:

- `bsa pack --format <token> "" out.bsa` packs the **entire CWD** (often exits 0
  with "packed N file(s)").
- `bsa unpack archive.bsa ""` extracts **into the CWD**.

Both silently succeed instead of failing the required path argument.

## Root-cause analysis of callers

`path_from_utf8` has exactly four call sites:

1. `tools/cli/main.cpp:1007` — pack `<input-dir>` (host path) — must reject empty.
2. `tools/cli/main.cpp:1012` — pack `<output-archive>` (host path) — must reject empty.
3. `tools/cli/main.cpp:1345` — archive entry path inside `safe_destination_path`.
   This caller **already** rejects empty entries *before* calling the helper
   (`tools/cli/main.cpp:1341-1343`), so it never passes an empty string.
4. `tools/cli/main.cpp:1515` — unpack `<output-dir>` (host path) — must reject empty.

Therefore fixing the helper itself is safe (no behavior change for caller #3) and
fixes all three host-path positionals at one chokepoint. It is also consistent
with the sibling validation already in the same function (embedded-NUL rejection
returns `invalid_argument`).

Out of scope: the `<archive>` positional for `unpack`/`list`/`info`/`validate` is
passed as a string directly to `libbsa::archive_reader::open` /
`validate_archive` (e.g. `tools/cli/main.cpp:1509`), not through `path_from_utf8`.
An empty value there cannot silently resolve to "CWD as an archive file"; the
library reports an open/format failure. No change is needed for that path.

## Fix

### Source change — `tools/cli/main.cpp`

In `path_from_utf8`, replace the empty-returns-default branch
(`tools/cli/main.cpp:432-434`) with an explicit `invalid_argument` error, and add
a why-comment explaining the CWD hazard:

```cpp
// An empty host path would otherwise resolve to the current working directory
// during std::filesystem::absolute(), silently packing/extracting the CWD
// instead of failing the required <input-dir>/<output-dir>/<output-archive>
// argument.
if (utf8_path.empty()) {
    return make_error(libbsa::error_code::invalid_argument, "path is empty");
}
```

Resulting behavior:

- `path_from_utf8("")` returns `error_code::invalid_argument` with message
  `"path is empty"`.
- pack input-dir / output-archive and unpack output-dir empty cases hit the
  existing `render_error(...) -> operational_failure` (exit code 1) flow at
  `tools/cli/main.cpp:1008-1011`, `1013-1016`, `1516-1519`. With an empty
  positional, `render_error`'s context is also empty, so it prints
  `error: invalid_argument: path is empty`.

No existing comment is removed. The NUL-check ordering is unchanged (empty input
contains no NUL, so it falls through to the new empty check). The Windows and
non-Windows decode paths below are unaffected because empty now returns early.

This keeps the static source assertion in
`tests/cli/cli_integration.cmake:261` valid, since the
`path_from_utf8(normalized_entry)` call site is untouched.

### Regression tests — `tests/cli/cli_integration.cmake`

Add behavioral tests. Use direct `execute_process` blocks with literal `""`
arguments (modeled on the existing block at
`tests/cli/cli_integration.cmake:422-438`) rather than the variadic `run_cli`
helper, because unquoted `${ARGN}` expansion drops empty list elements while a
literal quoted `""` in a `COMMAND` is preserved as an empty argument. Strict
assertions (exit code `1` + `invalid_argument`) distinguish the fixed behavior
from both the old CWD behavior (exit `0`) and an accidentally-dropped argument
(exit `2`, "requires ...").

1. After the existing pack diagnostics (after
   `tests/cli/cli_integration.cmake:333`, where `${missing_format_root}` is
   already defined):

   - `pack --format bsa-tes3 "" "${work_root}/empty-input-out.bsa"` → expect exit
     `1`, stderr contains `invalid_argument`, and assert the output archive was
     not created (`require_not_exists`).
   - `pack --format bsa-tes3 "${missing_format_root}" ""` → expect exit `1`,
     stderr contains `invalid_argument` (input decodes first, output decode
     fails before any write).

2. After `${selective_archive}` becomes available (it is defined at
   `tests/cli/cli_integration.cmake:371` and the roundtrip that creates it runs
   at line 335); place near the existing unpack refusal checks (~line 393):

   - `unpack "${selective_archive}" ""` → expect exit `1`, stderr contains
     `invalid_argument` (archive opens, output-dir decode fails before
     `prepare_output_root`).

## Verification

1. Configure/build the CLI and run the CLI integration test (the
   `cli_integration.cmake` driver), e.g. via CTest filtering to the CLI
   integration test. Confirm the three new cases pass.
2. Confirm existing roundtrip/UTF-8/reparse cases still pass (no regression from
   the helper change).
3. Confirm format/build hooks are satisfied (run `Format-Cpp.ps1` if part of the
   normal workflow) and `graphify update .` after the code change.

## Risk / notes

- Low risk: single-line semantic change at a shared chokepoint; the only
  archive-entry caller pre-validates emptiness, so no extraction behavior
  changes.
- Exit code is `1` (operational_failure), consistent with the existing
  path-decode error flow (embedded NUL, invalid UTF-8). Not changed to a usage
  error (exit `2`) to keep parity with sibling path-decode failures.
- If, during implementation, CMake is found to drop the literal `""` argument
  (CLI would then report "requires ..." with exit `2`), the new test's own
  `FATAL_ERROR` will surface it; the expectation is that `execute_process`
  preserves a literal quoted empty argument.
