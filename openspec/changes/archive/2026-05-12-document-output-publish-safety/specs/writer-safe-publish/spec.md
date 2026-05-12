## ADDED Requirements

### Requirement: Writer publish filesystem boundaries are documented and enforced
Archive writers SHALL document the host-filesystem assumptions behind final output publication and SHALL refuse unsupported destination shapes before replacing caller-owned data when those shapes are detectable by the writer publish helper. The documented contract SHALL state that libbsa creates a writer-owned temporary directory beside the destination for normal same-volume publication, no-overwrite publication fails if the destination exists at publish time, overwrite publication is intended for existing regular files, and network filesystems or reparse-point providers can expose platform-specific failure behavior.

#### Scenario: Writer publish constraints are documented
- **WHEN** a consumer reads the target-format guide or public writer API documentation
- **THEN** the documentation describes same-directory temporary output publication, no-overwrite failure behavior, overwrite regular-file expectations, reparse-point refusal expectations, and network filesystem caveats
- **AND** the documentation states that publish failures are returned as `io_error` results instead of exposing a partial archive at the destination path

#### Scenario: Existing directory overwrite target is refused
- **WHEN** a writer is asked to save with overwrite enabled to an existing directory path
- **THEN** the save fails with an `io_error` before replacing the destination
- **AND** the directory remains a directory owned by the caller

#### Scenario: Read-only overwrite target is preserved on publish failure
- **WHEN** a writer is asked to save with overwrite enabled to an existing read-only regular file and the host filesystem denies replacement
- **THEN** the save fails with an `io_error` containing the writer-specific diagnostic prefix
- **AND** the previous destination remains readable with its original bytes
- **AND** writer-owned temporary output is cleaned up on a best-effort basis

#### Scenario: Detectable reparse-point overwrite target is refused
- **WHEN** a writer is asked to save with overwrite enabled to an existing destination path that the Windows host reports as a reparse point
- **THEN** the save fails with an `io_error` before publication
- **AND** the reparse point and its target remain unmodified
