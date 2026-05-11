## Context

The archive parsers currently carry local copies of the same hardening helpers: checked multiplication and addition, span containment, bounded `std::ifstream` reads, byte-span to archive string conversion, and display separator normalization. Recent malformed archive fixes touch these helpers repeatedly across `tes3_bsa_parser.cpp`, `tes4_bsa_parser.cpp`, `ba2_gnrl_parser.cpp`, and `ba2_dx10_parser.cpp`, which makes it easy for one parser family to keep stale bounds or allocation behavior.

The project already has `detail::binary_reader` for span-backed little-endian reads and `detail::byte_vector` for allocation-safe byte buffers. This change should build on those internal helpers and keep public headers, dependencies, and TES5Edit untouched.

## Goals / Non-Goals

**Goals:**

- Provide a single internal parser primitive layer for repeated arithmetic, span, stream-read, string materialization, and display-separator operations.
- Refactor TES3 BSA, TES4-family BSA, BA2 GNRL, and BA2 DX10 parsers onto the shared helpers without changing supported archive behavior.
- Preserve format-specific parsing decisions, compatibility checks, and domain-specific diagnostics in the format modules.
- Add helper-level and parser-level tests that prove malformed archive hardening and result-based allocation translation remain consistent across archive families.

**Non-Goals:**

- Do not expose parser primitives through public libbsa headers.
- Do not change archive writer behavior, compression codecs, dependency ownership, CMake presets, or vcpkg configuration.
- Do not generalize platform support beyond the current Windows/MSVC/vcpkg target.
- Do not edit, format, stage, compile, or otherwise mutate `TES5Edit/`.

## Decisions

### Add `detail::parser_primitives`

Add an internal module such as `src/detail/parser_primitives.hpp` and `.cpp` for helpers that are parser-specific but shared across archive families. This keeps `binary_io` focused on span-backed endian reads and avoids turning individual format parsers into the ownership point for generic hardening code.

Alternatives considered:

- Put all helpers into `binary_io`: rejected because bounded host-file reads, span arithmetic, string materialization, and separator normalization are broader than binary endian I/O.
- Keep duplicated helpers and rely on review discipline: rejected because the reported failure mode is exactly that duplicated hardening is easy to miss.

### Keep format-specific validation in parser modules

The shared layer should answer primitive questions such as "does this addition fit", "does this span fit within the archive", "read these exact bytes", and "materialize this byte span as an archive string". TES3 data-section offset rules, TES4 folder/name table layout, BA2 record sentinel handling, BA2 DX10 chunk planning, payload overlap policy, duplicate canonical path detection, and target-specific diagnostics stay in the format modules.

Alternatives considered:

- Move parser table interpretation into a generic abstraction: rejected because the archive families have different byte layouts and compatibility rules, and over-centralizing them would hide important format behavior.

### Make archive string materialization result-based

Replace local `bytes_to_string` helpers with an allocation-safe `result<std::string>` helper. It should reserve capacity through a string allocation guard or an equivalent try/catch boundary and return `libbsa::error_code::format_error` if an archive-controlled string cannot be allocated.

Alternatives considered:

- Keep returning plain `std::string`: rejected because archive-declared name-table sizes can still drive allocation and should follow the same public result contract as byte buffers.

### Preserve diagnostics by passing descriptions

Shared helpers should accept concise caller-supplied descriptions so existing parser errors remain format-specific, such as `BA2 GNRL filename bytes is truncated` or `TES4 BSA metadata table is too large`. The helper owns common suffixes and exception translation; the parser owns archive-family context.

Alternatives considered:

- Standardize every helper error message: rejected because it would make debugging less precise and could churn tests without improving behavior.

### Validate at both helper and parser boundaries

Add unit coverage for parser primitives directly, then keep focused parser regression tests for each archive family. Helper tests make arithmetic, span, stream-read, and allocation translation behavior cheap to verify; parser tests prove the refactor did not drop archive-family compatibility checks.

Alternatives considered:

- Only rely on existing parser tests: rejected because this change creates a shared hardening layer whose contracts should be tested directly.

## Risks / Trade-offs

- Error message churn could mask real regressions. Mitigation: keep description-driven helper errors and only adjust assertions where the shared helper intentionally preserves the same meaning through a common suffix.
- A shared helper could accidentally flatten format-specific compatibility rules. Mitigation: move only primitive operations and leave archive layout validation in the existing parser translation units.
- Stream-read refactors can change cursor handling. Mitigation: require `read_file_bytes_at` to clear, seek, read exactly the requested byte count, and report seek/read/truncation outcomes as before.
- String allocation failures are difficult to force naturally. Mitigation: cover impossible-capacity or max-size paths at helper level, then rely on parser-level fixture tests for integration behavior.

## Migration Plan

1. Add the internal parser primitive module and wire it into the library target.
2. Add focused unit tests for checked arithmetic, span validation, bounded reads, string materialization, and separator normalization.
3. Refactor TES3 and TES4 BSA parsers to use the shared helpers, keeping existing parser-specific validation and diagnostics.
4. Refactor BA2 GNRL and BA2 DX10 parsers to use the shared helpers, including streaming filename-table reads where applicable.
5. Run focused parser and helper tests, then the normal Windows CTest preset used by the repo.

Rollback is straightforward because the change is internal: restore the parser-local helpers and remove the new detail module if validation exposes an unintended behavior change.

## Open Questions

- None currently. The implementation can choose exact helper names as long as the shared ownership boundary and result/error contracts are preserved.
