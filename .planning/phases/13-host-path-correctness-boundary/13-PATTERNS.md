# Phase 13: Host Path Correctness Boundary - Pattern Map

**Mapped:** 2026-05-13
**Files analyzed:** 20
**Analogs found:** 20 / 20

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|---|---|---|---|---|
| `src/detail/host_file.*` | utility | file-I/O | `src/detail/writer_disk_source.*` | exact |
| `src/detail/host_file_path.*` | model | transform | `src/detail/archive_path.hpp` + `src/archive.cpp` | role-match |
| `src/detail/writer_disk_source.*` (rename/move) | utility | file-I/O | `src/detail/writer_disk_source.*` | exact |
| `src/archive.cpp` | service | request-response + file-I/O | `src/archive.cpp` | exact |
| `src/validation.cpp` | service | request-response + file-I/O | `src/validation.cpp` | exact |
| `src/formats/bsa/tes3_bsa_parser.*` | service | file-I/O + transform | `src/formats/bsa/tes3_bsa_parser.*` | exact |
| `src/formats/bsa/tes4_bsa_parser.*` | service | file-I/O + transform | `src/formats/bsa/tes4_bsa_parser.*` | exact |
| `src/formats/bsa/tes3_bsa_reader.*` | service | streaming | `src/formats/bsa/tes3_bsa_reader.*` | exact |
| `src/formats/bsa/tes4_bsa_reader.*` | service | streaming | `src/formats/bsa/tes4_bsa_reader.*` | exact |
| `src/formats/ba2/ba2_gnrl_parser.*` | service | file-I/O + transform | `src/formats/ba2/ba2_gnrl_parser.*` | exact |
| `src/formats/ba2/ba2_dx10_parser.*` | service | file-I/O + transform | `src/formats/ba2/ba2_dx10_parser.*` | exact |
| `src/formats/ba2/ba2_gnrl_reader.*` | service | streaming | `src/formats/ba2/ba2_gnrl_reader.*` | exact |
| `src/formats/ba2/ba2_dx10_reader.*` | service | streaming | `src/formats/ba2/ba2_dx10_reader.*` | exact |
| `src/formats/bsa/tes4_bsa_prepare.cpp` | service | file-I/O | `src/formats/bsa/tes4_bsa_prepare.cpp` | exact |
| `src/formats/bsa/tes4_bsa_layout.cpp` | service | batch + file-I/O | `src/formats/bsa/tes4_bsa_layout.cpp` | exact |
| `src/formats/ba2/ba2_gnrl_prepare.cpp` | service | file-I/O | `src/formats/ba2/ba2_gnrl_prepare.cpp` | exact |
| `src/formats/ba2/ba2_dx10_prepare.cpp` | service | file-I/O | `src/formats/ba2/ba2_dx10_prepare.cpp` | exact |
| `tests/unit/host_path_correctness_boundary_tests.cpp` | test | request-response + file-I/O | `tests/unit/validation_api_tests.cpp` | role-match |
| `tests/unit/writer_disk_source_tests.cpp` / renamed successor | test | file-I/O | `tests/unit/writer_disk_source_tests.cpp` | exact |
| `tests/CMakeLists.txt` | config | transform | `tests/CMakeLists.txt` | exact |

## Pattern Assignments

### `src/detail/host_file.*` (utility, file-I/O)

**Analog:** `src/detail/writer_disk_source.hpp`, `src/detail/writer_disk_source.cpp`

**Imports and surface pattern** (`src/detail/writer_disk_source.hpp:3-10`, `14-24`):
```cpp
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

struct writer_disk_source_context {
  std::string_view open_error;
  std::string_view inspect_error;
  std::string_view read_error;
  std::string_view changed_error;
  std::string_view allocation_description;
};
```

**Open/error pattern** (`src/detail/writer_disk_source.cpp:15-23`):
```cpp
error io_error(std::string_view message) { return error{error_code::io_error, std::string{message}}; }

result<std::ifstream> open_disk_source(std::string_view host_path, const writer_disk_source_context& context) {
  std::ifstream input{std::filesystem::path{std::string{host_path}}, std::ios::binary};
  if (!input) {
    return io_error(context.open_error);
  }
  return input;
}
```

**Inspect/exact/prefix/chunk layering** (`src/detail/writer_disk_source.cpp:76-130`, `133-161`, `164-206`):
```cpp
result<std::uint64_t> inspect_disk_source_size(std::string_view host_path,
                                               const writer_disk_source_context& context) {
  const auto path = std::filesystem::path{std::string{host_path}};
  std::error_code fs_error;
  const bool regular_file = std::filesystem::is_regular_file(path, fs_error);
  if (fs_error || !regular_file) {
    return io_error(context.inspect_error);
  }
  const auto size = std::filesystem::file_size(path, fs_error);
  if (fs_error) {
    return io_error(context.inspect_error);
  }
  return size;
}

result<std::vector<std::byte>> read_disk_source_prefix(std::string_view host_path,
                                                       std::size_t max_bytes,
                                                       const writer_disk_source_context& context) {
  auto input = open_disk_source(host_path, context);
  ...
}

result<void> for_each_disk_source_chunk(..., std::size_t chunk_size) {
  if (chunk_size == 0U) {
    return error{error_code::invalid_argument, "writer disk source chunk size must be non-zero"};
  }
  ...
  return reject_appended_source_byte(input.value(), context);
}
```

**Copy for Phase 13:** keep this exact helper shape, but rename/generalize it and switch arguments from raw `std::string_view` to the new shared host-path value or resolved path primitive.

---

### `src/detail/host_file_path.*` (model, transform)

**Analog:** `src/detail/archive_path.hpp`, `src/archive.cpp`

**Small internal value-type pattern** (`src/detail/archive_path.hpp:10-19`):
```cpp
struct archive_path_key {
  std::string value;
};

result<archive_path_key> normalize_archive_path(std::string_view input);
```

**Reader-state storage pattern** (`src/archive.cpp:28-33`, `150-163`):
```cpp
struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  std::string host_path;
  bool is_ba2_dx10{false};
};

reader.state_ = std::make_shared<state>(
    state{ba2_archive.value().metadata, std::move(ba2_archive.value().entries), std::string{host_path}, false});
```

**Copy for Phase 13:** make `host_file_path` as a tiny detail struct like `archive_path_key`, then store it in `archive_reader::state` in the same one-shot construction pattern.

---

### `src/archive.cpp` (service, request-response + file-I/O)

**Analog:** `src/archive.cpp`

**Imports/dispatcher pattern** (`src/archive.cpp:3-17`):
```cpp
#include "formats/ba2/ba2_dx10_parser.hpp"
#include "formats/ba2/ba2_dx10_reader.hpp"
#include "formats/ba2/ba2_gnrl_parser.hpp"
#include "formats/ba2/ba2_gnrl_reader.hpp"
#include "formats/bsa/tes3_bsa_parser.hpp"
#include "formats/bsa/tes3_bsa_reader.hpp"
#include "formats/bsa/tes4_bsa_parser.hpp"
#include "formats/bsa/tes4_bsa_reader.hpp"

#include <detail/byte_vector.hpp>
#include <detail/parallel_work.hpp>
#include <detail/payload_stream.hpp>
```

**Open-flow pattern** (`src/archive.cpp:121-195`):
```cpp
result<archive_reader> archive_reader::open(std::string_view host_path) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
  }

  auto prefix = read_detection_prefix(host_path);
  if (!prefix) {
    return prefix.error();
  }
  ...
  auto archive_size = archive_file_size(host_path);
  if (!archive_size) {
    return archive_size.error();
  }
  ...
}
```

**Extraction dispatch pattern** (`src/archive.cpp:98-112`, `246-289`, `325-360`):
```cpp
result<void> extract_entry_payload(const archive_metadata& metadata,
                                   bool is_ba2_dx10,
                                   std::string_view host_path,
                                   const entry_metadata& entry,
                                   payload_sink& sink) {
  if (metadata.variant == archive_variant::tes3) {
    return formats::bsa::extract_tes3_bsa_payload(host_path, entry, sink);
  }
  ...
}

return extract_entry_payload(state_->metadata, state_->is_ba2_dx10, state_->host_path, *found.value(), sink);
```

**Copy for Phase 13:** keep facade control flow and stable error returns; only replace `host_path` storage and all direct read helpers with the shared host-file boundary.

---

### `src/validation.cpp` (service, request-response + file-I/O)

**Analog:** `src/validation.cpp`

**Result/report split** (`src/validation.cpp:55-59`, `178-220`):
```cpp
validation_report report_from_open_error(const error& err) {
  validation_report report;
  append_fatal(report, err.code);
  return report;
}

auto opened = archive_reader::open(host_path);
if (!opened) {
  const auto& err = opened.error();
  if (err.code == error_code::io_error || err.code == error_code::invalid_argument) {
    return err;
  }
  return report_from_open_error(err);
}
```

**Extractability proof pattern** (`src/validation.cpp:145-171`, `215-217`):
```cpp
void validate_extractability(const archive_reader& reader,
                             const std::vector<entry_metadata>& entries,
                             std::uint64_t max_entry_bytes,
                             validation_report& report) {
  discard_payload_sink sink;
  for (const auto& entry : entries) {
    ...
    extracted = reader.extract(entry.path, sink);
    ...
  }
}
```

**Delete/replace pattern:** remove the dedicated preflight in `host_path_can_be_opened` (`src/validation.cpp:50-53`, `182-184`) and keep everything else anchored on `archive_reader::open`.

---

### `src/formats/bsa/tes3_bsa_parser.*` (service, file-I/O + transform)

**Analog:** `src/formats/bsa/tes3_bsa_parser.hpp`, `src/formats/bsa/tes3_bsa_parser.cpp`

**Signature pattern** (`src/formats/bsa/tes3_bsa_parser.hpp:22-27`):
```cpp
result<tes3_bsa_archive> parse_tes3_bsa_archive(std::span<const std::byte> bytes, detected_bsa_format detected);
result<tes3_bsa_archive> parse_tes3_bsa_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected);
```

**File-open + bounded metadata read pattern** (`src/formats/bsa/tes3_bsa_parser.cpp:348-381`):
```cpp
if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
  return error{error_code::format_error, "TES3 BSA archive exceeds platform limits"};
}

std::ifstream input{std::string{host_path}, std::ios::binary};
if (!input) {
  return error{error_code::io_error, "failed to open archive host path"};
}
auto header_bytes = read_file_bytes_at(input, 0U, fixed_header_size, "TES3 BSA fixed header");
...
auto table_bytes = read_file_bytes_at(input, 0U, table_size.value(), "TES3 BSA metadata table");
```

**Copy for Phase 13:** keep the metadata-limit and `read_file_bytes_at(...)` pattern; only swap the file-open seam.

---

### `src/formats/bsa/tes4_bsa_parser.*` (service, file-I/O + transform)

**Analog:** `src/formats/bsa/tes4_bsa_parser.hpp`, `src/formats/bsa/tes4_bsa_parser.cpp`

**Signature pattern** (`src/formats/bsa/tes4_bsa_parser.hpp:22-30`):
```cpp
result<tes4_bsa_archive> parse_tes4_bsa_archive(std::span<const std::byte> bytes, detected_bsa_format detected);
result<tes4_bsa_archive> parse_tes4_bsa_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected);
```

**Payload-prefix callback pattern** (`src/formats/bsa/tes4_bsa_parser.cpp:616-657`):
```cpp
std::ifstream input{std::string{host_path}, std::ios::binary};
...
auto table_bytes = read_file_bytes_at(input, 0U, table_size.value(), "TES4 BSA metadata table");
...
auto read_payload_bytes = [&input](std::uint64_t offset, std::size_t count) -> result<std::vector<std::byte>> {
  return read_file_bytes_at(input, offset, count, "TES4 BSA payload prefix");
};
return parse_tes4_bsa_archive_impl(..., read_payload_bytes);
```

**Copy for Phase 13:** preserve the lambda-on-open-stream pattern; it is the best seam for passing a helper-opened stream.

---

### `src/formats/ba2/ba2_gnrl_parser.*` (service, file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_gnrl_parser.hpp`, `src/formats/ba2/ba2_gnrl_parser.cpp`

**Signature pattern** (`src/formats/ba2/ba2_gnrl_parser.hpp:22-27`):
```cpp
result<ba2_gnrl_archive> parse_ba2_gnrl_archive(std::span<const std::byte> bytes, detected_ba2_format detected);
result<ba2_gnrl_archive> parse_ba2_gnrl_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected);
```

**Header/offset validation pattern** (`src/formats/ba2/ba2_gnrl_parser.cpp:428-468`):
```cpp
std::ifstream input{std::string{host_path}, std::ios::binary};
...
auto fixed_header = read_file_bytes_at(input, 0U, header_size_for(detected.version), "BA2 GNRL fixed header");
...
if (!add_fits(header_size_for(header.value().version), records_size, records_end) ||
    header.value().file_table_offset < records_end || header.value().file_table_offset > archive_size) {
  return error{error_code::format_error, "BA2 GNRL FileTableOffset is outside the metadata span"};
}
```

---

### `src/formats/ba2/ba2_dx10_parser.*` (service, file-I/O + transform)

**Analog:** `src/formats/ba2/ba2_dx10_parser.hpp`, `src/formats/ba2/ba2_dx10_parser.cpp`

**Signature pattern** (`src/formats/ba2/ba2_dx10_parser.hpp:22-27`):
```cpp
result<ba2_dx10_archive> parse_ba2_dx10_archive(std::span<const std::byte> bytes, detected_ba2_format detected);
result<ba2_dx10_archive> parse_ba2_dx10_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected);
```

**Metadata read pattern** (`src/formats/ba2/ba2_dx10_parser.cpp:610-648`):
```cpp
std::ifstream input{std::string{host_path}, std::ios::binary};
...
auto fixed_header = read_file_bytes_at(input, 0U, header_size_for(detected.version), "BA2 DX10 fixed header");
...
auto metadata_bytes = read_file_bytes_at(input, 0U, static_cast<std::size_t>(header.value().file_table_offset),
                                         "BA2 DX10 header and texture records");
```

---

### `src/formats/bsa/tes3_bsa_reader.*` (service, streaming)

**Analog:** `src/formats/bsa/tes3_bsa_reader.hpp`, `src/formats/bsa/tes3_bsa_reader.cpp`

**Signature pattern** (`src/formats/bsa/tes3_bsa_reader.hpp:22-23`):
```cpp
result<void> extract_tes3_bsa_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink);
```

**Streaming/error pattern** (`src/formats/bsa/tes3_bsa_reader.cpp:28-63`, `97-104`):
```cpp
std::ifstream input{std::string{host_path}, std::ios::binary};
if (!input) {
  return error{error_code::io_error, "failed to open TES3 archive host path for extraction"};
}
input.seekg(static_cast<std::streamoff>(entry.payload_offset), std::ios::beg);
...
if (static_cast<std::size_t>(input.gcount()) != chunk_size) {
  return error{error_code::format_error, "TES3 BSA entry payload span is outside the archive"};
}
```

---

### `src/formats/bsa/tes4_bsa_reader.*` (service, streaming)

**Analog:** `src/formats/bsa/tes4_bsa_reader.hpp`, `src/formats/bsa/tes4_bsa_reader.cpp`

**Signature pattern** (`src/formats/bsa/tes4_bsa_reader.hpp:21-23`):
```cpp
result<void> extract_tes4_bsa_payload_from_file(std::string_view host_path, const entry_metadata& entry,
                                                payload_sink& sink);
```

**Stream-then-route pattern** (`src/formats/bsa/tes4_bsa_reader.cpp:40-73`, `106-113`):
```cpp
result<void> extract_file_payload(std::ifstream& input, const entry_metadata& entry, payload_sink& sink) {
  ...
  return detail::decompress_payload_exact_to_sink(..., input, compressed_payload_offset,
                                                  payload_size - 4U, expected_size, sink, extraction_chunk_size,
                                                  "TES4 BSA compressed payload");
}

std::ifstream input{std::string{host_path}, std::ios::binary};
if (!input) {
  return error{error_code::io_error, "failed to open archive host path for TES4 BSA extraction"};
}
return extract_file_payload(input, entry, sink);
```

---

### `src/formats/ba2/ba2_gnrl_reader.*` (service, streaming)

**Analog:** `src/formats/ba2/ba2_gnrl_reader.hpp`, `src/formats/ba2/ba2_gnrl_reader.cpp`

**Signature pattern** (`src/formats/ba2/ba2_gnrl_reader.hpp:22-26`):
```cpp
/// The entry's parsed compression metadata selects raw, deflate, or raw LZ4-block handling;
/// BA2 GNRL extraction deliberately never infers codec behavior from names or extensions.
result<void> extract_ba2_gnrl_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink);
```

**Raw/compressed split** (`src/formats/ba2/ba2_gnrl_reader.cpp:18-29`, `45-59`, `93-100`):
```cpp
if (entry.compression == entry_compression::none) {
  return stream_raw_payload(host_path, entry, sink);
}
return extract_compressed_payload(host_path, entry, sink);
```

---

### `src/formats/ba2/ba2_dx10_reader.*` (service, streaming)

**Analog:** `src/formats/ba2/ba2_dx10_reader.hpp`, `src/formats/ba2/ba2_dx10_reader.cpp`

**Header-first extraction pattern** (`src/formats/ba2/ba2_dx10_reader.hpp:22-27`, `src/formats/ba2/ba2_dx10_reader.cpp:92-131`):
```cpp
auto header = texture::build_dds_dxt10_header(layout);
if (!header) {
  return header.error();
}

auto wrote_header = detail::write_payload_exact(sink, header.value(), "BA2 DX10 DDS header");
if (!wrote_header) {
  return wrote_header.error();
}

std::ifstream input{std::string{host_path}, std::ios::binary};
...
for (const auto& chunk : entry.texture->chunks) {
  ...
}
```

---

### Writer call-site migrations (`tes4_bsa_prepare.cpp`, `tes4_bsa_layout.cpp`, `ba2_gnrl_prepare.cpp`, `ba2_dx10_prepare.cpp`)

**Analog:** existing `writer_disk_source` usages in those files

**Context-object pattern** (`src/formats/bsa/tes4_bsa_prepare.cpp:164-176`):
```cpp
constexpr detail::writer_disk_source_context tes4_prepare_source_context{
    "TES4 BSA writer failed to open disk source",
    "TES4 BSA writer failed to inspect disk source",
    "TES4 BSA writer failed while reading disk source",
    "TES4 BSA disk source changed during finalization",
    "TES4 BSA disk source"};

return detail::read_disk_source_exact(entry.host_path, expected_size, tes4_prepare_source_context);
```

**Chunk-comparison pattern** (`src/formats/bsa/tes4_bsa_layout.cpp:50-56`, `100-116`):
```cpp
constexpr detail::writer_disk_source_context tes4_dedupe_source_context{...};

auto compared = detail::for_each_disk_source_chunk(
    entry.raw_disk_host_path,
    entry.raw_disk_size,
    tes4_dedupe_source_context,
    [&](std::span<const std::byte> chunk) -> result<void> {
      ...
      return {};
    });
```

**BA2 prepare pattern** (`src/formats/ba2/ba2_gnrl_prepare.cpp:44-60`, `75-91`; `src/formats/ba2/ba2_dx10_prepare.cpp:62-80`):
```cpp
constexpr detail::writer_disk_source_context ba2_gnrl_prepare_source_context{...};
auto size = detail::inspect_disk_source_size(host_path, ba2_gnrl_prepare_source_context);

constexpr detail::writer_disk_source_context ba2_dx10_dds_source_context{...};
result<std::vector<std::byte>> read_dds_file(std::string_view dds_host_path) {
  return detail::read_disk_source_exact(dds_host_path, ba2_dx10_dds_source_context);
}
```

**Copy for Phase 13:** update includes/type names only; keep caller-owned diagnostic contexts and helper call style.

---

### `tests/unit/host_path_correctness_boundary_tests.cpp` (test, request-response + file-I/O)

**Primary analog:** `tests/unit/validation_api_tests.cpp`

**Temp-path and helper pattern** (`tests/unit/validation_api_tests.cpp:42-55`, `108-129`, `141-146`):
```cpp
std::filesystem::path validation_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_validation_api_tests";
  std::filesystem::create_directories(path);
  return path;
}

nlohmann::json read_json_file(const std::filesystem::path& path) {
  std::ifstream stream{path};
  REQUIRE(stream.is_open());
  return nlohmann::json::parse(stream);
}

auto validated = libbsa::validate_archive(host_path, options);
REQUIRE(validated.has_value());
```

**Extraction assertion patterns to reuse:**
- TES4: `tests/unit/tes4_bsa_reader_tests.cpp:518-531`, `654-672`
- BA2 GNRL: `tests/unit/ba2_gnrl_reader_tests.cpp:714-743`
- BA2 DX10: `tests/unit/ba2_dx10_extraction_tests.cpp:141-159`

Example:
```cpp
auto opened = libbsa::archive_reader::open(generated_archive_path("tes4_v103.bsa").string());
REQUIRE(opened.has_value());

auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);
REQUIRE(extracted.has_value());
REQUIRE(sink.bytes() == bytes_from_hex(expected.at("expected").at("bytes_hex").get<std::string>()));
```

**Copy for Phase 13:** one dedicated suite, file-local helpers in an anonymous namespace, manifest-backed expected bytes, and black-box public API calls only.

---

### `tests/unit/writer_disk_source_tests.cpp` / renamed successor (test, file-I/O)

**Analog:** `tests/unit/writer_disk_source_tests.cpp`

**Helper pattern** (`tests/unit/writer_disk_source_tests.cpp:16-45`):
```cpp
libbsa::detail::writer_disk_source_context test_context() noexcept {
  return {"test failed to open source",
          "test failed to inspect source",
          "test failed while reading source",
          "test source changed",
          "test source allocation"};
}
```

**Behavior-test pattern** (`tests/unit/writer_disk_source_tests.cpp:50-149`):
```cpp
auto bytes = libbsa::detail::read_disk_source_exact(path.string(), static_cast<std::uint64_t>(expected.size()),
                                                    test_context());
REQUIRE(bytes.has_value());
CHECK(bytes.value() == expected);
...
CHECK(bytes.error().message == "test source changed");
```

**Copy for Phase 13:** preserve this exact test matrix after rename: exact read, prefix read, chunk iteration, missing source, changed source, allocation failure.

---

### `tests/CMakeLists.txt` (config, transform)

**Analog:** `tests/CMakeLists.txt`

**Test registration pattern** (`tests/CMakeLists.txt:48-91`):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  ...
  unit/writer_disk_source_tests.cpp
  unit/bethesda_hash_tests.cpp
)
```

**Discovery pattern** (`tests/CMakeLists.txt:275-280`):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Copy for Phase 13:** update the source list for the renamed helper test and add `unit/host_path_correctness_boundary_tests.cpp` in the same block.

## Shared Patterns

### Host-file helper boundary
**Source:** `src/detail/writer_disk_source.cpp:17-23`, `76-90`, `164-206`
**Apply to:** `src/archive.cpp`, `src/validation.cpp`, all parser `*_archive_file` functions, all reader extraction reopen functions, renamed writer helper call sites
```cpp
std::ifstream input{std::filesystem::path{std::string{host_path}}, std::ios::binary};
if (!input) {
  return io_error(context.open_error);
}
...
return reject_appended_source_byte(input.value(), context);
```

### One-shot reader state construction
**Source:** `src/archive.cpp:28-33`, `150-163`, `181-194`
**Apply to:** `archive_reader::state` host-path storage changes
```cpp
struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  std::string host_path;
  bool is_ba2_dx10{false};
};

reader.state_ = std::make_shared<state>(state{..., std::string{host_path}, ...});
```

### Validation setup/result split
**Source:** `src/validation.cpp:186-220`
**Apply to:** `validate_archive`
```cpp
auto opened = archive_reader::open(host_path);
if (!opened) {
  const auto& err = opened.error();
  if (err.code == error_code::io_error || err.code == error_code::invalid_argument) {
    return err;
  }
  return report_from_open_error(err);
}
```

### Manifest-backed black-box extraction tests
**Source:** `tests/unit/tes4_bsa_reader_tests.cpp:520-531`, `tests/unit/ba2_gnrl_reader_tests.cpp:721-743`, `tests/unit/ba2_dx10_extraction_tests.cpp:143-159`
**Apply to:** `tests/unit/host_path_correctness_boundary_tests.cpp`
```cpp
const auto manifest = read_json_file(...);
auto opened = libbsa::archive_reader::open(archive_path.string());
REQUIRE(opened.has_value());

auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
REQUIRE(bytes.has_value());
```

## No Analog Found

None. The only new seam without an exact existing twin is `src/detail/host_file_path.*`, but it has a strong partial analog in `src/detail/archive_path.hpp` plus the existing `archive_reader::state` storage pattern.

## Metadata

**Analog search scope:** `src/detail/`, `src/archive.cpp`, `src/validation.cpp`, `src/formats/bsa/`, `src/formats/ba2/`, `tests/unit/`, `tests/CMakeLists.txt`
**Files scanned:** 20 code/test files, plus phase docs and codebase guidance docs
**Pattern extraction date:** 2026-05-13
