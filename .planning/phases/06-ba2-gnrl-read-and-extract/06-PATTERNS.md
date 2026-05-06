# Phase 06: ba2-gnrl-read-and-extract - Pattern Map

**Mapped:** 2026-05-05
**Files analyzed:** 5
**Analogs found:** 5 / 5

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/ba2.hpp` | public API / model wrapper | request-response + file-I/O contract | `include/libbsa/bsa.hpp` | exact |
| `src/ba2_reader.cpp` | parser/service | file-I/O + request-response extraction | `src/bsa_reader.cpp` | role-match |
| `tests/ba2_reader_tests.cpp` | test | fixture + file-I/O + codec validation | `tests/bsa_reader_tests.cpp` | role-match |
| `tests/public_header_smoke.cpp` | test | public API smoke | `tests/public_header_smoke.cpp` | exact-modification |
| `CMakeLists.txt` | config | build/test wiring | `CMakeLists.txt` | exact-modification |

## Pattern Assignments

### `include/libbsa/ba2.hpp` (public API / model wrapper, request-response + file-I/O contract)

**Analog:** `include/libbsa/bsa.hpp`

**Imports pattern** (lines 1-10):
```cpp
#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <string>
#include <vector>
```

**Metadata-only archive wrapper pattern** (lines 14-37):
```cpp
/// Owns parsed metadata for a BSA-family archive.
///
/// The archive object is a metadata view only: payload bytes remain owned by the
/// caller-provided `byte_source` passed to `open_bsa` and extraction calls.
class bsa_archive {
public:
    /// Builds an archive metadata view from a parsed summary and copied entries.
    bsa_archive(archive_summary summary, std::vector<entry_metadata> entries);

    /// Returns the parsed archive summary.
    [[nodiscard]] const archive_summary& summary() const noexcept;

    /// Returns normalized archive paths in deterministic sorted order.
    [[nodiscard]] std::vector<archive_path> paths() const;

    /// Returns true when `path` normalizes to an entry in this archive.
    [[nodiscard]] bool contains(std::string path) const;

    /// Returns copied entry metadata for `path`, or a structured lookup failure.
    [[nodiscard]] result<entry_metadata> entry(std::string path) const;

private:
    archive_view view_;
};
```

**Open/extract API pattern** (lines 39-52):
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

**Apply to BA2:** Rename `bsa_archive`/`open_bsa`/`extract_bsa_entry` to `ba2_archive`/`open_ba2`/`extract_ba2_entry`, keep the same include set, namespace, Doxygen style, `archive_view` member, `result<T>` API, and caller-owned `byte_source`/`byte_sink` lifetime contract.

---

### `src/ba2_reader.cpp` (parser/service, file-I/O + request-response extraction)

**Analog:** `src/bsa_reader.cpp`

**Imports pattern** (lines 1-12):
```cpp
#include <libbsa/bsa.hpp>
#include <libbsa/compression.hpp>

#include "bsa_reader.hpp"
#include "hash.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>
#include <utility>
```

**Bounded read and overflow/range validation pattern** (lines 52-90):
```cpp
bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& result) noexcept
{
    if (left > std::numeric_limits<std::uint64_t>::max() - right) {
        return false;
    }
    result = left + right;
    return true;
}

bool range_fits(std::uint64_t offset, std::uint64_t size, std::uint64_t source_size) noexcept
{
    std::uint64_t end = 0;
    return checked_add(offset, size, end) && end <= source_size;
}

result<std::vector<std::byte>> read_bytes(const byte_source& source, std::uint64_t offset, std::uint64_t size)
{
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) || !range_fits(offset, size, source.size())) {
        return failure<std::vector<std::byte>>(truncated_table_error());
    }
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    auto read = source.read_at(offset, std::span<std::byte>{bytes});
    if (!read.has_value()) {
        return failure<std::vector<std::byte>>(truncated_table_error());
    }
    return success(std::move(bytes));
}

result<std::vector<std::byte>> read_payload_bytes(const byte_source& source, std::uint64_t offset, std::uint64_t size)
{
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) || !range_fits(offset, size, source.size())) {
        return failure<std::vector<std::byte>>({error_code::malformed_archive, "BSA payload range exceeds source size"});
    }
```

**Little-endian parsing pattern** (lines 93-109):
```cpp
std::uint32_t le_u32(std::span<const std::byte> bytes, std::size_t offset) noexcept
{
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3])) << 24U);
}

std::uint64_t le_u64(std::span<const std::byte> bytes, std::size_t offset) noexcept
{
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[offset + static_cast<std::size_t>(shift / 8)]))
                 << static_cast<unsigned>(shift);
    }
    return value;
}
```

**Table parse → `entry_metadata` population pattern** (lines 418-448):
```cpp
for (std::uint32_t i = 0; i < file_count; ++i) {
    const auto& partial = partials[i];
    const auto stored_size = partial.stored_size_field & ~detail::file_size_compress;
    auto state = compression_for(version, flags, partial.stored_size_field);
    entry_metadata metadata{};
    metadata.path = detail::bsa_join_path(partial.folder, names.value()[i]);
    metadata.offset = partial.offset;
    metadata.stored_size = stored_size;
    metadata.packed_size = stored_size;
    metadata.name_hash = partial.name_hash == 0 ? detail::hash_tes4_path(names.value()[i]) : partial.name_hash;
    metadata.directory_hash = partial.folder_hash;
    metadata.compression = state;
    if (state == compression_state::raw) {
        metadata.size = stored_size;
    } else {
        std::uint64_t size_offset = partial.offset;
        if ((flags & detail::archive_embed_name) != 0) {
            auto prefix = read_bytes(source, size_offset, 1);
```

**Archive wrapper implementation pattern** (lines 301-324):
```cpp
bsa_archive::bsa_archive(archive_summary summary, std::vector<entry_metadata> entries)
    : view_(std::move(summary), std::move(entries))
{
}

const archive_summary& bsa_archive::summary() const noexcept
{
    return view_.summary();
}

std::vector<archive_path> bsa_archive::paths() const
{
    return view_.paths();
}

bool bsa_archive::contains(std::string path) const
{
    return view_.contains(std::move(path));
}

result<entry_metadata> bsa_archive::entry(std::string path) const
{
    return view_.entry(std::move(path));
}
```

**Extraction + codec dispatch pattern** (lines 476-523):
```cpp
result<void> extract_bsa_entry(const bsa_archive& archive, const byte_source& source, std::string path, byte_sink& sink)
{
    auto metadata = archive.entry(std::move(path));
    if (!metadata.has_value()) {
        return failure<void>(metadata.error());
    }

    auto payload = read_payload_bytes(source, metadata.value().offset, metadata.value().stored_size);
    if (!payload.has_value()) {
        return failure<void>(payload.error());
    }
    ...
    payload_codec_request request{};
    request.format = archive.summary().format;
    request.entry_state = metadata.value().compression;
    request.compression_method = archive.summary().compression_method;
    auto algorithm = resolve_payload_codec(request);
    if (!algorithm.has_value()) {
        return failure<void>(algorithm.error());
    }
    const auto packed = std::span<const std::byte>{payload.value()}.subspan(cursor);
    auto unpacked = decompress_payload(algorithm.value(), packed, expected_size);
    if (!unpacked.has_value()) {
        return failure<void>(unpacked.error());
    }
    return sink.write(std::span<const std::byte>{unpacked.value()});
}
```

**BA2 detection/header analog** from `src/detect.cpp` (lines 101-148):
```cpp
result<archive_summary> detect_ba2(const byte_source& source)
{
    auto base = read_header(source, 12);
    ...
    const auto version = read_u32(base.value(), 4);
    std::uint64_t required_size = 24;
    if (version == version_starfield_v2) {
        required_size = 32;
    } else if (version == version_starfield_v3) {
        required_size = 36;
    }
    ...
    summary.version = version;
    summary.subtype = read_u32(header.value(), 8);
    summary.file_count = read_u32(header.value(), 12);
    summary.file_table_offset = read_u64(header.value(), 16);
    if (version == version_starfield_v3) {
        summary.compression_method = read_u32(header.value(), 32);
    }
```

**Apply to BA2:** Use BSA bounded read/range helpers, but do not copy the BSA compressed-payload embedded-size-prefix behavior. For BA2 compressed entries, read exactly `entry_metadata::stored_size` from absolute `entry_metadata::offset` and pass `metadata.size` directly as `expected_size` to `decompress_payload`. Use `src/detect.cpp` BA2 header lengths and version/subtype constants as the detection analog. Add comments near BA2 hash/size/compression mapping because D-06/D-07/D-10 are compatibility constraints.

---

### `tests/ba2_reader_tests.cpp` (test, fixture + file-I/O + codec validation)

**Analog:** `tests/bsa_reader_tests.cpp`

**Imports and fixture helper style** (lines 1-15, 35-52):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/bsa.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/io.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>
...
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
```

**Generated archive fixture builder pattern** (lines 157-219):
```cpp
std::vector<std::byte> make_payload(const packed_entry& entry)
{
    std::vector<std::byte> payload;
    ...
    if (entry.algorithm == libbsa::compression_algorithm::none) {
        payload.insert(payload.end(), entry.output.begin(), entry.output.end());
        return payload;
    }

    append_u32(payload, static_cast<std::uint32_t>(entry.output.size()));
    auto compressed = libbsa::compress_payload(entry.algorithm, std::span<const std::byte>{entry.output});
    REQUIRE(compressed.has_value());
    payload.insert(payload.end(), compressed.value().begin(), compressed.value().end());
    return payload;
}

std::vector<std::byte> bsa_archive_bytes(std::uint32_t version, std::uint32_t flags, packed_entry entry)
{
    ...
    if (bytes.size() < payload_offset) {
        bytes.resize(payload_offset, std::byte{0});
    }
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}
```

**Open/list/metadata assertion pattern** (lines 290-305):
```cpp
TEST_CASE("open_bsa lists TES4 v103 metadata", "[fixture]")
{
    const auto opened = open_bytes(bsa_archive_bytes(VERSION_TES4, 0, {}));

    REQUIRE(opened.has_value());
    CHECK(opened.value().summary().format == libbsa::archive_format::tes4_bsa);
    CHECK(opened.value().summary().version == VERSION_TES4);
    const auto paths = opened.value().paths();
    REQUIRE(paths.size() == 1);
    CHECK(paths[0].string() == "meshes/armor/iron.nif");
    const auto metadata = opened.value().entry("meshes/armor/iron.nif");
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().size == 4);
    CHECK(metadata.value().packed_size == 4);
    CHECK(metadata.value().compression == libbsa::compression_state::raw);
}
```

**Extraction helper and payload assertion pattern** (lines 367-383):
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

TEST_CASE("extract_bsa_entry writes raw TES4 bytes", "[fixture]")
{
    const auto bytes = bsa_archive_bytes(VERSION_TES4, 0, {});

    CHECK(extract_bytes(bytes, "meshes/armor/iron.nif") ==
          std::vector{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}});
}
```

**Malformed/error assertion pattern** (lines 496-512):
```cpp
TEST_CASE("extract_bsa_entry rejects impossible payload ranges", "[unit]")
{
    packed_entry entry;
    entry.payload_offset_override = 0xfffffff0U;
    auto bytes = bsa_archive_bytes(VERSION_TES4, 0, entry);
    bytes.resize(128);
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    auto archive = libbsa::open_bsa(source);
    REQUIRE(archive.has_value());
    libbsa::memory_sink sink;

    auto extracted = libbsa::extract_bsa_entry(archive.value(), source, "meshes/armor/iron.nif", sink);

    REQUIRE_FALSE(extracted.has_value());
    CHECK(extracted.error().code == libbsa::error_code::malformed_archive);
    CHECK(extracted.error().message == "BSA payload range exceeds source size");
}
```

**BA2-specific detection fixture analog** from `tests/detection_tests.cpp` (lines 67-83):
```cpp
std::vector<std::byte> ba2_sample(std::uint32_t version, std::string_view subtype, std::uint32_t compression_method = 0)
{
    std::vector<std::byte> bytes;
    append_magic(bytes, "BTDX");
    append_u32(bytes, version);
    append_magic(bytes, subtype);
    append_u32(bytes, 5);
    append_u64(bytes, 48);
    if (version == 2) {
        append_u32(bytes, 0);
        append_u32(bytes, 0);
    } else if (version == 3) {
        append_u32(bytes, 0);
        append_u32(bytes, 0);
        append_u32(bytes, compression_method);
    }
    return bytes;
}
```

**Apply to BA2:** Build deterministic source fixtures in the test file. Keep local `append_u16`, `append_u32`, `append_u64`, `append_magic`, and compression helpers. Unlike BSA fixture payloads, BA2 compressed payloads should not include an embedded uncompressed-size prefix; use `PackedSize`/`Size` record fields to drive tests. Cover FO4 v1/v7/v8, Starfield v2/v3, file name tables, `.dds` names in GNRL, raw/deflate/LZ4-block, malformed headers/records/name tables, impossible payload offsets, and codec route confusion.

---

### `tests/public_header_smoke.cpp` (test, public API smoke)

**Analog:** `tests/public_header_smoke.cpp`

**Public header include block pattern** (lines 1-8):
```cpp
#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/bsa.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/detect.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>
```

**Consumer-style object/symbol use pattern** (lines 51-58):
```cpp
auto path = libbsa::normalize_archive_path("textures/actors/hero.dds");
const libbsa::archive_view view{summary, std::vector{metadata}};
const libbsa::bsa_archive bsa{summary, std::vector{bsa_metadata}};
const auto bsa_paths = bsa.paths();
const auto bsa_entry = bsa.entry("meshes/armor/iron.nif");
const auto open_bsa_fn = &libbsa::open_bsa;
const auto extract_bsa_entry_fn = &libbsa::extract_bsa_entry;
```

**Smoke return pattern** (lines 59-66):
```cpp
return ok.has_value() && source.size() == 2 && write.has_value() && sink.bytes().size() == 2 && codec.has_value() &&
        codec.value() == libbsa::compression_algorithm::lz4_block && write_compression.has_value() &&
        write_compression.value() == libbsa::compression_state::lz4_frame && path.has_value() &&
        path.value().string() == "textures/actors/hero.dds" && view.contains("textures\\actors\\hero.dds") &&
        bsa_paths.size() == 1 && bsa.contains("meshes\\armor\\iron.nif") && bsa_entry.has_value() &&
        open_bsa_fn != nullptr && extract_bsa_entry_fn != nullptr
    ? 0
    : 1;
```

**Apply to BA2:** Add `#include <libbsa/ba2.hpp>`, construct a `libbsa::ba2_archive` with BA2-format summary and copied metadata, call `paths()`/`entry()`/`contains()`, and take addresses of `libbsa::open_ba2` and `libbsa::extract_ba2_entry`. Keep the smoke test dependency-free beyond public headers.

---

### `CMakeLists.txt` (config, build/test wiring)

**Analog:** `CMakeLists.txt`

**Explicit source/header list pattern** (lines 45-72):
```cmake
# TES5Edit/ is read-only reference material; do not compile, link, vendor,
# or add it to libbsa source lists.
target_sources(libbsa
  PRIVATE
    src/libbsa.cpp
    src/detect.cpp
    src/archive_path.cpp
    src/archive_view.cpp
    src/bsa_reader.cpp
    src/compression.cpp
    src/compression/deflate_codec.cpp
    src/compression/lz4_block_codec.cpp
    src/compression/lz4_frame_codec.cpp
    src/hash.cpp
    src/io.cpp
  PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS include
    FILES
      include/libbsa/result.hpp
      include/libbsa/archive.hpp
      include/libbsa/archive_path.hpp
      include/libbsa/archive_view.hpp
      include/libbsa/bsa.hpp
      include/libbsa/compression.hpp
      include/libbsa/detect.hpp
      include/libbsa/io.hpp)
```

**Catch2 test target pattern** (lines 166-172):
```cmake
add_executable(libbsa_bsa_reader_tests tests/bsa_reader_tests.cpp)
target_link_libraries(libbsa_bsa_reader_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain)

catch_discover_tests(libbsa_bsa_reader_tests TEST_PREFIX "libbsa_bsa_reader_tests." ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;fixture")
```

**Public header smoke target pattern** (lines 174-180):
```cmake
add_executable(libbsa_public_header_smoke tests/public_header_smoke.cpp)
target_link_libraries(libbsa_public_header_smoke
  PRIVATE
    libbsa::libbsa)

add_test(NAME libbsa.public_header_smoke COMMAND libbsa_public_header_smoke)
set_tests_properties(libbsa.public_header_smoke PROPERTIES LABELS smoke)
```

**Apply to BA2:** Add `src/ba2_reader.cpp` to `target_sources(libbsa PRIVATE ...)`, add `include/libbsa/ba2.hpp` to the public header file set, add `libbsa_ba2_reader_tests` exactly like `libbsa_bsa_reader_tests`, and use labels `unit;fixture;codec` because BA2 GNRL tests must prove deflate and Starfield LZ4-block routing.

## Shared Patterns

### Metadata-only copied lookup
**Source:** `include/libbsa/archive_view.hpp` lines 13-37 and `src/archive_view.cpp` lines 7-16  
**Apply to:** `include/libbsa/ba2.hpp`, `src/ba2_reader.cpp`
```cpp
/// Metadata-only lookup view over copied archive entries.
///
/// The view owns normalized keys and copied metadata. It does not retain caller
/// vectors, byte sources, sinks, spans, or archive payload lifetimes.
class archive_view {
...
archive_view::archive_view(archive_summary summary, std::vector<entry_metadata> entries) : summary_(std::move(summary))
{
    for (auto metadata : entries) {
        auto normalized = normalize_archive_path(metadata.path);
        if (!normalized.has_value()) {
            continue;
        }
        metadata.path = normalized.value().string();
        entries_.insert_or_assign(metadata.path, std::move(metadata));
    }
}
```

### Structured error handling
**Source:** `src/bsa_reader.cpp` lines 35-38, 67-77, 80-90  
**Apply to:** `src/ba2_reader.cpp`, `tests/ba2_reader_tests.cpp`
```cpp
error truncated_table_error()
{
    return {error_code::malformed_archive, "truncated BSA table"};
}
...
if (!read.has_value()) {
    return failure<std::vector<std::byte>>(truncated_table_error());
}
return success(std::move(bytes));
```

### Compression routing, including Starfield BA2 raw LZ4 blocks
**Source:** `src/compression.cpp` lines 45-82 and `include/libbsa/compression.hpp` lines 32-46  
**Apply to:** `src/ba2_reader.cpp`, `tests/ba2_reader_tests.cpp`
```cpp
struct payload_codec_request {
    archive_format format{};
    compression_state entry_state{compression_state::archive_default};
    std::optional<std::uint32_t> compression_method;
};
...
case compression_state::lz4_block:
    if (is_starfield_ba2(request.format) && request.compression_method == 3) {
        return success(compression_algorithm::lz4_block);
    }
    return unsupported_route();
```

### Caller-owned I/O boundary
**Source:** `include/libbsa/io.hpp` lines 12-40  
**Apply to:** `include/libbsa/ba2.hpp`, `src/ba2_reader.cpp`, `tests/ba2_reader_tests.cpp`
```cpp
/// Provides caller-owned random-access bytes to archive operations.
///
/// Implementations must report the complete readable extent and reject reads
/// that cannot be satisfied exactly. The source object and backing storage are
/// owned by the caller for the duration of each operation.
class byte_source {
...
/// Receives bytes produced by future archive extraction operations.
class byte_sink {
```

### BA2 detection metadata contract
**Source:** `src/detect.cpp` lines 121-145 and `include/libbsa/archive.hpp` lines 37-46  
**Apply to:** `src/ba2_reader.cpp`, `tests/ba2_reader_tests.cpp`
```cpp
summary.version = version;
summary.subtype = read_u32(header.value(), 8);
summary.file_count = read_u32(header.value(), 12);
summary.file_table_offset = read_u64(header.value(), 16);
if (version == version_starfield_v3) {
    summary.compression_method = read_u32(header.value(), 32);
}
```

## No Analog Found

No files are missing close analogs. BA2-specific record layout and uint16 length-prefixed name-table parsing should use `06-RESEARCH.md` and TES5Edit reference behavior for format details, while copying project code patterns from the analogs above.

## Metadata

**Analog search scope:** `include/libbsa/*.hpp`, `src/*.cpp`, `tests/*.cpp`, `CMakeLists.txt`; semantic `ccc search` attempted for parser/extraction patterns and returned no results, so explicit project references from CONTEXT/RESEARCH were used.  
**Files scanned:** 17 discovered files; 12 files read directly for pattern extraction.  
**Pattern extraction date:** 2026-05-05
