# CLI unpack: publish extracted files only after payload success

## Problem

`bsa unpack` opens the *final* destination file up front, before the entry's
payload has been fully extracted:

- Windows (`open_file_payload_sink`, `tools/cli/main.cpp:1205`): with
  `--overwrite` on an existing file it calls `truncate_handle`
  (`tools/cli/main.cpp:1283`), zeroing the file before any payload byte is
  written. For a new destination it uses `CREATE_NEW`, leaving an empty/partial
  file on disk.
- POSIX fallback (`tools/cli/main.cpp:1314`): opens the destination with
  `std::ios::trunc`.

When extraction later fails during payload streaming or decompression
(`extract_entry_payload` returns an error inside
`archive_reader::extract_entries`, `src/archive.cpp:398`), the per-entry result
is reported as a failure (`tools/cli/main.cpp:1558-1564`), but the destination
has already been destroyed or partially written:

- `--overwrite` over an existing file ⇒ the previous file content is gone even
  though the entry "failed".
- new destination ⇒ an empty/partial file is left behind.

This is a real correctness/safety defect (P2 from review). The fix: stream each
entry into a temporary sibling and only publish it to the final path after the
entry's payload extraction succeeds; otherwise remove the temporary and leave
the final path untouched.

## Why this is non-trivial

The public `payload_sink` interface (`include/libbsa/archive.hpp:179`) has only
`write()` — there is **no commit/finalize hook**, and `extract_entries`
constructs the sink via the factory, streams to it, then **destroys the sink
inside the library** (the `work` lambda local in `src/archive.cpp:367-405`)
*before* the caller can observe per-entry success/failure in the returned
`bulk_extract_entry_result` vector. So the sink object itself cannot reliably
self-publish, and we must not change the public library API for a CLI fix.

Constraints from the existing CLI integration policy
(`tests/cli/cli_integration.cmake:235-243`) that the fix must keep satisfied:

- Destinations must use a Windows **handle-based** `CreateFileW(` path (atomic
  destination safety). The token `CreateFileW(` must remain present.
- The source must **not** contain `std::ofstream stream{destination.value()`
  (path-based truncating open of the destination after reparse checks).

The current handle-based reparse/junction validation in `open_file_payload_sink`
(root handle final-path check, `FILE_FLAG_OPEN_REPARSE_POINT`,
`final_path_is_within_root`) must be preserved.

## Chosen design: factory-owned staged temp + handle-based publish

Keep the open temp-file handle alive **in the factory** (outliving the sink), so
the per-entry outcome known after `extract_entries` returns can drive an atomic,
handle-relative publish (Windows `FILE_RENAME_INFO`) or a clean discard. No
public API change.

### New type: `staged_extraction` (in the `tools/cli/main.cpp` anonymous namespace)

Windows fields:
- `unique_windows_handle handle;` — open temp file handle (kept open until
  commit/discard).
- `std::filesystem::path temp_path;`
- `std::filesystem::path final_path;`
- `bool overwrite;`
- `bool finished{false};`

POSIX fields:
- `std::filesystem::path temp_path; final_path; bool overwrite; bool finished;`
  (the sink owns its own `std::ofstream` to `temp_path`).

Held via `std::shared_ptr<staged_extraction>`: the sink and the factory each
hold a `shared_ptr`. When the library destroys the sink, the factory's
`shared_ptr` keeps the temp handle/stream alive for publish.

### Temp path

A unique sibling in the already-created parent directory of the final path, e.g.
`parent / ("." + final_filename + ".bsa-tmp-" + token)` where `token` comes from
a process-unique atomic counter (plus PID). Leading-dot + numeric suffix keeps
the Windows reserved-device-name and trailing-dot/space checks satisfied (the
stem before the first `.` is empty, never a reserved device). Use `CREATE_NEW`
so a stale temp never collides silently; on `ERROR_FILE_EXISTS` regenerate the
token and retry a few times.

### Windows open helper (replaces the body of `open_file_payload_sink`)

`open_staged_destination(temp_path, final_path, output_root, overwrite) ->
result<std::shared_ptr<staged_extraction>>`:

1. Open `output_root` handle and validate it is not a reparse point and resolve
   its final path (unchanged from current logic at `main.cpp:1210-1231`).
2. `CreateFileW(temp_path, GENERIC_WRITE | FILE_READ_ATTRIBUTES | DELETE,
   FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, CREATE_NEW,
   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT |
   FILE_FLAG_SEQUENTIAL_SCAN, nullptr)`.
3. Immediately `mark_delete_on_close(handle)` so any abort/crash/early-return
   removes the temp automatically (delete-on-close stays set until commit).
4. Validate the temp handle is not a reparse point
   (`handle_is_reparse_point`) and `final_path_is_within_root(root_final,
   GetFinalPathNameByHandle(temp))`. The temp is a sibling inside the validated
   parent, so this holds; on failure return an error (delete-on-close cleans up).
5. Return the `staged_extraction` with the open handle and the paths.

`overwrite` no longer gates the temp open (temp always `CREATE_NEW`); the
"destination exists and `--overwrite` not specified" gate stays in
`file_sink_factory::create` (existing `std::filesystem::exists` check at
`main.cpp:1404-1415`), so that diagnostic is unchanged.

### Sink

- Windows `file_payload_sink`: hold `std::shared_ptr<staged_extraction>` instead
  of owning the handle; `write()` issues the same `WriteFile` loop against
  `staged_->handle.value`. The sink no longer closes the handle (the factory's
  `shared_ptr` owns its lifetime).
- POSIX `file_payload_sink`: hold `std::shared_ptr<staged_extraction>` plus an
  owned `std::ofstream` opened to `staged_->temp_path` (binary|trunc). Variable
  name must avoid the forbidden token (use e.g. `out`/`temp_stream`, never
  `std::ofstream stream{destination.value()`).

### Publish / discard helpers

`commit_staged(staged) -> result<void>`:
- If `staged->finished` ⇒ no-op success (handles duplicate coalesced paths).
- Windows: clear delete-on-close
  (`SetFileInformationByHandle(FileDispositionInfo, DeleteFile=FALSE)`), then
  `SetFileInformationByHandle(FileRenameInfo)` with `FileName = final_path`,
  `ReplaceIfExists = overwrite`. On failure return
  `windows_io_error("cannot publish extracted file: <final>")`. Then close the
  handle and set `finished`.
  - `ReplaceIfExists = overwrite` means non-overwrite publish fails (rather than
    clobbers) if a file appears at the final path in a race — strictly safer.
- POSIX: stream is already closed (sink destroyed first); `std::filesystem::
  rename(temp, final)` (for non-overwrite the create-time existence check is the
  gate). Set `finished`.

`discard_staged(staged)`:
- If `finished` ⇒ no-op.
- Windows: close the handle ⇒ delete-on-close removes the temp; set `finished`.
- POSIX: `std::filesystem::remove(temp)`; set `finished`.

### `file_sink_factory` changes

- Add `std::vector<std::shared_ptr<staged_extraction>> staged_;`, a
  `std::map<std::string, std::shared_ptr<staged_extraction>> by_path_;` keyed by
  the `path` argument passed to `create`, and a `std::mutex` (the CLI runs
  serial today, but `create` is documented as potentially concurrent under
  multiple workers, so guard the bookkeeping).
- `create(path, entry)`: keep all current validation (reparse ancestors,
  existence/overwrite gate, directory-vs-file check, `create_directories` of
  parent, re-check reparse). Then compute `temp_path`, call
  `open_staged_destination`, register the `staged_extraction` under `path`, and
  return a sink referencing it.
- Add `result<void> commit(std::string_view path)` and
  `void discard(std::string_view path)` that look up `by_path_` and delegate to
  the helpers (no-op success when the path was never staged, e.g. lookup or
  sink-creation failure).
- Destructor: for any `!finished` staged entry, discard it (close handle ⇒
  delete-on-close on Windows / remove temp on POSIX), guaranteeing no temp file
  leaks if `extract_entries` returns an outer error before publishing.

### `run_unpack` changes (`tools/cli/main.cpp:1558-1568`)

After `extract_entries` returns successfully, drive publish from the per-entry
results:

```
for (const auto& result : extracted.value()) {
    if (result.failure.has_value()) {
        sink_factory.discard(result.path);
        ++failure_count;
        render_error(*result.failure, result.path);
    } else {
        auto published = sink_factory.commit(result.path);
        if (!published) {
            ++failure_count;
            render_error(published.error(), result.path);
        }
    }
}
```

`commit`/`discard` are idempotent so coalesced duplicate request paths are safe.
The "extracted N of M" line and exit code keep using `failure_count`, so a
commit failure correctly downgrades that entry to failed.

## Behavior preserved (existing CLI integration cases)

- Overwrite refusal (`cli_integration.cmake:424`): still produced by the
  create-time existence gate.
- Reparse-point destination refusal (`:490-495`): create-time reparse checks +
  temp handle validation; nothing written outside root.
- Windows-unsafe names "extracted 0 of 6" (`:522-529`): all `create` calls fail
  before staging; `discard` no-ops; counts unchanged.
- Selective unpack with a missing path (`:420-422`): the found entry is staged
  and committed; the `not_found` entry is never staged.
- Round-trip + UTF-8 + mixed-case cases: unchanged (successful entries commit to
  the same final paths; no temp siblings remain after the process exits).

## Regression test (CLI integration)

Use the existing `malformed_corrupt_compressed_payload.bsa` fixture
(`tests/fixtures/generated/archives/`, manifest phase `extraction`,
`tools/cli/.../malformed_manifest.json:48-57`). It opens successfully but the
compressed entry `meshes/tiny/packedmesh.nif` fails during decompression after
the sink exists — exactly the failure window this fix addresses.

Add a `WIN32`-guarded block (mirrors other fixture-guarded blocks) in
`tests/cli/cli_integration.cmake`, gated on the fixture existing:

1. **New-destination case**: copy the fixture to a work dir, `unpack` to a fresh
   output (`run_cli(1 ...)` — at least one entry fails). Assert with
   `require_not_exists` that `<output>/meshes/tiny/packedmesh.nif` does **not**
   exist (no empty/partial file left behind), and assert `format_error` in
   stderr.
2. **Overwrite case**: pre-seed `<output>/meshes/tiny/packedmesh.nif` with a
   sentinel via `write_text`, run `unpack --overwrite` (`run_cli(1 ...)`), then
   `require_file_text` that the sentinel content is **unchanged** (the failed
   entry did not truncate/destroy the prior file).
3. Optionally assert no stray temp sibling remains in that directory (the only
   child of `.../tiny` should be `packedmesh.nif`).

This fails against the current truncate-up-front behavior and passes with the
staged-publish fix.

Also add/extend a focused unit-level test only if a non-CLI seam is introduced;
otherwise the CLI integration test is the authoritative coverage since the
staging logic lives entirely in `tools/cli/main.cpp`.

## Risks / notes

- `FILE_RENAME_INFO` requires same-volume (temp is a sibling ⇒ satisfied) and
  `DELETE` access on the handle (requested). Replacing an existing read-only
  final file can fail; that surfaces as a per-entry commit failure rather than a
  silent loss — acceptable and safer than today.
- Residual TOCTOU on the final path name is no worse than the current code; the
  temp handle is validated within root, and non-overwrite publish uses
  `ReplaceIfExists = FALSE`. Optional hardening (re-running
  `reject_reparse_ancestors` immediately before the rename in `commit_staged`)
  can be added; note it in the commit message if included.
- Keep the AGENTS.md comment policy: add Doxygen `///` comments for the new
  `staged_extraction` type and the `open_staged_destination` / `commit_staged` /
  `discard_staged` helpers, documenting the why (atomic publish, delete-on-close
  abort cleanup, handle kept open past sink lifetime).
- Project is Windows-only; the POSIX fallback is kept correct and symmetric but
  is secondary.

## Out of scope

- Changing the public `payload_sink` / `extract_entries` API (no commit hook).
- Parallel-extraction changes (CLI stays `worker_count = 1`).
- `pack` output atomicity (separate writer code path; reviewer comment is
  unpack-specific).

## Affected files

- `tools/cli/main.cpp` — staging type/helpers, sink rework, factory
  bookkeeping, `run_unpack` publish loop.
- `tests/cli/cli_integration.cmake` — regression assertions using
  `malformed_corrupt_compressed_payload.bsa`.
