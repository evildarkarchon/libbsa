# Phase 08: ba2-gnrl-write-new-support - Pattern Map

**Mapped:** 2026-05-09
**Files analyzed:** 8
**Analogs found:** 8 / 8

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/writer.hpp` | public API | request-response | `include/libbsa/writer.hpp` TES4 writer section | exact |
| `include/libbsa/libbsa.hpp` | public umbrella config | request-response | `include/libbsa/libbsa.hpp` | exact |
| `src/formats/ba2/ba2_gnrl_writer.hpp` | private writer model/service | transform + file-I/O | `src/formats/bsa/tes4_bsa_writer.hpp` | exact |
| `src/formats/ba2/ba2_gnrl_writer.cpp` | service | transform + file-I/O | `src/formats/bsa/tes4_bsa_writer.cpp` | exact |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | parser/service | file-I/O + transform | `src/formats/ba2/ba2_gnrl_parser.cpp` | exact |
| `tests/unit/ba2_gnrl_writer_tests.cpp` | test | request-response + file-I/O | `tests/unit/tes4_bsa_writer_tests.cpp` + `tests/unit/ba2_gnrl_reader_tests.cpp` | exact |
| `tests/unit/public_include_boundary_tests.cpp` | test | transform | `tests/unit/public_include_boundary_tests.cpp` | exact |
| `CMakeLists.txt` / `tests/CMakeLists.txt` | config | batch | existing CMake source/test registration | exact |

## Pattern Assignments

### `include/libbsa/writer.hpp` (public API, request-response)

**Analog:** `include/libbsa/writer.hpp`

**Imports pattern** (lines 3-8):
```cpp
#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

#include <libbsa/result.hpp>
```

**Public enum/options pattern** (lines 22-40):
```cpp
/// Archive-wide compression policy applied before per-entry overrides.
enum class archive_compression_policy {
  target_default,
  all_raw,
  all_compressed,
};

/// Per-entry compression override relative to the archive-wide policy.
enum class entry_compression_policy {
  inherit,
  raw,
  compressed,
};
```

**Writer class pattern** (lines 57-100):
```cpp
/// Public writer for creating new TES4-family BSA archives.
///
/// Entries are added with explicit archive-internal paths and finalized to a
/// host-path archive. Memory-buffer entries are copied into writer-owned state.
class tes4_bsa_writer {
 public:
  explicit tes4_bsa_writer(tes4_bsa_target target);
  explicit tes4_bsa_writer(tes4_bsa_target target, tes4_bsa_writer_options options);
  [[nodiscard]] tes4_bsa_target target() const noexcept;
  [[nodiscard]] const tes4_bsa_writer_options& options() const noexcept;
  result<void> add_file(std::string_view archive_path,
                        std::string_view host_path,
                        entry_compression_policy compression = entry_compression_policy::inherit);
  result<void> add_bytes(std::string_view archive_path,
                         std::span<const std::byte> bytes,
                         entry_compression_policy compression = entry_compression_policy::inherit);
  result<void> write_to(std::string_view host_path) const;

 private:
  struct state;
  std::shared_ptr<state> state_;
};
```

**Apply to Phase 8:** Add `ba2_gnrl_target`, `ba2_gnrl_writer_options`, and `ba2_gnrl_writer` beside the TES4 types. Preserve Doxygen comments, `result<void>`, `std::string_view`, `std::span<const std::byte>`, shared `state`, copied memory semantics, and no private dependency types in the public header.

---

### `include/libbsa/libbsa.hpp` (public umbrella config, request-response)

**Analog:** `include/libbsa/libbsa.hpp`

**Umbrella include pattern** (lines 1-6):
```cpp
#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>
#include <libbsa/version.hpp>
#include <libbsa/writer.hpp>
```

**Apply to Phase 8:** If BA2 writer types are added to `writer.hpp`, no umbrella change is needed. If a new public writer header is created, include it here and add it to the CMake public file set.

---

### `src/formats/ba2/ba2_gnrl_writer.hpp` (private writer model/service, transform + file-I/O)

**Analog:** `src/formats/bsa/tes4_bsa_writer.hpp`

**Imports pattern** (lines 3-9):
```cpp
#include <libbsa/writer.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>
```

**Private entry model + write function pattern** (lines 13-25):
```cpp
struct tes4_writer_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::string host_path;
  std::vector<std::byte> memory_bytes;
  bool from_memory{false};
  entry_compression_policy compression{entry_compression_policy::inherit};
};

result<void> write_tes4_bsa_archive(tes4_bsa_target target,
                                    const tes4_bsa_writer_options& options,
                                    std::span<const tes4_writer_entry> entries,
                                    std::string_view output_host_path);
```

**Apply to Phase 8:** Create `ba2_gnrl_writer_entry` with original/canonical paths, host path, copied memory bytes, source kind, compression override, and optional advanced record-flags override. Expose a private `write_ba2_gnrl_archive(...)` function in `libbsa::formats::ba2`.

---

### `src/formats/ba2/ba2_gnrl_writer.cpp` (service, transform + file-I/O)

**Analog:** `src/formats/bsa/tes4_bsa_writer.cpp`

**Imports pattern** (lines 1-19):
```cpp
#include "formats/bsa/tes4_bsa_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
```

**Public wrapper/state pattern** (lines 23-97):
```cpp
struct tes4_bsa_writer::state {
  tes4_bsa_target target;
  tes4_bsa_writer_options options;
  std::vector<formats::bsa::tes4_writer_entry> entries;
};

result<formats::bsa::tes4_writer_entry> make_entry(std::string_view archive_path,
                                                   entry_compression_policy compression) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }

  formats::bsa::tes4_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.compression = compression;
  return entry;
}

result<void> tes4_bsa_writer::add_file(...) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA disk source host path must not be empty"};
  }
  auto entry = make_entry(archive_path, compression);
  if (!entry) {
    return entry.error();
  }
  entry.value().host_path = std::string{host_path};
  entry.value().from_memory = false;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}
```

**Source reading pattern** (lines 227-244):
```cpp
result<std::vector<std::byte>> read_source_bytes(const tes4_writer_entry& entry) {
  if (entry.from_memory) {
    return entry.memory_bytes;
  }

  std::ifstream input{entry.host_path, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "TES4 BSA writer failed to open disk source"};
  }
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  if (input.bad()) {
    return error{error_code::io_error, "TES4 BSA writer failed while reading disk source"};
  }
  return bytes;
}
```

**Compression policy pattern** (lines 246-279):
```cpp
bool archive_default_compressed(tes4_bsa_target target, archive_compression_policy policy) noexcept {
  switch (policy) {
  case archive_compression_policy::target_default:
    return target != tes4_bsa_target::oblivion;
  case archive_compression_policy::all_raw:
    return false;
  case archive_compression_policy::all_compressed:
    return true;
  }
  return false;
}

bool requested_entry_compression(bool archive_default, entry_compression_policy policy) noexcept {
  switch (policy) {
  case entry_compression_policy::inherit:
    return archive_default;
  case entry_compression_policy::raw:
    return false;
  case entry_compression_policy::compressed:
    return true;
  }
  return false;
}
```

**Payload encode + router pattern** (lines 311-327):
```cpp
auto method = compression_method_for_target(target);
if (!method) {
  return method.error();
}
auto compressed = detail::compress_payload(method.value(), raw_payload);
if (!compressed) {
  return compressed.error();
}

stored.reserve(stored.size() + 4U + compressed.value().size());
append_u32_le(stored, raw_size.value());
stored.insert(stored.end(), compressed.value().begin(), compressed.value().end());
return stored;
```

**Dedupe by final stored bytes pattern** (lines 468-494):
```cpp
std::map<std::vector<std::byte>, payload_assignment> deduplicated_payloads;
for (auto& folder : folders) {
  for (auto& entry : folder.entries) {
    if (deduplicate_payloads) {
      // D-19 requires dedupe after the complete stored encoding is built, so
      // the key includes embedded-name prefixes, raw-size prefixes, and codec bytes.
      const auto duplicate = deduplicated_payloads.find(entry.stored_payload);
      if (duplicate != deduplicated_payloads.end()) {
        entry.payload_offset = duplicate->second.offset;
        entry.stored_size = duplicate->second.stored_size;
        entry.owns_payload_bytes = false;
        continue;
      }
    }
```

**Serialization + temp publish pattern** (lines 530-604, 688-711):
```cpp
detail::binary_writer writer;
const std::byte magic[] = {std::byte{'B'}, std::byte{'S'}, std::byte{'A'}, std::byte{0}};
auto written = writer.write_bytes(magic);
if (!written) {
  return written.error();
}
// ... write fixed fields, records, tables, and payloads with checked writer calls ...

std::ofstream output{output_path, std::ios::binary | std::ios::trunc};
if (!output) {
  return error{error_code::io_error, "TES4 BSA writer failed to create temporary output"};
}
const auto bytes = writer.bytes();
output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
if (!output) {
  return error{error_code::io_error, "TES4 BSA writer failed while writing temporary output"};
}
```

```cpp
auto temp_path = output_path;
temp_path += ".tmp";
std::error_code fs_error;
std::filesystem::remove(temp_path, fs_error);
auto written = write_archive_bytes(..., temp_path);
if (!written) {
  std::filesystem::remove(temp_path, fs_error);
  return written.error();
}
if (options.overwrite_existing) {
  std::filesystem::remove(output_path, fs_error);
  if (fs_error) {
    std::filesystem::remove(temp_path, fs_error);
    return error{error_code::io_error, "TES4 BSA writer failed to replace output host path"};
  }
}
std::filesystem::rename(temp_path, output_path, fs_error);
if (fs_error) {
  std::filesystem::remove(temp_path, fs_error);
  return error{error_code::io_error, "TES4 BSA writer failed to publish output host path"};
}
```

**BA2-specific parser record shape to serialize** from `src/formats/ba2/ba2_gnrl_parser.cpp` (lines 151-170):
```cpp
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
if (sentinel.value() != ba2_record_sentinel) {
  return error{error_code::format_error, "BA2 GNRL record BAADF00D sentinel is invalid"};
}
```

**BA2-specific target codec pattern** from `src/formats/ba2/ba2_format_detector.cpp` (lines 96-106):
```cpp
// TES5Edit routes CompressionMethod 3 to raw LZ4 block. The generated fixture corpus keeps
// method 0 as the evidence-bounded non-LZ4 deflate path and rejects unknown methods.
if (compression_method.value() == starfield_lz4_block_method) {
  return detected_header(archive_variant::starfield, version.value(), entry_compression::lz4_block, ba2, is_gnrl,
                         is_dx10, file_count.value());
}
if (compression_method.value() == starfield_deflate_method) {
  return detected_header(archive_variant::starfield, version.value(), entry_compression::deflate, ba2, is_gnrl,
                         is_dx10, file_count.value());
}
return error{error_code::unsupported, "Starfield BA2 v3 CompressionMethod is unsupported"};
```

---

### `src/formats/ba2/ba2_gnrl_parser.cpp` (parser/service, file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_gnrl_parser.cpp`

**Header/version parsing pattern** (lines 120-148):
```cpp
const auto magic = reader.read_u32_le();
const auto version = reader.read_u32_le();
const auto subtype = reader.read_u32_le();
const auto file_count = reader.read_u32_le();
const auto file_table_offset = reader.read_u64_le();
if (!magic || !version || !subtype || !file_count || !file_table_offset) {
  return error{error_code::format_error, "BA2 GNRL fixed header is truncated before FileTableOffset"};
}

ba2_archive_metadata ba2{};
if (version.value() >= starfield_v2_version) {
  const auto unknown1 = reader.read_u32_le();
  const auto unknown2 = reader.read_u32_le();
  // ... assign version-gated optionals ...
}
if (version.value() >= starfield_v3_version) {
  const auto compression_method = reader.read_u32_le();
  // ... assign ba2.compression_method ...
}
```

**Name-table read pattern** (lines 175-195):
```cpp
result<std::vector<std::string>> read_names(std::span<const std::byte> name_table, std::uint32_t file_count,
                                            std::size_t& consumed) {
  detail::binary_reader reader{name_table};
  std::vector<std::string> names;
  names.reserve(file_count);
  for (std::uint32_t index = 0; index < file_count; ++index) {
    const auto length = reader.read_u16_le();
    if (!length) {
      return error{error_code::format_error, "BA2 GNRL filename table is truncated before UInt16 length"};
    }
    const auto bytes = reader.read_bytes(length.value());
    if (!bytes) {
      return error{error_code::format_error, "BA2 GNRL filename table is truncated before name bytes"};
    }
    if (bytes.value().empty()) {
      return error{error_code::format_error, "BA2 GNRL filename table contains an empty name"};
    }
    names.push_back(bytes_to_string(bytes.value()));
  }
  consumed = reader.position();
  return names;
}
```

**Current end-table blocker to replace** (lines 390-411):
```cpp
auto first_payload_offset = first_payload_offset_for(records.value(), archive_size);
if (!first_payload_offset) {
  return first_payload_offset.error();
}
if (header.value().file_table_offset > first_payload_offset.value()) {
  return error{error_code::format_error, "BA2 GNRL filename table overlaps payload data"};
}
const auto name_table_size_u64 = first_payload_offset.value() - header.value().file_table_offset;
// ... reads filename table only up to first payload ...
```

**Apply to Phase 8:** Keep bounded host-file parsing and `read_names(consumed)` but stop deriving name-table size from first payload. Seek to `FileTableOffset`, read enough bounded bytes to parse exactly `file_count` UInt16-prefixed names, then validate consumed table span without rejecting payload-before-name-table archives.

---

### `tests/unit/ba2_gnrl_writer_tests.cpp` (test, request-response + file-I/O)

**Analogs:** `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`

**Test imports/utilities pattern** from TES4 writer tests (lines 1-15, 18-31):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

std::filesystem::path writer_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_tes4_bsa_writer_tests";
  std::filesystem::create_directories(path);
  return path;
}
```

**Reader-backed round-trip pattern** from TES4 writer tests (lines 175-227):
```cpp
auto written = writer.write_to(output.string());
REQUIRE(written.has_value());

auto opened = libbsa::archive_reader::open(output.string());
REQUIRE(opened.has_value());

auto metadata = opened.value().metadata();
REQUIRE(metadata.has_value());
CHECK(metadata.value().type == libbsa::archive_type::bsa);
CHECK(metadata.value().file_count == expected_entries.size() + 1U);

for (const auto& [path, expected] : expected_entries) {
  auto contained = opened.value().contains(path);
  REQUIRE(contained.has_value());
  CHECK(contained.value());
  auto found = opened.value().find(path);
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  CHECK(found.value()->original_path == path);
  auto extracted = opened.value().extract_bytes(path);
  REQUIRE(extracted.has_value());
  CHECK(extracted.value() == expected);
}
```

**Stable error-code pattern** from TES4 writer tests (lines 249-259, 261-269):
```cpp
auto written = writer.write_to(output_path("duplicate-canonical-path.bsa").string());

REQUIRE_FALSE(written.has_value());
REQUIRE(written.error().code == libbsa::error_code::format_error);
```

```cpp
auto added = writer.add_bytes(invalid_path, sample_bytes());

REQUIRE_FALSE(added.has_value());
REQUIRE(added.error().code == libbsa::error_code::invalid_argument);
```

**BA2 metadata assertions pattern** from BA2 reader tests (lines 160-170, 189-217):
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

```cpp
REQUIRE(metadata.value().ba2->starfield_unknown1 == manifest.at("starfield_unknown1").get<std::uint32_t>());
REQUIRE(metadata.value().ba2->starfield_unknown2 == manifest.at("starfield_unknown2").get<std::uint32_t>());
REQUIRE(metadata.value().ba2->compression_method == manifest.at("compression_method").get<std::uint32_t>());
```

**BA2 lookup/extraction pattern** from BA2 reader tests (lines 345-384, 388-424):
```cpp
auto found = opened.value().find(variant.get<std::string>());
REQUIRE(found.has_value());
REQUIRE(found.value().has_value());
REQUIRE(found.value()->path == expected.at("path").get<std::string>());

auto contains = opened.value().contains(variant.get<std::string>());
REQUIRE(contains.has_value());
REQUIRE(contains.value());
```

```cpp
auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

REQUIRE(extracted.has_value());
REQUIRE(sink.bytes() == expected_bytes);

auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
REQUIRE(bytes.has_value());
REQUIRE(bytes.value() == expected_bytes);
```

**Apply to Phase 8:** Use synthetic temp files, create FO4/SFv2/SFv3 writer outputs, reopen with `archive_reader::open`, assert BA2 metadata optionals, compression routes (`none`, `deflate`, `lz4_block`), end filename-table physical layout, payload offsets/dedupe, lookup case normalization, and stable `error_code` only.

---

### `tests/unit/public_include_boundary_tests.cpp` (test, transform)

**Analog:** `tests/unit/public_include_boundary_tests.cpp`

**Static boundary pattern** (lines 16-34):
```cpp
static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");
static_assert(std::is_enum_v<libbsa::archive_type>);
static_assert(std::is_enum_v<libbsa::entry_compression>);
static_assert(std::is_enum_v<libbsa::tes4_bsa_target>);
static_assert(std::is_enum_v<libbsa::archive_compression_policy>);
static_assert(std::is_enum_v<libbsa::entry_compression_policy>);
static_assert(std::is_constructible_v<libbsa::tes4_bsa_writer, libbsa::tes4_bsa_target>);

static_assert(requires(libbsa::tes4_bsa_writer& writer, std::span<const std::byte> bytes) {
  { writer.add_bytes("Meshes/Memory.nif", bytes) } -> std::same_as<libbsa::result<void>>;
  { writer.add_file("Textures/Disk.dds", "source.dds") } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.bsa") } -> std::same_as<libbsa::result<void>>;
});
```

**Forbidden public dependency token pattern** (lines 67-93):
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                   "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
// Iterates include/libbsa/*.hpp and rejects forbidden tokens outside comments.
```

**Apply to Phase 8:** Add static assertions for `ba2_gnrl_target`, `ba2_gnrl_writer_options`, and `ba2_gnrl_writer` constructors/methods. Keep forbidden-token scan green; do not add `libdeflate`, `lz4`, DirectXTex, TES5Edit, or private detail names to public headers.

---

### `CMakeLists.txt` / `tests/CMakeLists.txt` (config, batch)

**Analogs:** root and tests CMake files

**Root private source registration pattern** from `CMakeLists.txt` (lines 44-78):
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
        include/libbsa/writer.hpp
  PRIVATE
    src/archive.cpp
    src/detail/archive_path.cpp
    src/detail/bethesda_hash.cpp
    src/detail/binary_io.cpp
    src/detail/compression_router.cpp
    src/formats/ba2/ba2_gnrl_parser.cpp
    src/formats/ba2/ba2_gnrl_reader.cpp
    src/formats/bsa/tes4_bsa_writer.cpp
)
```

**Test source registration pattern** from `tests/CMakeLists.txt` (lines 4-26):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/tes4_bsa_writer_tests.cpp
  unit/tes4_bsa_reader_tests.cpp
  unit/ba2_gnrl_reader_tests.cpp
  unit/public_include_boundary_tests.cpp
  unit/compression_router_tests.cpp
  unit/bethesda_hash_tests.cpp
)
```

**Catch label wiring pattern** from `tests/CMakeLists.txt` (lines 192-197):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Apply to Phase 8:** Add `src/formats/ba2/ba2_gnrl_writer.cpp` to root private sources and `unit/ba2_gnrl_writer_tests.cpp` to `libbsa_tests`. Only add public header file-set entries if a new public header is created.

## Shared Patterns

### Archive path normalization and duplicate detection
**Source:** `src/detail/archive_path.cpp` lines 24-57; `src/formats/bsa/tes4_bsa_writer.cpp` lines 500-519  
**Apply to:** BA2 writer add/validation, BA2 parser end-table names, writer tests
```cpp
result<archive_path_key> normalize_archive_path(std::string_view input) {
  if (input.empty() || input.front() == '/' || input.front() == '\\' || is_drive_rooted(input)) {
    return invalid_path_error();
  }
  // lowercases ASCII, normalizes '\\' to '/', rejects empty/dot/dot-dot segments.
  return key;
}
```

```cpp
std::unordered_set<std::string> canonical_paths;
for (const auto& entry : entries) {
  if (!canonical_paths.insert(entry.archive_path_canonical).second) {
    return error{error_code::format_error, "TES4 BSA writer has duplicate canonical archive paths"};
  }
}
```

### FO4/BA2 hash helper
**Source:** `src/detail/bethesda_hash.cpp` lines 141-156  
**Apply to:** BA2 writer record hash field computation
```cpp
std::uint32_t hash_fo4(std::string_view archive_path) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashFO4 uses CRC32 with initial value 0,
  // ASCII LowerByte folding, skips bytes >127, and treats '/' as '\\'.
  std::uint32_t result = 0;
  for (char ch : archive_path) {
    auto byte = static_cast<unsigned char>(ch);
    if (byte > 127U) {
      continue;
    }
    if (ch == '/') {
      ch = '\\';
    }
    result = (result >> 8U) ^ crc32_lookup((result ^ lower_byte(ch)) & 0xFFU);
  }
  return result;
}
```

### Little-endian serialization
**Source:** `src/detail/binary_io.hpp` lines 52-71  
**Apply to:** BA2 header, records, payload offsets/sizes, filename table
```cpp
class binary_writer {
 public:
  result<void> write_u8(std::uint8_t value);
  result<void> write_u16_le(std::uint16_t value);
  result<void> write_u32_le(std::uint32_t value);
  result<void> write_u64_le(std::uint64_t value);
  result<void> write_bytes(std::span<const std::byte> bytes);
  [[nodiscard]] std::span<const std::byte> bytes() const noexcept;
};
```

### Compression router
**Source:** `src/detail/compression_router.cpp` lines 18-30  
**Apply to:** BA2 writer compressed entries and BA2 reader extraction compatibility
```cpp
result<std::vector<std::byte>> compress_payload(compression_method method, std::span<const std::byte> input) {
  switch (method) {
  case compression_method::none:
    return copy_bytes(input);
  case compression_method::deflate:
    return compress_deflate(input);
  case compression_method::lz4_frame:
    return compress_lz4_frame(input);
  case compression_method::lz4_block:
    return compress_lz4_block(input);
  }
  return unsupported_method_error();
}
```

### BA2 extraction route as acceptance oracle
**Source:** `src/formats/ba2/ba2_gnrl_reader.cpp` lines 126-159, 193-200  
**Apply to:** Writer tests; writer must emit metadata compatible with this route
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

```cpp
result<void> extract_ba2_gnrl_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  if (entry.has_embedded_name || entry.embedded_name_prefix_size != 0U) {
    return error{error_code::format_error, "BA2 GNRL entries must not carry embedded-name prefixes"};
  }
  if (entry.compression == entry_compression::none) {
    return stream_raw_payload(host_path, entry, sink);
  }
  return extract_compressed_payload(host_path, entry, sink);
}
```

## No Analog Found

All Phase 8 files have close analogs in the current codebase. New BA2 writer code should combine the Phase 7 TES4 writer object/publish/dedupe pattern with the existing BA2 GNRL parser/reader record and metadata patterns.

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|

## Metadata

**Analog search scope:** `include/libbsa/*.hpp`, `src/formats/bsa/*writer*`, `src/formats/ba2/*`, `src/detail/*`, `tests/unit/*writer*`, `tests/unit/ba2_gnrl_reader_tests.cpp`, CMake files  
**Files scanned:** 19  
**Pattern extraction date:** 2026-05-09
