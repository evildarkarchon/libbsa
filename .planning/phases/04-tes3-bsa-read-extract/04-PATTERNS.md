# Phase 04: tes3-bsa-read-extract - Pattern Map

**Mapped:** 2026-05-08  
**Files analyzed:** 16  
**Analogs found:** 16 / 16

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/archive.hpp` | model / public API | request-response metadata | `include/libbsa/archive.hpp` | exact-modification |
| `src/archive.cpp` | service / facade | request-response + file-I/O | `src/archive.cpp` | exact-modification |
| `src/formats/bsa/bsa_format_detector.hpp` | detector API | transform | `src/formats/bsa/bsa_format_detector.hpp` | exact-modification |
| `src/formats/bsa/bsa_format_detector.cpp` | detector | transform | `src/formats/bsa/bsa_format_detector.cpp` | exact-modification |
| `src/formats/bsa/tes3_bsa_parser.hpp` | parser API | transform + file-I/O | `src/formats/bsa/tes4_bsa_parser.hpp` | role-match |
| `src/formats/bsa/tes3_bsa_parser.cpp` | parser | transform + file-I/O | `src/formats/bsa/tes4_bsa_parser.cpp` | role-match |
| `src/formats/bsa/tes3_bsa_reader.hpp` | reader helper API | request-response + file-I/O | `src/formats/bsa/tes4_bsa_reader.hpp` | role-match |
| `src/formats/bsa/tes3_bsa_reader.cpp` | reader helper | request-response + file-I/O | `src/formats/bsa/tes4_bsa_reader.cpp` | role-match |
| `src/formats/bsa/tes4_bsa_parser.*` | parser | transform + file-I/O | `src/formats/bsa/tes4_bsa_parser.*` | exact-modification |
| `src/formats/bsa/tes4_bsa_reader.*` | reader helper | request-response + file-I/O | `src/formats/bsa/tes4_bsa_reader.*` | exact-modification |
| `src/detail/bethesda_hash.*` | utility | transform | `src/detail/bethesda_hash.*` | exact-modification |
| `CMakeLists.txt` | config | build graph | `CMakeLists.txt` | exact-modification |
| `tests/CMakeLists.txt` | test config | build/test graph | `tests/CMakeLists.txt` | exact-modification |
| `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` | fixture generator | file-I/O + batch | `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` | role-match |
| `tests/fixtures/generated/archives/tes3_*` | fixture data | file-I/O | `tests/fixtures/generated/archives/tes4_*` via generator manifest pattern | role-match |
| `tests/unit/tes3_bsa_reader_tests.cpp` | test | request-response + file-I/O | `tests/unit/tes4_bsa_reader_tests.cpp` | role-match |

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

**Public metadata/doc pattern** (lines 58-74):
```cpp
/// Entry-level metadata exposed for lookup, listing, and extraction.
///
/// `path` is the canonical normalized lookup key. `original_path` preserves the
/// archive-derived display spelling joined with `/` separators, independent of
/// host filesystem path rules.
struct entry_metadata {
  std::string path;
  std::string original_path;
  std::uint64_t raw_size;
  std::uint64_t stored_size;
  std::uint64_t payload_offset;
  std::uint64_t tes4_hash;
  entry_compression compression;
  std::uint32_t record_flags;
  bool has_embedded_name;
  std::uint32_t embedded_name_prefix_size;
};
```

**Apply:** Rename `tes4_hash` to `archive_hash` here and update docs for `payload_offset` to explicitly say archive-absolute for every variant. Keep Doxygen `///` style.

---

### `src/archive.cpp` (service / facade, request-response + file-I/O)

**Analog:** `src/archive.cpp`

**Imports and private state pattern** (lines 1-19):
```cpp
#include <libbsa/archive.hpp>

#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_reader.hpp"

struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  std::string host_path;
};
```

**Open dispatch pattern** (lines 112-134):
```cpp
auto prefix = read_detection_prefix(host_path);
if (!prefix) {
  return prefix.error();
}

auto detected = formats::bsa::detect_bsa_format(prefix.value());
if (!detected) {
  return detected.error();
}

auto archive_size = archive_file_size(host_path);
if (!archive_size) {
  return archive_size.error();
}
auto archive = formats::bsa::parse_tes4_bsa_archive_file(host_path, archive_size.value(), detected.value());
if (!archive) {
  return archive.error();
}
```

**Archive-absolute extraction seek pattern** (lines 75-102):
```cpp
result<std::vector<std::byte>> read_stored_payload(std::string_view host_path, const entry_metadata& entry) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path for extraction"};
  }
  if (entry.payload_offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, "TES4 BSA payload offset exceeds stream limits"};
  }
  ...
  input.seekg(static_cast<std::streamoff>(entry.payload_offset), std::ios::beg);
```

**Apply:** Add TES3 parser include and variant-aware dispatch around `detected.value().variant`; keep selected-entry bounded file reads and absolute seek. TES3 parser must convert raw offsets before this function sees metadata.

---

### `src/formats/bsa/bsa_format_detector.hpp` / `.cpp` (detector, transform)

**Analog:** `src/formats/bsa/bsa_format_detector.*`

**Detector result shape** (hpp lines 12-20):
```cpp
/// Byte-classified TES4-family BSA variant selected before parser dispatch.
struct detected_bsa_format {
  archive_variant variant;
  std::uint32_t version;
  entry_compression default_compression;
};

/// Classifies BSA bytes by magic and version without using the host filename.
result<detected_bsa_format> detect_bsa_format(std::span<const std::byte> bytes);
```

**Checked prefix read and unsupported error pattern** (cpp lines 14-30):
```cpp
detail::binary_reader reader{bytes};
const auto magic = reader.read_bytes(4);
if (!magic) {
  return magic.error();
}

const auto magic_bytes = magic.value();
if (magic_bytes[0] != std::byte{'B'} || magic_bytes[1] != std::byte{'S'} || magic_bytes[2] != std::byte{'A'} ||
    magic_bytes[3] != std::byte{0}) {
  return error{error_code::unsupported, "archive bytes do not start with BSA magic"};
}
```

**Version dispatch pattern** (cpp lines 32-40):
```cpp
switch (version.value()) {
case tes4_version:
case fo3_version:
  return detected_bsa_format{archive_variant::tes4, version.value(), entry_compression::deflate};
case sse_version:
  return detected_bsa_format{archive_variant::tes4, version.value(), entry_compression::lz4_frame};
default:
  return error{error_code::unsupported, "BSA header version is not supported"};
}
```

**Apply:** Recognize TES3 magic/version bytes before requiring `BSA\0`; return `{archive_variant::tes3, 0x00000100, entry_compression::none}` or equivalent parsed identity. Preserve `unsupported` for unrelated bytes and `format_error` for truncated recognized headers.

---

### `src/formats/bsa/tes3_bsa_parser.hpp` (parser API, transform + file-I/O)

**Analog:** `src/formats/bsa/tes4_bsa_parser.hpp`

**Parser header shape** (lines 14-31):
```cpp
namespace libbsa::formats::bsa {

/// Parsed TES4-family archive metadata and deterministic public entry values.
struct tes4_bsa_archive {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
};

/// Parses checked TES4-family BSA header, table, name, and entry metadata state.
result<tes4_bsa_archive> parse_tes4_bsa_archive(std::span<const std::byte> bytes, detected_bsa_format detected);

/// Parses checked TES4-family BSA state from bounded host-file metadata and payload-prefix reads.
result<tes4_bsa_archive> parse_tes4_bsa_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected);
```

**Apply:** Mirror this with `tes3_bsa_archive`, `parse_tes3_bsa_archive`, and `parse_tes3_bsa_archive_file`. Keep private parser types out of public headers.

---

### `src/formats/bsa/tes3_bsa_parser.cpp` (parser, transform + file-I/O)

**Analog:** `src/formats/bsa/tes4_bsa_parser.cpp`

**Imports pattern** (lines 1-13):
```cpp
#include "formats/bsa/tes4_bsa_parser.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>

#include <algorithm>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>
```

**Checked arithmetic and span helpers** (lines 64-82):
```cpp
bool multiply_fits(std::uint32_t count, std::size_t width, std::size_t& total) noexcept {
  if (width != 0U && count > std::numeric_limits<std::size_t>::max() / width) {
    return false;
  }
  total = static_cast<std::size_t>(count) * width;
  return true;
}

bool span_fits(std::size_t start, std::size_t length, std::size_t total) noexcept {
  return start <= total && length <= total - start;
}
```

**Host-file bounded metadata read pattern** (lines 84-107):
```cpp
result<std::vector<std::byte>> read_file_bytes_at(std::ifstream& input, std::uint64_t offset, std::size_t count,
                                                  std::string_view description) {
  if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
    return error{error_code::format_error, std::string{description} + " offset exceeds stream limits"};
  }
  ...
  if (static_cast<std::size_t>(input.gcount()) != bytes.size()) {
    return error{error_code::format_error, std::string{description} + " is truncated"};
  }
  return bytes;
}
```

**Entry materialization pattern** (lines 340-399):
```cpp
std::vector<entry_metadata> entries;
entries.reserve(header.file_count);
std::unordered_set<std::string> canonical_paths;
...
auto canonical = detail::normalize_archive_path(original_path);
if (!canonical) {
  return canonical.error();
}
if (!canonical_paths.insert(canonical.value().value).second) {
  return error{error_code::format_error, "TES4 BSA contains duplicate canonical archive paths"};
}
...
entries.push_back(entry_metadata{canonical.value().value,
                                 std::move(original_path),
                                 raw_size.value(),
                                 stored_size,
                                 record.offset,
                                 detail::hash_tes4(file_name),
                                 compression,
                                 record.size_flags & file_size_compression_toggle,
                                 has_embedded_names,
                                 prefix_size});
...
std::sort(entries.begin(), entries.end(), [](const entry_metadata& lhs, const entry_metadata& rhs) {
  return lhs.path < rhs.path;
});
```

**File parser wrapper pattern** (lines 578-612):
```cpp
result<tes4_bsa_archive> parse_tes4_bsa_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected) {
  if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return error{error_code::format_error, "TES4 BSA archive exceeds platform limits"};
  }

  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  ...
  return parse_tes4_bsa_archive_impl(table_bytes.value(), static_cast<std::size_t>(archive_size), detected,
                                     read_payload_bytes);
}
```

**Apply:** Use these checked read/allocation/error patterns for TES3 header, size/offset table, name offsets, zstring name section, and hash table. Add a compatibility comment at TES3 raw-offset conversion citing `TES5Edit/Core/wbBSArchive.pas:1128-1129,2114-2118` and UESP.

---

### `src/formats/bsa/tes3_bsa_reader.hpp` / `.cpp` (reader helper, request-response + file-I/O)

**Analog:** `src/formats/bsa/tes4_bsa_reader.*`

**Reader API shape** (hpp lines 13-24):
```cpp
/// Returns stable TES4-family entry metadata copies sorted by canonical archive path.
result<std::vector<entry_metadata>> tes4_bsa_entries(std::span<const entry_metadata> entries);

/// Looks up one TES4-family entry using public archive-path normalization semantics.
result<std::optional<entry_metadata>> find_tes4_bsa_entry(std::span<const entry_metadata> entries, std::string_view path);

/// Reports TES4-family entry presence using the same normalization and errors as find.
result<bool> contains_tes4_bsa_entry(std::span<const entry_metadata> entries, std::string_view path);
```

**Lookup pattern** (cpp lines 106-131):
```cpp
result<std::vector<entry_metadata>> tes4_bsa_entries(std::span<const entry_metadata> entries) {
  return std::vector<entry_metadata>{entries.begin(), entries.end()};
}

result<std::optional<entry_metadata>> find_tes4_bsa_entry(std::span<const entry_metadata> entries, std::string_view path) {
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
}
```

**Raw/uncompressed sink pattern** (cpp lines 47-67, 85-87):
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

if (entry.compression == entry_compression::none) {
  return write_in_chunks(sink, payload.value());
}
```

**Apply:** Prefer neutral shared helper names if planner allows; otherwise mirror TES4 helper shape for TES3. TES3 extraction must only allow `entry_compression::none` and must not invoke `compression_router`.

---

### `src/detail/bethesda_hash.*` (utility, transform)

**Analog:** `src/detail/bethesda_hash.*`

**Public internal declarations** (hpp lines 8-15):
```cpp
/// Returns the TES3 BSA hash for an archive path using TES5Edit-compatible byte rules.
std::uint64_t hash_tes3(std::string_view archive_path);

/// Returns the TES4-family BSA hash after splitting `name_or_path` at its final dot.
std::uint64_t hash_tes4(std::string_view name_or_path);
```

**TES3 compatibility comment and implementation pattern** (cpp lines 55-77):
```cpp
std::uint64_t hash_tes3(std::string_view archive_path) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashTES3 halves the byte string, lowercases
  // ASCII only via LowerByte, then XORs shifted bytes with a rotate in the low half.
  const auto half = archive_path.size() >> 1U;
  std::uint32_t sum = 0;
  ...
  return result | sum;
}
```

**Apply:** Reuse `hash_tes3(parsed_archive_name)` for parser validation and generator output. If adding `tes3_hash_low32`, `tes3_hash_high32`, or `tes3_hash_sort_key`, follow this header doc + cpp helper pattern and test with `tests/unit/bethesda_hash_tests.cpp`.

---

### `CMakeLists.txt` (config, build graph)

**Analog:** `CMakeLists.txt`

**Library source registration pattern** (lines 42-65):
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
    src/detail/bethesda_hash.cpp
    src/detail/binary_io.cpp
    src/formats/bsa/bsa_format_detector.cpp
    src/formats/bsa/tes4_bsa_parser.cpp
    src/formats/bsa/tes4_bsa_reader.cpp
    src/libbsa.cpp
)
```

**Apply:** Add `src/formats/bsa/tes3_bsa_parser.cpp` and `src/formats/bsa/tes3_bsa_reader.cpp` under `PRIVATE`; do not add private TES3 headers to public `FILE_SET`.

---

### `tests/CMakeLists.txt` (test config, build/test graph)

**Analog:** `tests/CMakeLists.txt`

**Test source pattern** (lines 4-18):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/tes4_bsa_reader_tests.cpp
  unit/public_include_boundary_tests.cpp
  ...
  unit/bethesda_hash_tests.cpp
)
```

**Fixture generator pattern** (lines 39-67):
```cmake
add_executable(generate_tes4_bsa_fixtures_tool
  fixtures/generated/generate_tes4_bsa_fixtures.cpp
)

target_link_libraries(generate_tes4_bsa_fixtures_tool
  PRIVATE
    libbsa::libbsa
)
...
add_custom_target(generate_tes4_bsa_fixtures
  COMMAND generate_tes4_bsa_fixtures_tool --success --output ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_tes4_bsa_fixtures_tool
  BYPRODUCTS
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v103.bsa
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/tes4_v103_manifest.json
  COMMENT "Generating TES4-family BSA success fixtures"
  VERBATIM
)
```

**CTest discovery pattern** (lines 69-74):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Apply:** Add `unit/tes3_bsa_reader_tests.cpp`, `generate_tes3_bsa_fixtures_tool`, and a TES3 custom target/byproducts list. Keep Catch2 tags as labels.

---

### `tests/fixtures/generated/generate_tes3_bsa_fixtures.cpp` (fixture generator, file-I/O + batch)

**Analog:** `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp`

**Byte writer pattern** (lines 27-73):
```cpp
struct byte_buffer {
  std::vector<std::byte> bytes;

  void u8(std::uint8_t value) { bytes.push_back(static_cast<std::byte>(value)); }

  void u32(std::uint32_t value) {
    for (std::uint32_t index = 0; index < 4; ++index) {
      u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
  }

  void u64(std::uint64_t value) {
    for (std::uint32_t index = 0; index < 8; ++index) {
      u8(static_cast<std::uint8_t>((value >> (index * 8U)) & 0xFFU));
    }
  }
```

**Canonical/path helper pattern** (lines 164-177):
```cpp
std::string canonicalize(std::string value) {
  std::replace(value.begin(), value.end(), '\\', '/');
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::uint32_t checked_u32(std::size_t value, std::string_view what) {
  if (value > UINT32_MAX) {
    throw std::runtime_error(std::string(what) + " does not fit in uint32");
  }
  return static_cast<std::uint32_t>(value);
}
```

**Provenance manifest pattern** (lines 399-402, 407-425):
```cpp
out << "  \"provenance\": {\n";
out << "    \"generator\": \"tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp\",\n";
out << "    \"source\": \"synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied\"\n";
...
out << "      \"path\": \"" << json_escape(entry.canonical_path) << "\",\n";
out << "      \"original_path\": \"" << json_escape(entry.path) << "\",\n";
out << "      \"hash\": \"0x" << std::hex << std::setw(16) << entry.hash << "\",\n";
out << "      \"raw_size\": " << entry.raw_size << ",\n";
out << "      \"stored_size\": " << entry.stored_size << ",\n";
out << "      \"offset\": " << entry.offset << ",\n";
```

**Malformed mutation pattern** (lines 544-590):
```cpp
void generate_malformed(const std::filesystem::path& output_dir) {
  std::filesystem::create_directories(output_dir);
  ...
  auto truncated_table = make_v103();
  truncated_table.stem = "malformed_truncated_table";
  write_archive(truncated_table, output_dir);
  auto truncated_table_bytes = read_file(output_dir / "malformed_truncated_table.bsa");
  truncated_table_bytes.resize(48);
  write_file(output_dir / "malformed_truncated_table.bsa", truncated_table_bytes);
  ...
  write_text(output_dir / "malformed_manifest.json", malformed_manifest());
}
```

**Apply:** Use TES3-specific table layout and manifests: include `raw_tes3_data_offset`, archive-absolute `payload_offset`, `archive_hash`, `hash_low32`, `hash_high32`, and expected payload hex. Generate success and mandatory malformed cases from repository-owned bytes only.

---

### `tests/unit/tes3_bsa_reader_tests.cpp` (test, request-response + file-I/O)

**Analog:** `tests/unit/tes4_bsa_reader_tests.cpp`

**Imports and fixture paths pattern** (lines 1-25):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
...
#include <nlohmann/json.hpp>

std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}
```

**Manifest helpers and sinks pattern** (lines 74-126):
```cpp
std::uint64_t hex_u64_from_manifest(const nlohmann::json& value) {
  return std::stoull(value.get<std::string>(), nullptr, 16);
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  return nlohmann::json::parse(stream);
}
...
class collecting_sink final : public libbsa::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }
```

**Metadata/listing assertion pattern** (lines 273-306):
```cpp
TEST_CASE("tes4_bsa_entry_metadata materializes table paths, hashes, sizes, and embedded names",
          "[unit][fixture][tes4_bsa_entry_metadata][tes4_bsa_listing][tes4_bsa_embedded_name]") {
  ...
  REQUIRE(actual.path == expected.at("path").get<std::string>());
  REQUIRE(actual.original_path == archive_original_path_from_manifest(expected.at("original_path").get<std::string>()));
  REQUIRE(actual.raw_size == expected.at("raw_size").get<std::uint64_t>());
  REQUIRE(actual.stored_size == expected.at("stored_size").get<std::uint64_t>());
  REQUIRE(actual.payload_offset == expected.at("offset").get<std::uint64_t>());
  REQUIRE(actual.tes4_hash == hex_u64_from_manifest(expected.at("hash")));
```

**Lookup assertion pattern** (lines 318-356):
```cpp
for (const auto& expected : manifest.at("entries")) {
  for (const auto& variant : expected.at("lookup_variants")) {
    auto found = opened.value().find(variant.get<std::string>());
    REQUIRE(found.has_value());
    REQUIRE(found.value().has_value());
    REQUIRE(found.value()->path == expected.at("path").get<std::string>());
    ...
    auto contains = opened.value().contains(variant.get<std::string>());
    REQUIRE(contains.has_value());
    REQUIRE(contains.value());
  }
}
```

**Extraction and error patterns** (lines 360-417, 449-464):
```cpp
collecting_sink sink;

auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

REQUIRE(extracted.has_value());
REQUIRE(sink.bytes() == bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>()));
...
REQUIRE_FALSE(extracted.has_value());
REQUIRE(extracted.error().code == error_code_from_manifest(test_case.at("expected_error").get<std::string>()));
```

**Apply:** Mirror test shape with TES3 tags (`tes3_bsa_detection`, `tes3_bsa_metadata`, `tes3_bsa_entries`, `tes3_bsa_lookup`, `tes3_bsa_extract`, `tes3_bsa_malformed`). Update renamed field to `archive_hash` in both TES3 and existing TES4 tests.

---

### `tests/unit/tes4_bsa_reader_tests.cpp` (test modification, request-response + file-I/O)

**Analog:** `tests/unit/tes4_bsa_reader_tests.cpp`

**Field rename sites** (lines 300-302, 331-333):
```cpp
REQUIRE(actual.payload_offset == expected.at("offset").get<std::uint64_t>());
REQUIRE(actual.tes4_hash == hex_u64_from_manifest(expected.at("hash")));
...
REQUIRE(found.value()->path == expected.at("path").get<std::string>());
REQUIRE(found.value()->tes4_hash == hex_u64_from_manifest(expected.at("hash")));
```

**Apply:** Change these to `archive_hash` after public model rename; keep all TES4 behavior assertions otherwise unchanged.

## Shared Patterns

### Error Handling
**Source:** `src/formats/bsa/tes4_bsa_parser.cpp` lines 56-61, 485-503  
**Apply to:** TES3 parser/detector/open-path validation
```cpp
result<void> skip_checked(detail::binary_reader& reader, std::size_t count) {
  auto skipped = reader.skip(count);
  if (!skipped) {
    return error{error_code::format_error, "TES4 BSA table is truncated"};
  }
  return {};
}
...
if (table_bytes.size() < fixed_header_size) {
  return error{error_code::format_error, "TES4 BSA header is truncated"};
}
```

### Path Normalization and Duplicate Rejection
**Source:** `src/formats/bsa/tes4_bsa_parser.cpp` lines 344-363; `src/detail/archive_path.hpp` lines 15-19  
**Apply to:** TES3 parser, lookup tests, malformed duplicate canonical path case
```cpp
std::unordered_set<std::string> canonical_paths;
...
auto canonical = detail::normalize_archive_path(original_path);
if (!canonical) {
  return canonical.error();
}
if (!canonical_paths.insert(canonical.value().value).second) {
  return error{error_code::format_error, "TES4 BSA contains duplicate canonical archive paths"};
}
```

### Deterministic Entry Ordering
**Source:** `src/formats/bsa/tes4_bsa_parser.cpp` lines 396-399  
**Apply to:** TES3 parser entries before storing reader state
```cpp
std::sort(entries.begin(), entries.end(), [](const entry_metadata& lhs, const entry_metadata& rhs) {
  return lhs.path < rhs.path;
});
return entries;
```

### TES3 Hash Compatibility
**Source:** `src/detail/bethesda_hash.cpp` lines 55-77; `tests/unit/bethesda_hash_tests.cpp` lines 5-24  
**Apply to:** TES3 parser hash validation, generator hash manifest, new helper tests
```cpp
// TES5Edit/Core/wbBSArchive.pas CreateHashTES3 halves the byte string, lowercases
// ASCII only via LowerByte, then XORs shifted bytes with a rotate in the low half.
const auto half = archive_path.size() >> 1U;
```

### Sink Contract / Partial Write Handling
**Source:** `src/formats/bsa/tes4_bsa_reader.cpp` lines 47-67  
**Apply to:** TES3 extraction helper or shared neutral raw payload extraction
```cpp
auto written = sink.write(bytes);
if (!written) {
  return written.error();
}
if (written.value() != bytes.size()) {
  return error{error_code::io_error, "payload sink accepted a partial chunk"};
}
```

### Fixture Provenance
**Source:** `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` lines 399-402, 462-470  
**Apply to:** TES3 success and malformed manifests
```cpp
"provenance": {
  "generator": "tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp",
  "source": "synthetic strings generated for libbsa tests; no game or TES5Edit bytes copied"
}
```

## No Analog Found

All planned Phase 4 files have a close existing analog. The weakest analog is committed generated TES3 archive bytes themselves; use the TES4 generated fixture outputs as provenance/manifest analogs and the new TES3 generator as the source of truth.

## Metadata

**Analog search scope:** `include/**/*.hpp`, `src/**/*.hpp`, `src/**/*.cpp`, `tests/**/*.cpp`, `**/CMakeLists.txt`  
**Files scanned:** 34 listed by glob plus targeted grep for hash/detector/extraction references  
**Pattern extraction date:** 2026-05-08
