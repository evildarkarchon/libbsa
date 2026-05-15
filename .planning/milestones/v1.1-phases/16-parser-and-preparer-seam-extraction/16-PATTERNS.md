# Phase 16: Parser and Preparer Seam Extraction - Pattern Map

**Mapped:** 2026-05-14
**Files analyzed:** 16 new/modified files
**Analogs found:** 16 / 16

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/formats/bsa/tes4_bsa_parser.cpp` | parser/coordinator | request-response + file-I/O + transform | `src/formats/bsa/tes4_bsa_parser.cpp` | exact/current hotspot |
| `src/formats/bsa/tes4_bsa_table.hpp` | internal model/parser seam | transform | `src/formats/bsa/tes4_bsa_parser.hpp` + current table helpers in `.cpp` | exact extracted seam |
| `src/formats/bsa/tes4_bsa_table.cpp` | parser seam | transform | `src/formats/bsa/tes4_bsa_parser.cpp` lines 70-255,398-490 | exact extracted seam |
| `src/formats/bsa/tes4_bsa_payload_descriptor.hpp` | internal model/utility seam | transform + file-I/O callback | `src/formats/bsa/tes4_bsa_parser.hpp` + current payload helpers in `.cpp` | exact extracted seam |
| `src/formats/bsa/tes4_bsa_payload_descriptor.cpp` | parser utility seam | transform + file-I/O callback | `src/formats/bsa/tes4_bsa_parser.cpp` lines 257-396 | exact extracted seam |
| `src/formats/ba2/ba2_dx10_prepare.cpp` | preparer/coordinator | file-I/O + transform + batch | `src/formats/ba2/ba2_dx10_prepare.cpp` | exact/current hotspot |
| `src/formats/ba2/ba2_dx10_snapshot_builder.hpp` | internal model/preparer seam | file-I/O + transform | `src/formats/ba2/ba2_dx10_prepare.hpp` + current snapshot helpers in `.cpp` | exact extracted seam |
| `src/formats/ba2/ba2_dx10_snapshot_builder.cpp` | preparer service seam | file-I/O + transform | `src/formats/ba2/ba2_dx10_prepare.cpp` lines 62-142,285-337 | exact extracted seam |
| `src/formats/ba2/ba2_dx10_chunk_assembler.hpp` | internal model/preparer seam | batch + file-I/O + transform | `src/formats/ba2/ba2_dx10_prepare.hpp` | exact extracted seam |
| `src/formats/ba2/ba2_dx10_chunk_assembler.cpp` | preparer service seam | batch + file-I/O + transform | `src/formats/ba2/ba2_dx10_prepare.cpp` lines 206-267,399-461,466-538 | exact extracted seam |
| `tests/unit/tes4_bsa_parser_seam_tests.cpp` | test | request-response + transform | `tests/unit/tes4_bsa_reader_tests.cpp` | role-match |
| `tests/unit/ba2_dx10_preparer_seam_tests.cpp` | test | file-I/O + batch + transform | `tests/unit/writer_stage_tests.cpp` | exact role/data-flow |
| `tests/unit/parser_preparer_seam_policy_tests.cpp` | policy test | file-I/O + transform | `tests/unit/archive_reader_dispatch_policy_tests.cpp` | exact role/data-flow |
| `tests/unit/tes4_bsa_reader_tests.cpp` | regression test | request-response + file-I/O | `tests/unit/tes4_bsa_reader_tests.cpp` | exact/extend |
| `tests/unit/writer_stage_tests.cpp` / `tests/unit/ba2_dx10_writer_tests.cpp` | regression test | file-I/O + batch + transform | same files | exact/extend |
| `CMakeLists.txt` / `tests/CMakeLists.txt` | build config | config | same files | exact/extend |

## Pattern Assignments

### `src/formats/bsa/tes4_bsa_parser.cpp` (parser/coordinator, request-response + file-I/O + transform)

**Analog:** `src/formats/bsa/tes4_bsa_parser.cpp`

**Imports pattern** (lines 1-18):
```cpp
#include "formats/bsa/tes4_bsa_parser.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/binary_io.hpp>
#include <detail/byte_vector.hpp>
#include <detail/host_file.hpp>
#include <detail/parser_primitives.hpp>
```

**Coordinator pattern to preserve and narrow** (lines 492-586):
```cpp
template <typename PayloadReader>
result<tes4_bsa_archive> parse_tes4_bsa_archive_impl(std::span<const std::byte> table_bytes, std::size_t archive_size,
                                                     detected_bsa_format detected, PayloadReader& read_payload_bytes) {
  if (table_bytes.size() < tes4_bsa_header_size) {
    return error{error_code::format_error, "TES4 BSA header is truncated"};
  }

  detail::binary_reader reader{table_bytes};
  auto header = read_header(reader);
  if (!header) {
    return header.error();
  }
  // ... validate header/counts, read raw tables, then materialize entries ...
  return tes4_bsa_archive{archive_metadata{archive_type::bsa,
                                           detected.variant,
                                           header.value().version,
                                           header.value().archive_flags,
                                           header.value().file_count,
                                           detected.default_compression},
                           std::move(entries.value())};
}
```

**Host-file boundary pattern** (lines 617-663):
```cpp
const detail::host_file_context host_context{"failed to open archive host path",
                                              "failed to determine archive host path size",
                                              "failed while reading archive host path",
                                              "archive host path changed while reading",
                                              "TES4 BSA metadata table"};
auto input = detail::open_host_file(host_path, host_context);
if (!input) {
  return input.error();
}
// ... read metadata table once, payload prefixes through read_file_bytes_at ...
auto read_payload_bytes = [&input](std::uint64_t offset, std::size_t count) -> result<std::vector<std::byte>> {
  return read_file_bytes_at(input.value(), offset, count, "TES4 BSA payload prefix");
};
return parse_tes4_bsa_archive_impl(table_bytes.value(), static_cast<std::size_t>(archive_size), detected,
                                   read_payload_bytes);
```

---

### `src/formats/bsa/tes4_bsa_table.hpp` / `.cpp` (internal parser seam, transform)

**Analog:** table helpers currently inside `src/formats/bsa/tes4_bsa_parser.cpp`

**Header/data model pattern** (lines 23-49):
```cpp
struct header_fields {
  std::uint32_t version;
  std::uint32_t folder_offset;
  std::uint32_t archive_flags;
  std::uint32_t folder_count;
  std::uint32_t file_count;
  std::uint32_t total_folder_name_length;
  std::uint32_t total_file_name_length;
  std::uint32_t file_flags;
};

struct folder_record {
  std::uint64_t hash;
  std::uint32_t file_count;
  std::uint64_t offset;
};
```

**Table read + validation pattern** (lines 70-121):
```cpp
result<header_fields> read_header(detail::binary_reader& reader) {
  const auto magic = reader.read_u32_le();
  if (!magic) {
    return magic.error();
  }
  if (magic.value() != tes4_bsa_magic) {
    return error{error_code::unsupported, "TES4 BSA magic is not supported"};
  }
  // read fixed fields and return format_error on truncation
}

result<std::size_t> metadata_table_size(const header_fields& header, std::size_t folder_record_size,
                                        std::size_t archive_size) {
  std::size_t folder_records_size = 0;
  std::size_t file_records_size = 0;
  if (!multiply_fits(header.folder_count, folder_record_size, folder_records_size) ||
      !multiply_fits(header.file_count, tes4_bsa_file_record_size, file_records_size)) {
    return error{error_code::format_error, "TES4 BSA metadata table is too large"};
  }
  // checked add_fits/span_fits before returning total
}
```

**Raw table bundle boundary** (lines 155-255,398-490):
```cpp
result<std::vector<folder_block>> read_folder_blocks(detail::binary_reader& reader, const header_fields& header,
                                                      std::span<const folder_record> folders) {
  // validates folder offsets, folder-name hashes, file-record table sizes, and total folder-name bytes
}

result<std::vector<std::string>> read_file_names(detail::binary_reader& reader, std::uint32_t file_count,
                                                 std::uint32_t total_file_name_length) {
  auto table_bytes = reader.read_bytes(total_file_name_length);
  if (!table_bytes) {
    return error{error_code::format_error, "TES4 BSA file name table is truncated"};
  }
  // parse null-terminated names and validate exact byte consumption
}
```

**Planner note:** keep this seam raw. It should return checked table/header state, folder records/blocks, and file names only; duplicate canonical path checks and `entry_metadata` construction belong outside this seam.

---

### `src/formats/bsa/tes4_bsa_payload_descriptor.hpp` / `.cpp` (internal parser utility seam, transform + file-I/O callback)

**Analog:** payload helpers currently inside `src/formats/bsa/tes4_bsa_parser.cpp`

**Compression interpretation pattern** (lines 257-264):
```cpp
entry_compression compression_for(const header_fields& header, std::uint32_t size_flags) noexcept {
  const bool default_compressed = (header.archive_flags & tes4_bsa_archive_compress_by_default) != 0U;
  const bool toggled = (size_flags & tes4_bsa_file_size_compression_toggle) != 0U;
  if (!(default_compressed ^ toggled)) {
    return entry_compression::none;
  }
  return header.version == tes4_bsa_skyrim_se_version ? entry_compression::lz4_frame : entry_compression::deflate;
}
```

**Payload prefix read pattern** (lines 266-307):
```cpp
template <typename PayloadReader>
result<std::uint32_t> embedded_prefix_size(std::size_t archive_size, const file_record& record,
                                           std::uint32_t stored_size, PayloadReader& read_payload_bytes) {
  if (!span_fits(record.offset, 1U, archive_size)) {
    return error{error_code::format_error, "TES4 BSA embedded-name prefix is outside the archive"};
  }
  auto bytes = read_payload_bytes(record.offset, 1U);
  if (!bytes) {
    return bytes.error();
  }
  // validate prefix_size <= stored_size and span_fits
}
```

**Payload span/error handling pattern** (lines 351-369):
```cpp
const auto stored_size = record.size_flags & ~tes4_bsa_file_size_compression_toggle;
if (!span_fits(record.offset, stored_size, archive_size)) {
  return error{error_code::format_error, "TES4 BSA entry payload span is outside the archive"};
}
if (non_empty_span_intersects_prefix(record.offset, stored_size, metadata_size)) {
  return error{error_code::format_error, "TES4 BSA entry payload span overlaps metadata"};
}
const auto compression = compression_for(header, record.size_flags);
```

**Allocation/error pattern** (lines 315-396):
```cpp
try {
  std::vector<entry_metadata> entries;
  auto reserved_entries = detail::reserve_metadata_vector(entries, header.file_count, "TES4 BSA entry metadata");
  if (!reserved_entries) {
    return reserved_entries.error();
  }
  // materialize and sort entries
} catch (const std::bad_alloc&) {
  return detail::metadata_allocation_error("TES4 BSA entry metadata");
} catch (const std::length_error&) {
  return detail::metadata_allocation_error("TES4 BSA entry metadata");
}
```

---

### `src/formats/ba2/ba2_dx10_prepare.cpp` (preparer/coordinator, file-I/O + transform + batch)

**Analog:** `src/formats/ba2/ba2_dx10_prepare.cpp`

**Imports/private dependency boundary** (lines 1-13):
```cpp
#include "formats/ba2/ba2_dx10_prepare.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/byte_vector.hpp>
#include <detail/parallel_work.hpp>
#include <detail/host_file.hpp>

#include "texture/dds_layout.hpp"
#include "texture/directxtex_analyzer.hpp"
```

**Narrow public/internal surface to preserve** (header lines 55-84):
```cpp
result<ba2_dx10_writer_entry> ba2_dx10_make_writer_entry(std::string_view archive_path,
                                                         std::string_view dds_host_path,
                                                         ba2_dx10_target target,
                                                         const std::filesystem::path& snapshot_dir,
                                                         std::size_t entry_index);

result<ba2_dx10_prepared_chunk> ba2_dx10_prepare_chunk(ba2_dx10_target target,
                                                       const ba2_dx10_writer_options& options,
                                                       const ba2_dx10_writer_entry& source,
                                                       const texture::planned_texture_chunk& planned);
```

---

### `src/formats/ba2/ba2_dx10_snapshot_builder.hpp` / `.cpp` (preparer service seam, file-I/O + transform)

**Analog:** snapshot/load helpers currently inside `src/formats/ba2/ba2_dx10_prepare.cpp`

**Host-file context and read pattern** (lines 62-84):
```cpp
constexpr detail::host_file_context ba2_dx10_dds_source_context{
    "BA2 DX10 writer failed to open DDS source",
    "BA2 DX10 writer failed to inspect DDS source",
    "BA2 DX10 writer failed while reading DDS source",
    "BA2 DX10 DDS source changed during analysis",
    "BA2 DX10 DDS source"};

result<std::vector<std::byte>> read_dds_file(std::string_view dds_host_path) {
  auto source_path = detail::resolve_host_file_path(dds_host_path);
  if (!source_path) {
    return source_path.error();
  }
  return detail::read_host_file_exact(source_path.value(), ba2_dx10_dds_source_context);
}
```

**Snapshot temp directory pattern** (lines 86-130):
```cpp
result<std::string> make_snapshot_random_suffix() {
  std::array<std::byte, snapshot_random_suffix_bytes> random_bytes{};
  const auto status = BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(random_bytes.data()),
                                      static_cast<ULONG>(random_bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  if (status < 0) {
    return error{error_code::io_error, "BA2 DX10 writer failed to generate snapshot temp directory name"};
  }
  // encode lowercase hex suffix
}

result<std::filesystem::path> make_unique_snapshot_directory() {
  // temp_directory_path + create_directory is the atomic reservation boundary
}
```

**Snapshot builder core pattern** (lines 297-337):
```cpp
auto canonical = detail::normalize_archive_path(archive_path);
if (!canonical) {
  return canonical.error();
}

auto dds_bytes = read_dds_file(dds_host_path);
if (!dds_bytes) {
  return dds_bytes.error();
}

auto source = texture::analyze_dds_source(dds_bytes.value());
if (!source) {
  return source.error();
}
auto target_format = ba2_dx10_validate_texture_format_for_target(target, source.value().metadata.dxgi_format);
if (!target_format) {
  return target_format.error();
}
// write one snapshot temp file per analyzed subresource and store snapshot handles in ba2_dx10_writer_entry
```

---

### `src/formats/ba2/ba2_dx10_chunk_assembler.hpp` / `.cpp` (preparer service seam, batch + file-I/O + transform)

**Analog:** chunk collection/assembly helpers currently inside `src/formats/ba2/ba2_dx10_prepare.cpp`

**Compression routing pattern** (lines 206-220):
```cpp
result<detail::compression_method> compression_method_for(ba2_dx10_target target, std::uint32_t starfield_method) {
  switch (target) {
  case ba2_dx10_target::fallout4:
    return detail::compression_method::deflate;
  case ba2_dx10_target::starfield_v3:
    if (starfield_method == ba2_starfield_compression_deflate) {
      return detail::compression_method::deflate;
    }
    if (starfield_method == ba2_starfield_compression_lz4_block) {
      return detail::compression_method::lz4_block;
    }
    return error{error_code::unsupported, "BA2 DX10 Starfield v3 compression method is unsupported"};
  }
  return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
}
```

**Streamed snapshot assembly pattern** (lines 222-229):
```cpp
result<void> append_snapshot_bytes(std::vector<std::byte>& bytes, const ba2_dx10_subresource_snapshot& snapshot) {
  return detail::for_each_host_file_chunk(
      snapshot.snapshot_path,
      snapshot.size,
      ba2_dx10_snapshot_source_context,
      [&](std::span<const std::byte> chunk) -> result<void> {
        return detail::append_byte_vector(bytes, chunk, "BA2 DX10 raw texture chunk bytes");
      });
}
```

**Planned chunk collection pattern** (lines 237-267):
```cpp
result<ba2_dx10_chunk_snapshot_batch> collect_chunk_snapshots(const ba2_dx10_writer_entry& source,
                                                              const texture::planned_texture_chunk& chunk) {
  if (chunk.start_mip > chunk.end_mip) {
    return error{error_code::format_error, "BA2 DX10 planned chunk mip range is invalid"};
  }
  // find matching array/face/mip snapshots in BA2-required chunk layout order
}
```

**Chunk assemble/compress pattern** (lines 399-461):
```cpp
auto snapshots = collect_chunk_snapshots(source, planned);
if (!snapshots) {
  return snapshots.error();
}
if (snapshots.value().aggregate_size != planned.raw_size) {
  return error{error_code::format_error, "BA2 DX10 planned chunk size does not match source DDS bytes"};
}
// reserve raw bytes, append each snapshot with for_each_host_file_chunk, then compress via detail::compress_payload
```

**Plan-then-indexed-work pattern** (lines 470-537):
```cpp
auto planned_chunks = texture::plan_dx10_chunks(layout, options.max_decoded_chunk_bytes);
if (!planned_chunks) {
  return planned_chunks.error();
}
std::vector<std::optional<ba2_dx10_prepared_chunk>> chunks_by_index(planned_chunks.value().size());
auto work = [&](std::size_t index) -> result<void> {
  auto chunk = ba2_dx10_prepare_chunk(target, options, entry, planned_chunks.value()[index]);
  if (!chunk) {
    return chunk.error();
  }
  chunks_by_index[index] = std::move(chunk.value());
  return {};
};
auto prepared_chunks = detail::run_indexed_work(planned_chunks.value().size(), worker_count, work);
```

---

### `tests/unit/tes4_bsa_parser_seam_tests.cpp` (test, request-response + transform)

**Analog:** `tests/unit/tes4_bsa_reader_tests.cpp`

**Fixture/import pattern** (lines 1-20):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/libbsa.hpp>

#include <detail/parser_primitives.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>
```

**Fixture helper pattern** (lines 23-30,93-100,158-166):
```cpp
std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  std::vector<std::byte> bytes;
  for (char ch = 0; input.get(ch);) {
    bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
  }
  return bytes;
}

void overwrite_u32_le(std::vector<std::byte>& bytes, std::size_t offset, std::uint32_t value) {
  for (std::uint32_t index = 0; index < 4U; ++index) {
    bytes.at(offset + index) = static_cast<std::byte>((value >> (index * 8U)) & 0xFFU);
  }
}
```

**Malformed regression pattern** (lines 373-388):
```cpp
TEST_CASE("tes4_bsa_malformed_open rejects payload spans inside metadata",
          "[unit][fixture][malformed][tes4_bsa_malformed_open]") {
  auto bytes = read_binary_file(generated_archive_path("tes4_v103.bsa"));
  const auto folder_count = read_u32_le(bytes, 16U);
  const auto folder_name_bytes = read_u32_le(bytes, 24U);
  const auto first_file_record = 36U + folder_count * 16U + folder_name_bytes;
  overwrite_u32_le(bytes, first_file_record + 12U, 0U);
  // write temp archive, open, assert format_error
}
```

**Metadata success pattern** (lines 431-465):
```cpp
TEST_CASE("tes4_bsa_entry_metadata materializes table paths, hashes, sizes, and embedded names",
          "[unit][fixture][tes4_bsa_entry_metadata][tes4_bsa_listing][tes4_bsa_embedded_name]") {
  for (const auto& fixture : success_fixtures()) {
    const auto manifest = read_json_file(generated_archive_path(fixture.archive_filename.substr(0, fixture.archive_filename.size() - 4) + "_manifest.json"));
    auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
    REQUIRE(opened.has_value());
    // compare entry path, hash, sizes, offset, compression, embedded-name prefix
  }
}
```

---

### `tests/unit/ba2_dx10_preparer_seam_tests.cpp` (test, file-I/O + batch + transform)

**Analog:** `tests/unit/writer_stage_tests.cpp` and `tests/unit/ba2_dx10_writer_tests.cpp`

**Direct internal include pattern** (`writer_stage_tests.cpp` lines 1-12):
```cpp
#include "formats/ba2/ba2_gnrl_layout.hpp"
#include "formats/ba2/ba2_gnrl_prepare.hpp"
#include "formats/ba2/ba2_dx10_layout.hpp"
#include "formats/ba2/ba2_dx10_prepare.hpp"
#include "formats/bsa/tes3_bsa_layout.hpp"
#include "formats/bsa/tes3_bsa_prepare.hpp"
#include "formats/bsa/tes4_bsa_layout.hpp"
#include "formats/bsa/tes4_bsa_prepare.hpp"
#include "formats/bsa/tes4_bsa_serialize.hpp"
#include "texture/dds_layout.hpp"

#include <catch2/catch_test_macros.hpp>
```

**Synthetic snapshot entry builder pattern** (`writer_stage_tests.cpp` lines 120-152):
```cpp
libbsa::formats::ba2::ba2_dx10_writer_entry ba2_dx10_stage_entry(std::string archive_path,
                                                                  const libbsa::texture::dds_texture_layout& layout,
                                                                  std::string snapshot_prefix) {
  libbsa::formats::ba2::ba2_dx10_writer_entry entry;
  entry.archive_path_original = archive_path;
  entry.archive_path_canonical = archive_path;
  entry.metadata = libbsa::texture_metadata{layout.width, layout.height, layout.mip_count,
                                            layout.dxgi_format, layout.array_size, layout.is_cubemap,
                                            0U, layout.is_cubemap ? 2049U : 2048U, {}};
  // write per-subresource snapshot temp files and push ba2_dx10_subresource_snapshot records
  return entry;
}
```

**Chunk-order assertion pattern** (`writer_stage_tests.cpp` lines 464-497):
```cpp
TEST_CASE("ba2 dx10 writer preparation stage preserves multi-mip snapshot chunk order",
          "[unit][writer-stage][ba2_dx10_writer]") {
  const libbsa::texture::dds_texture_layout layout{4U, 4U, 3U, 28U, 1U, false};
  auto source = ba2_dx10_stage_entry("Textures/Stage/MultiChunk.dds", layout, "dx10-multi-chunk");
  auto planned = libbsa::texture::plan_dx10_chunks(layout, 0U);
  // prepare chunk, decompress exact payload, compare expected concatenated mip bytes
}
```

**Snapshot immutability integration pattern** (`ba2_dx10_writer_tests.cpp` lines 723-749):
```cpp
TEST_CASE("ba2_dx10_writer::add_file snapshots DDS bytes before later source file changes",
          "[unit][ba2_dx10_writer][bounded_memory_policy][add]") {
  const auto original_bytes = read_binary_file(generated_source_dir() / source_case.at("file").get<std::string>());
  auto original_source = libbsa::texture::analyze_dds_source(original_bytes);
  // add_file from scratch path, mutate and remove source, write archive, extract and compare to original_source
}
```

---

### `tests/unit/parser_preparer_seam_policy_tests.cpp` (policy test, file-I/O + transform)

**Analog:** `tests/unit/archive_reader_dispatch_policy_tests.cpp`

**Source-reading utility pattern** (lines 14-42):
```cpp
std::filesystem::path source_root() { return std::filesystem::path{LIBBSA_SOURCE_DIR}; }

std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.is_open());

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void require_absent_tokens(std::string_view body, std::span<const std::string_view> forbidden_tokens) {
  for (const auto token : forbidden_tokens) {
    INFO("forbidden token: " << token);
    REQUIRE(body.find(token) == std::string_view::npos);
  }
}
```

**Negative invariant pattern** (lines 46-103):
```cpp
TEST_CASE("archive_reader_dispatch_policy forbids repeated family dispatch in public reader methods",
          "[unit][archive_reader_dispatch_policy]") {
  const auto archive_text = read_text_file(source_root() / "src/archive.cpp");
  const auto entries_body = function_body(archive_text,
                                          "result<std::vector<entry_metadata>> archive_reader::entries() const",
                                          "result<std::optional<entry_metadata>> archive_reader::find(std::string_view path) const");
  constexpr auto forbidden_dispatch_tokens = std::to_array<std::string_view>({
      "metadata.variant",
      "metadata.type",
      "is_ba2_dx10",
      "backend_identity",
  });
  require_absent_tokens(entries_body, forbidden_dispatch_tokens);
}
```

**Planner note:** policy checks should be role-based: assert dedicated private seams exist and monolithic-collapse tokens/responsibilities are absent from the coordinator bodies. Do not freeze exact helper names beyond the required existence of separate TES4 table/payload and BA2 snapshot/chunk seams.

---

### `CMakeLists.txt` and `tests/CMakeLists.txt` (build config, config data flow)

**Analog:** existing source/test registration.

**Library source registration pattern** (`CMakeLists.txt` lines 112-157):
```cmake
set(libbsa_library_sources
  src/archive.cpp
  src/validation.cpp
  src/detail/archive_path.cpp
  # ...
  src/formats/ba2/ba2_dx10_prepare.cpp
  src/formats/ba2/ba2_dx10_layout.cpp
  src/formats/ba2/ba2_dx10_serialize.cpp
  # ...
  src/formats/bsa/tes4_bsa_parser.cpp
  src/formats/bsa/tes4_bsa_reader.cpp
  src/formats/bsa/tes4_bsa_prepare.cpp
  # ...
)
```

**Internal test support and test registration pattern** (`tests/CMakeLists.txt` lines 65-129):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  unit/archive_reader_tests.cpp
  unit/archive_reader_dispatch_tests.cpp
  unit/archive_reader_dispatch_policy_tests.cpp
  # ... add new seam tests here ...
)

target_include_directories(libbsa_tests
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)
```

**CTest label discovery pattern** (`tests/CMakeLists.txt` lines 297-302):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

## Shared Patterns

### `libbsa::result<T>` and stable error-code propagation
**Source:** `src/formats/bsa/tes4_bsa_parser.cpp` lines 500-578; `src/formats/ba2/ba2_dx10_prepare.cpp` lines 403-443.
**Apply to:** all new parser/preparer seams and seam tests.
```cpp
auto value = some_fallible_operation();
if (!value) {
  return value.error();
}
```

### Host-file boundary and bounded reads
**Source:** `src/formats/bsa/tes4_bsa_parser.cpp` lines 617-663; `src/formats/ba2/ba2_dx10_prepare.cpp` lines 62-84,222-229.
**Apply to:** TES4 file parser callbacks, BA2 snapshot builder, BA2 chunk assembler.
```cpp
auto source_path = detail::resolve_host_file_path(dds_host_path);
if (!source_path) {
  return source_path.error();
}
return detail::read_host_file_exact(source_path.value(), ba2_dx10_dds_source_context);
```

### Metadata allocation guards
**Source:** `src/formats/bsa/tes4_bsa_parser.cpp` lines 315-324,391-395; `src/formats/ba2/ba2_dx10_prepare.cpp` lines 415-419.
**Apply to:** new raw table bundles, entry materialization, chunk byte buffers.
```cpp
auto reserved_entries = detail::reserve_metadata_vector(entries, header.file_count, "TES4 BSA entry metadata");
if (!reserved_entries) {
  return reserved_entries.error();
}
```

### Compression routing stays behind existing router
**Source:** `src/formats/ba2/ba2_dx10_prepare.cpp` lines 206-217,437-441.
**Apply to:** BA2 DX10 chunk assembler; do not call libdeflate/lz4 directly.
```cpp
auto method = compression_method_for(target, options.starfield_compression_method);
if (!method) {
  return method.error();
}
auto compressed = detail::compress_payload(method.value(), raw_bytes);
```

### Indexed parallel chunk work
**Source:** `src/formats/ba2/ba2_dx10_prepare.cpp` lines 517-526.
**Apply to:** BA2 DX10 plan-then-assemble seam.
```cpp
std::vector<std::optional<ba2_dx10_prepared_chunk>> chunks_by_index(planned_chunks.value().size());
auto work = [&](std::size_t index) -> result<void> {
  auto chunk = ba2_dx10_prepare_chunk(target, options, entry, planned_chunks.value()[index]);
  if (!chunk) {
    return chunk.error();
  }
  chunks_by_index[index] = std::move(chunk.value());
  return {};
};
auto prepared_chunks = detail::run_indexed_work(planned_chunks.value().size(), worker_count, work);
```

### Catch2 policy tests read source directly
**Source:** `tests/unit/archive_reader_dispatch_policy_tests.cpp` lines 16-42.
**Apply to:** `parser_preparer_seam_policy_tests.cpp`.
```cpp
std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream input{path};
  REQUIRE(input.is_open());
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}
```

## No Analog Found

All planned Phase 16 source, test, and build-config changes have direct or role-equivalent analogs in the current codebase. No files require planner fallback to research-only patterns.

## Metadata

**Analog search scope:** `src/formats/bsa`, `src/formats/ba2`, `src/texture`, `src/detail`, `tests/unit`, root/test CMake files.
**Files scanned:** 22 candidate source/test/config files from phase references and glob searches.
**Pattern extraction date:** 2026-05-14
**Project constraints applied:** `AGENTS.md`; `.claude/skills/*/SKILL.md` indexes checked; `TES5Edit/` treated as read-only and not modified.
