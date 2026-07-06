## Purpose

Specify CLI worker-count controls for parallel `pack` and `unpack` execution, including accepted values, validation, library invocation, and help discoverability.

## Requirements

### Requirement: Worker-count control on packing and extraction

The CLI SHALL expose a worker-count control on the `pack` and `unpack` subcommands that selects how many worker threads the operation uses. The control MUST be accepted as `--threads <value>` and as the short alias `-j <value>`. The control MUST be optional; when omitted, the subcommand MUST run with a single worker so the default observable behavior is unchanged from before this capability existed.

#### Scenario: Pack accepts the worker-count control
- **WHEN** the user runs `pack` with `--threads` (or `-j`) set to a valid worker count
- **THEN** the pack operation runs using that number of worker threads and completes the requested archive

#### Scenario: Unpack accepts the worker-count control
- **WHEN** the user runs `unpack` with `--threads` (or `-j`) set to a valid worker count
- **THEN** the extraction runs using that number of worker threads and completes the requested extraction

#### Scenario: Omitting the control runs single-threaded
- **WHEN** the user runs `pack` or `unpack` without specifying the worker-count control
- **THEN** the operation runs with exactly one worker thread

### Requirement: Worker-count value taxonomy and auto resolution

The worker-count control SHALL accept a positive integer count, the literal token `auto`, or `0` as an alias for `auto`. When the value is `auto` or `0`, the CLI MUST resolve it to the host's available hardware concurrency before invoking the library, and MUST resolve to at least `1` worker when the host reports an indeterminate hardware-concurrency value. The CLI MUST perform this resolution itself and MUST NOT pass `auto` or `0` to the library, because the library treats `0` as invalid and never as "auto".

#### Scenario: Positive integer selects that worker count
- **WHEN** the user supplies a positive integer to the worker-count control
- **THEN** the operation uses exactly that many worker threads

#### Scenario: auto resolves to host hardware concurrency
- **WHEN** the user supplies `auto` to the worker-count control
- **THEN** the CLI resolves the count to the host's available hardware concurrency and runs the operation with that many workers

#### Scenario: Zero is an alias for auto
- **WHEN** the user supplies `0` to the worker-count control
- **THEN** the CLI resolves the count to the host's available hardware concurrency (at least `1`) and runs the operation with that many workers, identically to `auto`

#### Scenario: auto resolves to at least one worker
- **WHEN** the user supplies `auto` or `0` and the host reports an indeterminate hardware-concurrency value
- **THEN** the CLI resolves the worker count to `1` and the operation runs with a single worker

### Requirement: Invalid worker-count values are rejected as usage errors

The CLI SHALL reject an invalid worker-count value as a usage error before performing any archive work or writing any output. Invalid values MUST include negative numbers, non-numeric text, empty values, and values that exceed the supported maximum worker count. `0` MUST NOT be treated as invalid; it is an alias for `auto`. The diagnostic MUST be written to standard error and the process MUST exit with the usage-error code.

#### Scenario: Negative value is rejected before work begins
- **WHEN** the user supplies a negative worker count
- **THEN** the CLI writes a usage diagnostic to standard error, exits with the usage-error code, and does not create or modify any archive or extracted file

#### Scenario: Non-numeric value is rejected
- **WHEN** the user supplies a value that is none of: a positive integer, `0`, or `auto`
- **THEN** the CLI writes a usage diagnostic to standard error and exits with the usage-error code

#### Scenario: Out-of-range value is rejected
- **WHEN** the user supplies a worker count greater than the supported maximum
- **THEN** the CLI writes a usage diagnostic to standard error and exits with the usage-error code without starting the operation

### Requirement: Resolved worker count is applied to the underlying operation

The CLI SHALL apply the resolved worker count to the library operation that performs the work. For `pack`, the resolved count MUST be supplied as the write execution worker count for every supported format family (TES3, TES4-family BSA, BA2 GNRL, and BA2 DX10). For `unpack`, the resolved count MUST be supplied as the bulk-extraction worker count. The CLI MUST NOT introduce any threading types into the public library headers.

#### Scenario: Pack forwards the worker count for every format
- **WHEN** the user runs `pack` with a resolved worker count for any supported `--format`
- **THEN** the CLI invokes the corresponding writer with that worker count as its write execution worker count

#### Scenario: Unpack forwards the worker count to bulk extraction
- **WHEN** the user runs `unpack` with a resolved worker count
- **THEN** the CLI invokes bulk extraction with that worker count

### Requirement: Worker-count control is discoverable through help

The worker-count control and its accepted values SHALL be discoverable through subcommand help. `pack --help` and `unpack --help` MUST each describe the control, its short alias, and that it accepts a positive integer or `auto` (with `0` as an alias for `auto`).

#### Scenario: Pack help documents the control
- **WHEN** the user runs `pack --help`
- **THEN** the output describes the worker-count control, its `-j` alias, and that it accepts a positive integer or `auto` (with `0` as an alias for `auto`)

#### Scenario: Unpack help documents the control
- **WHEN** the user runs `unpack --help`
- **THEN** the output describes the worker-count control, its `-j` alias, and that it accepts a positive integer or `auto` (with `0` as an alias for `auto`)
