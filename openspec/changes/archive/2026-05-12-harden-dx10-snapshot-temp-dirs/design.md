## Context

The BA2 DX10 writer snapshots DDS subresource bytes into a temporary directory during `add_file` so later source-file changes do not affect the staged archive. That directory is currently reserved under `std::filesystem::temp_directory_path()` with names of the form `libbsa-dx10-snapshot-<counter>`.

`std::filesystem::create_directory` already provides an atomic reservation boundary, and `ba2_dx10_writer::state` already owns best-effort cleanup in its destructor. The weakness is only name predictability: another process can know the next likely path before libbsa reserves it.

## Goals / Non-Goals

**Goals:**

- Generate BA2 DX10 snapshot directory names with a cryptographically strong random suffix or an equivalent Windows temporary-name reservation mechanism.
- Preserve atomic directory reservation by creating the candidate directory as the reservation operation.
- Preserve writer-state cleanup ownership and result-based error reporting.
- Avoid changing public writer APIs, archive bytes, compression routing, DirectXTex behavior, or vcpkg dependencies.

**Non-Goals:**

- Changing snapshot file names inside the reserved directory.
- Moving snapshot staging out of the system temp root.
- Adding cross-platform temporary-directory abstractions.
- Reworking the BA2 DX10 add-time snapshot model.

## Decisions

- Use Windows-provided cryptographic randomness for the suffix.

  Rationale: libbsa is Windows-only, and a Windows system RNG avoids adding third-party dependencies. `BCryptGenRandom` with `BCRYPT_USE_SYSTEM_PREFERRED_RNG` is the preferred implementation path because it returns explicit failure status that can be translated into `libbsa::result` errors.

  Alternatives considered: `std::random_device` is implementation-dependent and not explicitly cryptographic; a timestamp/process-id/counter mix remains guessable; `std::filesystem::unique_path` is not in the C++ standard library.

- Keep create-directory reservation semantics.

  Rationale: Random names reduce predictability, but the security boundary still depends on only using a path after successfully creating it. The implementation should continue to call `std::filesystem::create_directory` for each candidate and should retry on name collision.

  Alternatives considered: checking `exists` before create would reintroduce a time-of-check/time-of-use gap; using a pre-opened temp file would not directly reserve the directory needed for multiple snapshot files.

- Use a large encoded random suffix and bounded retries.

  Rationale: A 128-bit random suffix encoded as hex is compact, readable in diagnostics, and collision-resistant enough that retry exhaustion indicates an environmental failure. The existing 1024-attempt retry shape can remain, but each attempt must request fresh random bytes.

  Alternatives considered: keeping the monotonic counter as a fallback would preserve predictability; using fewer random bytes would make policy tests less convincing and reduce collision margin.

- Keep cleanup in `ba2_dx10_writer::state`.

  Rationale: State ownership already ensures snapshots live long enough for `write_to` after `add_file`, including moved writer state. This change should only alter reservation naming, not lifetime ownership.

  Alternatives considered: cleaning up immediately after archive preparation would risk breaking retry, move, or multiple-write scenarios that rely on staged snapshot-backed entries.

## Risks / Trade-offs

- Windows RNG API use may require build-system linkage to `bcrypt.lib` -> add the system library at the libbsa target boundary if needed and keep it private.
- RNG failure creates a new early `add_file` failure path -> return `io_error` with BA2 DX10 snapshot reservation wording and do not create partial writer state.
- Source-level checks cannot prove cryptographic quality -> combine a policy test for the chosen Windows RNG path with existing writer behavior tests that prove snapshots and cleanup still work.
- Randomized paths make exact directory names unsuitable for assertions -> tests should assert policy and cleanup behavior, not a specific suffix value.

## Migration Plan

Implement the random suffix helper behind the existing `make_unique_snapshot_directory` call path, update any required private build linkage, then add focused tests. No data migration or downstream API migration is required.

## Open Questions

None.
