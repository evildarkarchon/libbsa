## ADDED Requirements

### Requirement: BA2 DX10 snapshot-backed chunks are assembled through a batched path
The BA2 DX10 writer SHALL assemble each planned texture chunk through an internal batched snapshot-read path that collects the required subresources for that chunk, validates their aggregate size against the planned raw chunk size, and prepares the raw chunk buffer before copying snapshot bytes.

#### Scenario: Planned chunk reserves the raw byte count before snapshot copy
- **WHEN** a BA2 DX10 writer prepares a planned texture chunk from snapshot-backed subresources
- **THEN** the raw chunk buffer is pre-sized or pre-reserved using the planned raw chunk size before payload bytes are appended
- **AND** the final raw chunk byte count matches the planned raw chunk size before compression is invoked

#### Scenario: Planned chunk reads snapshots in BA2-required subresource order
- **WHEN** a planned BA2 DX10 chunk spans multiple mip subresources for the same array item and face
- **THEN** the writer copies snapshot payload bytes in ascending planned mip order for that chunk
- **AND** the emitted archive's extracted texture payload bytes remain equivalent to the source DDS image payload bytes

#### Scenario: Snapshot size mismatch prevents archive publication
- **WHEN** the snapshot subresources selected for a planned BA2 DX10 chunk no longer sum to the planned raw chunk size
- **THEN** `write_to` returns a failed `result` without publishing a BA2 archive
- **AND** the failure uses libbsa result-based format or I/O error semantics rather than throwing allocation or stream exceptions

### Requirement: BA2 DX10 snapshot ownership semantics are preserved
The BA2 DX10 writer SHALL continue to own add-time DDS snapshot bytes after `add_file`, so later caller changes to the original DDS host file do not affect `write_to` output.

#### Scenario: Source DDS mutation after add does not affect output
- **WHEN** a caller successfully adds a DDS source to a BA2 DX10 writer and then mutates or deletes the original host file before `write_to`
- **THEN** `write_to` uses the writer-owned snapshot bytes captured during `add_file`
- **AND** the extracted texture payload bytes match the add-time DDS image payload bytes, not the later host-file state

#### Scenario: Snapshot cleanup ownership remains unchanged
- **WHEN** a BA2 DX10 writer with staged snapshot-backed entries is destroyed after this optimization
- **THEN** writer state remains responsible for best-effort cleanup of the reserved snapshot temporary directory
- **AND** cleanup failure does not mask any previously returned writer operation result

### Requirement: BA2 DX10 benchmarks expose snapshot overhead
The benchmark executable SHALL include BA2 DX10 measurements that make add-time snapshot staging and final archive writing costs visible while retaining correctness checks.

#### Scenario: Benchmark reports DX10 add-time staging cost
- **WHEN** `libbsa_benchmarks` runs the BA2 DX10 pack scenario
- **THEN** the reported results include a measurement for adding the DDS source and staging snapshot bytes
- **AND** the measurement identifies the BA2 DX10 scenario and worker-count context clearly enough to compare benchmark runs

#### Scenario: Benchmark reports DX10 write finalization cost
- **WHEN** `libbsa_benchmarks` writes the BA2 DX10 archive after snapshot staging
- **THEN** the reported results include a measurement for the final `write_to` and validation path
- **AND** benchmark correctness checks still verify reopened texture metadata and extracted DDS payload shape
