# Phase 10: TES3 Write Support and BSA Format Completeness - Pattern Map

**Mapped:** 2026-05-09
**Files analyzed:** 12 new/modified file groups
**Analogs found:** 12 / 12

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/writer.hpp` | public API/config | request-response | `include/libbsa/writer.hpp` existing TES4/BA2 writer surfaces | exact |
| `include/libbsa/libbsa.hpp` | public umbrella config | request-response | `include/libbsa/libbsa.hpp` existing umbrella include | exact |
| `src/formats/bsa/tes3_bsa_writer.hpp` | private writer model/service contract | batch + file-I/O | `src/formats/bsa/tes4_bsa_writer.hpp` | exact |
| `src/formats/bsa/tes3_bsa_writer.cpp` | service/serializer | batch + file-I/O | `src/formats/bsa/tes4_bsa_writer.cpp`; `src/formats/ba2/ba2_gnrl_writer.cpp`; `src/formats/bsa/tes3_bsa_parser.cpp` | exact |
| `CMakeLists.txt` | build config | batch | `CMakeLists.txt` existing source registration | exact |
| `tests/unit/tes3_bsa_writer_tests.cpp` | test | batch + file-I/O | `tests/unit/tes4_bsa_writer_tests.cpp`; `tests/unit/ba2_dx10_writer_tests.cpp`; `tests/unit/tes3_bsa_reader_tests.cpp` | exact |
| `tests/unit/public_include_boundary_tests.cpp` | test/public-boundary | request-response | existing TES4/BA2 public contract assertions in same file | exact |
| `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp` | fixture generator | batch + file-I/O | `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` | exact |
| `tests/fixtures/generated/archives/tes3_writer_*.bsa` | fixture data | file-I/O | `tests/fixtures/generated/archives/tes3_success.bsa` produced by generator | exact |
| `tests/fixtures/generated/archives/tes3_writer_*_manifest.json` | fixture manifest | batch | `tests/fixtures/generated/archives/tes3_success_manifest.json` / generator manifest code | exact |
| `tests/CMakeLists.txt` | test build config | batch | existing fixture generator/test registrations in `tests/CMakeLists.txt` | exact |
| optional shared publish helper under `src/formats/bsa/` or `src/formats/common/` | utility | file-I/O | `src/formats/ba2/ba2_gnrl_writer.cpp` publish helpers + `src/formats/ba2/ba2_publish.hpp` rollback helper | role-match |

## Pattern Assignments

### `include/libbsa/writer.hpp` (public API/config, request-response)

**Analog:** existing writer declarations in `include/libbsa/writer.hpp`

**Imports pattern** (lines 3-10):
```cpp
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

#include <libbsa/result.hpp>
```

**Options struct pattern** (lines 64-77):
```cpp
/// Options controlling TES4-family write-new archive finalization.
struct tes4_bsa_writer_options {
  /// Archive-wide compression behavior used by entries whose policy is `inherit`.
  archive_compression_policy compression_policy{archive_compression_policy::target_default};

  /// Emits target-compatible embedded file-name payload prefixes when true.
  bool embed_file_names{false};

  /// Shares identical stored payload regions only when explicitly enabled.
  bool deduplicate_payloads{false};

  /// Allows `write_to` to replace an existing host-path archive when true.
  bool overwrite_existing{false};
};
```

**Writer class pattern** (lines 148-192):
```cpp
/// Public writer for creating new TES4-family BSA archives.
///
/// Entries are added with explicit archive-internal paths and finalized to a
/// host-path archive. Memory-buffer entries are copied into writer-owned state.
class tes4_bsa_writer {
 public:
  /// Creates a writer for `target` using default writer options.
  explicit tes4_bsa_writer(tes4_bsa_target target);

  /// Creates a writer for `target` using the supplied compatibility options.
  explicit tes4_bsa_writer(tes4_bsa_target target, tes4_bsa_writer_options options);

  /// Returns the immutable writer options selected at construction time.
  [[nodiscard]] const tes4_bsa_writer_options& options() const noexcept;

  /// Adds a host-file payload with an explicit archive-internal path.
  result<void> add_file(std::string_view archive_path,
                        std::string_view host_path,
                        entry_compression_policy compression = entry_compression_policy::inherit);

  /// Adds bytes copied from caller memory with an explicit archive-internal path.
  result<void> add_bytes(std::string_view archive_path,
                         std::span<const std::byte> bytes,
                         entry_compression_policy compression = entry_compression_policy::inherit);

  /// Finalizes the writer state into a new archive at `host_path`.
  result<void> write_to(std::string_view host_path) const;

 private:
  struct state;
  std::shared_ptr<state> state_;
};
```

**Apply to TES3:** add `tes3_bsa_writer_options { bool overwrite_existing{false}; }` and `tes3_bsa_writer` with default/options constructors, `options()`, `add_file`, `add_bytes`, and `write_to`. Do **not** copy TES4 target enum or compression parameters; Phase 10 requires raw/uncompressed TES3 only.

---

### `src/formats/bsa/tes3_bsa_writer.hpp` (private writer model/service contract, batch + file-I/O)

**Analog:** `src/formats/bsa/tes4_bsa_writer.hpp`

**Private header pattern** (lines 1-10):
```cpp
#pragma once

#include <libbsa/writer.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>
```

**Entry state + serializer declaration pattern** (lines 11-25):
```cpp
namespace libbsa::formats::bsa {

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

**Apply to TES3:** same shape with `tes3_writer_entry`, but omit `entry_compression_policy compression` and target argument. Keep `archive_path_original`, `archive_path_canonical`, `host_path`, `memory_bytes`, and `from_memory`.

---

### `src/formats/bsa/tes3_bsa_writer.cpp` (service/serializer, batch + file-I/O)

**Analogs:** `src/formats/bsa/tes4_bsa_writer.cpp`, `src/formats/bsa/tes3_bsa_parser.cpp`, `src/formats/ba2/ba2_gnrl_writer.cpp`

**Imports pattern** (`tes4_bsa_writer.cpp` lines 1-19):
```cpp
#include "formats/bsa/tes4_bsa_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
```

**Public bridge/state pattern** (`tes4_bsa_writer.cpp` lines 23-97):
```cpp
struct tes4_bsa_writer::state {
  tes4_bsa_target target;
  tes4_bsa_writer_options options;
  std::vector<formats::bsa::tes4_writer_entry> entries;
};

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

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

result<void> tes4_bsa_writer::add_file(std::string_view archive_path,
                                       std::string_view host_path,
                                       entry_compression_policy compression) {
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

result<void> tes4_bsa_writer::add_bytes(std::string_view archive_path,
                                        std::span<const std::byte> bytes,
                                        entry_compression_policy compression) {
  auto entry = make_entry(archive_path, compression);
  if (!entry) {
    return entry.error();
  }

  entry.value().memory_bytes.assign(bytes.begin(), bytes.end());
  entry.value().from_memory = true;
  state_->entries.push_back(std::move(entry.value()));
  return {};
}

result<void> tes4_bsa_writer::write_to(std::string_view host_path) const {
  return formats::bsa::write_tes4_bsa_archive(state_->target, state_->options, state_->entries, host_path);
}
```

**TES3 layout constants and parser oracle** (`tes3_bsa_parser.cpp` lines 19-24):
```cpp
constexpr std::uint32_t tes3_magic_version = 0x00000100U;
constexpr std::size_t fixed_header_size = 12U;
constexpr std::size_t file_record_size = 8U;
constexpr std::size_t name_offset_size = 4U;
constexpr std::size_t hash_record_size = 8U;
```

**Source read pattern** (`tes4_bsa_writer.cpp` lines 227-244):
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

**Validation pattern** (`tes4_bsa_writer.cpp` lines 500-520):
```cpp
result<void> validate_entries(std::span<const tes4_writer_entry> entries) {
  if (entries.empty()) {
    return error{error_code::invalid_argument, "TES4 BSA writer requires at least one file entry"};
  }

  std::unordered_set<std::string> canonical_paths;
  for (const auto& entry : entries) {
    if (!canonical_paths.insert(entry.archive_path_canonical).second) {
      return error{error_code::format_error, "TES4 BSA writer has duplicate canonical archive paths"};
    }

    if (!entry.from_memory) {
      std::ifstream input{entry.host_path, std::ios::binary};
      if (!input) {
        return error{error_code::io_error, "TES4 BSA writer failed to open disk source"};
      }
    }
  }

  return {};
}
```

**Hash/sort pattern** (`bethesda_hash.cpp` lines 55-85):
```cpp
std::uint64_t hash_tes3(std::string_view archive_path) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashTES3 halves the byte string, lowercases
  // ASCII only via LowerByte, then XORs shifted bytes with a rotate in the low half.
  const auto half = archive_path.size() >> 1U;
  ...
  return result | sum;
}

std::uint32_t tes3_hash_low32(std::uint64_t hash) noexcept { return static_cast<std::uint32_t>(hash & 0xFFFF'FFFFULL); }
std::uint32_t tes3_hash_high32(std::uint64_t hash) noexcept { return static_cast<std::uint32_t>(hash >> 32U); }

std::uint64_t tes3_hash_sort_key(std::uint64_t hash) noexcept {
  return (static_cast<std::uint64_t>(tes3_hash_low32(hash)) << 32U) | tes3_hash_high32(hash);
}
```

**Serialization pattern** (`generate_tes3_bsa_fixtures.cpp` lines 136-175):
```cpp
std::vector<std::byte> build_tes3_archive(std::vector<entry_spec>& entries) {
  std::uint32_t name_table_size = 0;
  for (const auto& entry : entries) {
    name_table_size += checked_u32(entry.path.size() + 1U, "TES3 name table");
  }
  const std::uint32_t records_size = checked_u32(entries.size() * 8U, "TES3 file records");
  const std::uint32_t name_offsets_size = checked_u32(entries.size() * 4U, "TES3 name offsets");
  const std::uint32_t hash_table_start = tes3_header_size + records_size + name_offsets_size + name_table_size;
  const std::uint32_t data_section_start = hash_table_start + checked_u32(entries.size() * 8U, "TES3 hash table");

  std::uint32_t next_raw_offset = 0;
  for (auto& entry : entries) {
    entry.raw_tes3_data_offset = next_raw_offset;
    entry.payload_offset = data_section_start + next_raw_offset;
    next_raw_offset += checked_u32(entry.payload.size(), "TES3 payload");
  }

  byte_buffer writer;
  writer.u32(tes3_magic);
  writer.u32(hash_table_start - tes3_header_size);
  writer.u32(checked_u32(entries.size(), "TES3 file count"));
  for (const auto& entry : entries) {
    writer.u32(checked_u32(entry.payload.size(), "TES3 file size"));
    writer.u32(entry.raw_tes3_data_offset);
  }
  std::uint32_t name_offset = 0;
  for (const auto& entry : entries) {
    writer.u32(name_offset);
    name_offset += checked_u32(entry.path.size() + 1U, "TES3 name offset");
  }
  for (const auto& entry : entries) {
    writer.zstring(entry.path);
  }
  for (const auto& entry : entries) {
    writer.u64(entry.archive_hash);
  }
  for (const auto& entry : entries) {
    writer.raw(entry.payload);
  }
  return writer.bytes;
}
```

**Binary writer pattern** (`binary_io.hpp` lines 52-71):
```cpp
/// Writes little-endian archive fields into an owned byte buffer.
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

**Safe publish pattern to prefer over TES4 `.tmp`** (`ba2_gnrl_writer.cpp` lines 482-504, 634-693):
```cpp
result<std::filesystem::path> make_unique_publish_directory(const std::filesystem::path& output_path) {
  const auto parent = output_path.parent_path();
  const auto filename = output_path.filename();
  for (std::uint32_t counter = 0; counter < 64U; ++counter) {
    auto candidate_name = filename;
    candidate_name += ".libbsa-tmp-" + std::to_string(counter);
    const auto candidate = parent.empty() ? candidate_name : parent / candidate_name;
    std::error_code fs_error;
    // A unique directory avoids deleting caller-owned deterministic siblings such as `<archive>.tmp`.
    if (std::filesystem::create_directory(candidate, fs_error)) {
      return candidate;
    }
    if (fs_error) {
      return error{error_code::io_error, "BA2 GNRL writer failed to reserve temporary output directory"};
    }
  }
  return error{error_code::io_error, "BA2 GNRL writer exhausted temporary output directory names"};
}

void cleanup_publish_directory(const std::filesystem::path& temp_dir) noexcept {
  std::error_code fs_error;
  // Cleanup is best-effort because callers should receive the primary write/publish failure, not cleanup noise.
  std::filesystem::remove_all(temp_dir, fs_error);
}
```

---

### `tests/unit/tes3_bsa_writer_tests.cpp` (test, batch + file-I/O)

**Analogs:** `tests/unit/tes4_bsa_writer_tests.cpp`, `tests/unit/ba2_dx10_writer_tests.cpp`

**Imports/helper pattern** (`tes4_bsa_writer_tests.cpp` lines 1-18):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <detail/bethesda_hash.hpp>

#include <algorithm>
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
```

**Temp-file and byte-reader helpers** (`tes4_bsa_writer_tests.cpp` lines 21-62):
```cpp
std::filesystem::path writer_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_tes4_bsa_writer_tests";
  std::filesystem::create_directories(path);
  return path;
}

void write_binary_file(const std::filesystem::path& path, std::vector<std::byte> bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.good());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(output.good());
}

std::uint32_t read_u32_le_at(const std::vector<std::byte>& bytes, std::size_t offset) {
  REQUIRE(offset + 4U <= bytes.size());
  return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1U])) << 8U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2U])) << 16U) |
         (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3U])) << 24U);
}
```

**Reader-backed round-trip pattern** (`tes4_bsa_writer_tests.cpp` lines 193-256):
```cpp
REQUIRE(writer.add_file(expected_entries[0].first, model_source.string()).has_value());
REQUIRE(writer.add_file(expected_entries[1].first, diffuse_source.string()).has_value());
REQUIRE(writer.add_bytes(expected_entries[2].first, expected_entries[2].second).has_value());
REQUIRE(writer.add_bytes(expected_entries[3].first, expected_entries[3].second).has_value());
REQUIRE(writer.add_bytes(expected_entries[4].first, expected_entries[4].second).has_value());

auto written = writer.write_to(output.string());
REQUIRE(written.has_value());

auto opened = libbsa::archive_reader::open(output.string());
REQUIRE(opened.has_value());

auto metadata = opened.value().metadata();
REQUIRE(metadata.has_value());
CHECK(metadata.value().type == libbsa::archive_type::bsa);
CHECK(metadata.value().variant == libbsa::archive_variant::tes4);
CHECK(metadata.value().file_count == expected_entries.size() + 1U);

for (const auto& [path, expected] : expected_entries) {
  auto contained = opened.value().contains(path);
  REQUIRE(contained.has_value());
  CHECK(contained.value());

  auto found = opened.value().find(path);
  REQUIRE(found.has_value());
  REQUIRE(found.value().has_value());
  CHECK(found.value()->original_path == path);
  CHECK(found.value()->compression == libbsa::entry_compression::none);

  auto extracted = opened.value().extract_bytes(path);
  REQUIRE(extracted.has_value());
  CHECK(extracted.value() == expected);

  collecting_sink sink;
  auto streamed = opened.value().extract(path, sink);
  REQUIRE(streamed.has_value());
  CHECK(sink.bytes() == expected);
}
```

**Validation/error-code pattern** (`tes4_bsa_writer_tests.cpp` lines 326-394):
```cpp
TEST_CASE("TES4 BSA writer copies memory entries into writer-owned state", "[unit][tes4_bsa_writer]") {
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::oblivion};
  auto bytes = sample_bytes();

  auto added = writer.add_bytes("Meshes/Copy.nif", bytes);
  REQUIRE(added.has_value());

  bytes.clear();
  bytes.shrink_to_fit();

  auto duplicate = writer.add_bytes("meshes/copy.nif", std::vector<std::byte>{std::byte{0x00}});
  REQUIRE(duplicate.has_value());

  auto written = writer.write_to(output_path("copied-memory-duplicate.bsa").string());

  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::format_error);
}

TEST_CASE("TES4 BSA writer reports invalid archive paths as invalid arguments", "[unit][tes4_bsa_writer]") {
  for (const std::string invalid_path : {"/rooted/file.txt", "C:/drive/file.txt", "folder/../file.txt", ""}) {
    libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::oblivion};
    auto added = writer.add_bytes(invalid_path, sample_bytes());
    REQUIRE_FALSE(added.has_value());
    REQUIRE(added.error().code == libbsa::error_code::invalid_argument);
  }
}

TEST_CASE("TES4 BSA writer reports missing disk sources as I/O errors", "[unit][tes4_bsa_writer]") {
  libbsa::tes4_bsa_writer writer{libbsa::tes4_bsa_target::skyrim_se};
  const auto missing_source = output_path("missing-source-input.dds");
  std::filesystem::remove(missing_source);

  auto added = writer.add_file("Textures/Disk.dds", missing_source.string());
  REQUIRE(added.has_value());

  auto written = writer.write_to(output_path("missing-source-output.bsa").string());
  REQUIRE_FALSE(written.has_value());
  REQUIRE(written.error().code == libbsa::error_code::io_error);
}
```

**Publish preservation assertions** (`ba2_dx10_writer_tests.cpp` lines 734-772):
```cpp
TEST_CASE("BA2 DX10 writer refuses to overwrite existing output by default and preserves bytes",
          "[unit][ba2_dx10_writer][publish][overwrite]") {
  const auto output = writer_test_dir() / "dx10-overwrite-default.ba2";
  const std::vector<std::byte> sentinel{std::byte{0x4F}, std::byte{0x4C}, std::byte{0x44}};
  write_binary_file(output, sentinel);
  ...
  REQUIRE_FALSE(written.has_value());
  CHECK(written.error().code == libbsa::error_code::io_error);
  CHECK(read_binary_file(output) == sentinel);
}

TEST_CASE("BA2 DX10 writer preserves caller-owned temp-name sibling files during unique temp publish",
          "[unit][ba2_dx10_writer][publish][temp]") {
  const auto collision = output.string() + ".tmp";
  const std::vector<std::byte> sentinel{std::byte{0x54}, std::byte{0x4D}, std::byte{0x50}};
  write_binary_file(collision, sentinel);
  ...
  REQUIRE(written.has_value());
  REQUIRE(std::filesystem::exists(collision));
  CHECK(read_binary_file(collision) == sentinel);
}
```

---

### `tests/fixtures/generated/generate_tes3_bsa_writer_fixtures.cpp` (fixture generator, batch + file-I/O)

**Analog:** `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp`

**Generator imports and utility pattern** (lines 1-17, 59-75):
```cpp
#include <detail/bethesda_hash.hpp>

#include <algorithm>
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

std::vector<std::byte> bytes_from_string(std::string_view value) {
  std::vector<std::byte> result;
  result.reserve(value.size());
  for (const char ch : value) {
    result.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return result;
}

std::string to_hex(std::span<const std::byte> bytes) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (const auto value : bytes) {
    out << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(value));
  }
  return out.str();
}
```

**Manifest shape pattern** (lines 208-249):
```cpp
std::string success_manifest(const std::vector<entry_spec>& entries) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  out << "{\n";
  out << "  \"variant\": \"tes3\",\n";
  out << "  \"version\": " << std::dec << tes3_magic << ",\n";
  out << "  \"file_count\": " << entries.size() << ",\n";
  out << "  \"data_section_start\": " << (entries.empty() ? 0U : entries.front().payload_offset - entries.front().raw_tes3_data_offset) << ",\n";
  out << "  \"provenance\": {\n";
  out << "    \"generator\": \"tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp\",\n";
  out << "    \"source\": \"synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied\"\n";
  out << "  },\n";
  out << "  \"entries\": [\n";
  ...
  out << "      \"archive_hash\": \"0x" << std::hex << std::setw(16) << entry.archive_hash << "\",\n";
  out << "      \"hash_low32\": \"0x" << std::hex << std::setw(8) << hash_low32(entry.archive_hash) << "\",\n";
  out << "      \"hash_high32\": \"0x" << std::hex << std::setw(8) << hash_high32(entry.archive_hash) << "\",\n";
  out << "      \"raw_tes3_data_offset\": " << entry.raw_tes3_data_offset << ",\n";
  out << "      \"payload_offset\": " << entry.payload_offset << ",\n";
  out << "      \"expected\": {\n";
  out << "        \"bytes_hex\": \"" << to_hex(entry.payload) << "\"\n";
  ...
}
```

**Main/error handling pattern** (lines 351-362):
```cpp
/// Generates deterministic TES3 BSA fixtures and manifests from synthetic repository-owned bytes.
int main(int argc, char** argv) {
  try {
    const auto output_dir = parse_output_dir(argc, argv);
    generate_success(output_dir);
    generate_malformed(output_dir);
  } catch (const std::exception& exception) {
    std::cerr << "generate_tes3_bsa_fixtures: " << exception.what() << '\n';
    return 1;
  }
  return 0;
}
```

**Apply to Phase 10:** generator must use the **public** `libbsa::tes3_bsa_writer` API to produce the `.bsa`; it may still compute/emit expected hash/offset facts independently for the JSON manifest.

---

### `tests/unit/public_include_boundary_tests.cpp` (test/public-boundary, request-response)

**Analog:** existing public writer boundary assertions in same file

**Static type registration pattern** (lines 16-43):
```cpp
static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");
static_assert(std::is_enum_v<libbsa::archive_type>);
static_assert(std::is_enum_v<libbsa::archive_variant>);
static_assert(std::is_enum_v<libbsa::entry_compression>);
static_assert(std::is_enum_v<libbsa::tes4_bsa_target>);
static_assert(std::is_class_v<libbsa::ba2_gnrl_writer_options>);
static_assert(std::is_class_v<libbsa::ba2_gnrl_writer>);
static_assert(std::is_class_v<libbsa::ba2_dx10_writer_options>);
static_assert(std::is_class_v<libbsa::ba2_dx10_writer>);
static_assert(std::is_constructible_v<libbsa::tes4_bsa_writer, libbsa::tes4_bsa_target>);
static_assert(std::is_abstract_v<libbsa::payload_sink>);
```

**Writer method contract pattern** (lines 45-49):
```cpp
static_assert(requires(libbsa::tes4_bsa_writer& writer, std::span<const std::byte> bytes) {
  { writer.add_bytes("Meshes/Memory.nif", bytes) } -> std::same_as<libbsa::result<void>>;
  { writer.add_file("Textures/Disk.dds", "source.dds") } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.bsa") } -> std::same_as<libbsa::result<void>>;
});
```

**Forbidden private-token pattern** (lines 146-170):
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                   "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
...
REQUIRE(line.find(token) == std::string::npos);
```

**Apply to TES3:** add `tes3_bsa_writer_options` and `tes3_bsa_writer` static assertions, default/options constructibility, `options()`, `add_file`, `add_bytes`, and `write_to` return types. Also assert no TES3 compression/dedupe/embedded-name public knobs if planner wants contract-level negative coverage.

---

### `CMakeLists.txt` and `tests/CMakeLists.txt` (build config, batch)

**Root source registration analog:** `CMakeLists.txt` lines 44-80
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
    ...
    src/formats/bsa/tes3_bsa_parser.cpp
    src/formats/bsa/tes3_bsa_reader.cpp
    src/formats/bsa/tes4_bsa_parser.cpp
    src/formats/bsa/tes4_bsa_reader.cpp
    src/formats/bsa/tes4_bsa_writer.cpp
)
```

**Test executable source pattern:** `tests/CMakeLists.txt` lines 4-28
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/ba2_gnrl_writer_tests.cpp
  unit/tes4_bsa_writer_tests.cpp
  unit/tes4_bsa_reader_tests.cpp
  unit/tes3_bsa_reader_tests.cpp
  ...
  unit/public_include_boundary_tests.cpp
)
```

**Fixture generator pattern:** `tests/CMakeLists.txt` lines 65-79, 127-146
```cmake
add_executable(generate_tes3_bsa_fixtures_tool
  fixtures/generated/generate_tes3_bsa_fixtures.cpp
)

target_link_libraries(generate_tes3_bsa_fixtures_tool
  PRIVATE
    libbsa::libbsa
)

target_compile_features(generate_tes3_bsa_fixtures_tool PRIVATE cxx_std_20)

target_include_directories(generate_tes3_bsa_fixtures_tool
  PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

add_custom_target(generate_tes3_bsa_fixtures
  COMMAND generate_tes3_bsa_fixtures_tool --output ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_tes3_bsa_fixtures_tool
  BYPRODUCTS
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_success.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes3_success_manifest.json
  COMMENT "Generating TES3 BSA success and malformed fixtures"
  VERBATIM
)
```

---

## Shared Patterns

### Archive path validation and preserved spelling
**Source:** `src/detail/archive_path.cpp` lines 24-57 + `src/formats/bsa/tes4_bsa_writer.cpp` lines 31-49
**Apply to:** `tes3_bsa_writer.cpp`, `tes3_bsa_writer_tests.cpp`, fixture manifest expectations
```cpp
result<archive_path_key> normalize_archive_path(std::string_view input) {
  if (input.empty() || input.front() == '/' || input.front() == '\\' || is_drive_rooted(input)) {
    return invalid_path_error();
  }
  ...
  const char normalized = raw == '\\' ? '/' : lower_ascii(raw);
  ...
  return key;
}

std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}
```

### Stable `result` error-code style
**Source:** `src/formats/bsa/tes4_bsa_writer.cpp` lines 66-78, 500-520; tests lines 357-394
**Apply to:** all public TES3 writer methods and tests
```cpp
if (host_path.empty()) {
  return error{error_code::invalid_argument, "TES4 BSA disk source host path must not be empty"};
}
...
if (!canonical_paths.insert(entry.archive_path_canonical).second) {
  return error{error_code::format_error, "TES4 BSA writer has duplicate canonical archive paths"};
}
...
REQUIRE_FALSE(written.has_value());
REQUIRE(written.error().code == libbsa::error_code::io_error);
```

### TES3 hash and offset compatibility
**Source:** `src/formats/bsa/tes3_bsa_parser.cpp` lines 215-270 and `src/detail/bethesda_hash.cpp` lines 55-85
**Apply to:** `tes3_bsa_writer.cpp`, byte-level writer tests, writer fixture manifest
```cpp
const auto stored_hash = hashes[index];
const auto sort_key = detail::tes3_hash_sort_key(stored_hash);
if (previous_hash_sort_key && sort_key < previous_hash_sort_key.value()) {
  return error{error_code::format_error, "TES3 BSA hash records are not sorted"};
}
...
const auto computed_hash = detail::hash_tes3(names[index]);
if (stored_hash != computed_hash) {
  return error{error_code::format_error, "TES3 BSA stored hash does not match parsed name"};
}
...
// TES5Edit/Core/wbBSArchive.pas:1128-1129,2114-2118 and UESP document TES3 payload offsets as
// data-section-relative; libbsa stores only archive-absolute offsets in runtime metadata.
if (!span_fits(absolute_payload_offset, records[index].size, archive_size)) {
  return error{error_code::format_error, "TES3 BSA entry payload span is outside the archive"};
}
```

### Safe publish / no partial output
**Source:** `src/formats/ba2/ba2_gnrl_writer.cpp` lines 482-504, 634-693; `src/formats/ba2/ba2_publish.hpp` lines 11-24
**Apply to:** `tes3_bsa_writer.cpp`, optional shared publish helper, publish tests
```cpp
// A unique directory avoids deleting caller-owned deterministic siblings such as `<archive>.tmp`.
if (std::filesystem::create_directory(candidate, fs_error)) {
  return candidate;
}
...
// Move the old archive aside before publish so a failed replacement can roll back to the last good file.
std::filesystem::rename(output_path, backup_path.value(), fs_error);
...
result<void> restore_backup_after_publish_failure(const std::filesystem::path& backup_path,
                                                  const std::filesystem::path& output_path,
                                                  RenameFile&& rename_file) {
  std::error_code rollback_error;
  std::forward<RenameFile>(rename_file)(backup_path, output_path, rollback_error);
  if (rollback_error) {
    return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path and failed to restore backup"};
  }
  return error{error_code::io_error, "BA2 DX10 writer failed to publish output host path"};
}
```

### Fixture provenance
**Source:** `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` lines 216-219 and `AGENTS.md` lines 16-29, 59-63
**Apply to:** committed TES3 writer fixtures and manifests
```cpp
out << "  \"provenance\": {\n";
out << "    \"generator\": \"tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp\",\n";
out << "    \"source\": \"synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied\"\n";
out << "  },\n";
```

## No Analog Found

All planned Phase 10 file groups have close analogs. Use research guidance rather than existing code only for the **TES3-specific public writer omission rules**: no compression controls, no embedded names, no dedupe knobs, no caller-provided hashes, and no public raw-offset controls.

## Metadata

**Analog search scope:** `include/libbsa`, `src/formats/bsa`, `src/formats/ba2`, `src/detail`, `tests/unit`, `tests/fixtures/generated`, root/test CMake files.
**Files scanned:** 70+ candidate C++/CMake/JSON files via repository glob; 13 strong analog files read.
**Pattern extraction date:** 2026-05-09
