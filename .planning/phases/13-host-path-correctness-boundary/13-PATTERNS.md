# Phase 13: host-path-correctness-boundary - Pattern Map

**Mapped:** 2026-05-13  
**Files analyzed:** 20  
**Analogs found:** 20 / 20

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|---|---|---|---|---|
| `src/detail/<renamed-host-file>.hpp` | utility | file-I/O | `src/detail/writer_disk_source.hpp` | exact |
| `src/detail/<renamed-host-file>.cpp` | utility | file-I/O | `src/detail/writer_disk_source.cpp` | exact |
| `src/detail/<shared-host-path-value>.hpp` | model | transform | `src/detail/archive_path.hpp` | role-match |
| `src/archive.cpp` | service | request-response | `src/archive.cpp` | exact |
| `src/validation.cpp` | service | request-response | `src/validation.cpp` | exact |
| `src/formats/bsa/tes3_bsa_parser.cpp` | service | file-I/O | `src/formats/bsa/tes3_bsa_parser.cpp` | exact |
| `src/formats/bsa/tes3_bsa_reader.cpp` | service | streaming | `src/formats/bsa/tes3_bsa_reader.cpp` | exact |
| `src/formats/bsa/tes4_bsa_parser.cpp` | service | file-I/O | `src/formats/bsa/tes4_bsa_parser.cpp` | exact |
| `src/formats/bsa/tes4_bsa_reader.cpp` | service | streaming | `src/formats/bsa/tes4_bsa_reader.cpp` | exact |
| `src/formats/ba2/ba2_gnrl_parser.cpp` | service | file-I/O | `src/formats/ba2/ba2_gnrl_parser.cpp` | exact |
| `src/formats/ba2/ba2_gnrl_reader.cpp` | service | streaming | `src/formats/ba2/ba2_gnrl_reader.cpp` | exact |
| `src/formats/ba2/ba2_dx10_parser.cpp` | service | file-I/O | `src/formats/ba2/ba2_dx10_parser.cpp` | exact |
| `src/formats/ba2/ba2_dx10_reader.cpp` | service | streaming | `src/formats/ba2/ba2_dx10_reader.cpp` | exact |
| `src/formats/bsa/tes4_bsa_prepare.cpp` | service | file-I/O | `src/formats/bsa/tes4_bsa_prepare.cpp` | exact |
| `src/formats/bsa/tes4_bsa_layout.cpp` | service | file-I/O | `src/formats/bsa/tes4_bsa_layout.cpp` | exact |
| `src/formats/ba2/ba2_gnrl_prepare.cpp` | service | file-I/O | `src/formats/ba2/ba2_gnrl_prepare.cpp` | exact |
| `src/formats/ba2/ba2_dx10_prepare.cpp` | service | file-I/O | `src/formats/ba2/ba2_dx10_prepare.cpp` | exact |
| `tests/unit/host_path_correctness_boundary_tests.cpp` | test | request-response | `tests/unit/validation_api_tests.cpp` | role-match |
| `tests/CMakeLists.txt` | config | batch | `tests/CMakeLists.txt` | exact |
| `tests/unit/<renamed-host-file>_tests.cpp` | test | file-I/O | `tests/unit/writer_disk_source_tests.cpp` | exact |

## Pattern Assignments

### `src/detail/<renamed-host-file>.hpp` and `src/detail/<renamed-host-file>.cpp` (utility, file-I/O)

**Analog:** `src/detail/writer_disk_source.hpp`, `src/detail/writer_disk_source.cpp`

**Public helper surface** (`src/detail/writer_disk_source.hpp:14-50`):
```cpp
/// Diagnostics supplied by archive-family writer code for shared disk-source reads.
struct writer_disk_source_context {
  std::string_view open_error;
  std::string_view inspect_error;
  std::string_view read_error;
  std::string_view changed_error;
  std::string_view allocation_description;
};

result<std::uint64_t> inspect_disk_source_size(std::string_view host_path,
                                               const writer_disk_source_context& context);
result<std::vector<std::byte>> read_disk_source_exact(std::string_view host_path,
                                                      std::uint64_t expected_size,
                                                      const writer_disk_source_context& context);
result<std::vector<std::byte>> read_disk_source_prefix(std::string_view host_path,
                                                       std::size_t max_bytes,
                                                       const writer_disk_source_context& context);
```

**Windows-aware open + diagnostics** (`src/detail/writer_disk_source.cpp:15-23`):
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

**Inspect + exact read pattern** (`src/detail/writer_disk_source.cpp:76-121`):
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
```

**Chunk iteration pattern** (`src/detail/writer_disk_source.cpp:164-206`):
```cpp
result<void> for_each_disk_source_chunk(
    std::string_view host_path,
    std::uint64_t expected_size,
    const writer_disk_source_context& context,
    const std::function<result<void>(std::span<const std::byte>)>& callback,
    std::size_t chunk_size) {
  if (chunk_size == 0U) {
    return error{error_code::invalid_argument, "writer disk source chunk size must be non-zero"};
  }

  auto size_valid = validate_expected_source_size(host_path, expected_size, context);
  if (!size_valid) {
    return size_valid.error();
  }

  auto input = open_disk_source(host_path, context);
  ...
}
```

**Use for Phase 13:** keep this exact helper shape, but rename it to a neutral host-file family and accept/resume from the shared resolved host path rather than letting read-side code open files directly.

---

### `src/detail/<shared-host-path-value>.hpp` (model, transform)

**Analog:** `src/detail/archive_path.hpp`, plus `src/archive.cpp`

**Small internal value-object pattern** (`src/detail/archive_path.hpp:10-19`):
```cpp
/// Canonical archive virtual path key used for internal lookup and hashing.
struct archive_path_key {
  std::string value;
};

result<archive_path_key> normalize_archive_path(std::string_view input);
```

**Reader-state ownership target** (`src/archive.cpp:28-33`):
```cpp
struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  std::string host_path;
  bool is_ba2_dx10{false};
};
```

**Use for Phase 13:** copy the tiny-value-object style from `archive_path.hpp`, but store both `original_utf8_text` and resolved `std::filesystem::path` in one internal type. Then replace the lone `std::string host_path` field in `archive_reader::state` with that type.

---

### `src/archive.cpp` (service, request-response)

**Analog:** `src/archive.cpp`

**State and dispatch pattern** (`src/archive.cpp:98-112`):
```cpp
result<void> extract_entry_payload(const archive_metadata& metadata,
                                   bool is_ba2_dx10,
                                   std::string_view host_path,
                                   const entry_metadata& entry,
                                   payload_sink& sink) {
  if (metadata.variant == archive_variant::tes3) {
    return formats::bsa::extract_tes3_bsa_payload(host_path, entry, sink);
  }
  if (metadata.type == archive_type::ba2) {
    return is_ba2_dx10 ? formats::ba2::extract_ba2_dx10_payload(host_path, entry, sink)
                       : formats::ba2::extract_ba2_gnrl_payload(host_path, entry, sink);
  }
  return formats::bsa::extract_tes4_bsa_payload_from_file(host_path, entry, sink);
}
```

**Open flow pattern** (`src/archive.cpp:121-195`):
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
  reader.state_ = std::make_shared<state>(
      state{..., std::move(...entries), std::string{host_path}, ...});
  return reader;
}
```

**Extraction reuse pattern** (`src/archive.cpp:246-289`):
```cpp
return extract_entry_payload(state_->metadata, state_->is_ba2_dx10, state_->host_path, *found.value(), sink);
```

**Use for Phase 13:** keep this orchestration and error contract, but resolve the host path once up front, store both forms in state, use the shared helper for prefix read and size inspection, and make follow-on extraction reopen from stored resolved state only.

---

### `src/validation.cpp` (service, request-response)

**Analog:** `src/validation.cpp`

**Setup/result/report split** (`src/validation.cpp:178-194`):
```cpp
result<validation_report> validate_archive(std::string_view host_path, validation_options options) {
  if (host_path.empty()) {
    return error{error_code::invalid_argument, "archive path must not be empty"};
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

**Extractability proof pattern** (`src/validation.cpp:145-171`):
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

**Use for Phase 13:** remove the duplicate preflight (`host_path_can_be_opened` at `src/validation.cpp:50-53`), preserve the current result/report boundary, and keep extractability routed through public reader extraction.

---

### `src/formats/bsa/tes3_bsa_parser.cpp` (service, file-I/O)

**Analog:** `src/formats/bsa/tes3_bsa_parser.cpp`

**File-open parse pattern** (`src/formats/bsa/tes3_bsa_parser.cpp:348-381`):
```cpp
result<tes3_bsa_archive> parse_tes3_bsa_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected) {
  if (archive_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    return error{error_code::format_error, "TES3 BSA archive exceeds platform limits"};
  }

  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  auto header_bytes = read_file_bytes_at(input, 0U, fixed_header_size, "TES3 BSA fixed header");
  ...
}
```

**Use for Phase 13:** preserve the same parser bounds/error text, but obtain `input` through the shared host-file helper instead of opening locally.

---

### `src/formats/bsa/tes3_bsa_reader.cpp` (service, streaming)

**Analog:** `src/formats/bsa/tes3_bsa_reader.cpp`

**Streaming extraction pattern** (`src/formats/bsa/tes3_bsa_reader.cpp:28-64`):
```cpp
result<void> stream_from_host(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  ...
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open TES3 archive host path for extraction"};
  }
  input.seekg(static_cast<std::streamoff>(entry.payload_offset), std::ios::beg);
  ...
}
```

**Use for Phase 13:** same seek/read loop, same streaming semantics, shared helper for open.

---

### `src/formats/bsa/tes4_bsa_parser.cpp` (service, file-I/O)

**Analog:** `src/formats/bsa/tes4_bsa_parser.cpp`

**File-open parse pattern** (`src/formats/bsa/tes4_bsa_parser.cpp:610-657`):
```cpp
result<tes4_bsa_archive> parse_tes4_bsa_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_bsa_format detected) {
  ...
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  auto header_bytes = read_file_bytes_at(input, 0U, tes4_bsa_header_size, "TES4 BSA fixed header");
  ...
}
```

**Payload-prefix callback pattern** (`src/formats/bsa/tes4_bsa_parser.cpp:653-655`):
```cpp
auto read_payload_bytes = [&input](std::uint64_t offset, std::size_t count) -> result<std::vector<std::byte>> {
  return read_file_bytes_at(input, offset, count, "TES4 BSA payload prefix");
};
```

**Use for Phase 13:** keep the same `read_file_bytes_at` callback structure; only replace the archive open boundary.

---

### `src/formats/bsa/tes4_bsa_reader.cpp` (service, streaming)

**Analog:** `src/formats/bsa/tes4_bsa_reader.cpp`

**Open + extract pattern** (`src/formats/bsa/tes4_bsa_reader.cpp:106-114`):
```cpp
result<void> extract_tes4_bsa_payload_from_file(std::string_view host_path,
                                                const entry_metadata& entry,
                                                payload_sink& sink) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path for TES4 BSA extraction"};
  }
  return extract_file_payload(input, entry, sink);
}
```

**Use for Phase 13:** keep the two-stage split (`open` then `extract_file_payload`), shared helper provides the stream.

---

### `src/formats/ba2/ba2_gnrl_parser.cpp` (service, file-I/O)

**Analog:** `src/formats/ba2/ba2_gnrl_parser.cpp`

**Host-file name-table parse pattern** (`src/formats/ba2/ba2_gnrl_parser.cpp:428-513`):
```cpp
result<ba2_gnrl_archive> parse_ba2_gnrl_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected) {
  ...
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  auto fixed_header = read_file_bytes_at(input, 0U, header_size_for(detected.version), "BA2 GNRL fixed header");
  ...
  auto names = read_names_from_file(input, header.value().file_table_offset, header.value().file_count, archive_size,
                                    name_table_consumed);
  ...
}
```

**Use for Phase 13:** preserve the bounded-open/name-table-from-file logic exactly; only centralize opening.

---

### `src/formats/ba2/ba2_gnrl_reader.cpp` (service, streaming)

**Analog:** `src/formats/ba2/ba2_gnrl_reader.cpp`

**Raw/compressed branch pattern** (`src/formats/ba2/ba2_gnrl_reader.cpp:18-60, 93-100`):
```cpp
result<void> stream_raw_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  ...
  std::ifstream input{std::string{host_path}, std::ios::binary};
  ...
}

result<void> extract_compressed_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  std::ifstream input{std::string{host_path}, std::ios::binary};
  ...
}

result<void> extract_ba2_gnrl_payload(std::string_view host_path, const entry_metadata& entry, payload_sink& sink) {
  if (entry.compression == entry_compression::none) {
    return stream_raw_payload(host_path, entry, sink);
  }
  return extract_compressed_payload(host_path, entry, sink);
}
```

**Use for Phase 13:** preserve raw/compressed branching and error text; use shared helper for both open sites.

---

### `src/formats/ba2/ba2_dx10_parser.cpp` (service, file-I/O)

**Analog:** `src/formats/ba2/ba2_dx10_parser.cpp`

**File-open parse pattern** (`src/formats/ba2/ba2_dx10_parser.cpp:610-681`):
```cpp
result<ba2_dx10_archive> parse_ba2_dx10_archive_file(std::string_view host_path, std::uint64_t archive_size,
                                                     detected_ba2_format detected) {
  ...
  std::ifstream input{std::string{host_path}, std::ios::binary};
  if (!input) {
    return error{error_code::io_error, "failed to open archive host path"};
  }
  auto fixed_header = read_file_bytes_at(input, 0U, header_size_for(detected.version), "BA2 DX10 fixed header");
  ...
  auto name_table_bytes = parse_ba2_dx10_names_from_file(input,
                                                         header.value().file_table_offset,
                                                         first_payload_offset.value(),
                                                         header.value().file_count);
  ...
}
```

**Use for Phase 13:** same DX10 sparse/count-delimited table logic, shared helper for open.

---

### `src/formats/ba2/ba2_dx10_reader.cpp` (service, streaming)

**Analog:** `src/formats/ba2/ba2_dx10_reader.cpp`

**DDS-header-then-chunks pattern** (`src/formats/ba2/ba2_dx10_reader.cpp:92-130`):
```cpp
auto header = texture::build_dds_dxt10_header(layout);
...
auto wrote_header = detail::write_payload_exact(sink, header.value(), "BA2 DX10 DDS header");
...
std::ifstream input{std::string{host_path}, std::ios::binary};
if (!input) {
  return error{error_code::io_error, "failed to open BA2 archive host path for DX10 extraction"};
}

for (const auto& chunk : entry.texture->chunks) {
  ...
}
```

**Use for Phase 13:** preserve header-first behavior and per-chunk loop; only centralize open.

---

### Writer call-site migrations: `src/formats/bsa/tes4_bsa_prepare.cpp`, `src/formats/bsa/tes4_bsa_layout.cpp`, `src/formats/ba2/ba2_gnrl_prepare.cpp`, `src/formats/ba2/ba2_dx10_prepare.cpp`

**Analogs:** the files themselves

**Context struct declaration pattern**

- `src/formats/bsa/tes4_bsa_prepare.cpp:164-169`
- `src/formats/bsa/tes4_bsa_layout.cpp:50-55`
- `src/formats/ba2/ba2_gnrl_prepare.cpp:44-49`
- `src/formats/ba2/ba2_dx10_prepare.cpp:62-74`

```cpp
constexpr detail::writer_disk_source_context tes4_prepare_source_context{
    "TES4 BSA writer failed to open disk source",
    "TES4 BSA writer failed to inspect disk source",
    "TES4 BSA writer failed while reading disk source",
    "TES4 BSA disk source changed during finalization",
    "TES4 BSA disk source"};
```

**Helper-call usage pattern**

- `src/formats/bsa/tes4_bsa_prepare.cpp:171-205`
- `src/formats/ba2/ba2_gnrl_prepare.cpp:51-64`
- `src/formats/ba2/ba2_dx10_prepare.cpp:78-80`

```cpp
return detail::read_disk_source_exact(entry.host_path, expected_size, tes4_prepare_source_context);
...
auto size = detail::inspect_disk_source_size(host_path, tes4_prepare_source_context);
...
return detail::read_disk_source_exact(dds_host_path, ba2_dx10_dds_source_context);
```

**Use for Phase 13:** rename these includes/types/functions in one sweep to the neutral shared helper family. Keep the caller-owned diagnostic-context pattern unchanged.

---

### `tests/unit/host_path_correctness_boundary_tests.cpp` (test, request-response)

**Primary analog:** `tests/unit/validation_api_tests.cpp`  
**Support analogs:** `tests/unit/tes4_bsa_reader_tests.cpp`, `tests/unit/ba2_gnrl_reader_tests.cpp`, `tests/unit/ba2_dx10_extraction_tests.cpp`

**Fixture-root and temp-path helper pattern** (`tests/unit/validation_api_tests.cpp:30-55`):
```cpp
std::filesystem::path generated_archive_dir() {
  return std::filesystem::path{LIBBSA_SOURCE_DIR} / "tests" / "fixtures" / "generated" / "archives";
}

std::filesystem::path validation_test_dir() {
  auto path = std::filesystem::temp_directory_path() / "libbsa_validation_api_tests";
  std::filesystem::create_directories(path);
  return path;
}
```

**Black-box validation assertion pattern** (`tests/unit/validation_api_tests.cpp:125-139`):
```cpp
void require_valid_archive(std::string_view host_path,
                           libbsa::archive_type expected_type,
                           libbsa::archive_variant expected_variant,
                           libbsa::validation_options options = {}) {
  auto validated = libbsa::validate_archive(host_path, options);
  REQUIRE(validated.has_value());
  ...
}
```

**Public open assertions** (`tests/unit/tes4_bsa_reader_tests.cpp:179-185`, `tests/unit/ba2_gnrl_reader_tests.cpp:212-255`):
```cpp
auto opened = libbsa::archive_reader::open(generated_archive_path(fixture.archive_filename).string());
REQUIRE(opened.has_value());
```

**Manifest-backed extraction assertion** (`tests/unit/ba2_dx10_extraction_tests.cpp:141-159`):
```cpp
auto extracted = opened.value().extract(expected.at("path").get<std::string>(), sink);
REQUIRE(extracted.has_value());

auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
REQUIRE(bytes.has_value());
REQUIRE(bytes.value() == sink.bytes());
```

**Use for Phase 13:** build one dedicated suite file that:
1. Copies only the archive under test into a non-ASCII temp directory and non-ASCII filename.
2. Calls `archive_reader::open` on that copied path.
3. Calls `validate_archive(..., {.validate_entry_extractability = true})` on that copied path.
4. Performs one manifest-backed extraction assertion per representative archive.

---

### `tests/CMakeLists.txt` (config, batch)

**Analog:** `tests/CMakeLists.txt`

**Unit-source registration pattern** (`tests/CMakeLists.txt:48-91`):
```cmake
add_executable(libbsa_tests
  unit/result_tests.cpp
  ...
  unit/validation_api_tests.cpp
  ...
  unit/writer_disk_source_tests.cpp
)
```

**Catch discovery pattern** (`tests/CMakeLists.txt:275-280`):
```cmake
include(Catch)
catch_discover_tests(libbsa_tests
  ADD_TAGS_AS_LABELS
  DISCOVERY_MODE PRE_TEST
  DL_PATHS $<TARGET_FILE_DIR:libbsa>
)
```

**Use for Phase 13:** add the new suite beside the other `unit/*.cpp` entries and keep tag-driven discovery unchanged.

---

### `tests/unit/<renamed-host-file>_tests.cpp` (test, file-I/O)

**Analog:** `tests/unit/writer_disk_source_tests.cpp`

**Context helper pattern** (`tests/unit/writer_disk_source_tests.cpp:16-22`):
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
```

**Use for Phase 13:** rename this suite with the helper rename and preserve the same black-box exact/prefix/chunk/missing-source/change-detection coverage.

## Shared Patterns

### Host-file diagnostics context
**Source:** `src/detail/writer_disk_source.hpp:14-21`  
**Apply to:** neutral host-file helper, writer call-site migrations, any new read-side helper contexts

```cpp
struct writer_disk_source_context {
  std::string_view open_error;
  std::string_view inspect_error;
  std::string_view read_error;
  std::string_view changed_error;
  std::string_view allocation_description;
};
```

### Windows-aware `std::filesystem::path` open boundary
**Source:** `src/detail/writer_disk_source.cpp:17-23, 76-89`  
**Apply to:** `archive.cpp`, `validation.cpp` cleanup, all parser/reader file opens

```cpp
std::ifstream input{std::filesystem::path{std::string{host_path}}, std::ios::binary};
if (!input) {
  return io_error(context.open_error);
}
```

### Open-as-single-setup validation contract
**Source:** `src/validation.cpp:178-194`  
**Apply to:** `validate_archive` only

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

### Reader-state path ownership
**Source:** `src/archive.cpp:28-33, 150-194`  
**Apply to:** archive open state and follow-on extraction reopen paths

```cpp
struct archive_reader::state {
  archive_metadata metadata;
  std::vector<entry_metadata> entries;
  std::string host_path;
  bool is_ba2_dx10{false};
};
```

Use the same central-state ownership idea, but swap `host_path` for the new shared internal two-form host-path value.

### Manifest-backed black-box regression tests
**Source:** `tests/unit/validation_api_tests.cpp:125-139`, `tests/unit/ba2_dx10_extraction_tests.cpp:141-159`  
**Apply to:** `tests/unit/host_path_correctness_boundary_tests.cpp`

```cpp
auto validated = libbsa::validate_archive(host_path, options);
REQUIRE(validated.has_value());

auto bytes = opened.value().extract_bytes(expected.at("path").get<std::string>());
REQUIRE(bytes.has_value());
```

## No Analog Found

None. The only unresolved item is naming: the final neutral replacement for `writer_disk_source` and the exact filename for the new shared internal host-path value are not locked by context. The implementation shape is clear; the spelling is still planner discretion.

## Metadata

**Analog search scope:** `src/detail/`, `src/`, `src/formats/bsa/`, `src/formats/ba2/`, `tests/unit/`, `tests/`  
**Files scanned:** 21  
**Pattern extraction date:** 2026-05-13
