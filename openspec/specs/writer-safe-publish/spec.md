## Purpose

Specify the shared internal writer publication lifecycle for host-path archive saves, including temporary output handling, overwrite behavior, and writer-specific diagnostics.

## Requirements

### Requirement: Shared writer publish lifecycle
Archive writers SHALL use a shared internal publish helper for final host-path publication after format-specific archive bytes are written to a temporary file. The helper SHALL own temporary output directory reservation, final output existence checks, publication, error shaping, and best-effort cleanup. Publish safety coverage SHALL prove the helper-owned behavior through executable helper and writer tests, with source-text checks limited to the minimal delegation boundary that cannot be observed through public archive output alone.

#### Scenario: Writer delegates final publication
- **WHEN** a TES3 BSA, TES4 BSA, BA2 GNRL, or BA2 DX10 writer saves an archive to a host path
- **THEN** the writer performs format-specific archive serialization through the shared publish helper's temporary-file callback
- **AND** the writer does not duplicate temporary-directory reservation, final publish, backup, rollback, or cleanup control flow locally

#### Scenario: Publish coverage exercises helper behavior
- **WHEN** writer publish policy coverage verifies no-overwrite, overwrite, diagnostic-prefix, temporary cleanup, or rollback behavior
- **THEN** the test exercises the shared publish helper or a writer-family save path
- **AND** the assertions inspect observable save results, destination bytes, returned diagnostics, and cleanup state instead of relying only on private source-token searches

### Requirement: No-overwrite publication preserves existing destinations
When overwrite is disabled, the shared publish helper SHALL fail if the destination exists before or during publication, and it SHALL NOT replace or truncate the existing destination.

#### Scenario: Destination exists before writing
- **WHEN** a writer is asked to save to an existing destination with overwrite disabled
- **THEN** the save fails with an `io_error`
- **AND** the existing destination bytes remain unchanged

#### Scenario: Destination appears during publish
- **WHEN** the destination does not exist during initial validation but exists by the time the helper publishes the temporary archive
- **THEN** publication fails with an `io_error`
- **AND** the raced destination bytes remain unchanged
- **AND** no partial archive is exposed at the destination path

### Requirement: Overwrite publication replaces existing regular archives uniformly
When overwrite is enabled, the shared publish helper SHALL use one overwrite strategy for all writer families and SHALL either publish the completed temporary archive or preserve a readable previous destination on failure.

#### Scenario: Existing regular archive is overwritten
- **WHEN** a writer is asked to save to an existing regular archive with overwrite enabled
- **THEN** the completed temporary archive becomes the destination
- **AND** TES3 BSA, TES4 BSA, BA2 GNRL, and BA2 DX10 writers use the same helper-owned overwrite path

#### Scenario: Overwrite publication fails
- **WHEN** the helper cannot publish the completed temporary archive over an existing destination
- **THEN** the helper returns an `io_error` containing the writer-specific diagnostic prefix
- **AND** the helper preserves or restores the previous readable destination when the platform operation allows recovery
- **AND** cleanup of temporary and backup paths is best-effort and does not mask the primary publish error

### Requirement: Writer diagnostics remain format-specific
The shared publish helper SHALL accept a writer-specific diagnostic prefix and include that prefix in publish, temporary-directory, backup, rollback, and cleanup-related errors returned to callers.

#### Scenario: Format-specific publish error
- **WHEN** BA2 DX10 publication fails inside the shared helper
- **THEN** the returned error message identifies the failure as a BA2 DX10 writer publish failure

#### Scenario: BSA publish error
- **WHEN** TES4 BSA publication fails inside the shared helper
- **THEN** the returned error message identifies the failure as a TES4 BSA writer publish failure
