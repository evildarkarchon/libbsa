## ADDED Requirements

### Requirement: Pack a host directory into a new archive

The `pack` subcommand SHALL create a new archive at a caller-specified output path from the regular files contained in a caller-specified input directory, recursing into subdirectories. Each packed file MUST be added through the libbsa writer selected by `--format`. The subcommand MUST require both the input directory and the output path, and MUST report a usage error when either is missing.

#### Scenario: Directory tree is packed
- **WHEN** the user runs `pack` with an existing input directory, an output path, and a valid `--format`
- **THEN** the CLI creates an archive at the output path containing one entry per regular file found under the input directory

#### Scenario: Missing required argument
- **WHEN** the user runs `pack` without an input directory or without an output path
- **THEN** the CLI reports a usage error and does not create an archive

#### Scenario: Input directory does not exist
- **WHEN** the user runs `pack` with an input directory that does not exist
- **THEN** the CLI reports an operational failure and exits with the failure code without creating an output archive

### Requirement: Explicit writer and target selection via format

The `pack` subcommand SHALL select the writer family and target profile solely from the `--format` token and MUST NOT infer the target from the output file extension or file contents. The selectable set MUST cover the TES3 BSA writer, each TES4-family BSA target (Oblivion, Fallout 3, Skyrim SE), each BA2 GNRL target (Fallout 4, Starfield v2, Starfield v3), and each BA2 DX10 target (Fallout 4, Starfield v3). A `pack` invocation without `--format` MUST be rejected as a usage error.

#### Scenario: Missing format is rejected
- **WHEN** the user runs `pack` without `--format`
- **THEN** the CLI reports a usage error and does not create an archive

#### Scenario: TES3 BSA target
- **WHEN** the user packs with the TES3 BSA format token
- **THEN** the CLI uses the TES3 BSA writer to produce a raw/uncompressed Morrowind archive

#### Scenario: TES4-family BSA target
- **WHEN** the user packs with a TES4-family format token (Oblivion, Fallout 3, or Skyrim SE)
- **THEN** the CLI uses the TES4 BSA writer configured for the matching target profile

#### Scenario: BA2 GNRL target
- **WHEN** the user packs with a BA2 GNRL format token (Fallout 4, Starfield v2, or Starfield v3)
- **THEN** the CLI uses the BA2 GNRL writer configured for the matching target profile

#### Scenario: BA2 DX10 target
- **WHEN** the user packs with a BA2 DX10 format token (Fallout 4 or Starfield v3)
- **THEN** the CLI uses the BA2 DX10 writer configured for the matching target profile

### Requirement: Archive-internal path mapping

When packing, the CLI SHALL derive each entry's archive-internal path from the file's path relative to the input directory, joining components with `/` and preserving the file name. The CLI MUST NOT embed host-absolute path prefixes or the input directory's own name into archive-internal paths.

#### Scenario: Relative path becomes archive path
- **WHEN** a file at `<input>/textures/stone.dds` is packed from input directory `<input>`
- **THEN** the entry is added with archive-internal path `textures/stone.dds`

#### Scenario: Host absolute prefix is excluded
- **WHEN** files are packed from an absolute input directory path
- **THEN** no entry's archive-internal path contains the host-absolute prefix of the input directory

### Requirement: Pack compression and overwrite options

The `pack` subcommand SHALL expose the archive-wide compression policy and existing-output overwrite behavior offered by the selected writer. By default the subcommand MUST NOT overwrite an existing output path, and MUST require an explicit overwrite option to replace it. Compression options that the selected writer does not support MUST be reported as usage errors rather than silently ignored.

#### Scenario: Refuse to overwrite by default
- **WHEN** the user runs `pack` to an output path that already exists without requesting overwrite
- **THEN** the CLI reports an operational failure and leaves the existing file unchanged

#### Scenario: Overwrite when explicitly requested
- **WHEN** the user runs `pack` with the overwrite option to an existing output path
- **THEN** the CLI replaces the file with the newly written archive

#### Scenario: Unsupported compression option for the target
- **WHEN** the user requests a compression policy the selected `--format` writer does not support
- **THEN** the CLI reports a usage error and does not create an archive

### Requirement: DX10 packing requires DDS inputs

When the selected `--format` is a BA2 DX10 target, the `pack` subcommand SHALL add inputs as DDS texture files through the DX10 writer. Inputs that the DX10 writer rejects as invalid DDS data MUST be surfaced as operational failures with the libbsa diagnostic rather than silently skipped.

#### Scenario: Valid DDS files are packed into a DX10 archive
- **WHEN** the user packs a directory of valid DDS files with a BA2 DX10 format token
- **THEN** the CLI produces a BA2 DX10 texture archive containing those textures

#### Scenario: Invalid DDS input is reported
- **WHEN** a file added under a BA2 DX10 format token is not valid DDS data
- **THEN** the CLI reports the libbsa diagnostic and exits with the failure code
