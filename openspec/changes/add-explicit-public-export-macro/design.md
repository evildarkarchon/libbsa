## Context

The root `CMakeLists.txt` currently sets `WINDOWS_EXPORT_ALL_SYMBOLS ON` for the `libbsa` target. That keeps shared builds easy to link while the library is young, but it also asks CMake to discover and export implementation symbols from every translation unit. The public headers in `include/libbsa/` do not yet identify which declarations are intentionally exported from a Windows DLL.

libbsa is Windows-only, C++20, and must support both static and shared builds. The fix should improve shared-library discipline without changing archive behavior, adding dependencies, or claiming long-term binary ABI stability.

## Goals / Non-Goals

**Goals:**

- Replace broad Windows auto-export behavior with an explicit `LIBBSA_API` public export/import macro.
- Keep the macro usable from installed headers by making static/shared and producer/consumer compile definitions flow through CMake targets.
- Annotate the intended public shared-library entry points while keeping private implementation symbols unexported.
- Add validation that proves shared builds export public API symbols and hide internal implementation names.
- Preserve static builds and C++20 source compatibility for existing consumers.

**Non-Goals:**

- Do not define a frozen binary ABI policy, symbol versioning scheme, or backwards-compatible ABI guarantee.
- Do not redesign public API shapes or move implementation out of existing translation units.
- Do not introduce platform-portability work beyond the Windows DLL/static-library boundary this project supports.
- Do not add a new dependency or replace vcpkg/CMake packaging.

## Decisions

1. Add a committed public export header instead of generated CMake export headers.

   `include/libbsa/export.hpp` will define `LIBBSA_API` using `__declspec(dllexport)` when compiling the shared library, `__declspec(dllimport)` for shared-library consumers, and an empty definition for static builds. A committed header keeps the installed public surface simple and avoids adding generated-header install rules to this small package.

   Alternative considered: `GenerateExportHeader`. That would work, but it introduces generated public header plumbing and naming that is more machinery than this project needs right now.

2. Drive macro behavior through target compile definitions.

   CMake should define a private producer macro such as `LIBBSA_BUILDING_LIBRARY` only for shared-library builds and a public static macro such as `LIBBSA_STATIC_DEFINE` for static builds. Static consumers then include the same headers without accidentally seeing `dllimport`.

   Alternative considered: require consumers to define `LIBBSA_STATIC_DEFINE` manually. That is easy to document but fragile for installed package consumers and inconsistent with the existing target-based CMake package shape.

3. Remove `WINDOWS_EXPORT_ALL_SYMBOLS` after annotations are in place.

   Explicit annotations become the only export mechanism for the shared `libbsa` target. Internal namespaces, parser helpers, codec wrappers, texture analyzers, and writer-stage entry points remain private implementation details.

   Alternative considered: keep auto-export as a fallback during transition. That would preserve the original problem and make the validation ambiguous, so it should not remain once the public entry points are annotated.

4. Annotate emitted public entry points, not every public value type.

   Public classes with out-of-line methods, public abstract interfaces that cross the DLL boundary, non-inline public member functions, and free functions should carry `LIBBSA_API`. Header-only templates and plain metadata value types do not need export decoration unless they own an emitted out-of-line function.

   Alternative considered: decorate every public enum and metadata struct. That is noisier, can trigger MSVC STL-member export warnings, and does not provide useful exported code symbols for plain value declarations.

5. Validate with a shared-build export inspection plus consumer smoke coverage.

   A shared-build validation step should inspect the built DLL export table on Windows, assert representative public symbols are present, and assert implementation namespaces or known private symbols are absent. A small consumer/link smoke check should still compile against `libbsa::libbsa` to prove headers and import/static definitions work through the target.

   Alternative considered: source-text checks only. They are useful for catching `WINDOWS_EXPORT_ALL_SYMBOLS`, but they cannot prove the produced DLL exports the intended surface.

## Risks / Trade-offs

- Public symbol name matching can be compiler-decoration-sensitive -> Keep export inspection focused on stable substrings for representative public symbols and private namespaces rather than exact decorated signatures.
- Missing an annotation could break shared consumers even while static builds pass -> Add a shared consumer/link check that calls representative reader, writer, and validation APIs.
- Exporting full classes can expose more C++ ABI surface than desired -> Use pimpl-backed class exports where out-of-line methods already exist, and avoid decorating plain STL-bearing metadata structs unless they have emitted methods.
- Static/shared compile definition mistakes can produce `dllimport` in static consumers -> Make the static definition public on static builds and verify through the installed target or shared/static preset smoke builds.

## Migration Plan

1. Add `include/libbsa/export.hpp` with a tight Doxygen comment explaining shared vs static behavior.
2. Include the export header from public headers that declare emitted API.
3. Annotate public entry points and remove `WINDOWS_EXPORT_ALL_SYMBOLS`.
4. Add CMake compile definitions for shared producer and static consumers.
5. Add focused validation for shared exports, private-symbol absence, and consumer link behavior.
6. Build and test the existing Windows static and shared presets.

Rollback is straightforward: revert the export header, annotations, CMake compile definitions, and export-surface tests together. No archive format data or persisted user state is migrated.

## Open Questions

None.
