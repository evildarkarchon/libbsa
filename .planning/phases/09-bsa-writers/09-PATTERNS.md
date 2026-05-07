# Phase 09: bsa-writers - Pattern Map

**Mapped:** 2026-05-06
**Files analyzed:** 7 new/modified files
**Analogs found:** 7 / 7

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/bsa_writer.hpp` or additions to `include/libbsa/bsa.hpp` | public API | request-response | `include/libbsa/writer.hpp`; `include/libbsa/bsa.hpp` | exact |
| `src/bsa_writer.cpp` | service | transform + file-I/O + request-response | `src/writer.cpp`; `src/bsa_reader.cpp` | exact |
| `src/bsa_reader.hpp` modifications or shared BSA constants header | config/utility | transform | `src/bsa_reader.hpp`; `src/hash.cpp` | role-match |
| `tests/bsa_writer_tests.cpp` | test | request-response + file-I/O + transform | `tests/writer_core_tests.cpp`; `tests/bsa_reader_tests.cpp` | exact |
| `tests/bsa_writer_fixture_helpers.cpp` / `.hpp` (optional) | test utility | transform | `tests/bsa_reader_tests.cpp`; `tests/writer_harness_helpers.cpp` | role-match |
| `tests/public_header_smoke.cpp` | test | request-response | `tests/public_header_smoke.cpp` | exact |
| `CMakeLists.txt` | config | batch | `CMakeLists.txt` existing source/test wiring | exact |

## Pattern Assignments

### `include/libbsa/bsa_writer.hpp` or additions to `include/libbsa/bsa.hpp` (public API, request-response)

**Analog:** `include/libbsa/writer.hpp` and `include/libbsa/bsa.hpp`

**Imports pattern** (`include/libbsa/writer.hpp` lines 3-13):
```cpp
#include <libbsa/archive.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>
```

**Public Doxygen/value type pattern** (`include/libbsa/writer.hpp` lines 17-29):
```cpp
/// Describes the archive family and writer capabilities for a planning request.
///
/// The target is a libbsa-owned value type so public writer callers do not need
/// private codec, platform, or format implementation headers. The compression
/// fields carry explicit archive policy metadata used during planning instead of
/// inferring writer behavior from file extensions.
struct writer_target {
    archive_format format{};
    bool archive_default_compressed{};
    bool supports_compression{true};
    bool supports_shared_data_regions{};
    std::optional<std::uint32_t> compression_method;
};
```

**Plan/finalize contract pattern** (`include/libbsa/writer.hpp` lines 90-120):
```cpp
/// Owns a deterministic preview of an archive write operation.
///
/// The plan-then-finalize API shape follows D-01/D-09: planning owns all layout
/// decisions and stored region bytes, while finalization only streams the planned
/// bytes to a caller-owned `byte_sink` for the duration of that operation.
struct write_plan {
    writer_target target;
    writer_options options;
    std::vector<planned_table_region> table_regions;
    std::vector<planned_data_region> data_regions;
    std::vector<planned_entry> entries;
    std::uint64_t total_size{};
};

[[nodiscard]] result<write_plan> plan_archive_write(const writer_target& target,
                                                    std::span<const writer_entry> entries,
                                                    writer_options options = {});

[[nodiscard]] result<void> finalize_archive_write(const write_plan& plan, byte_sink& sink);
```

**BSA operation-style API pattern** (`include/libbsa/bsa.hpp` lines 39-52):
```cpp
/// Opens a BSA-family archive and parses its metadata tables.
///
/// The source is read through bounded random-access calls; payload data remains
/// in the source until a caller extracts a selected entry.
[[nodiscard]] result<bsa_archive> open_bsa(const byte_source& source);

/// Extracts a single BSA entry to a caller-owned sink.
///
/// `path` is normalized with the same rules as metadata lookup. The function
/// validates the stored payload range before writing bytes to `sink`.
[[nodiscard]] result<void> extract_bsa_entry(const bsa_archive& archive,
                                            const byte_source& source,
                                            std::string path,
                                            byte_sink& sink);
```

**Apply:** Keep BSA writer public API libbsa-owned, Doxygen-commented, C++20-only, and free of private codec/TES5Edit/platform types. Mirror `plan_*` / `finalize_*` shape and `open_bsa` discoverability.

---

### `src/bsa_writer.cpp` (service, transform + file-I/O + request-response)

**Analog:** `src/writer.cpp` with reader semantics from `src/bsa_reader.cpp`

**Imports pattern** (`src/writer.cpp` lines 1-12; `src/bsa_reader.cpp` lines 1-6):
```cpp
#include <libbsa/writer.hpp>

#include <libbsa/archive_path.hpp>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
```

```cpp
#include <libbsa/bsa.hpp>
#include <libbsa/compression.hpp>

#include "bsa_reader.hpp"
#include "hash.hpp"
```

**Structured errors and checked arithmetic pattern** (`src/writer.cpp` lines 21-62):
```cpp
error writer_layout_overflow()
{
    return {error_code::malformed_archive, "writer layout overflow"};
}

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& result) noexcept
{
    if (left > std::numeric_limits<std::uint64_t>::max() - right) {
        return false;
    }
    result = left + right;
    return true;
}

bool checked_mul(std::uint64_t left, std::uint64_t right, std::uint64_t& result) noexcept
{
    if (left != 0 && right > std::numeric_limits<std::uint64_t>::max() / left) {
        return false;
    }
    result = left * right;
    return true;
}
```

**Normalize/reject duplicate input pattern** (`src/writer.cpp` lines 130-155):
```cpp
result<std::vector<normalized_writer_entry>> normalize_entries(std::span<const writer_entry> entries)
{
    std::vector<normalized_writer_entry> normalized_entries;
    normalized_entries.reserve(entries.size());
    std::vector<std::string> normalized_paths;
    normalized_paths.reserve(entries.size());

    for (const auto& entry : entries) {
        auto normalized = normalize_archive_path(entry.path);
        if (!normalized.has_value()) {
            return failure<std::vector<normalized_writer_entry>>(invalid_writer_path());
        }

        auto path = normalized.value().string();
        if (std::find(normalized_paths.begin(), normalized_paths.end(), path) != normalized_paths.end()) {
            return failure<std::vector<normalized_writer_entry>>(duplicate_writer_path());
        }
        normalized_paths.push_back(path);
        normalized_entries.push_back(normalized_writer_entry{std::move(path), &entry});
    }

    std::sort(normalized_entries.begin(), normalized_entries.end(), [](const auto& left, const auto& right) {
        return left.path < right.path;
    });
    return success(std::move(normalized_entries));
}
```

**Compression routing/no-fallback pattern** (`src/writer.cpp` lines 157-188):
```cpp
auto resolved = resolve_write_compression(target.format, entry.source->compression, target.archive_default_compressed);
if (!resolved.has_value()) {
    return failure<stored_writer_entry>(resolved.error());
}

payload_codec_request request{};
request.format = target.format;
request.entry_state = writer_compression;
request.compression_method = target.compression_method;
auto algorithm = resolve_payload_codec(request);
if (!algorithm.has_value()) {
    return failure<stored_writer_entry>(algorithm.error());
}

auto stored = compress_payload(algorithm.value(), std::span<const std::byte>{entry.source->payload});
if (!stored.has_value()) {
    return failure<stored_writer_entry>(stored.error());
}
```

**Finalization sink propagation pattern** (`src/writer.cpp` lines 230-238, 438-477):
```cpp
result<void> write_chunk(byte_sink& sink, const std::vector<std::byte>& chunk)
{
    auto written = sink.write(std::span<const std::byte>{chunk});
    if (!written.has_value()) {
        return failure<void>(written.error());
    }

    return success();
}
```

```cpp
for (const auto& region : plan.data_regions) {
    written = sink.write(std::span<const std::byte>{region.stored_payload});
    if (!written.has_value()) {
        return failure<void>(written.error());
    }
}
```

**TES3 native layout reader constraints to mirror** (`src/bsa_reader.cpp` lines 207-224, 263-285):
```cpp
const auto hash_offset = le_u32(header.value(), 4);
const auto file_count = le_u32(header.value(), 8);
...
// TES3 stores HashOffset relative to byte 12; enforce that the hash table comes after names.
if (hash_table_start < name_block_start || data_offset > source.size()) {
    return failure<bsa_archive>(truncated_table_error());
}
```

```cpp
if (!checked_add(name_block_start, records[i].name_offset, name_offset) || name_offset >= hash_table_start ||
    !checked_add(data_offset, records[i].relative_offset, payload_offset)) {
    return failure<bsa_archive>(truncated_table_error());
}
...
metadata.offset = payload_offset;
metadata.name_hash = le_u64(hash_bytes.value(), 0);
metadata.directory_hash = 0;
metadata.compression = compression_state::raw;
```

**TES4-family native layout reader constraints to mirror** (`src/bsa_reader.cpp` lines 340-367, 410-445):
```cpp
const auto folders_offset = le_u32(header_bytes.value(), 8);
const auto flags = le_u32(header_bytes.value(), 12);
const auto folder_count = le_u32(header_bytes.value(), 16);
const auto file_count = le_u32(header_bytes.value(), 20);
const auto total_file_name_length = le_u32(header_bytes.value(), 28);
const auto folder_record_size = version == detail::version_sse ? detail::folder_record_size_sse : detail::folder_record_size_legacy;
```

```cpp
const auto stored_size = partial.stored_size_field & ~detail::file_size_compress;
auto state = compression_for(version, flags, partial.stored_size_field);
metadata.path = detail::bsa_join_path(partial.folder, names.value()[i]);
metadata.offset = partial.offset;
metadata.stored_size = stored_size;
metadata.packed_size = stored_size;
metadata.name_hash = partial.name_hash == 0 ? detail::hash_tes4_path(names.value()[i]) : partial.name_hash;
metadata.directory_hash = partial.folder_hash;
metadata.compression = state;
```

**Embedded/compressed payload shape to produce** (`src/bsa_reader.cpp` lines 488-519):
```cpp
if (archive.summary().flags.has_value() && ((*archive.summary().flags & detail::archive_embed_name) != 0)) {
    // Reference: TES5Edit/Core/wbBSArchive.pas ExtractFileData skips the embedded archive name before payload bytes.
    if (payload.value().empty()) {
        return failure<void>({error_code::malformed_archive, "truncated embedded BSA name"});
    }
    const auto name_length = static_cast<std::size_t>(std::to_integer<unsigned char>(payload.value()[0]));
    if (name_length + 1U > payload.value().size()) {
        return failure<void>({error_code::malformed_archive, "truncated embedded BSA name"});
    }
    cursor = name_length + 1U;
}

std::uint64_t expected_size = payload.value().size() - cursor;
if (metadata.value().compression != compression_state::raw && metadata.value().compression != compression_state::none) {
    if (payload.value().size() - cursor < 4U) {
        return failure<void>({error_code::malformed_archive, "BSA payload range exceeds source size"});
    }
    expected_size = le_u32(payload.value(), cursor);
    cursor += 4U;
}
```

---

### `src/bsa_reader.hpp` modifications or shared BSA constants header (config/utility, transform)

**Analog:** `src/bsa_reader.cpp` internal detail constants usage and `src/hash.cpp`

**Hash implementation pattern** (`src/hash.cpp` lines 45-50, 66-88, 144-150):
```cpp
// Compatibility with TES5Edit's LowerByte requires ASCII-only lowercase; locale-aware case folding would change archive hashes.
constexpr std::uint8_t lower_byte(char ch) noexcept
{
    const auto value = static_cast<unsigned char>(ch);
    return (value >= 'A' && value <= 'Z') ? static_cast<std::uint8_t>(value + ('a' - 'A')) : static_cast<std::uint8_t>(value);
}
```

```cpp
std::uint64_t hash_tes3_path(std::string_view path)
{
    const auto half = path.size() >> 1U;
    std::uint32_t sum = 0;
    std::uint32_t off = 0;
    for (std::size_t i = 0; i < half; ++i) {
        const auto temp = static_cast<std::uint32_t>(lower_byte(path[i])) << (off & 0x1fU);
        sum ^= temp;
        off += 8;
    }

    std::uint64_t result = static_cast<std::uint64_t>(sum) << 32U;
    sum = 0;
    off = 0;
    for (std::size_t i = half; i < path.size(); ++i) {
        const auto temp = static_cast<std::uint32_t>(lower_byte(path[i])) << (off & 0x1fU);
        sum ^= temp;
        sum = rotate_right_by_temp_low_bits(sum, temp);
        off += 8;
    }

    return result | sum;
}
```

```cpp
std::uint64_t hash_tes4_path(std::string_view path)
{
    const auto dot = last_dot(path);
    const auto name = path.substr(0, dot);
    const auto ext = dot == path.size() ? std::string_view{} : path.substr(dot);
    return hash_tes4_name(name, ext);
}
```

**Apply:** Do not duplicate hash logic in the writer. Reuse internal `detail` routines or move declarations only as needed. Keep constants private/internal; public headers must not expose TES5Edit or private implementation tokens.

---

### `tests/bsa_writer_tests.cpp` (test, request-response + file-I/O + transform)

**Analog:** `tests/writer_core_tests.cpp` and `tests/bsa_reader_tests.cpp`

**Catch2/import pattern** (`tests/writer_core_tests.cpp` lines 1-15):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/io.hpp>
#include <libbsa/writer.hpp>

#include "writer_harness_helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>
```

**Plan determinism pattern** (`tests/writer_core_tests.cpp` lines 154-180):
```cpp
TEST_CASE("plans entries deterministically independent of caller order", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const libbsa::writer_options options{};
    const std::vector first_order{writer_entry("textures/z.dds", {std::byte{0x7a}}),
                                  writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}}),
                                  writer_entry("textures/m.dds", {std::byte{0x6d}})};
    const std::vector second_order{writer_entry("textures/m.dds", {std::byte{0x6d}}),
                                   writer_entry("textures/z.dds", {std::byte{0x7a}}),
                                   writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}})};

    const auto first = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{first_order}, options);
    const auto second = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{second_order}, options);

    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    require_plans_equal(first.value(), second.value());
}
```

**Finalization and sink failure tests** (`tests/writer_core_tests.cpp` lines 281-344):
```cpp
TEST_CASE("finalizes planned bytes to caller-owned sink", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const std::vector entries{writer_entry("textures/z.dds", {std::byte{0x7a}}),
                              writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}}),
                              writer_entry("textures/m.dds", {std::byte{0x6d}})};
    const auto plan = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});
    REQUIRE(plan.has_value());
    libbsa::memory_sink sink;

    const auto finalized = libbsa::finalize_archive_write(plan.value(), sink);

    REQUIRE(finalized.has_value());
    REQUIRE(sink.bytes().size() == plan.value().total_size);
}
```

```cpp
TEST_CASE("returns sink failure during finalization", "[unit][writer]")
{
    const auto target = raw_fo4_target();
    const std::vector entries{writer_entry("meshes/a.nif", {std::byte{0x61}, std::byte{0x62}})};
    const auto plan = libbsa::plan_archive_write(target, std::span<const libbsa::writer_entry>{entries});
    REQUIRE(plan.has_value());
    failing_sink sink{8};

    const auto finalized = libbsa::finalize_archive_write(plan.value(), sink);

    REQUIRE_FALSE(finalized.has_value());
    CHECK(finalized.error().code == libbsa::error_code::io_failure);
    CHECK(finalized.error().message == "injected sink failure");
}
```

**Read-after-write verification pattern** (`tests/bsa_reader_tests.cpp` lines 367-375):
```cpp
std::vector<std::byte> extract_bytes(const std::vector<std::byte>& bytes, std::string path)
{
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    auto archive = libbsa::open_bsa(source);
    REQUIRE(archive.has_value());
    libbsa::memory_sink sink;
    auto extracted = libbsa::extract_bsa_entry(archive.value(), source, std::move(path), sink);
    REQUIRE(extracted.has_value());
    return sink.bytes();
}
```

**Malformed/failure assertion pattern** (`tests/bsa_reader_tests.cpp` lines 442-460):
```cpp
TEST_CASE("open_bsa rejects unsupported BSA versions", "[unit]")
{
    const auto opened = open_bytes(bsa_header(0x6a, 0, 0));

    REQUIRE_FALSE(opened.has_value());
    CHECK(opened.error().code == libbsa::error_code::unsupported_format);
    CHECK(opened.error().message == "unsupported BSA version");
}
```

**Apply:** BSA writer tests should cover TES3, v103, v104, v105, disk/memory entry equivalence, read-after-write through `open_bsa` and `extract_bsa_entry`, XOR compression cases, embedded-name opt-in, dedup sharing, offset/table byte assertions, and structured failures.

---

### `tests/bsa_writer_fixture_helpers.cpp` / `.hpp` (optional test utility, transform)

**Analog:** fixture-builder style in `tests/bsa_reader_tests.cpp`

**Little-endian append/name helper pattern** (`tests/bsa_reader_tests.cpp` lines 35-81):
```cpp
void append_u8(std::vector<std::byte>& bytes, std::uint8_t value)
{
    bytes.push_back(static_cast<std::byte>(value));
}

void append_u32(std::vector<std::byte>& bytes, std::uint32_t value)
{
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void append_u64(std::vector<std::byte>& bytes, std::uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

void append_cstring(std::vector<std::byte>& bytes, std::string_view value)
{
    for (char ch : value) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    bytes.push_back(std::byte{0});
}
```

**Native BSA fixture layout pattern** (`tests/bsa_reader_tests.cpp` lines 180-219):
```cpp
std::vector<std::byte> bsa_archive_bytes(std::uint32_t version, std::uint32_t flags, packed_entry entry)
{
    const auto folder_record_size = version == VERSION_SSE ? 24U : 16U;
    std::vector<std::byte> folder_block;
    append_len8_string(folder_block, entry.folder);
    append_u64(folder_block, 0);
    const auto payload = make_payload(entry);
    const auto names_offset = 36U + folder_record_size + static_cast<std::uint32_t>(folder_block.size()) + 8U;
    const auto payload_offset = entry.payload_offset_override == 0
        ? names_offset + static_cast<std::uint32_t>(entry.file.size()) + 1U
        : entry.payload_offset_override;
    const auto stored_size = entry.stored_size_override == 0 ? static_cast<std::uint32_t>(payload.size()) : entry.stored_size_override;
    append_u32(folder_block, stored_size | (entry.file_compress_flag ? FILE_SIZE_COMPRESS : 0U));
    append_u32(folder_block, payload_offset);
    ...
}
```

**Apply:** If helpers are split out, keep them test-only, source-reviewable, and linked only to test executables. Do not turn harness helper APIs into production archive format APIs.

---

### `tests/public_header_smoke.cpp` (test, request-response)

**Analog:** existing `tests/public_header_smoke.cpp`

**Include and API pointer pattern** (`tests/public_header_smoke.cpp` lines 1-10, 94-121):
```cpp
#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/ba2.hpp>
#include <libbsa/bsa.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/detect.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>
#include <libbsa/writer.hpp>
```

```cpp
const auto open_bsa_fn = &libbsa::open_bsa;
const auto extract_bsa_entry_fn = &libbsa::extract_bsa_entry;
...
const auto plan_writer_fn = &libbsa::plan_archive_write;
const auto finalize_writer_fn = &libbsa::finalize_archive_write;
```

**Consumer-style writer smoke pattern** (`tests/public_header_smoke.cpp` lines 99-121):
```cpp
libbsa::writer_target target{};
target.format = libbsa::archive_format::fo4_ba2_gnrl;
target.archive_default_compressed = false;
target.supports_compression = true;
target.supports_shared_data_regions = true;

libbsa::writer_entry writer_first{};
writer_first.path = "meshes/armor/iron.nif";
writer_first.payload = {std::byte{0x01}, std::byte{0x02}};
writer_first.compression = libbsa::compression_policy::force_raw;
...
const auto writer_plan = libbsa::plan_archive_write(
    target, std::span<const libbsa::writer_entry>{writer_entries}, libbsa::writer_options{.deduplicate = true});
libbsa::memory_sink writer_sink;
const auto writer_finalized = writer_plan.has_value() ? libbsa::finalize_archive_write(writer_plan.value(), writer_sink)
                                                      : libbsa::failure<void>({libbsa::error_code::unsupported_format,
                                                                               "writer smoke planning failed"});
```

**Apply:** Extend this with public-only BSA writer target values, memory entry values, disk mapping values if public, plan/finalize function pointers, and success conditions. Do not include private headers or codec library headers.

---

### `CMakeLists.txt` (config, batch)

**Analog:** existing explicit source/header/test wiring

**TES5Edit boundary and public file-set pattern** (`CMakeLists.txt` lines 45-78):
```cmake
# TES5Edit/ is read-only reference material; do not compile, link, vendor,
# or add it to libbsa source lists.
target_sources(libbsa
  PRIVATE
    src/libbsa.cpp
    src/detect.cpp
    src/archive_path.cpp
    src/archive_view.cpp
    src/ba2_reader.cpp
    src/bsa_reader.cpp
    src/compression.cpp
    src/compression/deflate_codec.cpp
    src/compression/lz4_block_codec.cpp
    src/compression/lz4_frame_codec.cpp
    src/hash.cpp
    src/io.cpp
    src/writer.cpp
  PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS include
    FILES
      include/libbsa/result.hpp
      include/libbsa/archive.hpp
      include/libbsa/bsa.hpp
      include/libbsa/writer.hpp)
```

**Catch2 target pattern** (`CMakeLists.txt` lines 201-209):
```cmake
add_executable(libbsa_writer_tests
  tests/writer_core_tests.cpp
  tests/writer_harness_helpers.cpp)
target_link_libraries(libbsa_writer_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain)

catch_discover_tests(libbsa_writer_tests TEST_PREFIX "libbsa_writer_tests." ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;fixture;codec;roundtrip")
```

**Public smoke pattern** (`CMakeLists.txt` lines 222-228):
```cmake
add_executable(libbsa_public_header_smoke tests/public_header_smoke.cpp)
target_link_libraries(libbsa_public_header_smoke
  PRIVATE
    libbsa::libbsa)

add_test(NAME libbsa.public_header_smoke COMMAND libbsa_public_header_smoke)
set_tests_properties(libbsa.public_header_smoke PROPERTIES LABELS smoke)
```

**Apply:** Add `src/bsa_writer.cpp`, any public `include/libbsa/bsa_writer.hpp`, and a focused `libbsa_bsa_writer_tests` target explicitly. Never add `TES5Edit/` to sources.

## Shared Patterns

### Public API boundaries and documentation
**Source:** `include/libbsa/writer.hpp` lines 17-46; `include/libbsa/bsa.hpp` lines 39-52  
**Apply to:** BSA writer public target/options/entry/plan/finalize declarations

- Use libbsa-owned value types and `result<T>`.
- Add Doxygen comments for every public type/function.
- Keep private codec, DirectXTex, platform, Delphi, and TES5Edit types out of public headers.

### Archive path normalization and duplicate rejection
**Source:** `include/libbsa/archive_path.hpp` lines 38-43; `src/writer.cpp` lines 130-155  
**Apply to:** all BSA memory and disk entry planning
```cpp
auto normalized = normalize_archive_path(entry.path);
if (!normalized.has_value()) {
    return failure<std::vector<normalized_writer_entry>>(invalid_writer_path());
}
```

### Structured error handling
**Source:** `src/writer.cpp` lines 21-44; `src/compression.cpp` lines 12-25  
**Apply to:** planning validation, unsupported target/options, codec route failures, layout overflow, disk read failures
```cpp
return failure<compression_algorithm>({error_code::unsupported_format, "unsupported compression route"});
```

### Checked layout arithmetic
**Source:** `src/writer.cpp` lines 46-62; `src/bsa_reader.cpp` lines 52-65  
**Apply to:** table lengths, payload offsets, 32-bit BSA field fit checks, total size
```cpp
if (!checked_add(entry_table_offset, entry_table_size, data_region_table_offset) ||
    !checked_add(data_region_table_offset, data_region_table_size, payloads_offset)) {
    return failure<write_plan>(writer_layout_overflow());
}
```

### Compression and codec routing
**Source:** `include/libbsa/compression.hpp` lines 42-69; `src/compression.cpp` lines 45-107  
**Apply to:** TES4 v103/v104 deflate, v105 LZ4-frame, raw entries, no-fallback failures
```cpp
payload_codec_request request{};
request.format = target.format;
request.entry_state = writer_compression;
request.compression_method = target.compression_method;
auto algorithm = resolve_payload_codec(request);
```

### BSA compression XOR and embedded-name semantics
**Source:** `src/bsa_reader.cpp` lines 189-198 and 488-507  
**Apply to:** TES4-family size field and native payload construction
```cpp
// Reference: TES5Edit/Core/wbBSArchive.pas TwbBSFileTES4.Compressed uses archive default XOR per-file flag.
const bool default_compressed = (archive_flags & detail::archive_compress) != 0;
const bool toggled = (stored_size_field & detail::file_size_compress) != 0;
if (!(default_compressed ^ toggled)) {
    return compression_state::raw;
}
```

### Read-after-write tests
**Source:** `tests/bsa_reader_tests.cpp` lines 367-375  
**Apply to:** all required BSA variants
```cpp
const libbsa::memory_source source{std::span<const std::byte>{bytes}};
auto archive = libbsa::open_bsa(source);
REQUIRE(archive.has_value());
libbsa::memory_sink sink;
auto extracted = libbsa::extract_bsa_entry(archive.value(), source, std::move(path), sink);
REQUIRE(extracted.has_value());
```

## No Analog Found

No files are without analogs. Disk-backed BSA entry ingestion has no exact existing production analog, but it should copy the public value-type style from `include/libbsa/writer.hpp`, the planning-time ownership invariant from `src/writer.cpp`, and the caller-owned I/O boundary language from `include/libbsa/io.hpp`.

## Metadata

**Analog search scope:** `include/libbsa/*.hpp`, `src/**/*.{cpp,hpp}`, `tests/**/*.{cpp,hpp}`, `CMakeLists.txt`  
**Files scanned:** 32 source/header/test/config files via glob/grep, 12 strong analog files read  
**Pattern extraction date:** 2026-05-06
