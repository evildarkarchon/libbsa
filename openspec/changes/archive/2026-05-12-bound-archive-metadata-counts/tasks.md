## 1. Shared Parser Safety Helpers

- [x] 1.1 Add internal metadata-count limit constants for entry counts, BSA folder counts, and BA2 DX10 aggregate chunk counts.
- [x] 1.2 Add a result-returning helper that rejects counts above a named internal limit with `libbsa::error_code::format_error`.
- [x] 1.3 Add result-returning helpers or wrappers for reserving typed metadata vectors and unordered sets while translating `std::bad_alloc` and `std::length_error` into `format_error`.
- [x] 1.4 Add focused unit tests for the new shared count-limit and metadata-allocation helper behavior.

## 2. Parser Count-Limit Enforcement

- [x] 2.1 Update the TES3 BSA parser to validate `file_count` before record/name/hash vector reservations, payload span tracking, duplicate hash/path sets, and entry materialization.
- [x] 2.2 Update the TES4-family BSA parser to validate header `folder_count`, header `file_count`, and per-folder `file_count` before folder, file, filename, duplicate-set, and entry materialization work.
- [x] 2.3 Update the BA2 GNRL parser to validate `file_count` before record, filename, duplicate-set, and entry materialization work in memory and host-file open paths.
- [x] 2.4 Update the BA2 DX10 parser to validate `file_count` and reject aggregate parsed texture chunk counts above the internal chunk limit before public texture metadata or entries are materialized.

## 3. Allocation Error Translation

- [x] 3.1 Replace affected parser `reserve` calls for record, name, entry, payload-span, and chunk vectors with result-based metadata allocation helpers.
- [x] 3.2 Reserve or guard duplicate-detection unordered sets so allocation failures become `format_error` instead of escaping parser APIs.
- [x] 3.3 Add narrow parser allocation exception boundaries for metadata materialization paths that still perform archive-controlled container growth.

## 4. Malformed Archive Coverage

- [x] 4.1 Add TES3 BSA malformed coverage for an excessive declared file count returning `format_error` without materializing metadata.
- [x] 4.2 Add TES4-family BSA malformed coverage for excessive declared folder count, total file count, and per-folder file count returning `format_error`.
- [x] 4.3 Add BA2 GNRL malformed coverage for an excessive declared file count returning `format_error` in both in-memory and host-file parser paths where practical.
- [x] 4.4 Add BA2 DX10 malformed coverage for excessive declared file count and excessive aggregate texture chunk count returning `format_error`.

## 5. Verification

- [x] 5.1 Run the focused parser, primitive, and validation tests that cover the changed code paths.
- [x] 5.2 Run the full configured CTest suite for the active build preset or document any environment blocker.
