# Phase 08: writer-planning-streaming-emit-and-dedup-core - Pattern Map

**Mapped:** 2026-05-06  
**Files analyzed:** 7  
**Analogs found:** 7 / 7

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/writer.hpp` | model / service API | transform + streaming | `include/libbsa/ba2.hpp`, `include/libbsa/io.hpp`, `include/libbsa/compression.hpp` | role-match |
| `src/writer.cpp` | service / utility | transform + streaming + batch | `src/ba2_reader.cpp`, `src/compression.cpp`, `src/archive_path.cpp` | role-match |
| `tests/writer_core_tests.cpp` | test | transform + streaming + request-response | `tests/ba2_reader_tests.cpp`, `tests/io_tests.cpp`, `tests/compression_policy_tests.cpp` | role-match |
| `tests/writer_harness_helpers.hpp` | test utility | transform | `tests/ba2_dds_fixture_helpers.hpp` | exact |
| `tests/writer_harness_helpers.cpp` | test utility | transform + batch | `tests/ba2_dds_fixture_helpers.cpp` | exact |
| `tests/public_header_smoke.cpp` | test | request-response / compile smoke | `tests/public_header_smoke.cpp` | exact-modify |
| `CMakeLists.txt` | config | build graph | `CMakeLists.txt` | exact-modify |

## Pattern Assignments

### `include/libbsa/writer.hpp` (model / service API, transform + streaming)

**Analogs:** `include/libbsa/ba2.hpp`, `include/libbsa/bsa.hpp`, `include/libbsa/io.hpp`, `include/libbsa/compression.hpp`, `include/libbsa/archive.hpp`

**Imports pattern** (`include/libbsa/ba2.hpp` lines 3-13):
```cpp
#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>
```

**Public value-type metadata pattern** (`include/libbsa/ba2.hpp` lines 32-58):
```cpp
/// Describes one stored BA2 DDS texture chunk.
///
/// Offsets are archive-absolute payload offsets. `packed_size` is the stored
/// payload byte count, while `size` is the reconstructed uncompressed chunk byte
/// count expected after codec routing.
struct texture_chunk_metadata {
    std::uint32_t mip_level{};
    std::uint64_t offset{};
    std::uint64_t packed_size{};
    std::uint64_t size{};
    compression_state compression{compression_state::unknown};
};

/// Copied public metadata for a BA2 DDS texture entry.
///
/// The value is independent of parser storage and contains only libbsa-owned
/// types so consumers can inspect texture layout without private dependencies.
struct texture_metadata {
    std::string path;
    dxgi_format format{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t mip_count{};
    std::uint32_t array_size{};
    bool is_cubemap{};
    std::vector<texture_chunk_metadata> chunks;
};
```

**Operation-style API pattern** (`include/libbsa/bsa.hpp` lines 39-52):
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

**Caller-owned sink contract to reuse** (`include/libbsa/io.hpp` lines 31-40):
```cpp
/// Receives bytes produced by future archive extraction operations.
///
/// The sink does not prescribe where bytes are stored; callers keep ownership
/// of the concrete sink object for the duration of each write operation.
class byte_sink {
public:
    virtual ~byte_sink() = default;

    /// Writes the supplied bytes to the sink in order.
    [[nodiscard]] virtual result<void> write(std::span<const std::byte> bytes) = 0;
};
```

**Compression-policy API to expose/use, not duplicate** (`include/libbsa/compression.hpp` lines 25-54):
```cpp
/// Describes how writer code should choose an entry's archive compression state.
enum class compression_policy {
    archive_default,
    force_compressed,
    force_raw,
};

/// Carries all archive metadata needed to route a packed payload to a codec.
///
/// Starfield BA2 v3 stores raw block codec selection in `compression_method`, so
/// callers must pass the detected header value instead of guessing from paths.
struct payload_codec_request {
    archive_format format{};
    compression_state entry_state{compression_state::archive_default};
    std::optional<std::uint32_t> compression_method;
};

/// Resolves archive format and entry metadata into the exact codec implementation.
///
/// Unsupported combinations return `error_code::unsupported_format` rather than
/// falling back to another codec, preventing frame/block or deflate confusion.
[[nodiscard]] result<compression_algorithm> resolve_payload_codec(const payload_codec_request& request);

/// Resolves a writer policy into the archive-native compression state to emit.
///
/// `archive_default_compressed` is the caller's known archive default flag; it is
/// only used when the policy asks to preserve the archive default behavior.
[[nodiscard]] result<compression_state> resolve_write_compression(archive_format format,
                                                                  compression_policy policy,
                                                                  bool archive_default_compressed);
```

**Apply to Phase 8:** define `writer_entry`, `writer_target` / capabilities, `writer_options`, `planned_*` records, and `write_plan` as libbsa-owned public C++20 values with Doxygen comments. Functions should mirror `open_*` / `extract_*` operation style: e.g. `result<write_plan> plan_archive_write(...)` and `result<void> finalize_archive_write(const write_plan&, byte_sink&)`.

---

### `src/writer.cpp` (service / utility, transform + streaming + batch)

**Analogs:** `src/ba2_reader.cpp`, `src/bsa_reader.cpp`, `src/compression.cpp`, `src/archive_path.cpp`, `src/archive_view.cpp`

**Imports pattern** (`src/ba2_reader.cpp` lines 1-14):
```cpp
#include <libbsa/ba2.hpp>
#include <libbsa/compression.hpp>

#include "texture/dds_reconstruction.hpp"
#include "texture/dds_validation.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>
```

**Private helper/error namespace pattern** (`src/ba2_reader.cpp` lines 16-37):
```cpp
namespace libbsa {
namespace {

constexpr std::uint32_t magic_btdx = 0x58445442U;
constexpr std::uint32_t magic_gnrl = 0x4c524e47U;
constexpr std::uint32_t magic_dx10 = 0x30315844U;

error truncated_table_error()
{
    return {error_code::malformed_archive, "truncated BA2 table"};
}
```

**Checked arithmetic pattern** (`src/ba2_reader.cpp` lines 39-52):
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
```

**Path normalization + duplicate rejection pattern** (`src/ba2_reader.cpp` lines 299-329):
```cpp
std::vector<entry_metadata> entries;
entries.reserve(file_count);
std::vector<std::string> normalized_paths;
normalized_paths.reserve(file_count);
for (std::uint32_t i = 0; i < file_count; ++i) {
    const auto& record = records[i];
    entry_metadata metadata{};
    auto normalized = normalize_archive_path(names.value().names[i]);
    if (!normalized.has_value()) {
        return failure<ba2_archive>({error_code::malformed_archive, "invalid BA2 name"});
    }
    metadata.path = normalized.value().string();
    // BA2 name tables associate names by record index; reject duplicate normalized
    // keys before archive_view::insert_or_assign can overwrite and hide a bad table.
    if (std::find(normalized_paths.begin(), normalized_paths.end(), metadata.path) != normalized_paths.end()) {
        return failure<ba2_archive>({error_code::malformed_archive, "duplicate BA2 name"});
    }
    normalized_paths.push_back(metadata.path);
    metadata.offset = record.offset;
    metadata.size = record.size;
    // BA2 records store archive-absolute payload offsets. PackedSize == 0 means
    // raw bytes, so public packed/stored size follows Size rather than zero.
    metadata.packed_size = stored_size_for_record(record);
    metadata.stored_size = metadata.packed_size;
    metadata.compression = compression_for_record(version, compression_method, record.packed_size);
    metadata.name_hash = record.name_hash;
    metadata.directory_hash = record.directory_hash;
    if (!range_fits(metadata.offset, metadata.stored_size, source.size())) {
        return failure<ba2_archive>({error_code::malformed_archive, "BA2 payload range exceeds source size"});
    }
```

**Deterministic sorted storage precedent** (`src/archive_view.cpp` lines 7-16, 24-35):
```cpp
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

std::vector<archive_path> archive_view::paths() const
{
    std::vector<archive_path> result;
    result.reserve(entries_.size());
    for (const auto& [path, metadata] : entries_) {
        (void)metadata;
        auto normalized = normalize_archive_path(path);
        if (normalized.has_value()) {
            result.push_back(std::move(normalized.value()));
        }
    }
    return result;
}
```

**Compression planning/dispatch pattern** (`src/compression.cpp` lines 84-107, 131-145):
```cpp
result<compression_state> resolve_write_compression(archive_format format,
                                                    compression_policy policy,
                                                    bool archive_default_compressed)
{
    switch (policy) {
    case compression_policy::force_raw:
        return success(compression_state::raw);
    case compression_policy::archive_default:
        if (!archive_default_compressed) {
            return success(compression_state::raw);
        }
        [[fallthrough]];
    case compression_policy::force_compressed:
        if (format == archive_format::sse_bsa) {
            return success(compression_state::lz4_frame);
        }
        if (supports_deflate(format)) {
            return success(compression_state::deflate);
        }
        return unsupported_policy();
    }

    return unsupported_policy();
}

result<std::vector<std::byte>> compress_payload(compression_algorithm algorithm, std::span<const std::byte> unpacked)
{
    switch (algorithm) {
    case compression_algorithm::none:
        return success(std::vector<std::byte>{unpacked.begin(), unpacked.end()});
    case compression_algorithm::deflate:
        return detail::deflate_compress(unpacked);
    case compression_algorithm::lz4_frame:
        return detail::lz4_frame_compress(unpacked);
    case compression_algorithm::lz4_block:
        return detail::lz4_block_compress(unpacked);
    }

    return unsupported_payload_codec();
}
```

**Sink emission pattern** (`src/ba2_reader.cpp` lines 705-721):
```cpp
payload_codec_request request{};
request.format = archive.summary().format;
request.entry_state = metadata.value().compression;
request.compression_method = archive.summary().compression_method;
auto algorithm = resolve_payload_codec(request);
if (!algorithm.has_value()) {
    return failure<void>(algorithm.error());
}

// BA2 records supply the unpacked Size and compressed PackedSize fields;
// unlike TES4-family BSA payloads, there is no embedded size prefix per D-11.
auto output = decompress_payload(algorithm.value(), std::span<const std::byte>{payload.value()}, metadata.value().size);
if (!output.has_value()) {
    return failure<void>(output.error());
}

return sink.write(std::span<const std::byte>{output.value()});
```

**Apply to Phase 8:** planning should normalize and sort first, reject duplicate normalized paths before any map can overwrite them, resolve compression and build stored bytes before dedup, compute offsets with checked helpers, then finalization should only iterate already-planned chunks and return the first `sink.write` failure.

---

### `tests/writer_core_tests.cpp` (test, transform + streaming + request-response)

**Analogs:** `tests/ba2_reader_tests.cpp`, `tests/ba2_dds_reader_tests.cpp`, `tests/io_tests.cpp`, `tests/compression_policy_tests.cpp`

**Imports/test namespace pattern** (`tests/ba2_reader_tests.cpp` lines 1-17):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/ba2.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/io.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
```

**Generated fixture builder style** (`tests/ba2_reader_tests.cpp` lines 61-69, 89-139):
```cpp
struct ba2_entry_fixture {
    std::string path{"meshes/armor/iron.nif"};
    std::uint32_t name_hash{0x11223344};
    std::uint32_t directory_hash{0xaabbccdd};
    std::uint32_t packed_size{};
    std::uint32_t size{4};
    std::uint64_t offset{};
    std::vector<std::byte> payload{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}};
};

std::vector<std::byte> ba2_gnrl_archive_bytes(std::uint32_t version,
                                              std::vector<ba2_entry_fixture> entries,
                                              std::uint32_t compression_method = 0)
{
    const auto file_count = static_cast<std::uint32_t>(entries.size());
    const auto header_size = header_size_for(version);
    const auto file_table_offset = header_size + (file_count * RECORD_SIZE);
    const auto names = name_table_bytes(entries);
    auto payload_cursor = static_cast<std::uint64_t>(file_table_offset + names.size());

    for (auto& entry : entries) {
        if (entry.offset == 0) {
            entry.offset = payload_cursor;
        }
        payload_cursor = entry.offset + (entry.packed_size == 0 ? entry.size : entry.packed_size);
    }

    std::vector<std::byte> bytes;
    append_magic(bytes, "BTDX");
    append_u32(bytes, version);
    append_magic(bytes, "GNRL");
    append_u32(bytes, file_count);
    append_u64(bytes, file_table_offset);
    // ...records, names, payloads appended deterministically...
    return bytes;
}
```

**Read-after-write / memory_sink extraction style** (`tests/ba2_reader_tests.cpp` lines 190-199):
```cpp
std::vector<std::byte> extract_ba2_bytes(const std::vector<std::byte>& bytes, std::string path)
{
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    auto archive = libbsa::open_ba2(source);
    REQUIRE(archive.has_value());
    libbsa::memory_sink sink;
    auto extracted = libbsa::extract_ba2_entry(archive.value(), source, std::move(path), sink);
    REQUIRE(extracted.has_value());
    return sink.bytes();
}
```

**Negative failure/no-write assertion style** (`tests/ba2_reader_tests.cpp` lines 395-408):
```cpp
TEST_CASE("extract_ba2_entry returns lookup failure without writing bytes", "[unit]")
{
    const auto entry = raw_entry(VERSION_FO4_V1);
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_FO4_V1, {entry});
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    auto archive = libbsa::open_ba2(source);
    REQUIRE(archive.has_value());
    libbsa::memory_sink sink;

    const auto extracted = libbsa::extract_ba2_entry(archive.value(), source, "missing/file.bin", sink);

    REQUIRE_FALSE(extracted.has_value());
    CHECK(sink.bytes().empty());
}
```

**Codec policy assertion style** (`tests/compression_policy_tests.cpp` lines 58-69):
```cpp
TEST_CASE("writer compression policy resolves to archive-specific states", "[unit][codec]")
{
    const auto sse_compressed = libbsa::resolve_write_compression(
        libbsa::archive_format::sse_bsa, libbsa::compression_policy::force_compressed, false);
    REQUIRE(sse_compressed.has_value());
    CHECK(sse_compressed.value() == libbsa::compression_state::lz4_frame);

    const auto fo4_raw = libbsa::resolve_write_compression(
        libbsa::archive_format::fo4_ba2_gnrl, libbsa::compression_policy::force_raw, true);
    REQUIRE(fo4_raw.has_value());
    CHECK(fo4_raw.value() == libbsa::compression_state::raw);
}
```

**Sink contract test pattern** (`tests/io_tests.cpp` lines 40-52):
```cpp
TEST_CASE("memory sink appends bytes through byte sink contract", "[unit]")
{
    libbsa::memory_sink sink;
    libbsa::byte_sink& writer = sink;
    const std::array first{std::byte{0x01}, std::byte{0x02}};
    const std::array second{std::byte{0x03}};

    REQUIRE(writer.write(std::span<const std::byte>{first}).has_value());
    REQUIRE(writer.write(std::span<const std::byte>{second}).has_value());

    const std::vector expected{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    CHECK(sink.bytes() == expected);
}
```

**Apply to Phase 8:** create tests for deterministic sorted plans from permuted input, exact layout preview, dedup on/off and unsupported capability failures, compression route failures, finalization to `memory_sink`, injected failing sink, and harness read-back comparing metadata plus extracted bytes.

---

### `tests/writer_harness_helpers.hpp` (test utility, transform)

**Analog:** `tests/ba2_dds_fixture_helpers.hpp`

**Header/include pattern** (`tests/ba2_dds_fixture_helpers.hpp` lines 1-10):
```cpp
#pragma once

#include <libbsa/archive.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace libbsa::test {
```

**Descriptor/fixture public test helper pattern** (`tests/ba2_dds_fixture_helpers.hpp` lines 17-45):
```cpp
/// Describes one generated BA2 DX10 chunk before archive bytes are materialized.
struct ba2_dds_chunk_descriptor {
    std::uint16_t start_mip{};
    std::uint16_t end_mip{};
    std::vector<std::byte> payload;
    compression_state compression{compression_state::raw};
};

/// Describes one generated BA2 DX10 texture entry.
struct ba2_dds_texture_descriptor {
    std::string path{"textures/generated/one_mip.dds"};
    std::uint32_t name_hash{0x11223344};
    std::uint32_t directory_hash{0xaabbccdd};
    std::uint16_t width{4};
    std::uint16_t height{4};
    std::uint8_t mip_count{1};
    std::uint8_t dxgi_format{71};
    std::uint16_t array_size{1};
    bool cubemap{};
    std::vector<ba2_dds_chunk_descriptor> chunks;
};

/// Owns generated BA2 DX10 archive bytes and the semantic texture descriptors used to build them.
struct ba2_dds_fixture {
    std::uint32_t version{};
    std::uint32_t compression_method{};
    std::vector<ba2_dds_texture_descriptor> textures;
    std::vector<std::byte> bytes;
};
```

**Factory declarations pattern** (`tests/ba2_dds_fixture_helpers.hpp` lines 47-65, 85-104):
```cpp
/// Builds BA2 DX10 bytes for caller-supplied texture descriptors.
[[nodiscard]] ba2_dds_fixture make_ba2_dds_fixture(std::uint32_t version,
                                                   std::vector<ba2_dds_texture_descriptor> textures,
                                                   std::uint32_t compression_method = 0);

/// Returns a Fallout 4 DX10 v1 one-mip, single raw chunk fixture.
[[nodiscard]] ba2_dds_fixture fo4_dx10_v1_one_mip_fixture();

/// Returns a fixture truncated inside the DX10 record table.
[[nodiscard]] ba2_dds_fixture malformed_truncated_record_fixture();

/// Checks only semantic archive identity bytes that are stable across all generated DX10 fixtures.
void require_ba2_dds_identity(const ba2_dds_fixture& fixture);
```

**Apply to Phase 8:** define `writer_harness_entry_descriptor`, `writer_harness_fixture`, harness parser/read-back result types, `make_writer_harness_fixture(...)`, malformed fixture factories, and `require_*` assertion helpers under `libbsa::test`, not production API.

---

### `tests/writer_harness_helpers.cpp` (test utility, transform + batch)

**Analog:** `tests/ba2_dds_fixture_helpers.cpp`

**Imports and namespace pattern** (`tests/ba2_dds_fixture_helpers.cpp` lines 1-13):
```cpp
#include "ba2_dds_fixture_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

#include <libbsa/compression.hpp>

#include <algorithm>
#include <span>
#include <string_view>
#include <utility>

namespace libbsa::test {
namespace {
```

**Little-endian append helpers pattern** (`tests/ba2_dds_fixture_helpers.cpp` lines 23-56):
```cpp
void append_u16(std::vector<std::byte>& bytes, std::uint16_t value)
{
    for (int shift = 0; shift < 16; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
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

void append_magic(std::vector<std::byte>& bytes, std::string_view magic)
{
    for (char ch : magic) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
}
```

**Stored payload via real compression pattern** (`tests/ba2_dds_fixture_helpers.cpp` lines 78-88):
```cpp
std::vector<std::byte> stored_payload(compression_state compression, std::span<const std::byte> payload)
{
    if (compression == compression_state::raw) {
        return {payload.begin(), payload.end()};
    }

    auto algorithm = compression == compression_state::lz4_block ? compression_algorithm::lz4_block : compression_algorithm::deflate;
    auto compressed = compress_payload(algorithm, payload);
    REQUIRE(compressed.has_value());
    return std::move(compressed.value());
}
```

**Fixture assembly pattern** (`tests/ba2_dds_fixture_helpers.cpp` lines 127-197):
```cpp
ba2_dds_fixture make_ba2_dds_fixture(std::uint32_t version,
                                     std::vector<ba2_dds_texture_descriptor> textures,
                                     std::uint32_t compression_method)
{
    const auto file_count = static_cast<std::uint32_t>(textures.size());
    const auto header_size = header_size_for(version);
    std::uint32_t record_table_size = 0;
    for (const auto& texture : textures) {
        record_table_size += dx10_record_size(texture);
    }
    const auto file_table_offset = header_size + record_table_size;
    const auto names = name_table_bytes(textures);
    auto payload_cursor = static_cast<std::uint64_t>(file_table_offset + names.size());

    std::vector<std::vector<std::byte>> stored_chunks;
    for (const auto& texture : textures) {
        for (const auto& chunk_descriptor : texture.chunks) {
            stored_chunks.push_back(stored_payload(chunk_descriptor.compression, std::span<const std::byte>{chunk_descriptor.payload}));
        }
    }

    std::vector<std::uint64_t> chunk_offsets;
    chunk_offsets.reserve(stored_chunks.size());
    for (const auto& bytes : stored_chunks) {
        chunk_offsets.push_back(payload_cursor);
        payload_cursor += static_cast<std::uint64_t>(bytes.size());
    }

    std::vector<std::byte> bytes;
    append_magic(bytes, "BTDX");
    // ...write header, records, names, and stored payloads deterministically...
    return ba2_dds_fixture{version, compression_method, std::move(textures), std::move(bytes)};
}
```

**Apply to Phase 8:** build harness bytes from source-visible descriptors; include a header, table/index region, data-region table, and payload area. Use real compression routes for compressed harness payloads. Keep parser/assertion helpers test-only.

---

### `tests/public_header_smoke.cpp` (test, request-response / compile smoke)

**Analog:** `tests/public_header_smoke.cpp`

**Public include list pattern** (`tests/public_header_smoke.cpp` lines 1-15):
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

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
```

**Consumer-style public API exercise pattern** (`tests/public_header_smoke.cpp` lines 17-24, 35-42, 83-96):
```cpp
int main()
{
    libbsa::result<void> ok = libbsa::success();
    const std::array bytes{std::byte{0x10}, std::byte{0x20}};
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    libbsa::memory_sink sink;
    const auto write = sink.write(std::span<const std::byte>{bytes});

    libbsa::payload_codec_request codec_request{};
    codec_request.format = libbsa::archive_format::starfield_ba2_gnrl;
    codec_request.entry_state = libbsa::compression_state::archive_default;
    codec_request.compression_method = 3;
    const auto codec = libbsa::resolve_payload_codec(codec_request);
    const auto write_compression = libbsa::resolve_write_compression(libbsa::archive_format::sse_bsa,
                                                                      libbsa::compression_policy::force_compressed,
                                                                      false);

    auto path = libbsa::normalize_archive_path("textures/actors/hero.dds");
    const libbsa::archive_view view{summary, std::vector{metadata}};
    const libbsa::bsa_archive bsa{summary, std::vector{bsa_metadata}};
    const libbsa::ba2_archive ba2{ba2_summary, std::vector{ba2_metadata}, std::vector{texture}};
    const auto bsa_paths = bsa.paths();
    const auto bsa_entry = bsa.entry("meshes/armor/iron.nif");
    const auto ba2_paths = ba2.paths();
    const auto ba2_entry = ba2.entry("textures/interface/lut.dds");
    const auto ba2_texture = ba2.texture_metadata("textures/interface/lut.dds");
    const auto missing_texture = ba2.texture_metadata("meshes/armor/iron.nif");
    const auto open_bsa_fn = &libbsa::open_bsa;
    const auto extract_bsa_entry_fn = &libbsa::extract_bsa_entry;
    const auto open_ba2_fn = &libbsa::open_ba2;
    const auto extract_ba2_entry_fn = &libbsa::extract_ba2_entry;
```

**Apply to Phase 8:** add `#include <libbsa/writer.hpp>`, construct target/options/entries, call plan/finalize, inspect vectors/total size, and take function pointers for public writer functions. Do not include private helper headers or dependency headers.

---

### `CMakeLists.txt` (config, build graph)

**Analog:** `CMakeLists.txt`

**Library source/header wiring pattern** (`CMakeLists.txt` lines 45-76):
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
    src/texture/dds_reconstruction.cpp
    src/texture/dds_validation.cpp
  PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS include
    FILES
      include/libbsa/result.hpp
      include/libbsa/archive.hpp
      include/libbsa/archive_path.hpp
      include/libbsa/archive_view.hpp
      include/libbsa/ba2.hpp
      include/libbsa/bsa.hpp
      include/libbsa/compression.hpp
      include/libbsa/detect.hpp
      include/libbsa/io.hpp)
```

**Test target pattern** (`CMakeLists.txt` lines 146-152, 186-197):
```cmake
  add_executable(libbsa_compression_policy_tests tests/compression_policy_tests.cpp)
  target_link_libraries(libbsa_compression_policy_tests
    PRIVATE
      libbsa::libbsa
      Catch2::Catch2WithMain)

  catch_discover_tests(libbsa_compression_policy_tests TEST_PREFIX "libbsa_compression_policy_tests." ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;codec")

  add_executable(libbsa_ba2_dds_reader_tests
    tests/ba2_dds_reader_tests.cpp
    tests/ba2_dds_fixture_helpers.cpp)
  target_include_directories(libbsa_ba2_dds_reader_tests
    PRIVATE
      src)
  target_link_libraries(libbsa_ba2_dds_reader_tests
    PRIVATE
      libbsa::libbsa
      Catch2::Catch2WithMain)

  catch_discover_tests(libbsa_ba2_dds_reader_tests TEST_PREFIX "libbsa_ba2_dds_reader_tests." ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;fixture;codec")
```

**Public-header smoke pattern** (`CMakeLists.txt` lines 210-216):
```cmake
  add_executable(libbsa_public_header_smoke tests/public_header_smoke.cpp)
  target_link_libraries(libbsa_public_header_smoke
    PRIVATE
      libbsa::libbsa)

  add_test(NAME libbsa.public_header_smoke COMMAND libbsa_public_header_smoke)
  set_tests_properties(libbsa.public_header_smoke PROPERTIES LABELS smoke)
```

**Apply to Phase 8:** add `src/writer.cpp` under `PRIVATE`, `include/libbsa/writer.hpp` to the public `FILE_SET`, a `libbsa_writer_tests` executable with `tests/writer_core_tests.cpp` and `tests/writer_harness_helpers.cpp`, and `catch_discover_tests(... ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;fixture;codec;roundtrip")`.

---

### `tests/compression_policy_tests.cpp` (test, transform / codec)

**Analog:** `tests/compression_policy_tests.cpp`

**Request helper pattern** (`tests/compression_policy_tests.cpp` lines 10-19):
```cpp
namespace {

libbsa::payload_codec_request request(libbsa::archive_format format,
                                      libbsa::compression_state state,
                                      std::optional<std::uint32_t> compression_method = std::nullopt)
{
    return libbsa::payload_codec_request{format, state, compression_method};
}

} // namespace
```

**Unsupported route failure pattern** (`tests/compression_policy_tests.cpp` lines 48-56):
```cpp
TEST_CASE("unsupported payload routes return structured failures", "[unit][codec]")
{
    const auto routed = libbsa::resolve_payload_codec(
        request(libbsa::archive_format::fo4_ba2_gnrl, libbsa::compression_state::lz4_block));

    REQUIRE_FALSE(routed.has_value());
    CHECK(routed.error().code == libbsa::error_code::unsupported_format);
    CHECK(routed.error().message == "unsupported compression route");
}
```

**Apply to Phase 8:** extend this file only if planner wants policy-only coverage close to the existing codec tests. Writer plan integration can also live in `writer_core_tests.cpp`; avoid duplicate coverage unless it catches direct `resolve_write_compression` behavior.

## Shared Patterns

### Public API boundaries and Doxygen comments
**Source:** `include/libbsa/io.hpp` lines 12-28; `include/libbsa/ba2.hpp` lines 45-58  
**Apply to:** `include/libbsa/writer.hpp`
```cpp
/// Provides caller-owned random-access bytes to archive operations.
///
/// Implementations must report the complete readable extent and reject reads
/// that cannot be satisfied exactly. The source object and backing storage are
/// owned by the caller for the duration of each operation.
class byte_source {
public:
    virtual ~byte_source() = default;

    /// Returns the number of bytes available from this source.
    [[nodiscard]] virtual std::uint64_t size() const noexcept = 0;

    /// Reads exactly `destination.size()` bytes starting at `offset`.
    ///
    /// Returns `error_code::io_failure` when the requested range lies outside
    /// the source extent; successful reads fill the destination span in order.
    [[nodiscard]] virtual result<void> read_at(std::uint64_t offset, std::span<std::byte> destination) const = 0;
};
```

### Structured error/result propagation
**Source:** `include/libbsa/result.hpp` lines 11-20, 150-160  
**Apply to:** all writer planning/finalization code and tests
```cpp
enum class error_code {
    unsupported_format,
    malformed_archive,
    io_failure,
    decompression_failure,
};

template <class T>
result<T> failure(libbsa::error err)
{
    return result<T>(std::move(err));
}

inline result<void> failure(libbsa::error err)
{
    return result<void>(std::move(err));
}
```

### Archive path normalization and duplicate rejection before map insertion
**Source:** `include/libbsa/archive_path.hpp` lines 38-43; `src/ba2_reader.cpp` lines 306-315  
**Apply to:** `src/writer.cpp` planning validation
```cpp
[[nodiscard]] result<archive_path> normalize_archive_path(std::string input);

auto normalized = normalize_archive_path(names.value().names[i]);
if (!normalized.has_value()) {
    return failure<ba2_archive>({error_code::malformed_archive, "invalid BA2 name"});
}
metadata.path = normalized.value().string();
// BA2 name tables associate names by record index; reject duplicate normalized
// keys before archive_view::insert_or_assign can overwrite and hide a bad table.
if (std::find(normalized_paths.begin(), normalized_paths.end(), metadata.path) != normalized_paths.end()) {
    return failure<ba2_archive>({error_code::malformed_archive, "duplicate BA2 name"});
}
```

### Checked arithmetic for offsets and sizes
**Source:** `src/ba2_reader.cpp` lines 39-52  
**Apply to:** `src/writer.cpp` layout cursor, table sizes, total planned size
```cpp
bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t& result) noexcept
{
    if (left > std::numeric_limits<std::uint64_t>::max() - right) {
        return false;
    }
    result = left + right;
    return true;
}
```

### Explicit compression routing, no fallback inference
**Source:** `src/compression.cpp` lines 45-82, 131-145  
**Apply to:** writer planning and harness compression tests
```cpp
result<compression_algorithm> resolve_payload_codec(const payload_codec_request& request)
{
    switch (request.entry_state) {
    case compression_state::none:
    case compression_state::raw:
        return success(compression_algorithm::none);
    case compression_state::deflate:
        if (supports_deflate(request.format)) {
            return success(compression_algorithm::deflate);
        }
        return unsupported_route();
    case compression_state::lz4_frame:
        if (request.format == archive_format::sse_bsa) {
            return success(compression_algorithm::lz4_frame);
        }
        return unsupported_route();
    case compression_state::lz4_block:
        if (is_starfield_ba2(request.format) && request.compression_method == 3) {
            return success(compression_algorithm::lz4_block);
        }
        return unsupported_route();
    // ...
    }
}
```

### Sink failure propagation
**Source:** `include/libbsa/io.hpp` lines 31-40; `src/bsa_reader.cpp` lines 518-523  
**Apply to:** `finalize_archive_write` and failing sink tests
```cpp
const auto packed = std::span<const std::byte>{payload.value()}.subspan(cursor);
auto unpacked = decompress_payload(algorithm.value(), packed, expected_size);
if (!unpacked.has_value()) {
    return failure<void>(unpacked.error());
}
return sink.write(std::span<const std::byte>{unpacked.value()});
```

### Generated fixture helpers stay test-only
**Source:** `tests/ba2_dds_fixture_helpers.hpp` lines 10-12, 47-50; `tests/ba2_dds_reader_tests.cpp` lines 6-7  
**Apply to:** `tests/writer_harness_helpers.hpp/.cpp`, `tests/writer_core_tests.cpp`
```cpp
namespace libbsa::test {

/// Builds BA2 DX10 bytes for caller-supplied texture descriptors.
[[nodiscard]] ba2_dds_fixture make_ba2_dds_fixture(std::uint32_t version,
                                                   std::vector<ba2_dds_texture_descriptor> textures,
                                                   std::uint32_t compression_method = 0);
```

## No Analog Found

All planned files have usable analogs. There is no exact production writer analog yet, so `src/writer.cpp` should combine the existing operation-style API, path normalization, checked arithmetic, compression dispatch, and sink emission patterns rather than copying a single file wholesale.

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| — | — | — | No unmatched files. |

## Metadata

**Analog search scope:** `include/libbsa/*.hpp`, `src/**/*.cpp`, `tests/**/*.cpp`, `tests/**/*.hpp`, `CMakeLists.txt`  
**Files scanned:** 27 candidates via glob; 18 files read including phase context/spec/research and project instructions  
**Project skills:** No `.claude/skills/` or `.agents/skills/` project skills found  
**Pattern extraction date:** 2026-05-06
