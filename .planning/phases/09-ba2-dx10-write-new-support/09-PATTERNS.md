# Phase 09: ba2-dx10-write-new-support - Pattern Map

**Mapped:** 2026-05-09
**Files analyzed:** 13
**Analogs found:** 13 / 13

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/writer.hpp` | public API/config | request-response | `include/libbsa/writer.hpp` BA2 GNRL surface | exact |
| `include/libbsa/libbsa.hpp` | public umbrella config | request-response | `include/libbsa/libbsa.hpp` | exact |
| `src/formats/ba2/ba2_dx10_writer.hpp` | private writer model/API | batch + file-I/O | `src/formats/ba2/ba2_gnrl_writer.hpp` | exact |
| `src/formats/ba2/ba2_dx10_writer.cpp` | writer service/serializer | batch + file-I/O | `src/formats/ba2/ba2_gnrl_writer.cpp` + DX10 parser/reader | exact |
| `src/texture/directxtex_analyzer.hpp` | texture adapter API | transform | `src/texture/directxtex_analyzer.hpp` | exact |
| `src/texture/directxtex_analyzer.cpp` | texture adapter service | transform + file-I/O validation | `src/texture/directxtex_analyzer.cpp` | exact |
| `src/texture/dds_layout.hpp` | utility/model | transform | `src/texture/dds_layout.hpp` | exact |
| `src/texture/dds_layout.cpp` | utility/service | transform | `src/texture/dds_layout.cpp` | exact |
| `tests/unit/ba2_dx10_writer_tests.cpp` | test | batch + file-I/O | `tests/unit/ba2_gnrl_writer_tests.cpp` + `ba2_dx10_extraction_tests.cpp` | exact |
| `tests/unit/dds_layout_tests.cpp` | test | transform | `tests/unit/dds_layout_tests.cpp` | exact |
| `tests/unit/public_include_boundary_tests.cpp` | test | request-response/config | `tests/unit/public_include_boundary_tests.cpp` | exact |
| `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` or adjacent writer fixture generator | test fixture generator | batch + file-I/O | `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` | exact |
| `CMakeLists.txt` and `tests/CMakeLists.txt` | build config | batch | existing source/test registration | exact |

## Pattern Assignments

### `include/libbsa/writer.hpp` (public API/config, request-response)

**Analog:** `include/libbsa/writer.hpp`

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

**Public target/options pattern** (lines 44-94):
```cpp
/// BA2 GNRL archive target profiles supported by the write-new API.
enum class ba2_gnrl_target {
  fallout4,
  starfield_v2,
  starfield_v3,
};

/// Options controlling BA2 GNRL write-new archive finalization.
struct ba2_gnrl_writer_options {
  archive_compression_policy compression = archive_compression_policy::target_default;
  bool overwrite_existing = false;
  bool deduplicate_payloads = false;
  std::uint32_t starfield_unknown1 = 1U;
  std::uint32_t starfield_unknown2 = 0U;
  std::uint32_t starfield_compression_method = 3U;
};
```

**Writer object pattern** (lines 154-207):
```cpp
/// Public writer for creating new BA2 GNRL archives.
class ba2_gnrl_writer {
 public:
  explicit ba2_gnrl_writer(ba2_gnrl_target target);
  explicit ba2_gnrl_writer(ba2_gnrl_target target, ba2_gnrl_writer_options options);

  [[nodiscard]] ba2_gnrl_target target() const noexcept;
  [[nodiscard]] const ba2_gnrl_writer_options& options() const noexcept;

  result<void> add_file(std::string_view archive_path,
                        std::string_view host_path,
                        entry_compression_policy compression = entry_compression_policy::inherit);

  result<void> write_to(std::string_view host_path) const;

 private:
  struct state;
  std::shared_ptr<state> state_;
};
```

**Apply to DX10:** copy the target/options/writer shape, but do **not** copy BA2 GNRL entry-level compression overrides (lines 96-106, 178-194) because CONTEXT D-05/D-07 require public DX10 compressed-only output. Add Doxygen comments for all new public DX10 types and methods.

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

**Apply to DX10:** if the DX10 writer remains in `writer.hpp`, no umbrella change is needed beyond keeping this include. If split into a new public header, add one `#include <libbsa/...>` here and register it in the public CMake file set.

---

### `src/formats/ba2/ba2_dx10_writer.hpp` (private writer model/API, batch + file-I/O)

**Analog:** `src/formats/ba2/ba2_gnrl_writer.hpp`

**Private entry/state handoff pattern** (lines 3-25):
```cpp
#include <libbsa/writer.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

struct ba2_gnrl_writer_entry {
  std::string archive_path_original;
  std::string archive_path_canonical;
  std::string host_path;
  std::vector<std::byte> memory_bytes;
  bool from_memory{false};
  ba2_gnrl_entry_options options{};
};

result<void> write_ba2_gnrl_archive(ba2_gnrl_target target,
                                    const ba2_gnrl_writer_options& options,
                                    std::span<const ba2_gnrl_writer_entry> entries,
                                    std::string_view output_host_path);
```

**Apply to DX10:** define `ba2_dx10_writer_entry` with original/canonical archive path plus writer-owned validated DDS bytes and translated texture/source-layout analysis. Omit GNRL `from_memory` and per-entry compression options unless needed internally; public input is DDS host-file-only.

---

### `src/formats/ba2/ba2_dx10_writer.cpp` (writer service/serializer, batch + file-I/O)

**Analogs:** `src/formats/ba2/ba2_gnrl_writer.cpp`, `src/formats/ba2/ba2_dx10_parser.cpp`, `src/formats/ba2/ba2_dx10_reader.cpp`

**Imports pattern** (`ba2_gnrl_writer.cpp` lines 1-21):
```cpp
#include "formats/ba2/ba2_gnrl_writer.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/compression_router.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
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
#include <vector>
```

**Public wrapper/state pattern** (`ba2_gnrl_writer.cpp` lines 23-115):
```cpp
struct ba2_gnrl_writer::state {
  ba2_gnrl_target target;
  ba2_gnrl_writer_options options;
  std::vector<formats::ba2::ba2_gnrl_writer_entry> entries;
};

result<formats::ba2::ba2_gnrl_writer_entry> make_entry(std::string_view archive_path,
                                                        ba2_gnrl_entry_options options) {
  auto canonical = detail::normalize_archive_path(archive_path);
  if (!canonical) {
    return canonical.error();
  }
  formats::ba2::ba2_gnrl_writer_entry entry;
  entry.archive_path_original = preserved_archive_path(archive_path);
  entry.archive_path_canonical = std::move(canonical.value().value);
  entry.options = options;
  return entry;
}

result<void> ba2_gnrl_writer::write_to(std::string_view host_path) const {
  return formats::ba2::write_ba2_gnrl_archive(state_->target, state_->options, state_->entries, host_path);
}
```

**Compression routing pattern** (`ba2_gnrl_writer.cpp` lines 247-263):
```cpp
result<detail::compression_method> compression_method_for_compressed_entry(ba2_gnrl_target target,
                                                                           std::uint32_t starfield_method) {
  switch (target) {
  case ba2_gnrl_target::fallout4:
  case ba2_gnrl_target::starfield_v2:
    return detail::compression_method::deflate;
  case ba2_gnrl_target::starfield_v3:
    if (starfield_method == starfield_deflate_method) {
      return detail::compression_method::deflate;
    }
    if (starfield_method == starfield_lz4_block_method) {
      return detail::compression_method::lz4_block;
    }
    return error{error_code::unsupported, "BA2 GNRL Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 GNRL writer target profile is not supported"};
}
```

**DX10 record/chunk read shape to mirror for serialization** (`ba2_dx10_parser.cpp` lines 224-280):
```cpp
const auto name_hash = reader.read_u32_le();
const auto skipped_ext = reader.skip(4U);
const auto directory_hash = reader.read_u32_le();
const auto unknown_tex = reader.read_u8();
const auto chunk_count = reader.read_u8();
const auto chunk_header_size = reader.read_u16_le();
const auto height = reader.read_u16_le();
const auto width = reader.read_u16_le();
const auto num_mips = reader.read_u8();
const auto dxgi_format = reader.read_u8();
const auto cube_maps_raw = reader.read_u16_le();
...
const auto offset = reader.read_u64_le();
const auto packed_size = reader.read_u32_le();
const auto raw_size = reader.read_u32_le();
const auto start_mip = reader.read_u16_le();
const auto end_mip = reader.read_u16_le();
const auto sentinel = reader.read_u32_le();
if (sentinel.value() != ba2_record_sentinel) {
  return error{error_code::format_error, "BA2 DX10 chunk BAADF00D sentinel is invalid"};
}
```

**DX10 constants and fixed chunk header pattern** (`ba2_dx10_parser.cpp` lines 21-30, 247-250):
```cpp
constexpr std::uint32_t ba2_btdx_magic = 0x5844'5442U;
constexpr std::uint32_t ba2_dx10_magic = 0x3031'5844U;
constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
constexpr std::uint16_t ba2_dx10_chunk_header_size = 24U;
constexpr std::uint16_t ba2_dx10_cubemap_raw = 2049U;
...
if (chunk_header_size.value() != ba2_dx10_chunk_header_size) {
  return error{error_code::format_error, "BA2 DX10 chunk_header_size is unsupported"};
}
```

**Dedupe and offset pattern** (`ba2_gnrl_writer.cpp` lines 336-382):
```cpp
std::map<std::vector<std::byte>, payload_assignment> deduplicated_payloads;
for (auto& entry : entries) {
  if (deduplicate_payloads) {
    const auto duplicate = deduplicated_payloads.find(entry.stored_payload);
    if (duplicate != deduplicated_payloads.end()) {
      entry.payload_offset = duplicate->second.offset;
      entry.owns_payload_bytes = false;
      continue;
    }
  }
  entry.payload_offset = entry.stored_payload.empty() ? first_payload_offset : cursor;
  entry.owns_payload_bytes = true;
  ...
}
```

**DX10 adjustment:** do not copy the GNRL key unchanged. CONTEXT D-20 requires the DX10 key to include final stored bytes plus raw size, stored/packed size, and compression route.

**Safe publish pattern** (`ba2_gnrl_writer.cpp` lines 482-504, 646-693):
```cpp
result<std::filesystem::path> make_unique_publish_directory(const std::filesystem::path& output_path) {
  ...
  // A unique directory avoids deleting caller-owned deterministic siblings such as `<archive>.tmp`.
  if (std::filesystem::create_directory(candidate, fs_error)) {
    return candidate;
  }
  ...
}

if (options.overwrite_existing) {
  ...
  // Move the old archive aside before publish so a failed replacement can roll back to the last good file.
  std::filesystem::rename(output_path, backup_path.value(), fs_error);
  ...
}
std::filesystem::rename(temp_path, output_path, fs_error);
```

**Filename table serialization pattern** (`ba2_gnrl_writer.cpp` lines 385-399, 454-458):
```cpp
result<void> write_name(detail::binary_writer& writer, std::string_view name) {
  auto length = checked_u16(name.size(), "BA2 GNRL filename-table entry length");
  ...
  for (const char ch : name) {
    if (!(written = writer.write_u8(static_cast<std::uint8_t>(static_cast<unsigned char>(ch))))) {
      return written.error();
    }
  }
  return {};
}
...
for (const auto& entry : entries) {
  if (!(written = write_name(writer, entry.archive_path_original))) {
    return written.error();
  }
}
```

---

### `src/texture/directxtex_analyzer.hpp` (texture adapter API, transform)

**Analog:** `src/texture/directxtex_analyzer.hpp`

**Private adapter declaration pattern** (lines 11-15):
```cpp
/// Loads DDS metadata through the private texture analyzer boundary.
///
/// The returned value is immediately translated into libbsa-owned texture metadata so internal
/// callers and future writer phases do not pass third-party metadata objects across module seams.
result<texture_metadata> analyze_dds_metadata(std::span<const std::byte> dds_bytes);
```

**Apply to DX10:** add a new internal analysis result that includes libbsa-owned metadata and validated image/subresource byte spans or copied payload bytes. Keep DirectXTex types out of the header.

---

### `src/texture/directxtex_analyzer.cpp` (texture adapter service, transform + validation)

**Analog:** `src/texture/directxtex_analyzer.cpp`

**Private DirectXTex include boundary** (lines 1-12):
```cpp
#include "texture/directxtex_analyzer.hpp"

// The analyzer includes Windows-backed texture headers privately; NOMINMAX keeps those headers
// from rewriting standard-library min/max calls in this translation unit.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <DirectXTex.h>

#include <cstdint>
#include <limits>
#include <string>
```

**Metadata translation/error pattern** (lines 26-59):
```cpp
result<texture_metadata> analyze_dds_metadata(std::span<const std::byte> dds_bytes) {
  DirectX::TexMetadata metadata{};
  const HRESULT hr = DirectX::GetMetadataFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, metadata);
  if (hr < 0) {
    return error{error_code::format_error, "DDS metadata could not be loaded by the texture analyzer"};
  }

  auto width = checked_u32(metadata.width, "DDS width");
  ...
  texture_metadata translated{};
  translated.width = width.value();
  translated.height = height.value();
  translated.mip_count = mip_count.value();
  translated.dxgi_format = static_cast<std::uint32_t>(metadata.format);
  translated.array_size = array_size.value();
  translated.is_cubemap = metadata.IsCubemap();
  translated.unknown_tex = 0U;
  translated.cube_maps_raw = 0U;
  return translated;
}
```

**Apply to DX10:** use the same `HRESULT < 0` structured `format_error` pattern. For writer add-time validation use `LoadFromDDSMemory`/`ScratchImage` in this `.cpp` only, then translate all metadata/layout into libbsa-owned values before returning.

---

### `src/texture/dds_layout.hpp` (utility/model, transform)

**Analog:** `src/texture/dds_layout.hpp`

**Layout model and validator API pattern** (lines 13-49):
```cpp
/// Texture dimensions and DDS DXT10 metadata needed to reconstruct a header.
struct dds_texture_layout {
  std::uint32_t width;
  std::uint32_t height;
  std::uint32_t mip_count;
  std::uint32_t dxgi_format;
  std::uint32_t array_size;
  bool is_cubemap;
};

/// Identity of a validated texture payload segment in computed DDS output order.
struct logical_texture_segment {
  std::uint32_t array_index;
  std::uint32_t face_index;
  std::uint32_t start_mip;
  std::uint32_t end_mip;
  std::size_t source_chunk_index;
};

[[nodiscard]] result<std::vector<logical_texture_segment>> validate_and_order_chunks(
    const dds_texture_layout& layout, std::span<const texture_chunk_metadata> chunks);
```

**Apply to DX10:** extend with planner result structs for source DDS chunk ranges and archive chunk records. Keep them in `libbsa::texture` and keep all DirectXTex/DXGI names as numeric/libbsa-owned abstractions.

---

### `src/texture/dds_layout.cpp` (utility/service, transform)

**Analog:** `src/texture/dds_layout.cpp`

**Checked arithmetic and mip sizing pattern** (lines 30-97):
```cpp
libbsa::error format_error(std::string message) {
  return {libbsa::error_code::format_error, std::move(message)};
}

bool checked_mul(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& total) noexcept {
  if (rhs != 0U && lhs > std::numeric_limits<std::uint64_t>::max() / rhs) {
    return false;
  }
  total = lhs * rhs;
  return true;
}

std::uint32_t mip_dimension(std::uint32_t dimension, std::uint32_t mip) noexcept {
  const auto shifted = mip >= 31U ? 0U : dimension >> mip;
  return std::max(1U, shifted);
}

result<std::uint64_t> mip_size_for_format(const dds_texture_layout& layout, std::uint32_t mip) {
  const auto width = mip_dimension(layout.width, mip);
  const auto height = mip_dimension(layout.height, mip);
  switch (layout.dxgi_format) {
  case 28U:
    return rgba8_mip_size(width, height);
  case 71U:
  case 72U:
    return bc1_mip_size(width, height);
  default:
    return format_error("DDS layout has unsupported DXGI fixture format");
  }
}
```

**Chunk ordering validation pattern** (lines 189-255):
```cpp
const std::uint32_t faces_per_array = layout.is_cubemap ? 6U : 1U;
std::uint32_t array_index = 0;
std::uint32_t face_index = 0;
std::uint32_t expected_start_mip = 0;
...
for (std::size_t source_chunk_index = 0; source_chunk_index < chunks.size(); ++source_chunk_index) {
  ...
  if (chunk.start_mip != expected_start_mip) {
    return format_error("DDS layout chunk sequence has a mip gap, duplicate, or contradicts BA2 order");
  }
  auto expected_size = mip_range_size(layout, chunk.start_mip, chunk.end_mip);
  ...
  if (expected_size.value() != chunk.raw_size) {
    return format_error("DDS layout chunk raw byte total does not match mip range");
  }
  segments.push_back(logical_texture_segment{array_index, face_index, chunk.start_mip, chunk.end_mip,
                                             source_chunk_index});
  ...
}
```

**Apply to DX10:** replace the current narrow format switch with a descriptor table for the locked DXGI set. Keep checked arithmetic and fail-closed `format_error` behavior. Add a chunk planner that repeats the same mip splits across every array slice/cubemap face.

---

### `tests/unit/ba2_dx10_writer_tests.cpp` (test, batch + file-I/O)

**Analogs:** `tests/unit/ba2_gnrl_writer_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`

**Test helper/import pattern** (`ba2_gnrl_writer_tests.cpp` lines 1-18):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
```

**Temp/read/write helpers** (`ba2_gnrl_writer_tests.cpp` lines 21-49):
```cpp
std::filesystem::path writer_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_ba2_gnrl_writer_tests";
  std::filesystem::create_directories(path);
  return path;
}

void write_binary_file(const std::filesystem::path& path, std::vector<std::byte> bytes) {
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  REQUIRE(output.good());
  output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(output.good());
}
```

**Reader-backed round trip pattern** (`ba2_gnrl_writer_tests.cpp` lines 223-265):
```cpp
auto opened = libbsa::archive_reader::open(output.string());
REQUIRE(opened.has_value());
auto metadata = opened.value().metadata();
REQUIRE(metadata.has_value());
CHECK(metadata.value().type == libbsa::archive_type::ba2);
CHECK(metadata.value().variant == expected_variant);
CHECK(metadata.value().version == expected_version);
CHECK(metadata.value().default_compression == expected_compression);
...
auto extracted = opened.value().extract_bytes(expected_entry.path);
REQUIRE(extracted.has_value());
CHECK(extracted.value() == expected_entry.bytes);
```

**DX10 DirectXTex validation pattern** (`ba2_dx10_extraction_tests.cpp` lines 126-137, 192-202):
```cpp
void require_directxtex_metadata_matches_manifest(std::span<const std::byte> dds_bytes, const nlohmann::json& expected) {
  auto metadata = libbsa::texture::analyze_dds_metadata(dds_bytes);
  REQUIRE(metadata.has_value());
  ...
  REQUIRE(metadata.value().dxgi_format == expected_metadata.at("format").get<std::uint32_t>());
}

TEST_CASE("ba2_dx10_directxtex validates reconstructed DDS metadata", "[unit][fixture][ba2_dx10_directxtex]") {
  ...
  auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
  REQUIRE(bytes.has_value());
  require_directxtex_metadata_matches_manifest(bytes.value(), expected);
}
```

**Compression/dedupe tests to mirror** (`ba2_gnrl_writer_tests.cpp` lines 464-505, 559-613):
```cpp
TEST_CASE("BA2 GNRL writer all-compressed policy routes through target compression methods",
          "[unit][ba2_gnrl_writer]") { ... }

TEST_CASE("BA2 GNRL writer keeps duplicate payload offsets distinct by default", "[unit][ba2_gnrl_writer]") { ... }

TEST_CASE("BA2 GNRL writer shares offsets for byte-identical stored payloads when dedupe is enabled",
          "[unit][ba2_gnrl_writer]") { ... }
```

**Apply to DX10:** test add-time DDS validation/snapshotting, FO4 deflate chunks, Starfield v3 method `3` raw LZ4 block chunks, optional method `0` where valid, chunk cap planning, dedupe disabled/enabled offsets, safe publish, and reader-backed DDS extraction/DirectXTex validation. Do not add raw DX10 public override tests.

---

### `tests/unit/dds_layout_tests.cpp` (test, transform)

**Analog:** `tests/unit/dds_layout_tests.cpp`

**Header and chunk validation test style** (lines 41-64, 126-201):
```cpp
TEST_CASE("dds_layout builds deterministic DDS DXT10 headers", "[unit][dds_layout]") {
  const libbsa::texture::dds_texture_layout layout{
      .width = 4,
      .height = 4,
      .mip_count = 1,
      .dxgi_format = 71,
      .array_size = 1,
      .is_cubemap = false,
  };

  const auto header = libbsa::texture::build_dds_dxt10_header(layout);
  REQUIRE(header.has_value());
  CHECK(read_u32_le(header.value(), 128) == layout.dxgi_format);
}

TEST_CASE("dds_layout rejects gaps, duplicate coverage, impossible sizes, and unsupported formats",
          "[unit][dds_layout]") {
  ...
  const auto result = libbsa::texture::validate_and_order_chunks(unsupported_format, chunks);
  REQUIRE_FALSE(result.has_value());
  CHECK(result.error().code == libbsa::error_code::format_error);
}
```

**Apply to DX10:** add table-driven format-size cases for all locked formats and planner tests for reference default, explicit byte cap, arrays, cubemaps, and impossible cap/overflow failure.

---

### `tests/unit/public_include_boundary_tests.cpp` (test, public boundary/config)

**Analog:** `tests/unit/public_include_boundary_tests.cpp`

**Compile-time public surface assertions** (lines 16-36, 44-61):
```cpp
static_assert(__cplusplus >= 202002L, "libbsa public headers require C++20 or newer");
static_assert(std::is_enum_v<libbsa::ba2_gnrl_target>);
static_assert(std::is_class_v<libbsa::ba2_gnrl_writer_options>);
static_assert(std::is_class_v<libbsa::ba2_gnrl_writer>);
...
static_assert(requires(libbsa::ba2_gnrl_writer& writer,
                       std::span<const std::byte> bytes,
                       libbsa::ba2_gnrl_entry_options entry_options) {
  { writer.target() } -> std::same_as<libbsa::ba2_gnrl_target>;
  { writer.options() } -> std::same_as<const libbsa::ba2_gnrl_writer_options&>;
  { writer.add_file("Meshes/Disk.nif", "source.nif") } -> std::same_as<libbsa::result<void>>;
  { writer.write_to("out.ba2") } -> std::same_as<libbsa::result<void>>;
});
```

**Forbidden token scan pattern** (lines 97-123):
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                   "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
...
REQUIRE(line.find(token) == std::string::npos);
```

**Apply to DX10:** add static asserts for `ba2_dx10_target`, options, constructibility, `target()`, `options()`, DDS-host-file `add_file`, and `write_to`. Do not assert public `add_bytes` or entry compression override for DX10 unless the public contract intentionally adds them (CONTEXT says it should not).

---

### `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp` or adjacent writer fixture generator (test fixture generator, batch + file-I/O)

**Analog:** `tests/fixtures/generated/generate_ba2_dx10_fixtures.cpp`

**Generator structure and binary writer pattern** (lines 21-73):
```cpp
namespace {

constexpr std::uint32_t ba2_fo4_version = 1U;
constexpr std::uint32_t ba2_sfv3_version = 3U;
constexpr std::uint32_t ba2_record_sentinel = 0xBAAD'F00DU;
constexpr std::uint16_t ba2_dx10_chunk_header_size = 24U;
constexpr std::uint16_t ba2_dx10_cubemap_raw = 2049U;

struct byte_buffer {
  std::vector<std::byte> bytes;
  void u8(std::uint8_t value) { bytes.push_back(static_cast<std::byte>(value)); }
  void u16(std::uint16_t value) { ... }
  void u32(std::uint32_t value) { ... }
  void u64(std::uint64_t value) { ... }
  void raw(std::span<const std::byte> values) { bytes.insert(bytes.end(), values.begin(), values.end()); }
  void ascii4(std::string_view value) { ... }
  void string_u16(std::string_view value) { ... }
};
```

**DX10 record/manifest serialization pattern** (lines 263-324, 468-530):
```cpp
void write_header(byte_buffer& writer, const archive_spec& archive, std::uint64_t file_table_offset) {
  writer.ascii4("BTDX");
  writer.u32(archive.version);
  writer.ascii4("DX10");
  writer.u32(checked_u32(archive.textures.size(), "BA2 DX10 file count"));
  writer.u64(file_table_offset);
  if (archive.version >= ba2_sfv3_version) {
    writer.u32(archive.starfield_unknown1);
    writer.u32(archive.starfield_unknown2);
    writer.u32(archive.compression_method);
  }
}

void write_records(byte_buffer& writer, const archive_spec& archive) {
  ...
  writer.u8(texture.unknown_tex);
  writer.u8(static_cast<std::uint8_t>(texture.chunks.size()));
  writer.u16(ba2_dx10_chunk_header_size);
  writer.u16(texture.height);
  writer.u16(texture.width);
  writer.u8(texture.num_mips);
  writer.u8(texture.dxgi_format);
  writer.u16(texture.cube_maps_raw);
  ...
}
```

**Source provenance/documentation pattern** (lines 637-648):
```cpp
/// Generates deterministic BA2 DX10 texture fixtures and manifests from repository-owned synthetic bytes.
int main(int argc, char** argv) {
  try {
    const auto output_dir = parse_output_dir(argc, argv);
    generate_success(output_dir);
    generate_malformed(output_dir);
  } catch (const std::exception& exception) {
    std::cerr << "generate_ba2_dx10_fixtures: " << exception.what() << '\n';
    return 1;
  }
  return 0;
}
```

**Apply to DX10 writer fixtures:** if adding committed DDS source fixtures, either extend this generator or create a sibling with the same deterministic CLI, provenance, and manifest style. Generated DDS files/manifests must stay outside `TES5Edit/`.

---

### `CMakeLists.txt` and `tests/CMakeLists.txt` (build config, batch)

**Analogs:** root `CMakeLists.txt`, `tests/CMakeLists.txt`

**Library source registration pattern** (`CMakeLists.txt` lines 44-79):
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
    src/formats/ba2/ba2_dx10_parser.cpp
    src/formats/ba2/ba2_dx10_reader.cpp
    src/formats/ba2/ba2_gnrl_writer.cpp
    src/texture/dds_layout.cpp
    src/texture/directxtex_analyzer.cpp
)
```

**Test source and fixture target pattern** (`tests/CMakeLists.txt` lines 4-27, 96-110, 171-190):
```cmake
add_executable(libbsa_tests
  unit/ba2_gnrl_writer_tests.cpp
  unit/dds_layout_tests.cpp
  unit/ba2_dx10_extraction_tests.cpp
  unit/public_include_boundary_tests.cpp
)

add_executable(generate_ba2_dx10_fixtures_tool
  fixtures/generated/generate_ba2_dx10_fixtures.cpp
)

add_custom_target(generate_ba2_dx10_fixtures
  COMMAND generate_ba2_dx10_fixtures_tool --output ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives
  DEPENDS generate_ba2_dx10_fixtures_tool
  BYPRODUCTS
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_fo4.ba2
    ${CMAKE_SOURCE_DIR}/tests/fixtures/generated/archives/ba2_dx10_fo4_manifest.json
  VERBATIM
)
```

**Apply to DX10:** add `src/formats/ba2/ba2_dx10_writer.cpp` to the library private sources, add `tests/unit/ba2_dx10_writer_tests.cpp`, and update/add fixture generator target byproducts for committed DDS source fixtures/manifests.

## Shared Patterns

### Public dependency boundary
**Source:** `tests/unit/public_include_boundary_tests.cpp` lines 97-123 and `src/texture/directxtex_analyzer.cpp` lines 3-8  
**Apply to:** public writer API, umbrella header, DirectXTex analyzer extensions
```cpp
constexpr auto forbidden_tokens = std::to_array<std::string_view>({"libdeflate", "lz4::", "DirectXTex", "DirectX::",
                                                                   "DXGI", "Windows.h", "DDS_HEADER_DXT10",
                                                                   "TES5Edit", "std::expected", "bethesda_hash",
                                                                   "compression_router", "archive_path_key"});
```

### Archive path normalization and preserved display spelling
**Source:** `src/formats/ba2/ba2_gnrl_writer.cpp` lines 33-50; `src/detail/archive_path.cpp` lines 24-56  
**Apply to:** `ba2_dx10_writer::add_file`, duplicate detection, filename table serialization
```cpp
std::string preserved_archive_path(std::string_view archive_path) {
  std::string preserved{archive_path};
  std::replace(preserved.begin(), preserved.end(), '\\', '/');
  return preserved;
}

auto canonical = detail::normalize_archive_path(archive_path);
if (!canonical) {
  return canonical.error();
}
entry.archive_path_original = preserved_archive_path(archive_path);
entry.archive_path_canonical = std::move(canonical.value().value);
```

### FO4 hash fields
**Source:** `src/detail/bethesda_hash.cpp` lines 141-155; `src/formats/ba2/ba2_gnrl_writer.cpp` lines 310-319  
**Apply to:** DX10 texture `name_hash` and `directory_hash`
```cpp
std::uint32_t hash_fo4(std::string_view archive_path) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashFO4 uses CRC32 with initial value 0,
  // ASCII LowerByte folding, skips bytes >127, and treats '/' as '\\'.
  std::uint32_t result = 0;
  for (char ch : archive_path) {
    ...
    result = (result >> 8U) ^ crc32_lookup((result ^ lower_byte(ch)) & 0xFFU);
  }
  return result;
}

const auto [directory, file_name] = split_directory_file(entry.archive_path_canonical);
detail::hash_fo4(file_name);
detail::hash_fo4(directory);
```

### Checked little-endian binary I/O
**Source:** `src/detail/binary_io.hpp` lines 52-71  
**Apply to:** DX10 header, texture record, chunk record, and filename table serialization
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
**Source:** `src/detail/compression_router.cpp` lines 18-29  
**Apply to:** every non-empty DX10 chunk; FO4 => deflate, Starfield v3 method `3` => raw LZ4 block, method `0` => deflate
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

### DX10 extraction oracle
**Source:** `src/formats/ba2/ba2_dx10_reader.cpp` lines 196-234  
**Apply to:** writer-output validation tests
```cpp
texture::dds_texture_layout layout{entry.texture->width,
                                   entry.texture->height,
                                   entry.texture->mip_count,
                                   entry.texture->dxgi_format,
                                   entry.texture->array_size,
                                   entry.texture->is_cubemap};
auto header = texture::build_dds_dxt10_header(layout);
...
for (const auto& chunk : entry.texture->chunks) {
  if (chunk.compression == entry_compression::none) {
    auto streamed = stream_raw_chunk(input, chunk, sink);
    ...
    continue;
  }
  auto extracted = extract_compressed_chunk(input, chunk, sink);
  ...
}
```

### Structured error handling
**Source:** multiple analogs: `ba2_gnrl_writer.cpp` lines 76-88, 599-620; `directxtex_analyzer.cpp` lines 26-31  
**Apply to:** all add-time validation, write-time validation, source DDS analysis, serialization, publish
```cpp
if (host_path.empty()) {
  return error{error_code::invalid_argument, "BA2 GNRL disk source host path must not be empty"};
}
...
if (!output_exists) {
  return output_exists.error();
}
...
if (hr < 0) {
  return error{error_code::format_error, "DDS metadata could not be loaded by the texture analyzer"};
}
```

## No Analog Found

All planned files have strong analogs in the current codebase. The only new design area is DDS source-file snapshot/subresource slicing, which should extend `src/texture/directxtex_analyzer.*` using the existing private DirectXTex boundary plus the research examples.

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| _None_ | — | — | Existing BA2 GNRL writer, DX10 parser/reader, DDS layout, and DirectXTex analyzer cover the required roles. |

## Metadata

**Analog search scope:** `include/libbsa/`, `src/formats/ba2/`, `src/texture/`, `src/detail/`, `tests/unit/`, `tests/fixtures/generated/`, root/test CMake files  
**Files scanned:** 24  
**Pattern extraction date:** 2026-05-09
