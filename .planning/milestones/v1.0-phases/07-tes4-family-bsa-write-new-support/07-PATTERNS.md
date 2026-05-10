# Phase 07: TES4-Family BSA Write-New Support - Pattern Map

**Mapped:** 2026-05-08
**Files analyzed:** 9
**Analogs found:** 9 / 9

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/writer.hpp` | public API | request-response | `include/libbsa/archive.hpp` | role-match |
| `include/libbsa/libbsa.hpp` | public umbrella header | config | `include/libbsa/libbsa.hpp` | exact-modify |
| `src/formats/bsa/tes4_bsa_writer.hpp` | private format interface | transform | `src/formats/bsa/tes4_bsa_parser.hpp` / `src/formats/bsa/tes4_bsa_reader.hpp` | role-match |
| `src/formats/bsa/tes4_bsa_writer.cpp` | format writer/serializer | file-I/O + transform | `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` | flow-match |
| `src/archive.cpp` | facade integration | request-response + file-I/O | `src/archive.cpp` | exact-modify |
| `CMakeLists.txt` | build config | config | `CMakeLists.txt` | exact-modify |
| `tests/unit/tes4_bsa_writer_tests.cpp` | test | file-I/O + request-response | `tests/unit/tes4_bsa_reader_tests.cpp` | role-match |
| `tests/unit/public_include_boundary_tests.cpp` | public API boundary test | config + request-response | `tests/unit/public_include_boundary_tests.cpp` | exact-modify |
| `tests/CMakeLists.txt` | test build config | config | `tests/CMakeLists.txt` | exact-modify |

## Pattern Assignments

### `include/libbsa/writer.hpp` (public API, request-response)

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

**Dependency-light enum pattern** (lines 33-42):
```cpp
/// Compression representation selected by archive metadata for an entry.
///
/// The values describe observable archive payload encoding without exposing the
/// private codec libraries used to implement each mode.
enum class entry_compression {
  none,
  deflate,
  lz4_frame,
  lz4_block,
};
```

**Public class/result pattern** (lines 171-214):
```cpp
/// Public archive reader for supported Bethesda archive files.
///
/// Use `open()` for fallible construction. A successfully opened reader exposes
/// archive metadata, deterministic entry listings, canonical path lookup, and
/// synchronous extraction for the archive variants implemented by libbsa.
class archive_reader {
 public:
  /// Attempts to open an archive from a host path string.
  ///
  /// Empty host paths return `error_code::invalid_argument`, missing or
  /// unreadable files return `error_code::io_error`, unsupported archive bytes
  /// return `error_code::unsupported`, and malformed supported archives return
  /// `error_code::format_error`.
  static result<archive_reader> open(std::string_view host_path);

  /// Returns archive-level metadata for a successfully opened archive.
  [[nodiscard]] result<archive_metadata> metadata() const;

 private:
  struct state;

  explicit archive_reader(archive_metadata metadata);

  std::shared_ptr<const state> state_;
};
```

**Apply to writer:** define libbsa-owned target/profile and compression-policy enums, Doxygen every public type/method, accept archive-internal paths as `std::string_view`, memory bytes as `std::span<const std::byte>`, host output as `std::string_view`, and return `result<void>`/`result<...>` with no public codec/TES5Edit types.

---

### `include/libbsa/libbsa.hpp` (public umbrella header, config)

**Analog:** `include/libbsa/libbsa.hpp`

**Umbrella include pattern** (lines 1-5):
```cpp
#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>
#include <libbsa/version.hpp>
```

**Apply to writer:** add `#include <libbsa/writer.hpp>` next to the existing public headers. Keep umbrella-only behavior; do not add declarations here.

---

### `src/formats/bsa/tes4_bsa_writer.hpp` (private format interface, transform)

**Analog:** `src/formats/bsa/tes4_bsa_parser.cpp` and `src/formats/bsa/tes4_bsa_reader.cpp`

**Namespace/import style** (parser lines 1-15):
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

namespace libbsa::formats::bsa {
```

**Result-returning helper style** (reader lines 19-24):
```cpp
result<std::size_t> checked_size(std::uint64_t value, std::string_view description) {
  if (value > std::numeric_limits<std::size_t>::max()) {
    return error{error_code::format_error, std::string{description} + " exceeds platform limits"};
  }
  return static_cast<std::size_t>(value);
}
```

**Apply to writer:** keep declarations under `libbsa::formats::bsa`, expose a small private entry/options model consumed by the public writer facade, and return `result<void>` or `result<writer_model>` for validation/build steps.

---

### `src/formats/bsa/tes4_bsa_writer.cpp` (format writer/serializer, file-I/O + transform)

**Analog:** `tests/fixtures/generated/generate_tes4_bsa_fixtures.cpp` (layout behavior only; production code must not remain test-only)

**Imports pattern for serializer helpers** (lines 1-18):
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

**Little-endian serialization pattern** (fixture lines 27-51; prefer `detail::binary_writer` in production):
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

  void raw(std::span<const std::byte> values) { bytes.insert(bytes.end(), values.begin(), values.end()); }

  void string_term(std::string_view value) {
    for (const char ch : value) {
      u8(static_cast<std::uint8_t>(ch));
    }
    u8(0);
  }
```

**Production binary writer analog** (`src/detail/binary_io.cpp` lines 82-113):
```cpp
result<void> binary_writer::write_u32_le(std::uint32_t value) {
  for (std::size_t index = 0; index < 4; ++index) {
    bytes_.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
  }
  return {};
}

result<void> binary_writer::write_u64_le(std::uint64_t value) {
  for (std::size_t index = 0; index < 8; ++index) {
    bytes_.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
  }
  return {};
}

result<void> binary_writer::write_bytes(std::span<const std::byte> bytes) {
  bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
  return {};
}
```

**Payload encoding/compression pattern** (fixture lines 179-205):
```cpp
void prepare_payload(entry_spec& entry) {
  entry.hash = libbsa::detail::hash_tes4(entry.file);
  entry.canonical_path = canonicalize(entry.path);
  entry.raw_size = checked_u32(entry.expected_bytes.size(), "raw payload size");

  byte_buffer payload;
  if (entry.has_embedded_name) {
    // FO3/SSE embedded names are a length-prefixed filename before the consumer payload.
    entry.embedded_name_prefix_size = checked_u32(entry.embedded_name.size() + 1, "embedded name prefix");
    payload.string_len_raw(entry.embedded_name);
  }

  if (entry.compression == libbsa::detail::compression_method::none) {
    payload.raw(entry.expected_bytes);
  } else {
    payload.u32(entry.raw_size);
    const auto compressed = libbsa::detail::compress_payload(entry.compression, entry.expected_bytes);
    if (!compressed) {
      throw std::runtime_error("failed to compress fixture payload: " + compressed.error().message);
    }
    payload.raw(compressed.value());
    entry.record_flags |= file_size_compression_toggle;
  }

  entry.stored_payload = std::move(payload.bytes);
  entry.stored_size = checked_u32(entry.stored_payload.size(), "stored payload size");
}
```

**Table/offset serialization pattern** (fixture lines 319-383):
```cpp
void write_archive(archive_spec& archive, const std::filesystem::path& output_dir) {
  for (auto& entry : archive.entries) {
    prepare_payload(entry);
  }
  archive.folder_hash = libbsa::detail::hash_tes4(archive.folder);

  const auto folder_record_size = archive.version == 0x69 ? 24U : 16U;
  const auto folder_block_size = checked_u32(archive.folder.size() + 2 + archive.entries.size() * 16U, "folder block size");
  std::uint32_t file_names_length = 0;
  for (const auto& entry : archive.entries) {
    file_names_length += checked_u32(entry.file.size() + 1, "file names length");
  }

  const std::uint32_t metadata_size = 36U + folder_record_size + folder_block_size + file_names_length;
  // TES5Edit stores each folder record offset with the file-name block contribution folded in.
  archive.folder_offset = 36U + folder_record_size + file_names_length;

  std::uint32_t next_payload_offset = metadata_size;
  for (auto& entry : archive.entries) {
    entry.offset = next_payload_offset;
    next_payload_offset += entry.stored_size;
  }

  byte_buffer writer;
  writer.u8('B');
  writer.u8('S');
  writer.u8('A');
  writer.u8(0);
  writer.u32(archive.version);
  writer.u32(36U);
  writer.u32(archive.flags);
  writer.u32(1U);
  writer.u32(checked_u32(archive.entries.size(), "file count"));
  writer.u32(checked_u32(archive.folder.size() + 2, "folder names length"));
  writer.u32(file_names_length);
  writer.u32(archive.file_flags);

  writer.u64(archive.folder_hash);
  writer.u32(checked_u32(archive.entries.size(), "folder file count"));
  if (archive.version == 0x69) {
    writer.u32(0U);
    writer.u64(archive.folder_offset);
  } else {
    writer.u32(archive.folder_offset);
  }
```

**Parser acceptance constraints to mirror** (`src/formats/bsa/tes4_bsa_parser.cpp` lines 216-220, 287-294, 348-392):
```cpp
// TES5Edit-compatible folder offsets include the later file-name table length,
// even though this parser consumes folder blocks sequentially from the stream.
if (folder.offset != static_cast<std::uint64_t>(reader.position()) + header.total_file_name_length) {
  return error{error_code::format_error, "TES4 BSA folder block offset does not match parsed table layout"};
}

entry_compression compression_for(const header_fields& header, std::uint32_t size_flags) noexcept {
  const bool default_compressed = (header.archive_flags & archive_compress_by_default) != 0U;
  const bool toggled = (size_flags & file_size_compression_toggle) != 0U;
  if (!(default_compressed ^ toggled)) {
    return entry_compression::none;
  }
  return header.version == sse_version ? entry_compression::lz4_frame : entry_compression::deflate;
}

const bool has_embedded_names = header.version != 0x67U && (header.archive_flags & archive_embed_names) != 0U;
```

**Apply to writer:** use `detail::binary_writer`, `detail::normalize_archive_path`, `detail::hash_tes4`, and `detail::compress_payload`. Convert fixture exceptions to structured `error{error_code::...}` returns. Keep the TES5Edit folder-offset comment or equivalent in production because it documents a non-obvious compatibility constraint.

---

### `src/archive.cpp` (facade integration, request-response + file-I/O)

**Analog:** `src/archive.cpp`

**Import grouping / facade dispatch pattern** (lines 1-18):
```cpp
#include <libbsa/archive.hpp>

#include "formats/ba2/ba2_format_detector.hpp"
#include "formats/ba2/ba2_dx10_parser.hpp"
#include "formats/ba2/ba2_dx10_reader.hpp"
#include "formats/ba2/ba2_gnrl_parser.hpp"
#include "formats/ba2/ba2_gnrl_reader.hpp"
#include "formats/bsa/bsa_format_detector.hpp"
#include "formats/bsa/tes3_bsa_parser.hpp"
#include "formats/bsa/tes3_bsa_reader.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_reader.hpp"

#include <cstddef>
#include <fstream>
#include <limits>
#include <string>
#include <vector>
```

**Public API error style** (lines 115-123):
```cpp
result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  auto prefix = read_detection_prefix(host_path);
  if (!prefix) {
    return prefix.error();
  }
```

**Apply to writer:** if public writer method definitions live in a facade `.cpp`, follow this pattern: validate arguments first, call private `formats::bsa` writer functions, propagate `.error()`, and store no global mutable state.

---

### `CMakeLists.txt` (build config, config)

**Analog:** `CMakeLists.txt`

**Public header file-set pattern** (lines 44-53):
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
```

**Private source registration pattern** (lines 54-76):
```cmake
    src/archive.cpp
    src/detail/archive_path.cpp
    src/detail/bethesda_hash.cpp
    src/detail/binary_io.cpp
    src/detail/compression_router.cpp
    src/detail/deflate_codec.cpp
    src/detail/lz4_block_codec.cpp
    src/detail/lz4_frame_codec.cpp
    src/detail/payload_stream.cpp
    src/formats/ba2/ba2_format_detector.cpp
    src/formats/ba2/ba2_dx10_parser.cpp
    src/formats/ba2/ba2_dx10_reader.cpp
    src/formats/ba2/ba2_gnrl_parser.cpp
    src/formats/ba2/ba2_gnrl_reader.cpp
    src/formats/bsa/bsa_format_detector.cpp
    src/formats/bsa/tes3_bsa_parser.cpp
    src/formats/bsa/tes3_bsa_reader.cpp
    src/formats/bsa/tes4_bsa_parser.cpp
    src/formats/bsa/tes4_bsa_reader.cpp
```

**Apply to writer:** add `include/libbsa/writer.hpp` to the public file set and `src/formats/bsa/tes4_bsa_writer.cpp` to private sources. Do not add new dependencies.

---

### `tests/unit/tes4_bsa_writer_tests.cpp` (test, file-I/O + request-response)

**Analog:** `tests/unit/tes4_bsa_reader_tests.cpp`

**Test imports and generated/temp path helpers** (lines 1-25):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}
```

**Sink and binary helpers for round-trip extraction** (lines 88-131):
```cpp
std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

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

void write_binary_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes) {
  std::ofstream output{path, std::ios::binary};
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}
```

**Open/metadata/listing assertion pattern** (lines 156-169, 273-306):
```cpp
auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
REQUIRE(opened.has_value());

auto metadata = opened.value().metadata();
REQUIRE(metadata.has_value());
REQUIRE(metadata.value().type == libbsa::archive_type::bsa);
REQUIRE(metadata.value().variant == expected_variant(fixture.version));
REQUIRE(metadata.value().version == fixture.version);
REQUIRE(metadata.value().archive_flags == fixture.flags);
REQUIRE(metadata.value().file_count == fixture.file_count);
REQUIRE(metadata.value().default_compression == expected_default_compression(fixture.version));

auto entries = opened.value().entries();
REQUIRE(entries.has_value());
REQUIRE(entries.value().size() == manifest.at("entries").size());
```

**Lookup/extract/byte-compare pattern** (lines 318-357, 360-405):
```cpp
auto found = opened.value().find(variant.get<std::string>());
REQUIRE(found.has_value());
REQUIRE(found.value().has_value());
REQUIRE(found.value()->path == expected.at("path").get<std::string>());

collecting_sink sink;

auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);

REQUIRE(extracted.has_value());
REQUIRE(sink.bytes() == bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>()));
```

**Stable error-code assertion pattern** (lines 420-431):
```cpp
auto invalid = opened.value().extract("/rooted/file.txt", sink);
REQUIRE_FALSE(invalid.has_value());
REQUIRE(invalid.error().code == libbsa::error_code::invalid_argument);

auto missing = opened.value().extract("valid/missing/path.txt", sink);
REQUIRE_FALSE(missing.has_value());
REQUIRE(missing.error().code == libbsa::error_code::not_found);
```

**Apply to writer tests:** create synthetic disk-source files under a temp directory, add memory-buffer entries, write output BSA, reopen with `archive_reader::open`, assert metadata/listing/find/contains/extract_bytes/extract sink behavior, and compare source payload bytes rather than compressed archive bytes. Use stable `error_code` assertions for invalid paths, duplicates, overwrite default, compression failure/invalid policy, and missing source file.

---

### `tests/unit/public_include_boundary_tests.cpp` (public API boundary test, config + request-response)

**Analog:** `tests/unit/public_include_boundary_tests.cpp`

**Static public type exposure pattern** (lines 14-20):
```cpp
static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");
static_assert(std::is_enum_v<libbsa::archive_type>);
static_assert(std::is_enum_v<libbsa::archive_variant>);
static_assert(std::is_enum_v<libbsa::entry_compression>);
static_assert(std::is_default_constructible_v<libbsa::ba2_archive_metadata>);
static_assert(std::is_abstract_v<libbsa::payload_sink>);
```

**Umbrella smoke pattern** (lines 21-46):
```cpp
TEST_CASE("public_include_boundary umbrella header exposes public boundary types", "[unit][public-api]") {
  [[maybe_unused]] libbsa::result<int> result{1};
  [[maybe_unused]] auto code = libbsa::error_code::unsupported;
  [[maybe_unused]] auto missing = libbsa::error_code::not_found;
  [[maybe_unused]] auto reader = libbsa::archive_reader::open("boundary-smoke.bsa");

  REQUIRE(result.has_value());
}
```

**Forbidden private token scan pattern** (lines 48-75):
```cpp
TEST_CASE("public_include_boundary excludes private Phase 2 implementation names", "[unit][public-api]") {
  constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                     "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                     "TES5Edit", "std::expected", "bethesda_hash",
                                                                     "compression_router", "archive_path_key"});
```

**Apply to writer:** add static assertions for new writer enums/classes/options, instantiate a writer from `libbsa.hpp`, and keep the forbidden token list green (no codec, DirectX, TES5Edit, or private helper names in public headers outside comments).

---

### `tests/CMakeLists.txt` (test build config, config)

**Analog:** `tests/CMakeLists.txt`

**Unit source registration pattern** (lines 4-25):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/tes4_bsa_reader_tests.cpp
  unit/tes3_bsa_reader_tests.cpp
  unit/ba2_gnrl_reader_tests.cpp
  unit/ba2_dx10_metadata_tests.cpp
  unit/dds_layout_tests.cpp
  unit/ba2_dx10_parser_tests.cpp
  unit/ba2_dx10_extraction_tests.cpp
  unit/ba2_dx10_malformed_tests.cpp
  unit/public_include_boundary_tests.cpp
  unit/local_game_fixture_tests.cpp
  unit/validation_policy_tests.cpp
  unit/binary_io_tests.cpp
  unit/archive_path_tests.cpp
  unit/payload_stream_tests.cpp
  unit/deflate_codec_tests.cpp
  unit/lz4_codec_tests.cpp
  unit/compression_router_tests.cpp
  unit/bethesda_hash_tests.cpp
)
```

**Catch discovery/label pattern** (lines 191-196):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Apply to writer:** add `unit/tes4_bsa_writer_tests.cpp` to `libbsa_tests`; tag tests with `[tes4_bsa_writer]` so `ctest -L tes4_bsa_writer` works through `ADD_TAGS_AS_LABELS`.

## Shared Patterns

### C++20 `result` / stable error handling
**Source:** `include/libbsa/result.hpp` lines 10-28, 31-36; `src/archive.cpp` lines 115-123
**Apply to:** public writer API, private writer implementation, writer tests
```cpp
/// Stable error categories returned by libbsa public APIs.
enum class error_code {
  unsupported,
  invalid_argument,
  not_found,
  io_error,
  format_error,
};

/// Structured error value returned by result-producing libbsa APIs.
struct error {
  error_code code;
  std::string message;
};
```

### Archive path validation and duplicate canonical keys
**Source:** `src/detail/archive_path.cpp` lines 24-57; `src/formats/bsa/tes4_bsa_parser.cpp` lines 357-363
**Apply to:** writer entry validation, duplicate detection tests
```cpp
result<archive_path_key> normalize_archive_path(std::string_view input) {
  if (input.empty() || input.front() == '/' || input.front() == '\\' || is_drive_rooted(input)) {
    return invalid_path_error();
  }
  // ... lowercases ASCII, converts '\\' to '/', rejects empty/. /.. segments ...
}

auto canonical = detail::normalize_archive_path(original_path);
if (!canonical) {
  return canonical.error();
}
if (!canonical_paths.insert(canonical.value().value).second) {
  return error{error_code::format_error, "TES4 BSA contains duplicate canonical archive paths"};
}
```

### TES4-family hash helper
**Source:** `src/detail/bethesda_hash.cpp` lines 87-138
**Apply to:** folder/file sorting and record hash serialization
```cpp
std::uint64_t hash_tes4(std::string_view name_or_path) {
  const auto dot = name_or_path.find_last_of('.');
  if (dot == std::string_view::npos) {
    return hash_tes4(name_or_path, {});
  }
  return hash_tes4(name_or_path.substr(0, dot), name_or_path.substr(dot));
}

std::uint64_t hash_tes4(std::string_view name_without_extension, std::string_view extension_with_dot) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashTES4 encodes name edge bytes and
  // extension special bits for .kf/.nif/.dds/.wav, then adds sdbm accumulators.
  const auto length = name_without_extension.size();
  if (length == 0) {
    return 0;
  }
  // ...
}
```

### Compression routing
**Source:** `src/detail/compression_router.cpp` lines 18-30
**Apply to:** writer payload encoding for raw, v103/v104 deflate, and v105 LZ4 frame
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

### Embedded-name and compression extraction semantics
**Source:** `src/formats/bsa/tes4_bsa_reader.cpp` lines 70-101
**Apply to:** writer stored payload construction and round-trip tests
```cpp
result<std::span<const std::byte>> consumer_payload_span(std::span<const std::byte> stored_payload,
                                                         const entry_metadata& entry) {
  if (entry.embedded_name_prefix_size > stored_payload.size()) {
    return error{error_code::format_error, "TES4 BSA embedded-name prefix exceeds stored payload"};
  }
  // Embedded names are part of the on-disk payload but not the consumer-visible file bytes.
  return stored_payload.subspan(entry.embedded_name_prefix_size);
}

if (payload.value().size() < 4U) {
  return error{error_code::format_error, "TES4 BSA compressed payload size prefix is truncated"};
}
const auto expected_size = read_u32_le(payload.value().first(4U));
```

### Public dependency boundary
**Source:** `tests/unit/public_include_boundary_tests.cpp` lines 48-75
**Apply to:** `include/libbsa/writer.hpp`, `include/libbsa/libbsa.hpp`
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                   "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
```

## No Analog Found

All expected Phase 7 files have close analogs. The only gap is semantic rather than structural: production `src/formats/bsa/tes4_bsa_writer.cpp` has no existing production writer analog, so use the TES4 fixture generator only for binary layout patterns and convert it to production `result`-style error handling.

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| — | — | — | No unassigned files. |

## Metadata

**Analog search scope:** `include/libbsa/*.hpp`, `src/archive.cpp`, `src/formats/bsa/*`, `src/detail/*`, `tests/unit/*`, `tests/fixtures/generated/*`, root/test CMake files
**Files scanned:** 20
**Pattern extraction date:** 2026-05-08
**Project constraints applied:** `TES5Edit/` read-only boundary; no new dependencies; Doxygen comments for public writer APIs; generated/synthetic writer tests only
