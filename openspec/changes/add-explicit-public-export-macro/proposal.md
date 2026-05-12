## Why

libbsa currently relies on CMake's Windows auto-export behavior for shared builds, which exposes implementation symbols alongside the intended public C++ API. This should be tightened before shared-library consumers start depending on accidental exports and before the project implies a stable binary boundary it has not explicitly chosen.

## What Changes

- Add an explicit public export/import macro for libbsa's installed C++ headers.
- Mark the supported public classes, structs, enums, and free functions that form the shared-library surface with that macro.
- Disable broad Windows auto-export behavior for the `libbsa` target once the public headers carry explicit annotations.
- Add validation that shared builds export intended public symbols and do not expose private implementation symbols.
- Preserve C++20 source compatibility and avoid introducing a binary ABI stability promise beyond the explicitly exported surface.

## Capabilities

### New Capabilities
- `shared-library-export-surface`: Defines libbsa's explicit Windows shared-library export contract and verifies that internal implementation symbols remain private.

### Modified Capabilities

None.

## Impact

- Affected files include `CMakeLists.txt`, public headers under `include/libbsa/`, and shared-build-focused tests or validation scripts.
- Dynamic-library consumers will link against an intentional exported API instead of CMake-discovered implementation symbols.
- Static-library builds and existing C++20 source-level includes should continue to compile without caller-side changes.
- No new runtime dependency is required.
