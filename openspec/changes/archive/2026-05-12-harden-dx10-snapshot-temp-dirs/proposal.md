## Why

BA2 DX10 writer snapshot directories are currently named with a process-local counter under the system temp directory, making paths easy to predict before reservation. This should be hardened now because snapshot files contain staged DDS subresource bytes and the existing atomic directory reservation/cleanup model can be preserved while reducing name-prediction risk.

## What Changes

- Replace counter-only DX10 snapshot directory names with names that include a cryptographically strong random suffix or an equivalent Windows temporary-name reservation mechanism.
- Keep atomic `std::filesystem::create_directory`-style reservation semantics so an existing path is never reused silently.
- Keep BA2 DX10 writer state as the owner responsible for best-effort snapshot directory cleanup.
- Add focused tests or implementation checks that the snapshot directory reservation path no longer depends on a predictable monotonic counter and continues to clean up after writer teardown.
- No public API or archive byte format changes are expected.

## Capabilities

### New Capabilities
- `dx10-snapshot-temp-directory-hardening`: Covers secure reservation and cleanup behavior for BA2 DX10 writer snapshot temporary directories.

### Modified Capabilities
- None.

## Impact

- Affected implementation files: `src/formats/ba2/ba2_dx10_prepare.cpp` and `src/formats/ba2/ba2_dx10_writer.cpp`.
- Affected behavior: internal temporary snapshot directory naming and reservation for BA2 DX10 writer staging.
- Tests should cover reservation uniqueness/unpredictability constraints and cleanup ownership without requiring changes to archive output fixtures.
- No new third-party dependencies should be introduced.
