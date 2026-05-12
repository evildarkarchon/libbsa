## ADDED Requirements

### Requirement: Public headers define an explicit DLL export macro
libbsa SHALL provide a public `LIBBSA_API` macro that is available from installed headers and selects Windows shared-library export, shared-library import, or no decoration for static builds.

#### Scenario: Shared library build exports public API declarations
- **WHEN** libbsa is built as a shared library
- **THEN** compilation units that build the `libbsa` target see `LIBBSA_API` as a Windows export decoration
- **AND** consumers that include installed public headers through `libbsa::libbsa` see `LIBBSA_API` as a Windows import decoration

#### Scenario: Static library build keeps public headers undecorated
- **WHEN** libbsa is built as a static library
- **THEN** the `libbsa::libbsa` target propagates the static-library compile definition required by the public export header
- **AND** consumers that include public headers through that target see `LIBBSA_API` as an empty decoration

### Requirement: Shared builds export only the intended public entry points
libbsa shared builds SHALL export the supported public reader, writer, validation, and sink/factory interface entry points through explicit `LIBBSA_API` annotations instead of CMake auto-export discovery.

#### Scenario: Public API symbols are exported
- **WHEN** a Windows shared-library build produces the `libbsa` DLL
- **THEN** the DLL export table includes representative public symbols for `archive_reader`, TES3/TES4/BSA/BA2 writer classes, `validation_report::is_valid`, and `validate_archive`
- **AND** a downstream consumer linked to `libbsa::libbsa` can compile and link calls to those representative APIs

#### Scenario: Auto-export is disabled
- **WHEN** the `libbsa` CMake target is configured
- **THEN** `WINDOWS_EXPORT_ALL_SYMBOLS` is not enabled for the target
- **AND** explicit public-header annotations are the mechanism that exports shared-library API symbols

### Requirement: Internal implementation symbols remain private
libbsa shared builds SHALL keep implementation details private, including parser internals, codec wrappers, texture analyzer functions, writer-stage helpers, and detail namespace utilities.

#### Scenario: Private implementation symbols are absent from exports
- **WHEN** the Windows shared-library export table is inspected
- **THEN** symbols from `libbsa::detail`, `libbsa::formats`, codec implementation helpers, texture analyzer internals, and writer-stage entry points are not exported
- **AND** consumers cannot link directly against those implementation details through the DLL import library

### Requirement: Export annotations preserve source compatibility
Adding explicit export annotations SHALL preserve existing C++20 source compatibility for public headers, static builds, and shared builds.

#### Scenario: Public aggregate metadata stays source-compatible
- **WHEN** consumer code includes `libbsa/libbsa.hpp` and declares public metadata values such as `archive_metadata`, `entry_metadata`, `validation_report`, or writer option structs
- **THEN** that code continues to compile without requiring new include ordering or consumer-defined macros

#### Scenario: Existing archive behavior is unchanged
- **WHEN** the existing reader, writer, validation, compression, fixture, and compatibility tests run after the export-surface change
- **THEN** archive parsing, writing, extraction, validation, and diagnostic behavior remains unchanged
