## Why

Writer output publication relies on Windows rename/replace behavior that is safest when the temporary file and destination are on the same local volume. The current implementation already narrows several hazards, but caller-facing documentation and edge-case tests should make the remaining host-filesystem expectations explicit.

## What Changes

- Document writer publication expectations for same-volume temporary publication, reparse points, network shares, non-regular destinations, and failure semantics.
- Add regression coverage for directory destinations and read-only existing files so the shared publish helper does not replace or corrupt unsupported targets.
- Add reparse-point refusal coverage when the Windows CI environment can create the required test path without elevated or unavailable privileges.
- Preserve the existing writer publish API and archive serialization behavior.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `writer-safe-publish`: Extends the shared writer publication contract with documented filesystem boundary expectations and targeted tests for unsupported destination kinds.

## Impact

- Affected source files: `src/detail/writer_publish.cpp`, `src/detail/atomic_file_ops.hpp`, `src/formats/bsa/tes3_bsa_writer.cpp`, `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`, and `src/formats/ba2/ba2_dx10_writer.cpp`.
- Affected docs: `docs/target-format-guide.md` and/or public writer API documentation.
- Affected tests: shared writer publish helper tests and focused writer save-path tests for invalid or unsupported destination targets.
- Public API impact: no signature changes expected; documentation clarifies host-filesystem constraints and failure behavior.
- Dependency impact: none.
