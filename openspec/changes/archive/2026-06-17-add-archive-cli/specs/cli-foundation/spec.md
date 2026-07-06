## ADDED Requirements

### Requirement: Command-line executable and subcommand dispatch

The CLI SHALL be provided as a single executable that dispatches to the subcommands `pack`, `unpack`, `list`, `info`, and `validate`. The executable MUST treat the first positional argument as the subcommand name and route remaining arguments to that subcommand. Invoking the executable with no subcommand MUST print top-level usage and exit with the usage-error code.

#### Scenario: Known subcommand is dispatched
- **WHEN** the executable is invoked with a known subcommand name as the first argument
- **THEN** that subcommand handles the remaining arguments

#### Scenario: Unknown subcommand is rejected
- **WHEN** the executable is invoked with an unrecognized first argument that is not a recognized global option
- **THEN** the CLI prints a diagnostic naming the unknown subcommand and exits with the usage-error code

#### Scenario: No subcommand prints usage
- **WHEN** the executable is invoked with no arguments
- **THEN** the CLI prints top-level usage listing the available subcommands and exits with the usage-error code

### Requirement: Global help and version output

The CLI SHALL support `--help`/`-h` and `--version`/`-V`. Top-level `--help` MUST list all subcommands; subcommand `--help` MUST describe that subcommand's options. `--version` MUST print a version string derived from the libbsa version. Requesting help or version MUST exit with the success code.

#### Scenario: Top-level help lists subcommands
- **WHEN** the user runs the CLI with `--help`
- **THEN** the output lists `pack`, `unpack`, `list`, `info`, and `validate` and the process exits successfully

#### Scenario: Subcommand help describes its options
- **WHEN** the user runs a subcommand with `--help`
- **THEN** the output describes that subcommand's options and the process exits successfully

#### Scenario: Version reflects the library version
- **WHEN** the user runs the CLI with `--version`
- **THEN** the output contains the libbsa version and the process exits successfully

### Requirement: Format taxonomy is explicit and discoverable

The CLI SHALL define a stable set of `--format` tokens, each mapping to exactly one supported writer and target profile across the TES3, TES4-family, BA2 GNRL, and BA2 DX10 writers. The supported tokens MUST be discoverable through CLI help. An unrecognized `--format` token MUST be rejected without attempting to create an archive.

#### Scenario: Each supported writer and target is selectable
- **WHEN** the user supplies any documented `--format` token
- **THEN** the CLI selects the corresponding writer and target profile for the operation

#### Scenario: Unknown format token is rejected
- **WHEN** the user supplies a `--format` token that is not in the documented set
- **THEN** the CLI prints a diagnostic listing the valid tokens and exits with the usage-error code without writing any output archive

#### Scenario: Format tokens are discoverable
- **WHEN** the user requests help for the `pack` subcommand
- **THEN** the output enumerates the supported `--format` tokens

### Requirement: Library errors are mapped to human-readable diagnostics

The CLI SHALL render every `libbsa::error` returned by the library as a human-readable diagnostic on the standard error stream that includes the stable `error_code` category and the library's diagnostic message. The CLI MUST NOT allow a `libbsa` result error to surface as an unhandled C++ exception or crash.

#### Scenario: Operational failure is reported on stderr
- **WHEN** a libbsa API returns an `error` during a subcommand
- **THEN** the CLI writes a diagnostic to standard error containing the error category and message, and exits with a failure code

#### Scenario: Errors do not crash the process
- **WHEN** any libbsa call within a subcommand fails
- **THEN** the CLI terminates through its normal exit path rather than by an uncaught exception or abnormal termination

### Requirement: Stable process exit-code conventions

The CLI SHALL use stable exit-code categories: success returns `0`; usage errors (unknown subcommand, missing required argument, invalid option, or unknown `--format` token) return a dedicated usage-error code distinct from `0`; operational failures originating from libbsa or host I/O return a failure code distinct from both `0` and the usage-error code.

#### Scenario: Success exits zero
- **WHEN** a subcommand completes its requested work without error
- **THEN** the process exits with code `0`

#### Scenario: Usage error exit code
- **WHEN** the invocation is rejected for a usage error
- **THEN** the process exits with the dedicated usage-error code

#### Scenario: Operational failure exit code
- **WHEN** a subcommand fails because of a libbsa or host I/O error
- **THEN** the process exits with the operational-failure code distinct from the usage-error code

### Requirement: Gated, dependency-light build deliverable

The CLI executable SHALL be built only when an explicit CMake option is enabled, and MUST link only `libbsa::libbsa` and the C++ standard library, introducing no new third-party runtime dependency. When the option is disabled, configuring and building the project MUST NOT build the CLI target.

#### Scenario: CLI builds when the option is enabled
- **WHEN** the project is configured with the CLI build option enabled
- **THEN** the CLI executable target is built and links `libbsa::libbsa`

#### Scenario: CLI is excluded when the option is disabled
- **WHEN** the project is configured with the CLI build option disabled
- **THEN** the CLI target is not built and the rest of the project still builds

#### Scenario: No new third-party dependency is introduced
- **WHEN** the CLI target's link dependencies are inspected
- **THEN** they consist only of `libbsa::libbsa` and standard-library/system libraries already required by libbsa
