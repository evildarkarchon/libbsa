## 1. Documentation

- [x] 1.1 Document writer output publication constraints in `docs/target-format-guide.md`, including same-directory temporary publication, overwrite regular-file expectations, no-overwrite failure behavior, reparse-point refusal, network filesystem caveats, and `io_error` failure semantics.
- [x] 1.2 Update public writer API documentation if needed so `write_to` callers can find the same host-filesystem publish constraints without reading implementation details.

## 2. Publish Helper Policy

- [x] 2.1 Add a private Windows reparse-point detection helper in the shared writer publish implementation without exposing Win32 types in public headers.
- [x] 2.2 Refuse detectable reparse-point overwrite destinations before final publication while preserving writer-specific diagnostic prefixes and existing regular-file validation.
- [x] 2.3 Preserve current no-overwrite, overwrite-regular-file, and writer-owned temporary-directory cleanup behavior while adding the reparse-point refusal path.

## 3. Regression Coverage

- [x] 3.1 Extend shared `writer_publish` tests for read-only existing destination failure, original-byte preservation, and writable-attribute cleanup after the test.
- [x] 3.2 Extend shared `writer_publish` tests for conditional Windows reparse-point refusal, verifying both the reparse point and its target remain unmodified when the fixture can be created.
- [x] 3.3 Fill any missing directory-target coverage so unsupported directory destinations are covered at the shared helper boundary and by writer-family save paths where delegation alone is insufficient.
- [x] 3.4 Update or add documentation policy tests so the target-format guide retains the new writer publication safety guidance.

## 4. Validation

- [x] 4.1 Build the test target with `cmake --build --preset windows-msvc-debug-static --target libbsa_tests --config Debug`.
- [x] 4.2 Run focused writer publish and documentation policy tests with the Windows Debug static CTest configuration.
- [x] 4.3 Run the full Windows Debug static CTest preset with output on failure when the preset is available.
- [x] 4.4 Run `openspec validate document-output-publish-safety --strict`.
