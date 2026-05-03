## MODIFIED Requirements

### Requirement: TES4-family BSA detection
libbsa SHALL detect TES4-family BSA archives by `BSA\0` magic bytes and supported versions `0x67`, `0x68`, and `0x69`. The detection logic SHALL first check for TES3 magic (`0x00000100`) and route to the TES3 parser before falling through to TES4-family version checks.

#### Scenario: Supported BSA version is opened
- **WHEN** a caller opens a `BSA\0` archive with version `0x67`, `0x68`, or `0x69`
- **THEN** libbsa reports the archive as TES4, FO3-family, or SSE-family respectively and makes the read API available.

#### Scenario: Unsupported BSA version is opened
- **WHEN** a caller opens a `BSA\0` archive with any other version
- **THEN** libbsa returns an unsupported-format error.

#### Scenario: TES3 magic is not misidentified as TES4
- **WHEN** a caller opens a file whose first 4 bytes are `0x00000100` (TES3 magic)
- **THEN** libbsa does NOT enter the TES4 detection path and instead routes to TES3 parsing.
