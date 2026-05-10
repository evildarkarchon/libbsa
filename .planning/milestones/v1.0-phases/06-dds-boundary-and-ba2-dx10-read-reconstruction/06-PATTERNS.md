# Phase 06: DDS Boundary and BA2 DX10 Read/Reconstruction - Pattern Map

**Mapped:** 2026-05-08
**Files analyzed:** 20 new/modified files
**Analogs found:** 20 / 20

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/archive.hpp` | model / public API | request-response metadata | `include/libbsa/archive.hpp` | exact-modification |
| `src/archive.cpp` | facade / route | request-response | `src/archive.cpp` | exact-modification |
| `src/formats/ba2/ba2_format_detector.hpp` | model / config | request-response | `src/formats/ba2/ba2_format_detector.hpp` | exact-modification |
| `src/formats/ba2/ba2_format_detector.cpp` | route / detector | request-response | `src/formats/ba2/ba2_format_detector.cpp` | exact-modification |
| `src/formats/ba2/ba2_dx10_parser.hpp` | parser API / model | file-I/O + transform | `src/formats/ba2/ba2_gnrl_parser.hpp` | role-match |
| `src/formats/ba2/ba2_dx10_parser.cpp` | parser | file-I/O + transform | `src/formats/ba2/ba2_gnrl_parser.cpp` | role-match |
| `src/formats/ba2/ba2_dx10_reader.hpp` | reader API / service | request-response + file-I/O | `src/formats/ba2/ba2_gnrl_reader.hpp` | role-match |
| `src/formats/ba2/ba2_dx10_reader.cpp` | reader / extractor service | file-I/O + transform | `src/formats/ba2/ba2_gnrl_reader.cpp` | role-match |
| `src/texture/dds_layout.hpp` | utility / model | transform | `src/detail/binary_io.hpp` | partial |
| `src/texture/dds_layout.cpp` | utility | transform | `src/detail/binary_io.hpp`, `src/formats/ba2/ba2_gnrl_parser.cpp` | partial |
| `src/texture/directxtex_analyzer.hpp` | adapter / utility | transform | `src/detail/compression_router.cpp` | partial |
| `src/texture/directxtex_analyzer.cpp` | adapter / service | transform | `src/detail/compression_router.cpp`, `tests/unit/public_include_boundary_tests.cpp` | partial |
| `CMakeLists.txt` | config | build graph | `CMakeLists.txt` | exact-modification |
| `tests/CMakeLists.txt` | config | build/test graph | `tests/CMakeLists.txt` | exact-modification |
| `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` | test fixture generator | file-I/O + batch | `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` | role-match |
| `tests/unit/ba2_dx10_metadata_tests.cpp` | test | request-response + file-I/O | `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/public_include_boundary_tests.cpp` | role-match |
| `tests/unit/dds_layout_tests.cpp` | test | transform | `tests/unit/ba2_gnrl_reader_tests.cpp`, `src/detail/binary_io.hpp` | partial |
| `tests/unit/ba2_dx10_parser_tests.cpp` | test | request-response + file-I/O | `tests/unit/ba2_gnrl_reader_tests.cpp` | role-match |
| `tests/unit/ba2_dx10_extraction_tests.cpp` | test | request-response + file-I/O | `tests/unit/ba2_gnrl_reader_tests.cpp` | role-match |
| `tests/unit/ba2_dx10_malformed_tests.cpp` | test | request-response + file-I/O | `tests/unit/ba2_gnrl_reader_tests.cpp` | role-match |

## Pattern Assignments

### `include/libbsa/archive.hpp` (model / public API, request-response metadata)

**Analog:** `include/libbsa/archive.hpp`

**Imports pattern** (lines 3-12):
```cpp
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <libbsa/result.hpp>
```

**Public dependency-light value type pattern** (lines 44-58):
```cpp
/// BA2-specific archive-level metadata exposed when `archive_metadata::type` is `archive_type::ba2`.
///
/// Starfield BA2 revisions append raw header fields after the common `BTDX`/subtype header.
/// These fields are intentionally version-gated optionals so Fallout 4 archives can distinguish
/// "field absent" from a Starfield field that is present with a zero value.
struct ba2_archive_metadata {
  /// Raw Starfield v2+ header field conventionally named `Unknown1` in current references.
  std::optional<std::uint32_t> starfield_unknown1;

  /// Raw Starfield v2+ header field conventionally named `Unknown2` in current references.
  std::optional<std::uint32_t> starfield_unknown2;

  /// Raw Starfield v3 compression method field; method `3` selects raw LZ4 block payloads.
  std::optional<std::uint32_t> compression_method;
};
```

**Entry metadata extension point** (lines 76-94):
```cpp
/// Entry-level metadata exposed for lookup, listing, and extraction.
///
/// `path` is the canonical normalized lookup key. `original_path` preserves the
/// archive-derived display spelling joined with `/` separators, independent of
/// host filesystem path rules. `payload_offset` is always an archive-absolute
/// byte offset for every archive variant; format-specific relative offsets stay
/// inside parser internals and fixture manifests.
struct entry_metadata {
  std::string path;
  std::string original_path;
  std::uint64_t raw_size;
  std::uint64_t stored_size;
  std::uint64_t payload_offset;
  std::uint64_t archive_hash;
  entry_compression compression;
  std::uint32_t record_flags;
  bool has_embedded_name;
  std::uint32_t embedded_name_prefix_size;
};
```

**Apply:** Add Doxygen-commented `texture_chunk_metadata` and `texture_metadata` near `ba2_archive_metadata`, then add `std::optional<texture_metadata>` to `entry_metadata`. Keep only standard-library includes and raw numeric DXGI fields.

---

### `src/archive.cpp` (facade / route, request-response)

**Analog:** `src/archive.cpp`

**Imports/dispatch include pattern** (lines 1-10):
```cpp
#include <libbsa/archive.hpp>

#include "formats/ba2/ba2_format_detector.hpp"
#include "formats/ba2/ba2_gnrl_parser.hpp"
#include "formats/ba2/ba2_gnrl_reader.hpp"
#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes3_bsa_parser.hpp"
#include "formats/bsa/tes3_bsa_reader.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_reader.hpp"
```

**BA2 open dispatch pattern** (lines 122-143):
```cpp
if (prefix.value().size() >= 4U && prefix.value()[0] == static_cast<std::byte>(static_cast<unsigned char>('B')) &&
    prefix.value()[1] == static_cast<std::byte>(static_cast<unsigned char>('T')) &&
    prefix.value()[2] == static_cast<std::byte>(static_cast<unsigned char>('D')) &&
    prefix.value()[3] == static_cast<std::byte>(static_cast<unsigned char>('X'))) {
  auto detected_ba2 = formats::ba2::detect_ba2_format(prefix.value());
  if (!detected_ba2) {
    return detected_ba2.error();
  }

  auto archive_size = archive_file_size(host_path);
  if (!archive_size) {
    return archive_size.error();
  }
  auto ba2_archive = formats::ba2::parse_ba2_gnrl_archive_file(host_path, archive_size.value(), detected_ba2.value());
  if (!ba2_archive) {
    return ba2_archive.error();
  }

  archive_reader reader{ba2_archive.value().metadata};
  reader.state_ = std::make_shared<state>(
      state{ba2_archive.value().metadata, std::move(ba2_archive.value().entries), std::string{host_path}});
  return reader;
}
```

**Find/extract route pattern** (lines 224-244):
```cpp
auto found = state_->metadata.variant == archive_variant::tes3
                  ? formats::bsa::find_tes3_bsa_entry(state_->entries, path)
                  : state_->metadata.type == archive_type::ba2
                        ? formats::ba2::find_ba2_gnrl_entry(state_->entries, path)
                        : formats::bsa::find_tes4_bsa_entry(state_->entries, path);
if (!found) {
  return found.error();
}
if (!found.value()) {
  return error{error_code::not_found, "archive path was not found"};
}
```

**Convenience extraction pattern** (lines 265-271):
```cpp
// Keep the convenience API bounded by the parser-derived size for exactly one entry.
vector_payload_sink sink{found.value()->raw_size};
auto extracted = extract(path, sink);
if (!extracted) {
  return extracted.error();
}
return std::move(sink).finish();
```

**Apply:** Branch BA2 dispatch on `detected_ba2.value().is_dx10` and call DX10 parser/reader helpers while preserving the same `archive_reader::state` shape and `not_found` behavior.

---

### `src/formats/ba2/ba2_format_detector.hpp` and `.cpp` (detector, request-response)

**Analog:** `src/formats/ba2/ba2_format_detector.hpp`, `src/formats/ba2/ba2_format_detector.cpp`

**Detected state pattern** (`ba2_format_detector.hpp` lines 12-24):
```cpp
/// Byte-classified BA2 variant selected before full GNRL parser dispatch.
struct detected_ba2_format {
  archive_variant variant;
  std::uint32_t version;
  entry_compression default_compression;
  ba2_archive_metadata ba2;
  bool is_gnrl;
  bool is_dx10;
  std::uint32_t file_count;
};

/// Classifies BA2 `BTDX` bytes by version and subtype without using the host filename.
result<detected_ba2_format> detect_ba2_format(std::span<const std::byte> bytes);
```

**Subtype recognition pattern** (`ba2_format_detector.cpp` lines 50-58):
```cpp
const auto subtype = reader.read_bytes(4);
if (!subtype) {
  return error{error_code::format_error, "BA2 header is truncated before subtype"};
}
const auto is_gnrl = matches_magic(subtype.value(), 'G', 'N', 'R', 'L');
const auto is_dx10 = matches_magic(subtype.value(), 'D', 'X', '1', '0');
if (!is_gnrl && !is_dx10) {
  return error{error_code::unsupported, "BA2 subtype is not GNRL or DX10"};
}
```

**Unsupported DX10 handoff to replace** (`ba2_format_detector.cpp` lines 69-71):
```cpp
if (is_dx10) {
  return error{error_code::unsupported, "BA2 DX10 texture archives are deferred to the DDS phase"};
}
```

**Starfield compression routing pattern** (`ba2_format_detector.cpp` lines 89-110):
```cpp
case starfield_v3_version: {
  const auto unknown1 = reader.read_u32_le();
  const auto unknown2 = reader.read_u32_le();
  const auto compression_method = reader.read_u32_le();
  if (!unknown1 || !unknown2 || !compression_method) {
    return error{error_code::format_error, "Starfield BA2 v3 header is truncated before CompressionMethod"};
  }
  ba2.starfield_unknown1 = unknown1.value();
  ba2.starfield_unknown2 = unknown2.value();
  ba2.compression_method = compression_method.value();

  // TES5Edit routes CompressionMethod 3 to raw LZ4 block. The generated fixture corpus keeps
  // method 0 as the evidence-bounded non-LZ4 deflate path and rejects unknown methods.
  if (compression_method.value() == starfield_lz4_block_method) {
    return detected_header(archive_variant::starfield, version.value(), entry_compression::lz4_block, ba2, is_gnrl,
                           is_dx10, file_count.value());
  }
```

**Apply:** Remove the DX10 early unsupported return; preserve `is_dx10` in `detected_ba2_format` so `archive.cpp` can route to `ba2_dx10_parser`.

---

### `src/formats/ba2/ba2_dx10_parser.hpp` (parser API / model, file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_gnrl_parser.hpp`

**Header/API pattern** (lines 1-27):
```cpp
#pragma once

#include "formats/ba2/ba2_format_detector.hpp"

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

/// Parsed BA2 GNRL archive metadata and deterministic public entry values.
struct ba2_gnrl_archive {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
};

/// Parses checked BA2 GNRL header, record, filename table, and entry metadata state.
result<ba2_gnrl_archive> parse_ba2_gnrl_archive(std::span<const std::byte> bytes, detected_ba2_format detected);

/// Parses checked BA2 GNRL state from bounded host-file metadata and filename-table reads.
result<ba2_gnrl_archive> parse_ba2_gnrl_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected);
```

**Apply:** Mirror naming as `ba2_dx10_archive`, `parse_ba2_dx10_archive`, and `parse_ba2_dx10_archive_file`. Public return should remain `archive_metadata` plus sorted `entry_metadata`; keep parser-private chunk structs out of the header unless needed by the reader.

---

### `src/formats/ba2/ba2_dx10_parser.cpp` (parser, file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_gnrl_parser.cpp`

**Imports pattern** (lines 1-12):
```cpp
#include "formats/ba2/ba2_gnrl_parser.hpp"

#include <detail/archive_path.hpp>
#include <detail/binary_io.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
```

**Overflow/span guard pattern** (lines 48-70):
```cpp
bool multiply_fits(std::uint32_t count, std::size_t width, std::size_t& total) noexcept {
  if (width != 0U && count > std::numeric_limits<std::size_t>::max() / width) {
    return false;
  }
  total = static_cast<std::size_t>(count) * width;
  return true;
}

bool add_fits(std::size_t lhs, std::size_t rhs, std::size_t& total) noexcept {
  if (lhs > std::numeric_limits<std::size_t>::max() - rhs) {
    return false;
  }
  total = lhs + rhs;
  return true;
}

bool span_fits_u64(std::uint64_t start, std::uint64_t length, std::uint64_t total) noexcept {
  return start <= total && length <= total - start;
}
```

**Checked file read pattern** (lines 82-104):
```cpp
result<std::vector<std::byte>> read_file_bytes_at(std::ifstream& input, std::uint64_t offset, std::size_t count,
                                                  std::string_view description) {
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, std::string{description} + " offset exceeds stream limits"};
  }
  if (count > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
    return error{error_code::format_error, std::string{description} + " size exceeds stream limits"};
  }

  std::vector<std::byte> bytes(count);
  input.clear();
  input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
  if (!input) {
    return error{error_code::io_error, std::string{"failed to seek while reading "} + std::string{description}};
  }
```

**Header parse pattern** (lines 120-149):
```cpp
result<header_fields> read_header(detail::binary_reader& reader) {
  const auto magic = reader.read_u32_le();
  const auto version = reader.read_u32_le();
  const auto subtype = reader.read_u32_le();
  const auto file_count = reader.read_u32_le();
  const auto file_table_offset = reader.read_u64_le();
  if (!magic || !version || !subtype || !file_count || !file_table_offset) {
    return error{error_code::format_error, "BA2 GNRL fixed header is truncated before FileTableOffset"};
  }
```

**Record table parse pattern** (lines 151-172):
```cpp
result<std::vector<gnrl_record>> read_records(detail::binary_reader& reader, std::uint32_t file_count) {
  std::vector<gnrl_record> records;
  records.reserve(file_count);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto name_hash = reader.read_u32_le();
    auto skipped_ext = reader.skip(4U);
    const auto directory_hash = reader.read_u32_le();
    const auto unknown = reader.read_u32_le();
    const auto offset = reader.read_u64_le();
    const auto packed_size = reader.read_u32_le();
    const auto size = reader.read_u32_le();
    const auto sentinel = reader.read_u32_le();
    if (!name_hash || !skipped_ext || !directory_hash || !unknown || !offset || !packed_size || !size || !sentinel) {
      return error{error_code::format_error, "BA2 GNRL record table is truncated"};
    }
```

**Name normalization and duplicate rejection pattern** (lines 229-238):
```cpp
auto original_path = names[index];
normalize_original_separators(original_path);
auto canonical = detail::normalize_archive_path(original_path);
if (!canonical) {
  return error{error_code::format_error, "BA2 GNRL filename table contains an invalid archive path"};
}
if (!canonical_paths.insert(canonical.value().value).second) {
  return error{error_code::format_error, "BA2 GNRL contains duplicate canonical archive paths"};
}
```

**Bounded open-time read pattern** (lines 390-411):
```cpp
auto first_payload_offset = first_payload_offset_for(records.value(), archive_size);
if (!first_payload_offset) {
  return first_payload_offset.error();
}
if (header.value().file_table_offset > first_payload_offset.value()) {
  return error{error_code::format_error, "BA2 GNRL filename table overlaps payload data"};
}
const auto name_table_size_u64 = first_payload_offset.value() - header.value().file_table_offset;
if (name_table_size_u64 > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
  return error{error_code::format_error, "BA2 GNRL filename table size exceeds platform limits"};
}

// Open/list parsing needs only the length-prefixed filename table. Payload bytes stay unread until extraction,
// so large valid BA2 archives cannot force open-time allocation of the payload region.
auto name_table_bytes = read_file_bytes_at(input, header.value().file_table_offset,
                                           static_cast<std::size_t>(name_table_size_u64),
                                           "BA2 GNRL filename table");
```

**Apply:** Replace `gnrl_record_size` with DX10 fixed texture record plus `chunk_count * 24`; preserve checked arithmetic, canonical path handling, sorted entries, and first-payload-offset bounded reads. Validate `chunk_header_size == 24`, chunk spans, mip/array/face coverage, and materialize `entry_metadata.texture` during open.

---

### `src/formats/ba2/ba2_dx10_reader.hpp` (reader API / service, request-response + file-I/O)

**Analog:** `src/formats/ba2/ba2_gnrl_reader.hpp`

**Header/API pattern** (lines 13-26):
```cpp
/// Returns stable BA2 GNRL entry metadata copies sorted by canonical archive path.
result<std::vector<entry_metadata>> ba2_gnrl_entries(std::span<const entry_metadata> entries);

/// Looks up one BA2 GNRL entry using public archive-path normalization semantics.
result<std::optional<entry_metadata>> find_ba2_gnrl_entry(std::span<const entry_metadata> entries, std::string_view path);

/// Reports BA2 GNRL entry presence using the same normalization and errors as find.
result<bool> contains_ba2_gnrl_entry(std::span<const entry_metadata> entries, std::string_view path);

/// Extracts one BA2 GNRL entry from the host archive into a caller-owned sink.
///
/// The entry's parsed compression metadata selects raw, deflate, or raw LZ4-block handling;
/// BA2 GNRL extraction deliberately never infers codec behavior from names or extensions.
result<void> extract_ba2_gnrl_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink);
```

**Apply:** Mirror as `ba2_dx10_entries`, `find_ba2_dx10_entry`, `contains_ba2_dx10_entry`, and `extract_ba2_dx10_payload`. Doc comment must mention DDS header-first output and metadata-driven chunk codec routing.

---

### `src/formats/ba2/ba2_dx10_reader.cpp` (reader / extractor service, file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_gnrl_reader.cpp`

**Imports pattern** (lines 1-10):
```cpp
#include "formats/ba2/ba2_gnrl_reader.hpp"

#include <detail/archive_path.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
```

**Sink partial-write guard pattern** (lines 24-33):
```cpp
result<void> write_all(payload_sink& sink, std::span<const std::byte> bytes) {
  auto written = sink.write(bytes);
  if (!written) {
    return written.error();
  }
  if (written.value() != bytes.size()) {
    return error{error_code::io_error, "payload sink accepted a partial chunk"};
  }
  return {};
}
```

**Chunked sink write pattern** (lines 35-45):
```cpp
result<void> write_in_chunks(payload_sink& sink, std::span<const std::byte> bytes) {
  for (std::size_t offset = 0; offset < bytes.size();) {
    const auto chunk_size = std::min(extraction_chunk_size, bytes.size() - offset);
    auto written = write_all(sink, bytes.subspan(offset, chunk_size));
    if (!written) {
      return written.error();
    }
    offset += chunk_size;
  }
  return {};
}
```

**Raw streaming pattern** (lines 66-93):
```cpp
std::ifstream input{std::string{host_path}, std::ios::binary};
if (!input) {
  return error{error_code::io_error, "failed to open BA2 archive host path for extraction"};
}
input.seekg(static_cast<std::streamoff>(entry.payload_offset), std::ios::beg);
if (!input) {
  return error{error_code::io_error, "failed to seek to BA2 GNRL payload"};
}

std::vector<std::byte> buffer(extraction_chunk_size);
std::uint64_t remaining = entry.stored_size;
while (remaining != 0U) {
  const auto chunk_size = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, buffer.size()));
  input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(chunk_size));
  if (input.bad()) {
    return error{error_code::io_error, "failed while reading BA2 GNRL payload"};
  }
  if (static_cast<std::size_t>(input.gcount()) != chunk_size) {
    return error{error_code::format_error, "BA2 GNRL entry payload span is outside the archive"};
  }

  auto written = write_all(sink, std::span<const std::byte>{buffer.data(), chunk_size});
  if (!written) {
    return written.error();
  }
  remaining -= chunk_size;
}
```

**Compression method mapping** (lines 126-138):
```cpp
result<detail::compression_method> compression_method_for(const entry_metadata& entry) {
  switch (entry.compression) {
  case entry_compression::deflate:
    return detail::compression_method::deflate;
  case entry_compression::lz4_block:
    return detail::compression_method::lz4_block;
  case entry_compression::none:
    return error{error_code::format_error, "BA2 GNRL raw entries must not enter decompression routing"};
  case entry_compression::lz4_frame:
    return error{error_code::format_error, "BA2 GNRL does not support LZ4 frame payloads"};
  }
  return error{error_code::format_error, "BA2 GNRL entry has unknown compression metadata"};
}
```

**Exact-size decompression pattern** (lines 153-159):
```cpp
// Corrupt BA2 compressed payloads are malformed archive bytes, so preserve the
// codec's stable format_error result instead of attempting partial extraction.
auto decoded = detail::decompress_payload_exact(method.value(), stored.value(), expected_size.value());
if (!decoded) {
  return decoded.error();
}
return write_in_chunks(sink, decoded.value());
```

**Lookup pattern** (lines 168-183):
```cpp
auto normalized = detail::normalize_archive_path(path);
if (!normalized) {
  return normalized.error();
}

const auto found = std::lower_bound(entries.begin(), entries.end(), normalized.value().value,
                                    [](const entry_metadata& entry, const std::string& key) {
                                      return entry.path < key;
                                    });
if (found == entries.end() || found->path != normalized.value().value) {
  return std::optional<entry_metadata>{};
}
return std::optional<entry_metadata>{*found};
```

**Apply:** For DX10, write `dds_layout` header bytes first using `write_all`, then iterate texture chunk metadata in validated DDS order. Raw chunks should stream from each chunk offset with one-chunk bounded buffering; compressed chunks should use `decompress_payload_exact` and return `format_error` on corrupt data or exact-size mismatch.

---

### `src/texture/dds_layout.hpp` and `.cpp` (utility/model, transform)

**Analog:** `src/detail/binary_io.hpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`

**Small internal utility API pattern** (`binary_io.hpp` lines 12-19, 52-68):
```cpp
/// Reads fixed-width little-endian archive fields from a bounded byte span.
///
/// The reader never advances after failed reads, which lets format parsers report
/// truncation without losing the offset that caused the failure.
class binary_reader {
 public:
  /// Creates a reader over immutable archive bytes owned by the caller.
  explicit binary_reader(std::span<const std::byte> bytes) noexcept;
```

```cpp
/// Writes little-endian archive fields into an owned byte buffer.
class binary_writer {
 public:
  /// Appends an unsigned 8-bit value.
  result<void> write_u8(std::uint8_t value);

  /// Appends an unsigned 16-bit value in little-endian byte order.
  result<void> write_u16_le(std::uint16_t value);
```

**Validation helper pattern** (`ba2_gnrl_parser.cpp` lines 48-70):
```cpp
bool multiply_fits(std::uint32_t count, std::size_t width, std::size_t& total) noexcept {
  if (width != 0U && count > std::numeric_limits<std::size_t>::max() / width) {
    return false;
  }
  total = static_cast<std::size_t>(count) * width;
  return true;
}

bool span_fits_u64(std::uint64_t start, std::uint64_t length, std::uint64_t total) noexcept {
  return start <= total && length <= total - start;
}
```

**Apply:** Create libbsa-owned structs/functions that compute DDS DXT10 header bytes and expected chunk order. Return `result<T>` with `format_error` for unsupported DXGI layout or impossible mip/array/cubemap coverage. Use `detail::binary_writer` or equivalent little-endian byte appending; do not include DirectXTex headers here.

---

### `src/texture/directxtex_analyzer.hpp` and `.cpp` (adapter/service, transform)

**Analog:** `src/detail/compression_router.cpp`, `tests/unit/public_include_boundary_tests.cpp`

**Private dependency wrapper pattern** (`compression_router.cpp` lines 1-6):
```cpp
#include <detail/compression_router.hpp>

#include <detail/deflate_codec.hpp>
#include <detail/lz4_block_codec.hpp>
#include <detail/lz4_frame_codec.hpp>
```

**Public-to-private enum translation pattern** (`compression_router.cpp` lines 32-48):
```cpp
result<std::vector<std::byte>> decompress_payload_exact(compression_method method, std::span<const std::byte> input,
                                                        std::size_t expected_size) {
  switch (method) {
  case compression_method::none:
    if (input.size() != expected_size) {
      return libbsa::error{libbsa::error_code::format_error, "uncompressed payload size did not match metadata"};
    }
    return copy_bytes(input);
  case compression_method::deflate:
    return decompress_deflate_exact(input, expected_size);
```

**Boundary test pattern to protect public headers** (`public_include_boundary_tests.cpp` lines 48-51):
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "Windows.h",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
```

**Apply:** Put DirectXTex includes only in `.cpp` if possible; the header should expose only libbsa-owned analyzer result structs. Translate `DirectX::TexMetadata` immediately to width/height/mips/raw DXGI id/array/cubemap values. Add/keep public include boundary tests so `DirectXTex`, `DXGI`, and Windows SDK tokens remain absent from `include/libbsa`.

---

### `CMakeLists.txt` (config, build graph)

**Analog:** `CMakeLists.txt`

**Private package lookup pattern** (lines 8-10):
```cmake
find_package(libdeflate CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
```

**Private link pattern** (lines 36-40):
```cmake
target_link_libraries(libbsa
  PRIVATE
    $<IF:$<TARGET_EXISTS:libdeflate::libdeflate_static>,libdeflate::libdeflate_static,libdeflate::libdeflate_shared>
    lz4::lz4
)
```

**Source registration pattern** (lines 42-70):
```cmake
target_sources(libbsa
  PUBLIC
    FILE_SET HEADERS
      BASE_DIRS include
      FILES
        include/libbsa/archive.hpp
        include/libbsa/libbsa.hpp
        include/libbsa/result.hpp
        include/libbsa/version.hpp
  PRIVATE
    src/archive.cpp
    src/detail/archive_path.cpp
```

**Apply:** Add `find_package(directxtex CONFIG REQUIRED)` after verifying target spelling, link it `PRIVATE`, and add new BA2 DX10/texture `.cpp` files under `PRIVATE` sources only. Do not add DirectXTex to public file sets or installed interface dependencies.

---

### `tests/CMakeLists.txt` (config, build/test graph)

**Analog:** `tests/CMakeLists.txt`

**Unit test registration pattern** (lines 4-20):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/tes4_bsa_reader_tests.cpp
  unit/tes3_bsa_reader_tests.cpp
  unit/ba2_gnrl_reader_tests.cpp
  unit/public_include_boundary_tests.cpp
```

**Test dependency pattern** (lines 22-39):
```cmake
target_link_libraries(libbsa_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

target_compile_features(libbsa_tests PRIVATE cxx_std_20)

target_include_directories(libbsa_tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(libbsa_tests
  PRIVATE
    LIBBSA_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
```

**Fixture generator target pattern** (lines 73-87):
```cmake
add_executable(generate_ba2_gnrl_fixtures_tool
  fixtures/generated/generate_ba2_gnrl_fixtures.cpp
)

target_link_libraries(generate_ba2_gnrl_fixtures_tool
  PRIVATE
    libbsa::libbsa
)

target_compile_features(generate_ba2_gnrl_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_ba2_gnrl_fixtures_tool
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
```

**Fixture byproducts pattern** (lines 124-146):
```cmake
add_custom_target(generate_ba2_gnrl_fixtures
  COMMAND generate_ba2_gnrl_fixtures_tool --output ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_ba2_gnrl_fixtures_tool
  BYPRODUCTS
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_fo4.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_gnrl_fo4_manifest.json
```

**Catch discovery pattern** (lines 148-153):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Apply:** Add split DX10 test sources `unit/ba2_dx10_metadata_tests.cpp`, `unit/dds_layout_tests.cpp`, `unit/ba2_dx10_parser_tests.cpp`, `unit/ba2_dx10_extraction_tests.cpp`, and `unit/ba2_dx10_malformed_tests.cpp`; add a `generate_ba2_dx10_fixtures_tool`, register DX10 byproducts, and ensure test target links any private DirectXTex target only if tests call DirectXTex directly outside libbsa.

---

### `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` (test fixture generator, file-I/O + batch)

**Analog:** `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp`

**Generator imports pattern** (lines 1-18):
```cpp
#include <detail/bethesda_hash.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
```

**Byte writer pattern** (lines 31-74):
```cpp
struct byte_buffer {
  std::vector<std::byte> bytes;

  void u8(std::uint8_t value) { bytes.push_back(static_cast<std::byte>(value)); }

  void u16(std::uint16_t value) {
    for (std::uint32_t index = 0; index < 2U; ++index) {
      u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
  }
```

**Compression helper pattern** (lines 178-196):
```cpp
void prepare_payload(entry_spec& entry) {
  entry.path = canonicalize(entry.original_path);
  entry.ext = extension_fourcc(entry.path);
  entry.archive_hash = libbsa::detail::hash_fo4(entry.path);
  entry.raw_size = checked_u32(entry.expected_bytes.size(), "BA2 raw payload");

  if (entry.compression == libbsa::detail::compression_method::none) {
    entry.stored_payload = entry.expected_bytes;
    entry.stored_size = 0U;
    return;
  }

  const auto compressed = libbsa::detail::compress_payload(entry.compression, entry.expected_bytes);
  if (!compressed) {
    throw std::runtime_error("failed to compress BA2 fixture payload: " + compressed.error().message);
  }
```

**Archive assembly pattern** (lines 244-268):
```cpp
std::vector<std::byte> build_archive(archive_spec& archive) {
  for (auto& entry : archive.entries) {
    prepare_payload(entry);
  }
  const std::uint64_t file_table_offset = header_size_for(archive) + archive.entries.size() * 36ULL;
  std::uint64_t next_payload_offset = file_table_offset + name_table_size(archive);
  for (auto& entry : archive.entries) {
    entry.payload_offset = next_payload_offset;
    next_payload_offset += entry.stored_payload.size();
  }

  byte_buffer writer;
  write_header(writer, archive, file_table_offset);
  write_records(writer, archive);
```

**Manifest provenance pattern** (lines 358-361):
```cpp
out << "  \"provenance\": {\n";
out << "    \"generator\": \"tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp\",\n";
out << "    \"source\": \"synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied\"\n";
out << "  },\n";
```

**Malformed manifest pattern** (lines 399-420):
```cpp
std::string malformed_manifest() {
  return R"json({
  "manifest_kind": "malformed_ba2_gnrl_cases",
  "requirements": ["GNRL-01", "GNRL-02", "GNRL-05", "GNRL-07"],
  "threat_references": ["T-05-01", "T-05-02"],
  "provenance": {
    "generator": "tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp",
    "source": "synthetic malformed bytes generated for libbsa tests; no game or TES5Edit bytes copied"
  },
```

**Main/error pattern** (lines 484-495):
```cpp
/// Generates deterministic BA2 GNRL fixtures and manifests from repository-owned synthetic bytes.
int main(int argc, char** argv) {
  try {
    const auto output_dir = parse_output_dir(argc, argv);
    generate_success(output_dir);
    generate_malformed(output_dir);
  } catch (const std::exception& exception) {
    std::cerr << "generate_ba2_gnrl_fixtures: " << exception.what() << '\n';
    return 1;
  }
  return 0;
}
```

**Apply:** Extend the fixture spec with DX10 record fields (`unknown_tex`, `chunk_count`, `chunk_header_size`, `height`, `width`, `num_mips`, `dxgi_format`, `cube_maps_raw`) and chunk records (`offset`, `packed_size`, `raw_size`, `start_mip`, `end_mip`, sentinel). Include expected public texture metadata, expected DirectXTex-loaded metadata, DDS payload bytes/hash, compression route, and legal provenance.

---

### Split BA2 DX10 unit tests (test, request-response + file-I/O)

**Analog:** `tests/unit/ba2_gnrl_reader_tests.cpp`

**Applies to:** `tests/unit/ba2_dx10_metadata_tests.cpp`, `tests/unit/dds_layout_tests.cpp`, `tests/unit/ba2_dx10_parser_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`, `tests/unit/ba2_dx10_malformed_tests.cpp`

**Test imports pattern** (lines 1-18):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include "formats/ba2/ba2_gnrl_reader.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <nlohmann/json.hpp>
```

**Fixture path/JSON pattern** (lines 21-32):
```cpp
std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

std::filesystem::path generated_archive_path(std::string_view filename) {
  return generated_archive_dir() / std::string{filename};
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  return nlohmann::json::parse(stream);
}
```

**Manifest conversion/error-code pattern** (lines 42-67):
```cpp
libbsa::error_code error_code_from_manifest(std::string_view value) {
  if (value == "format_error") {
    return libbsa::error_code::format_error;
  }
  if (value == "unsupported") {
    return libbsa::error_code::unsupported;
  }
  return libbsa::error_code::invalid_argument;
}

libbsa::entry_compression entry_compression_from_manifest(std::string_view value) {
  if (value == "raw") {
    return libbsa::entry_compression::none;
  }
  if (value == "deflate") {
    return libbsa::entry_compression::deflate;
  }
  if (value == "lz4_frame") {
    return libbsa::entry_compression::lz4_frame;
  }
  return libbsa::entry_compression::lz4_block;
}
```

**Collecting/partial sink pattern** (lines 85-103):
```cpp
class collecting_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

 private:
  std::vector<std::byte> bytes_;
};

class partial_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    return bytes.empty() ? 0U : bytes.size() - 1U;
  }
};
```

**Archive metadata assertion pattern** (lines 160-170):
```cpp
void require_common_ba2_metadata(const nlohmann::json& manifest,
                                 const libbsa::archive_metadata& metadata,
                                 libbsa::archive_variant expected_variant) {
  REQUIRE(metadata.type == libbsa::archive_type::ba2);
  REQUIRE(metadata.variant == expected_variant);
  REQUIRE(metadata.version == manifest.at("version").get<std::uint32_t>());
  REQUIRE(metadata.archive_flags == 0U);
  REQUIRE(metadata.file_count == manifest.at("file_count").get<std::uint32_t>());
  REQUIRE(metadata.default_compression == expected_default_compression(manifest));
  REQUIRE(metadata.ba2.has_value());
}
```

**Metadata table assertion pattern** (lines 316-337):
```cpp
for (std::size_t index = 0; index < entries.value().size(); ++index) {
  const auto& actual = entries.value().at(index);
  const auto& expected = *std::find_if(manifest.at("entries").begin(), manifest.at("entries").end(), [&](const auto& candidate) {
    return candidate.at("path").get<std::string>() == sorted_paths.at(index);
  });

  const auto compression = expected.at("compression").get<std::string>();
  saw_raw = saw_raw || compression == "raw";
  saw_deflate = saw_deflate || compression == "deflate";
  saw_lz4_block = saw_lz4_block || compression == "lz4_block";

  REQUIRE(actual.path == expected.at("path").get<std::string>());
  REQUIRE(actual.original_path == archive_original_path_from_manifest(expected.at("original_path").get<std::string>()));
  REQUIRE(actual.raw_size == expected.at("raw_size").get<std::uint64_t>());
```

**Extraction equivalence pattern** (lines 406-416):
```cpp
const auto expected_bytes = bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>());
collecting_sink sink;

auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

REQUIRE(extracted.has_value());
REQUIRE(sink.bytes() == expected_bytes);

auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
REQUIRE(bytes.has_value());
REQUIRE(bytes.value() == expected_bytes);
```

**Malformed manifest pattern** (lines 461-493):
```cpp
TEST_CASE("ba2_gnrl_malformed manifest cases fail with stable error codes", "[unit][fixture][ba2_gnrl_malformed]") {
  const auto manifest = read_json_file(generated_archive_path("ba2_gnrl_malformed_manifest.json"));
  constexpr auto required_cases = std::to_array<std::string_view>({"ba2_dx10_unsupported",
                                                                  "ba2_unsupported_v3_compression_method",
                                                                  "ba2_duplicate_canonical_path",
                                                                  "ba2_corrupt_compressed_payload",
                                                                  "ba2_exact_size_mismatch"});
```

**Apply:** Split DX10 tests by responsibility while reusing the BA2 GNRL fixture helpers/assertion style: `ba2_dx10_metadata_tests.cpp` covers optional texture metadata and public include-boundary-adjacent metadata assertions; `dds_layout_tests.cpp` covers DXT10 header constants and mip/array/cubemap coverage; `ba2_dx10_parser_tests.cpp` covers FO4/Starfield open, lookup, canonical paths, and parser metadata; `ba2_dx10_extraction_tests.cpp` covers sink/extract_bytes DDS equality, raw/deflate/raw-LZ4 decode, DirectXTex load validation, and partial sink behavior; `ba2_dx10_malformed_tests.cpp` covers manifest-driven malformed open/extract cases. Assert error codes, not messages.

---

## Shared Patterns

### Public dependency boundary
**Source:** `tests/unit/public_include_boundary_tests.cpp` lines 48-51
**Apply to:** `include/libbsa/archive.hpp`, DirectXTex CMake wiring, analyzer boundary
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "Windows.h",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
```

### Archive virtual path normalization
**Source:** `src/detail/archive_path.cpp` lines 24-56
**Apply to:** `ba2_dx10_parser.cpp`, `ba2_dx10_reader.cpp`, DX10 tests
```cpp
result<archive_path_key> normalize_archive_path(std::string_view input) {
  if (input.empty() || input.front() == '/' || input.front() == '\\' || is_drive_rooted(input)) {
    return invalid_path_error();
  }

  archive_path_key key;
  key.value.reserve(input.size());
  std::string segment;

  for (const char raw : input) {
    const char normalized = raw == '\\' ? '/' : lower_ascii(raw);
    if (normalized == '/') {
      if (segment.empty() || segment == "." || segment == "..") {
        return invalid_path_error();
      }
```

### Stable `result<T>` error propagation
**Source:** `src/archive.cpp` lines 117-143
**Apply to:** All parser/reader/analyzer functions
```cpp
auto prefix = read_detection_prefix(host_path);
if (!prefix) {
  return prefix.error();
}
```

### Exact-size compression/decompression routing
**Source:** `src/detail/compression_router.cpp` lines 32-48 and `src/formats/ba2/ba2_gnrl_reader.cpp` lines 153-159
**Apply to:** DX10 chunk extraction, fixture generation, malformed compressed chunk tests
```cpp
auto decoded = detail::decompress_payload_exact(method.value(), stored.value(), expected_size.value());
if (!decoded) {
  return decoded.error();
}
return write_in_chunks(sink, decoded.value());
```

### Sink-first / partial-write failure
**Source:** `src/formats/ba2/ba2_gnrl_reader.cpp` lines 24-33
**Apply to:** DX10 DDS header write and decoded chunk writes
```cpp
result<void> write_all(payload_sink& sink, std::span<const std::byte> bytes) {
  auto written = sink.write(bytes);
  if (!written) {
    return written.error();
  }
  if (written.value() != bytes.size()) {
    return error{error_code::io_error, "payload sink accepted a partial chunk"};
  }
  return {};
}
```

### Bounded open-time metadata reads
**Source:** `src/formats/ba2/ba2_gnrl_parser.cpp` lines 402-406
**Apply to:** `ba2_dx10_parser.cpp`
```cpp
// Open/list parsing needs only the length-prefixed filename table. Payload bytes stay unread until extraction,
// so large valid BA2 archives cannot force open-time allocation of the payload region.
auto name_table_bytes = read_file_bytes_at(input, header.value().file_table_offset,
                                           static_cast<std::size_t>(name_table_size_u64),
                                           "BA2 GNRL filename table");
```

### Fixture provenance and generated legal data
**Source:** `tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp` lines 358-361
**Apply to:** `generate_ba2_dx10_fixtures.cpp` manifests
```cpp
out << "  \"provenance\": {\n";
out << "    \"generator\": \"tests/fixtures/generated/generate_ba2_gnrl_fixtures.cpp\",\n";
out << "    \"source\": \"synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied\"\n";
out << "  },\n";
```

### CTest/Catch tag discovery
**Source:** `tests/CMakeLists.txt` lines 148-153
**Apply to:** New DX10 unit tests
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

## No Analog Found

All planned files have usable analogs, but these are partial rather than exact because the codebase has no existing texture/DDS layer:

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `src/texture/dds_layout.hpp` | utility / model | transform | No DDS layout module exists; copy internal utility style from `binary_io.hpp` and validation style from BA2 parser. |
| `src/texture/dds_layout.cpp` | utility | transform | No DDS header construction exists; use `binary_writer`/checked-layout patterns plus research constants. |
| `src/texture/directxtex_analyzer.hpp` | adapter / utility | transform | No private third-party analyzer adapter exists; closest is private codec wrapper routing. |
| `src/texture/directxtex_analyzer.cpp` | adapter / service | transform | No DirectXTex usage exists yet; keep dependency private like compression codec wrappers. |

## Metadata

**Analog search scope:** `include/libbsa/`, `src/archive.cpp`, `src/formats/ba2/`, `src/detail/`, `tests/unit/`, `tests/fixtures/generated/`, root and tests CMake files
**Files scanned:** 20
**Pattern extraction date:** 2026-05-08
