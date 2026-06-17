## Purpose

Specify CLI archive inspection behavior for listing entries, displaying archive metadata, and validating archives with structured diagnostics, compatibility warnings, and strict-mode failure handling.

## Requirements

### Requirement: List archive entries

The `list` subcommand SHALL open a caller-specified archive and print its entries in the deterministic order returned by `archive_reader::entries`, one entry per line. Each line MUST include the entry's canonical archive path, and the subcommand MUST expose an option to include per-entry size and compression details.

#### Scenario: Entries are listed deterministically
- **WHEN** the user runs `list` on a valid archive
- **THEN** the CLI prints one line per entry in canonical archive-path order

#### Scenario: Detailed listing includes sizes and compression
- **WHEN** the user runs `list` with the detail option
- **THEN** each line additionally includes the entry's raw size, stored size, and compression representation

#### Scenario: Listing an unsupported file fails
- **WHEN** the user runs `list` on a path that is not a supported archive
- **THEN** the CLI reports the libbsa diagnostic and exits with the failure code

### Requirement: Show archive metadata

The `info` subcommand SHALL open a caller-specified archive and print its archive-level metadata from `archive_reader::metadata`, including the container type, archive variant, version, archive flags, file count, and default compression. For BA2 archives the CLI MUST also print the BA2-specific metadata fields that are present.

#### Scenario: Archive header summary is printed
- **WHEN** the user runs `info` on a valid archive
- **THEN** the CLI prints the container type, variant, version, archive flags, file count, and default compression

#### Scenario: BA2 metadata fields are shown when present
- **WHEN** the user runs `info` on a BA2 archive whose metadata includes BA2-specific fields
- **THEN** the CLI prints the present BA2 metadata fields and omits absent optional fields

### Requirement: Validate an archive

The `validate` subcommand SHALL run `validate_archive` on a caller-specified archive and print the resulting structured diagnostics and compatibility warnings, reporting overall validity. When the report indicates the archive is invalid, the subcommand MUST exit with a failure code; when the archive is valid (even with advisory or risky warnings) it MUST exit successfully unless the user opts to treat warnings as failures.

#### Scenario: Valid archive reports success
- **WHEN** the user runs `validate` on an archive that produces a valid report
- **THEN** the CLI prints the result, including any warnings, and exits with the success code

#### Scenario: Invalid archive reports failure
- **WHEN** the user runs `validate` on an archive whose report contains fatal diagnostics
- **THEN** the CLI prints each diagnostic with its error category and exits with the failure code

#### Scenario: Warnings are surfaced with code and severity
- **WHEN** a validation report contains compatibility warnings
- **THEN** the CLI prints each warning's stable code, severity, message, and any associated archive path

#### Scenario: Optional strict mode escalates warnings
- **WHEN** the user runs `validate` with the strict option on an otherwise-valid archive that carries warnings
- **THEN** the CLI exits with the failure code
