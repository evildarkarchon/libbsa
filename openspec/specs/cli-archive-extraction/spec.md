## Purpose

Specify CLI archive extraction behavior for unpacking archives to host directories, including full and selective extraction, safe destination path handling, and overwrite control.

## Requirements

### Requirement: Unpack an archive to a host directory

The `unpack` subcommand SHALL open a caller-specified archive through `archive_reader` and extract its entries into a caller-specified output directory, recreating each entry's archive-internal path as a relative path beneath the output directory and creating intermediate directories as needed. The subcommand MUST require both the archive path and the output directory.

#### Scenario: Full archive extraction
- **WHEN** the user runs `unpack` with a valid archive and an output directory
- **THEN** every archive entry is written to a file at its archive-internal path beneath the output directory

#### Scenario: Intermediate directories are created
- **WHEN** an entry's archive-internal path contains directory components that do not yet exist under the output directory
- **THEN** the CLI creates those directories before writing the entry's file

#### Scenario: Missing required argument
- **WHEN** the user runs `unpack` without an archive path or without an output directory
- **THEN** the CLI reports a usage error and does not extract anything

#### Scenario: Unreadable or unsupported archive
- **WHEN** the user runs `unpack` on a path that is missing, unreadable, or not a supported archive
- **THEN** the CLI reports the libbsa diagnostic and exits with the failure code

### Requirement: Selective extraction by archive path

The `unpack` subcommand SHALL support extracting a caller-specified subset of entries identified by archive path in addition to extracting the whole archive. A requested path that does not exist in the archive MUST be reported as a per-entry failure without aborting extraction of the other requested entries.

#### Scenario: Extract a single named entry
- **WHEN** the user runs `unpack` requesting one specific archive path that exists
- **THEN** only that entry is written beneath the output directory

#### Scenario: Requested path not found
- **WHEN** the user requests an archive path that does not exist alongside paths that do
- **THEN** the CLI reports the missing path as a failure, still extracts the existing requested paths, and exits with the failure code

### Requirement: Extraction output path safety

The `unpack` subcommand SHALL write extracted files only within the output directory. Before writing, the CLI MUST reject any entry whose archive-internal path would resolve outside the output directory (for example via parent-directory traversal or an absolute path), reporting it as a failure rather than writing outside the tree.

#### Scenario: Traversal path is refused
- **WHEN** an archive entry's path would resolve outside the chosen output directory after normalization
- **THEN** the CLI refuses to write that entry, reports it as a failure, and writes no file outside the output directory

#### Scenario: Normal entry stays inside the tree
- **WHEN** an archive entry's path is a normal relative path
- **THEN** the extracted file is created at the corresponding location beneath the output directory

### Requirement: Extraction overwrite control

By default the `unpack` subcommand MUST NOT overwrite existing files in the output directory, and MUST require an explicit overwrite option to replace them. When overwrite is not enabled and a destination file already exists, the CLI MUST report that entry as a failure and leave the existing file unchanged.

#### Scenario: Existing file is preserved by default
- **WHEN** an entry would be written to a destination that already exists and overwrite is not enabled
- **THEN** the CLI reports that entry as a failure and leaves the existing file unchanged

#### Scenario: Overwrite replaces existing files
- **WHEN** the user runs `unpack` with the overwrite option and a destination already exists
- **THEN** the CLI replaces the existing file with the extracted content
