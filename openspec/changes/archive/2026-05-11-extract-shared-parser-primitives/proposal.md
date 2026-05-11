## Why

Parser hardening fixes are currently easy to miss because the same checked arithmetic, bounded file-read, byte-to-string, and separator-normalization helpers are copied into each archive-family parser. Centralizing those primitives now reduces the risk that a malformed-input or allocation-safety fix lands in one parser while TES3, TES4, BA2 GNRL, or BA2 DX10 keeps the old behavior.

## What Changes

- Add a shared internal parser primitive layer for checked size arithmetic, span validation, bounded host-file reads, byte-span string materialization, and archive display-separator normalization.
- Refactor TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 parsers to use the shared primitives for common hardening paths.
- Preserve format-specific compatibility checks, layout parsing, hashing, and archive-family error messages in the format modules.
- Extend allocation/error coverage so archive-controlled string materialization and bounded file reads continue to return `result<T>` failures instead of leaking allocation exceptions.
- Add focused regression coverage proving all parser families still reject malformed spans and translate allocation-sensitive parser operations through the shared result-based path.

## Capabilities

### New Capabilities
- `shared-parser-primitives`: Internal parser helpers for reusable checked arithmetic, bounded file reads, allocation-safe archive string materialization, and display-separator normalization.

### Modified Capabilities
- `allocation-error-translation`: Extend parser allocation-error requirements to cover shared parser file-read and string-materialization helpers used by all archive-family parsers.

## Impact

- Affected implementation files include `src/detail/binary_io.*`, a new internal parser primitive module, and the parser translation units under `src/formats/bsa/` and `src/formats/ba2/`.
- Affected tests include unit and malformed-fixture coverage for TES3 BSA, TES4-family BSA, BA2 GNRL, BA2 DX10, and any new helper-level tests.
- No public libbsa API, dependency, vcpkg, CMake preset, or TES5Edit submodule change is expected.
