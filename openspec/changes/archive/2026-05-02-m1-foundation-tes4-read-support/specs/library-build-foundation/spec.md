## ADDED Requirements

### Requirement: Static library project layout
libbsa SHALL provide a C++20 static-library project layout with public headers under `include/`, implementation sources under `src/`, and automated tests under `tests/`.

#### Scenario: Configure and build static library
- **WHEN** the project is configured and built through the repo CMake presets
- **THEN** CMake builds a static `libbsa` library target and exposes the `libbsa::libbsa` alias for consumers.

### Requirement: Public headers are reusable
Public libbsa headers SHALL define the milestone 1 archive-read API without including compression-library, test-framework, application UI, or TES5Edit headers.

#### Scenario: Consumer includes public API
- **WHEN** a downstream C++20 target includes the primary libbsa archive header
- **THEN** the target can compile against libbsa read types without directly depending on libdeflate, lz4, Catch2, or TES5Edit source files.

### Requirement: Dependency management is explicit
The build SHALL use vcpkg-managed dependencies for milestone 1 compression and tests, including libdeflate for deflate, the official lz4 library for LZ4 frame support, and a documented test framework.

#### Scenario: Dependencies restore through manifest mode
- **WHEN** the project is configured with vcpkg manifest mode
- **THEN** the required milestone 1 build and test dependencies are resolved by `vcpkg.json` without ad hoc system-library discovery.

### Requirement: Read API reports ordinary failures without exceptions
The public read API SHALL return typed errors for ordinary I/O, unsupported-format, malformed-archive, missing-file, and decompression failures.

#### Scenario: Unsupported archive is opened
- **WHEN** a caller opens a file whose magic or version is outside milestone 1 support
- **THEN** the API returns an unsupported-format error instead of constructing a partially usable archive reader.

### Requirement: Public APIs are documented
Public classes, functions, records, and enums added for milestone 1 SHALL have Doxygen-compliant comments that describe purpose, return semantics, and relevant lifetime or error contracts.

#### Scenario: Read API header is inspected
- **WHEN** a maintainer opens the public archive-read header
- **THEN** each public milestone 1 API entry point has a concise Doxygen comment.

### Requirement: Test harness is repo-native
The project SHALL provide CTest-discoverable automated tests for milestone 1 behavior without mutating or compiling source from the `TES5Edit/` reference submodule.

#### Scenario: Tests are executed
- **WHEN** the milestone test target is built and CTest is run
- **THEN** the tests exercise libbsa code and fixtures outside `TES5Edit/`.
