## 1. Export Macro and Build Wiring

- [ ] 1.1 Add `include/libbsa/export.hpp` with a Doxygen-documented `LIBBSA_API` macro for shared producer, shared consumer, and static build modes.
- [ ] 1.2 Add the export header to the public header file set so build-tree and installed consumers receive it.
- [ ] 1.3 Add target compile definitions that set the shared-library producer macro privately for shared builds and propagate the static-library macro publicly for static builds.
- [ ] 1.4 Remove `WINDOWS_EXPORT_ALL_SYMBOLS` from the `libbsa` target after explicit annotations are available.

## 2. Public API Annotations

- [ ] 2.1 Include the export header from public headers that declare emitted shared-library entry points.
- [ ] 2.2 Annotate public reader, writer, sink/factory interface, validation, and free-function entry points that require exported symbols from the DLL.
- [ ] 2.3 Review `result.hpp` and public metadata value types to keep header-only templates and plain aggregates source-compatible without unnecessary export decoration.
- [ ] 2.4 Confirm public header include order remains stable through `include/libbsa/libbsa.hpp` and direct includes of `archive.hpp`, `writer.hpp`, `validation.hpp`, and `result.hpp`.

## 3. Export Surface Validation

- [ ] 3.1 Add a shared-build consumer/link smoke check that calls representative reader, writer, and validation APIs through `libbsa::libbsa`.
- [ ] 3.2 Add a Windows shared-DLL export inspection check that asserts representative public symbols are exported.
- [ ] 3.3 Extend the export inspection check to fail if known private implementation namespaces or writer-stage/helper symbols appear in the DLL export table.
- [ ] 3.4 Add or update a build-system regression check proving `WINDOWS_EXPORT_ALL_SYMBOLS` is not enabled for the `libbsa` target.

## 4. Verification

- [ ] 4.1 Build and run the focused export-surface checks for a Windows shared preset.
- [ ] 4.2 Build and run the focused public include/consumer checks for a Windows static preset.
- [ ] 4.3 Run the existing Windows static test suite to confirm archive behavior is unchanged.
- [ ] 4.4 Run `git diff --check` and review the OpenSpec status for apply readiness.
