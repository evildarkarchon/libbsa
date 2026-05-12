## MODIFIED Requirements

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
