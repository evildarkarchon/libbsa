# Quick Research: BA2 GNRL and TES4 raw streaming host-path seam

**Researched:** 2026-05-14  
**Confidence:** HIGH [VERIFIED: codebase read]

## Project constraints (from AGENTS.md)

- libbsa is Windows-only; do not broaden this into cross-platform work. [VERIFIED: J:\libbsa\AGENTS.md]
- Keep implementation in C++ and preserve BSArchPro-compatible behavior without touching `TES5Edit/`. [VERIFIED: J:\libbsa\AGENTS.md]
- Do not add speculative dependencies; prefer existing standard-library/internal seams. [VERIFIED: J:\libbsa\AGENTS.md]
- Add focused tests for archive-writing behavior, and do not add production shims only for tests. [VERIFIED: J:\libbsa\AGENTS.md]
- Add comments for non-obvious why, and add Doxygen comments only for public APIs or substantially rewritten methods. [VERIFIED: J:\libbsa\AGENTS.md]

## Findings

1. The shared seam already exists: `detail::host_file_path` stores original UTF-8 text plus a resolved `std::filesystem::path`, and `detail::host_file` already has overloads that accept `host_file_path`. [VERIFIED: J:\libbsa\src\detail\host_file_path.hpp] [VERIFIED: J:\libbsa\src\detail\host_file.hpp]
2. `resolve_host_file_path()` explicitly decodes public UTF-8 text with `MultiByteToWideChar` before building the filesystem path, so the intended Windows contract is “decode once, reuse resolved path later.” [VERIFIED: J:\libbsa\src\detail\host_file_path.cpp]
3. BA2 GNRL prepare follows that policy for validation, sizing, hashing, and full reads, but raw finalization drops back to `std::ifstream input{host_path, ...}` in `ba2_gnrl_serialize.cpp`. [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_prepare.cpp] [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_serialize.cpp]
4. TES4 prepare follows the same resolve-once policy, but raw finalization also drops back to `std::ifstream input{host_path, ...}` in `tes4_bsa_serialize.cpp`. [VERIFIED: J:\libbsa\src\formats\bsa\tes4_bsa_prepare.cpp] [VERIFIED: J:\libbsa\src\formats\bsa\tes4_bsa_serialize.cpp]
5. The current prepared-entry structs only persist raw disk source text (`source_path`, `raw_disk_host_path`), not the resolved `host_file_path`, so finalization cannot currently stay on the established seam without an internal struct change. [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_prepare.hpp] [VERIFIED: J:\libbsa\src\formats\bsa\tes4_bsa_prepare.hpp]

## Recommended minimal change

**Recommendation:** store the resolved `detail::host_file_path` in the internal prepared-entry state for raw disk entries, then switch BA2/TES4 raw finalization helpers to consume that type instead of narrow path text. [VERIFIED: codebase read]

### Smallest seam-preserving shape

- In `ba2_gnrl_prepared_entry`, add a `detail::host_file_path` field for raw disk streaming state. [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_prepare.hpp]
- In `tes4_prepared_entry`, add a `detail::host_file_path` field for raw disk streaming state. [VERIFIED: J:\libbsa\src\formats\bsa\tes4_bsa_prepare.hpp]
- In prepare code, resolve once and store the resolved object when `stream_from_disk` / `stream_raw_disk` is selected, instead of storing only `entry.host_path`. [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_prepare.cpp] [VERIFIED: J:\libbsa\src\formats\bsa\tes4_bsa_prepare.cpp]
- In `ba2_gnrl_serialize.cpp` and `tes4_bsa_serialize.cpp`, change the raw streaming helper parameter from `const std::string&` to `const detail::host_file_path&`, and open via `detail::open_host_file(...)`. [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_serialize.cpp] [VERIFIED: J:\libbsa\src\formats\bsa\tes4_bsa_serialize.cpp] [VERIFIED: J:\libbsa\src\detail\host_file.hpp]

This keeps diagnostics on the original UTF-8 text while forcing finalization to reuse the same resolved native path that preparation already validated. [VERIFIED: J:\libbsa\src\detail\host_file_path.hpp] [VERIFIED: J:\libbsa\src\detail\host_file.cpp]

## Blast radius

### If you add a new resolved field and keep the old string field

- Only prepare/serialize call chains need behavior changes for this review item. [VERIFIED: codebase read]
- Existing BA2/TES4 dedupe code can keep compiling unchanged because it still reads the old string field. [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_layout.cpp] [VERIFIED: J:\libbsa\src\formats\bsa\tes4_bsa_layout.cpp]

### If you replace the string field outright

- BA2 dedupe helpers in `ba2_gnrl_layout.cpp` must be updated because they currently compare disk payloads through `std::ifstream` and `std::string` path inputs. [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_layout.cpp]
- TES4 dedupe helpers in `tes4_bsa_layout.cpp` already resolve on reopen, but their signatures and field access would need updating to the new member type. [VERIFIED: J:\libbsa\src\formats\bsa\tes4_bsa_layout.cpp]

**Practical guidance:** for this quick fix, adding a resolved field alongside the existing diagnostic string is the lowest-risk patch; replacing the string field is cleaner but touches dedupe code too. [VERIFIED: codebase read]

## Test approach

1. Extend the existing raw-disk writer coverage with a non-ASCII source-directory case for BA2 GNRL and TES4 BSA. Build the source path natively as `std::filesystem::path`, then pass the public API an explicit UTF-8 string derived from `path.u8string()` rather than `path.string()`. That matches the existing Phase 13 host-path proof style and avoids locale assumptions. [VERIFIED: J:\libbsa\tests\unit\host_file_path_tests.cpp] [VERIFIED: J:\libbsa\tests\unit\host_path_correctness_boundary_tests.cpp]
2. Keep the payload tiny and raw (`all_raw`) so the test exercises the exact finalization streaming path under review, not compression logic. [VERIFIED: J:\libbsa\tests\unit\ba2_gnrl_writer_tests.cpp] [VERIFIED: J:\libbsa\tests\unit\tes4_bsa_writer_tests.cpp]
3. Assert full round-trip success through the public reader (`archive_reader::open(...).extract_bytes(...)`) so the proof stays black-box and confirms the serialized archive is usable. [VERIFIED: J:\libbsa\tests\unit\ba2_gnrl_writer_tests.cpp] [VERIFIED: J:\libbsa\tests\unit\tes4_bsa_writer_tests.cpp]
4. Optional cheap policy test: extend `host_file_writer_name_tests.cpp` so the serializer sources are also checked for `detail::open_host_file(` and absence of direct `std::ifstream{host_path...}` raw-source opens. [VERIFIED: J:\libbsa\tests\unit\host_file_writer_name_tests.cpp] [ASSUMED]

## Comments and docstrings

- No public API change is needed; this should remain internal writer-state plumbing only. [VERIFIED: codebase read]
- Add a short comment near the stored resolved-path member or serializer helper explaining that raw finalization must reuse the resolved host path because Windows UTF-8 correctness was already locked at prepare time. [VERIFIED: J:\libbsa\AGENTS.md] [VERIFIED: J:\libbsa\src\detail\host_file_path.cpp]
- New Doxygen comments are probably unnecessary unless you substantially rewrite a non-trivial helper signature and want to keep the internal contract explicit. [VERIFIED: J:\libbsa\AGENTS.md] [ASSUMED]

## Open edge to note

BA2 raw dedupe comparisons still reopen disk sources from `std::string` paths in `ba2_gnrl_layout.cpp`; that is adjacent to the same seam, but it is a separate blast-radius decision from the review item that specifically called out raw finalization. [VERIFIED: J:\libbsa\src\formats\ba2\ba2_gnrl_layout.cpp]

## Assumptions log

- A1. Extending `host_file_writer_name_tests.cpp` with serializer source-policy assertions is worth the maintenance cost for this quick fix. Risk if wrong: extra brittle test with little value. [ASSUMED]
- A2. Internal helper-signature changes alone do not rise to the repo's “substantially rewritten methods” threshold for new Doxygen comments. Risk if wrong: minor doc-style mismatch. [ASSUMED]
