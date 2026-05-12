## ADDED Requirements

### Requirement: BA2 DX10 snapshot directory names use hardened uniqueness
The BA2 DX10 writer SHALL reserve snapshot temporary directories with a name component generated from cryptographically strong randomness or an equivalent Windows temporary-name reservation mechanism. Snapshot directory naming SHALL NOT rely on a process-local monotonic counter, timestamp, process identifier, or other predictable value as the only uniqueness source.

#### Scenario: Snapshot directory reservation uses unpredictable candidates
- **WHEN** a BA2 DX10 writer first needs a snapshot temporary directory during `add_file`
- **THEN** the candidate directory name includes fresh cryptographically strong randomness or is produced by an equivalent Windows temporary-name reservation mechanism
- **AND** the reservation path does not fall back to counter-only `libbsa-dx10-snapshot-<number>` names

### Requirement: BA2 DX10 snapshot directory reservation remains atomic
The BA2 DX10 writer SHALL only use a snapshot temporary directory after it has successfully reserved that exact path by creating the directory. Candidate name collisions SHALL be retried with a new candidate, and repeated reservation failures or random-source failures SHALL return a failed `result` without staging partial writer state.

#### Scenario: Existing candidate path is not reused
- **WHEN** snapshot directory reservation encounters a candidate path that already exists
- **THEN** the writer retries with a different freshly generated candidate
- **AND** it does not write snapshot files into the pre-existing path

#### Scenario: Reservation failure preserves result semantics
- **WHEN** the Windows random source or directory reservation fails before a snapshot directory is reserved
- **THEN** `add_file` returns a failed `result` with an `io_error`
- **AND** no BA2 DX10 writer entry is staged from that failed attempt

### Requirement: BA2 DX10 snapshot cleanup ownership remains unchanged
The BA2 DX10 writer SHALL keep writer state as the owner of the reserved snapshot temporary directory and SHALL remove that directory on writer state teardown using best-effort cleanup. Hardening snapshot directory naming SHALL NOT change public writer APIs, archive bytes, compression routing, add-time DDS validation, move-ownership semantics, or overwrite behavior.

#### Scenario: Writer teardown cleans reserved snapshots
- **WHEN** a BA2 DX10 writer with staged snapshot-backed entries is destroyed
- **THEN** the reserved snapshot temporary directory is removed on a best-effort basis
- **AND** cleanup failure does not mask any previously returned writer operation result

#### Scenario: Existing archive behavior remains stable
- **WHEN** BA2 DX10 writer fixture and round-trip tests run after snapshot directory hardening
- **THEN** emitted archive bytes, compression metadata, extracted payload bytes, and validation errors remain unchanged for existing covered cases
