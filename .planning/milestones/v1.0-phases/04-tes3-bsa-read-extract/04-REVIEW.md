---
phase: 04-tes3-bsa-read-extract
reviewed: 2026-05-08T12:27:55Z
depth: standard
files_reviewed: 17
files_reviewed_list:
  - CMakeLists.txt
  - include/libbsa/archive.hpp
  - src/archive.cpp
  - src/detail/bethesda_hash.hpp
  - src/detail/bethesda_hash.cpp
  - src/formats/bsa/bsa_format_detector.hpp
  - src/formats/bsa/bsa_format_detector.cpp
  - src/formats/bsa/tes3_bsa_parser.hpp
  - src/formats/bsa/tes3_bsa_parser.cpp
  - src/formats/bsa/tes3_bsa_reader.hpp
  - src/formats/bsa/tes3_bsa_reader.cpp
  - tests/CMakeLists.txt
  - tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp
  - tests/unit/bethesda_hash_tests.cpp
  - tests/unit/public_include_boundary_tests.cpp
  - tests/unit/tes3_bsa_reader_tests.cpp
  - tests/unit/tes4_bsa_reader_tests.cpp
findings:
  critical: 1
  warning: 1
  info: 0
  total: 2
status: issues_found
---

# Phase 04: Code Review Report

**Reviewed:** 2026-05-08T12:27:55Z
**Depth:** standard
**Files Reviewed:** 17
**Status:** issues_found

## Summary

Reviewed the Phase 04 TES3 BSA parser, detector, reader/extraction path, fixture generator, CMake wiring, and related unit tests. The parser has useful span/hash/path validation, but TES3 public sink extraction is not actually streaming from the archive file: it reads the whole stored payload into a vector before invoking the TES3 helper. There is also a weak malformed fixture that claims to test TES3 hash collisions but only exercises the earlier hash-mismatch branch.

## Critical Issues

### CR-01: BLOCKER — TES3 sink extraction buffers the entire payload before streaming

**File:** `src/archive.cpp:77-104,201-207`; `src/formats/bsa/tes3_bsa_reader.hpp:22-24`

**Issue:** `archive_reader::extract()` calls `read_stored_payload()` before variant dispatch, and `read_stored_payload()` allocates `std::vector<std::byte> payload(entry.stored_size)` and reads the whole file entry into memory. The TES3 helper therefore receives an already-buffered `span`, so Phase 04's sink-first/streaming contract is not met. A valid TES3 archive containing a large entry can fail with `format_error` from vector/platform limits or excessive allocation before the caller sink sees any chunk, even though `extract(path, sink)` should stream bounded chunks from `payload_offset`/`stored_size`.

**Fix:** Dispatch TES3 before `read_stored_payload()` and make the TES3 helper own bounded host-file reads. Keep `extract_bytes()` as the vector convenience wrapper over `extract()`.

```cpp
// archive.cpp
if (state_->metadata.variant == archive_variant::tes3) {
  return formats::bsa::extract_tes3_bsa_payload(
      state_->host_path, *found.value(), sink);
}

auto payload = read_stored_payload(state_->host_path, *found.value());
if (!payload) {
  return payload.error();
}
return formats::bsa::extract_tes4_bsa_payload(payload.value(), *found.value(), sink);
```

```cpp
// tes3_bsa_reader.hpp/.cpp
result<void> extract_tes3_bsa_payload(std::string_view host_path,
                                      const entry_metadata& entry,
                                      payload_sink& sink);

// Implementation should seek to entry.payload_offset and loop over a fixed
// scratch buffer, reading/writing at most extraction_chunk_size bytes per pass.
```

Add a regression test using a TES3 entry larger than `extraction_chunk_size` and a sink that records multiple writes; more importantly, verify the public `extract()` path no longer depends on constructing a payload-sized vector before calling the sink.

## Warnings

### WR-01: WARNING — Claimed TES3 hash-collision fixture only tests hash mismatch

**File:** `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp:321-323`; related parser branch `src/formats/bsa/tes3_bsa_parser.cpp:216-228`

**Issue:** The `tes3_hash_collision` malformed fixture overwrites the second stored hash with the first entry's hash while leaving the second name unchanged. During parsing, the second entry fails at the stored-vs-computed hash check (`stored_hash != computed_hash`) before reaching the duplicate stored-hash/collision check. The manifest and summaries therefore claim collision coverage, but the collision-specific branch is not actually exercised.

**Fix:** Replace or supplement this fixture with one that reaches the duplicate-hash branch. Prefer a real pair of distinct archive names with identical `hash_tes3()` values; if no compact collision pair is available, add a separate targeted test that intentionally constructs parsed metadata around the duplicate-hash validation boundary. The assertion should prove the duplicate/collision branch is reached, not just that `archive_reader::open()` returns a generic `format_error`.

---

_Reviewed: 2026-05-08T12:27:55Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
