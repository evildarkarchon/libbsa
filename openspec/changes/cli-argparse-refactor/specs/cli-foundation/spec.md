## MODIFIED Requirements

### Requirement: Gated, dependency-light build deliverable

The CLI executable SHALL be built only when an explicit CMake option is enabled, and MUST link only `libbsa::libbsa`, the C++ standard library, system libraries already required by libbsa, and a single header-only command-line argument-parsing dependency (`argparse`). The argument-parsing dependency MUST be header-only and MUST introduce no new runtime or shared-library dependency. The CLI MUST NOT introduce any additional third-party runtime or shared-library dependency beyond those already required by libbsa. When the option is disabled, configuring and building the project MUST NOT build the CLI target, and the argument-parsing dependency MUST NOT be required to build the rest of the project.

#### Scenario: CLI builds when the option is enabled
- **WHEN** the project is configured with the CLI build option enabled
- **THEN** the CLI executable target is built and links `libbsa::libbsa` and the header-only `argparse` argument-parsing dependency

#### Scenario: CLI is excluded when the option is disabled
- **WHEN** the project is configured with the CLI build option disabled
- **THEN** the CLI target is not built and the rest of the project still builds without requiring the argument-parsing dependency

#### Scenario: No new runtime dependency is introduced
- **WHEN** the CLI target's runtime and shared-library dependencies are inspected
- **THEN** they consist only of `libbsa::libbsa` and the standard-library/system libraries already required by libbsa, and the `argparse` argument parser is consumed as a header-only interface that contributes no runtime or shared-library dependency
