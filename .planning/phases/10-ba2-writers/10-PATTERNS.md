# Phase 10: ba2-writers - Pattern Map

**Mapped:** 2026-05-07  
**Files analyzed:** 8  
**Analogs found:** 8 / 8

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `include/libbsa/ba2_writer.hpp` | public API/header | request-response + batch | `include/libbsa/bsa_writer.hpp` | exact |
| `src/ba2_writer.cpp` | service/serializer | batch + file-I/O + transform | `src/bsa_writer.cpp`; `src/ba2_reader.cpp` | exact |
| `src/texture/dds_analysis.hpp` | private utility/header | transform | `src/texture/dds_validation.cpp` boundary + `include/libbsa/ba2.hpp` value types | role-match |
| `src/texture/dds_analysis.cpp` | private utility/service | transform + validation | `src/texture/dds_validation.cpp`; `tests/ba2_dds_fixture_helpers.cpp` | role-match |
| `tests/ba2_writer_tests.cpp` | test | batch + file-I/O + request-response | `tests/bsa_writer_tests.cpp`; `tests/ba2_dds_fixture_helpers.cpp` | exact |
| `tests/public_header_smoke.cpp` | smoke test | request-response | `tests/public_header_smoke.cpp` | exact |
| `CMakeLists.txt` | build config | batch | `CMakeLists.txt` existing source/test wiring | exact |
| `src/texture/dds_reconstruction.cpp` (possible format support extension) | private utility | transform + validation | `src/texture/dds_validation.cpp`; `src/ba2_reader.cpp` DDS extraction path | role-match |

## Pattern Assignments

### `include/libbsa/ba2_writer.hpp` (public API/header, request-response + batch)

**Analog:** `include/libbsa/bsa_writer.hpp`

**Imports pattern** (lines 3-12):
```cpp
#include <libbsa/archive.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
```

**Exact target enum pattern** (lines 16-25):
```cpp
/// Identifies the native BSA archive variant requested for writer planning.
///
/// Callers choose the target explicitly so planning never infers archive layout,
/// compression defaults, or table widths from filenames or host paths.
enum class bsa_write_target {
    tes3_morrowind,
    oblivion_v103,
    fo3_fnv_skyrim_le_v104,
    skyrim_se_ae_v105,
};
```

**Separate memory/disk input pattern** (lines 38-57):
```cpp
/// Carries one memory-backed BSA input entry.
///
/// `path` is an archive-virtual path normalized during planning. Payload bytes are
/// copied into the returned plan after compression and layout decisions are made.
struct bsa_memory_entry {
    std::string path;
    std::vector<std::byte> payload;
    compression_policy compression{compression_policy::archive_default};
};

/// Carries one disk-backed BSA input entry.
///
/// `host_path` names the caller-selected file to read during planning, while
/// `path` is the archive-virtual path stored in the BSA. Keeping these fields
/// separate prevents invalid memory/disk entry combinations in public code.
struct bsa_disk_entry {
    std::string host_path;
    std::string path;
    compression_policy compression{compression_policy::archive_default};
};
```

**Plan-owned bytes/finalize API pattern** (lines 102-142):
```cpp
/// Owns a deterministic native BSA write preview and all bytes needed to emit it.
///
/// Planning owns table, payload, compression, and dedup decisions. Finalization
/// only streams already planned bytes, so no caller-owned input files or sinks are
/// retained after public writer calls return.
struct bsa_write_plan {
    bsa_write_target target{};
    bsa_write_options options{};
    std::uint32_t flags{};
    /// Planned native header and metadata table bytes emitted before payloads.
    ///
    /// Finalization writes these bytes verbatim so compatibility-critical layout
    /// decisions made during planning are not recomputed against mutable inputs.
    std::vector<std::byte> table_bytes;
    std::vector<planned_bsa_table_region> table_regions;
    std::vector<planned_bsa_data_region> data_regions;
    std::vector<planned_bsa_entry> entries;
    std::uint64_t total_size{};
};

[[nodiscard]] result<bsa_write_plan> plan_bsa_write(bsa_write_target target,
                                                   std::span<const bsa_memory_entry> entries,
                                                   bsa_write_options options = {});

[[nodiscard]] result<void> finalize_bsa_write(const bsa_write_plan& plan, byte_sink& sink);
```

**Apply to BA2:** copy this public-header style: Doxygen on all public types/functions, exact `ba2_write_target`, separate GNRL/DDS and memory/disk entry values, subtype-specific preview structs, `result<ba2_write_plan>` planning, and `result<void> finalize_ba2_write(const ba2_write_plan&, byte_sink&)`. Keep DirectXTex/LZ4/libdeflate headers out.

---

### `src/ba2_writer.cpp` (service/serializer, batch + file-I/O + transform)

**Analogs:** `src/bsa_writer.cpp` for writer planning/finalization, `src/ba2_reader.cpp` for BA2 native field contract.

**Implementation imports pattern** (`src/bsa_writer.cpp` lines 1-18):
```cpp
#include <libbsa/bsa_writer.hpp>

#include <libbsa/archive_path.hpp>
#include <libbsa/compression.hpp>

#include "bsa_reader.hpp"
#include "hash.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
```

**Checked arithmetic + structured error pattern** (`src/bsa_writer.cpp` lines 63-92):
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

result<std::uint32_t> checked_u32(std::uint64_t value)
{
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return failure<std::uint32_t>(writer_layout_overflow());
    }
    return success(static_cast<std::uint32_t>(value));
}
```

**Path normalization + duplicate rejection pattern** (`src/bsa_writer.cpp` lines 181-207):
```cpp
result<std::vector<normalized_memory_entry>> normalize_entries(std::span<const bsa_memory_entry> entries)
{
    std::vector<normalized_memory_entry> normalized_entries;
    normalized_entries.reserve(entries.size());
    std::vector<std::string> paths;
    paths.reserve(entries.size());

    for (const auto& entry : entries) {
        auto normalized = normalize_archive_path(entry.path);
        if (!normalized.has_value()) {
            return failure<std::vector<normalized_memory_entry>>(normalized.error());
        }
        auto path = normalized.value().string();
        if (std::find(paths.begin(), paths.end(), path) != paths.end()) {
            return failure<std::vector<normalized_memory_entry>>({error_code::malformed_archive, "duplicate BSA writer path"});
        }
        const auto slash = path.find_last_of('/');
        if (slash == std::string::npos || slash == 0 || slash + 1U >= path.size()) {
            return failure<std::vector<normalized_memory_entry>>({error_code::malformed_archive, "BSA writer path requires folder and file"});
        }
        paths.push_back(path);
        normalized_entries.push_back(normalized_memory_entry{std::move(path), paths.back().substr(0, slash), paths.back().substr(slash + 1U),
                                                            entry.payload, entry.compression});
    }

    return success(std::move(normalized_entries));
}
```

**Compression dispatch pattern** (`src/bsa_writer.cpp` lines 256-297):
```cpp
result<planned_tes4_entry> plan_tes4_entry(const normalized_memory_entry& entry,
                                           bsa_write_target target,
                                           const bsa_write_options& options)
{
    const auto format = archive_format_for(target);
    auto compression = resolve_write_compression(format, entry.compression, options.archive_default_compressed);
    if (!compression.has_value()) {
        return failure<planned_tes4_entry>(compression.error());
    }

    std::vector<std::byte> native_payload;
    if (compression.value() == compression_state::raw) {
        native_payload.insert(native_payload.end(), entry.payload.begin(), entry.payload.end());
    } else {
        auto unpacked_size = checked_u32(entry.payload.size());
        if (!unpacked_size.has_value()) {
            return failure<planned_tes4_entry>(unpacked_size.error());
        }
        payload_codec_request request{};
        request.format = format;
        request.entry_state = compression.value();
        auto algorithm = resolve_payload_codec(request);
        if (!algorithm.has_value()) {
            return failure<planned_tes4_entry>(algorithm.error());
        }
        auto compressed = compress_payload(algorithm.value(), std::span<const std::byte>{entry.payload});
        if (!compressed.has_value()) {
            return failure<planned_tes4_entry>(compressed.error());
        }
        append_u32(native_payload, unpacked_size.value());
        native_payload.insert(native_payload.end(), compressed.value().begin(), compressed.value().end());
    }
```

**Dedup + data region assignment pattern** (`src/bsa_writer.cpp` lines 415-443):
```cpp
for (auto& folder : folders) {
    for (auto& entry : folder.entries) {
        const auto shared = options.deduplicate
            ? std::find_if(plan.data_regions.begin(), plan.data_regions.end(), [&entry](const auto& region) {
                  return region.stored_payload == entry.stored_payload;
              })
            : plan.data_regions.end();

        if (shared != plan.data_regions.end()) {
            auto payload_offset = checked_u32(shared->offset);
            if (!payload_offset.has_value()) {
                return failure<bsa_write_plan>(payload_offset.error());
            }
            entry.payload_offset = payload_offset.value();
            entry.data_region_id = shared->id;
        } else {
            auto payload_offset = checked_u32(payload_cursor);
            if (!payload_offset.has_value()) {
                return failure<bsa_write_plan>(payload_offset.error());
            }
            entry.payload_offset = payload_offset.value();
            entry.data_region_id = static_cast<std::uint32_t>(plan.data_regions.size());
            plan.data_regions.push_back(planned_bsa_data_region{entry.data_region_id, payload_cursor, entry.stored_payload.size(), entry.unpacked_size,
                                                                entry.compression, entry.stored_payload});
            if (!checked_add(payload_cursor, entry.stored_payload.size(), payload_cursor)) {
                return failure<bsa_write_plan>(writer_layout_overflow());
            }
        }
    }
}
```

**Disk planning reads now; finalization owns bytes pattern** (`src/bsa_writer.cpp` lines 715-742):
```cpp
result<bsa_write_plan> plan_bsa_write_from_disk(bsa_write_target target,
                                                std::span<const bsa_disk_entry> entries,
                                                bsa_write_options options)
{
    auto paths_validated = validate_disk_entry_archive_paths(entries);
    if (!paths_validated.has_value()) {
        return failure<bsa_write_plan>(paths_validated.error());
    }

    std::vector<bsa_memory_entry> memory_entries;
    memory_entries.reserve(entries.size());
    for (const auto& entry : entries) {
        std::ifstream stream{entry.host_path, std::ios::binary};
        if (!stream) {
            return failure<bsa_write_plan>({error_code::io_failure, "failed to open BSA writer input file"});
        }
        std::vector<std::byte> payload;
        char ch = 0;
        while (stream.get(ch)) {
            payload.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
        }
        if (!stream.eof()) {
            return failure<bsa_write_plan>({error_code::io_failure, "failed to read BSA writer input file"});
        }
        memory_entries.push_back(bsa_memory_entry{entry.path, std::move(payload), entry.compression});
    }
    return plan_bsa_write(target, std::span<const bsa_memory_entry>{memory_entries}, options);
}
```

**Finalizer pattern** (`src/bsa_writer.cpp` lines 744-757):
```cpp
result<void> finalize_bsa_write(const bsa_write_plan& plan, byte_sink& sink)
{
    auto written = write_chunk(sink, std::span<const std::byte>{plan.table_bytes});
    if (!written.has_value()) {
        return failure<void>(written.error());
    }
    for (const auto& region : plan.data_regions) {
        written = write_chunk(sink, std::span<const std::byte>{region.stored_payload});
        if (!written.has_value()) {
            return failure<void>(written.error());
        }
    }
    return success();
}
```

**BA2 GNRL reader contract to serialize against** (`src/ba2_reader.cpp` lines 19-33, 246-273):
```cpp
constexpr std::uint32_t magic_btdx = 0x58445442U;
constexpr std::uint32_t magic_gnrl = 0x4c524e47U;
constexpr std::uint32_t magic_dx10 = 0x30315844U;
constexpr std::uint32_t version_fo4_v1 = 0x01U;
constexpr std::uint32_t version_starfield_v2 = 0x02U;
constexpr std::uint32_t version_starfield_v3 = 0x03U;
constexpr std::uint32_t version_fo4_v7 = 0x07U;
constexpr std::uint32_t version_fo4_v8 = 0x08U;
constexpr std::uint64_t base_header_size = 24U;
constexpr std::uint64_t starfield_v2_header_size = 32U;
constexpr std::uint64_t starfield_v3_header_size = 36U;
constexpr std::uint64_t record_size = 36U;
```

```cpp
const auto file_count = le_u32(header.value(), 12);
const auto file_table_offset = le_u64(header.value(), 16);
const auto compression_method = version == version_starfield_v3 ? le_u32(header.value(), 32) : 0U;
...
ba2_record record{};
record.name_hash = le_u32(record_bytes.value(), 0);
record.directory_hash = le_u32(record_bytes.value(), 8);
record.offset = le_u64(record_bytes.value(), 16);
record.packed_size = le_u32(record_bytes.value(), 24);
record.size = le_u32(record_bytes.value(), 28);
```

**BA2 DX10 reader contract to serialize against** (`src/ba2_reader.cpp` lines 387-427):
```cpp
ba2_texture_record record{};
record.name_hash = le_u32(record_bytes.value(), 0);
record.directory_hash = le_u32(record_bytes.value(), 8);
const auto chunk_count = std::to_integer<unsigned char>(record_bytes.value()[13]);
const auto chunk_header_size = le_u16(record_bytes.value(), 14);
record.height = le_u16(record_bytes.value(), 16);
record.width = le_u16(record_bytes.value(), 18);
record.mip_count = std::to_integer<unsigned char>(record_bytes.value()[20]);
record.dxgi_format = std::to_integer<unsigned char>(record_bytes.value()[21]);
record.cube_maps = le_u16(record_bytes.value(), 22);
...
chunk.offset = le_u64(chunk_bytes.value(), 0);
chunk.packed_size = le_u32(chunk_bytes.value(), 8);
chunk.size = le_u32(chunk_bytes.value(), 12);
chunk.start_mip = le_u16(chunk_bytes.value(), 16);
chunk.end_mip = le_u16(chunk_bytes.value(), 18);
```

---

### `src/texture/dds_analysis.hpp` / `src/texture/dds_analysis.cpp` (private utility, transform + validation)

**Analogs:** `src/texture/dds_validation.cpp`, `include/libbsa/ba2.hpp`, `tests/ba2_dds_fixture_helpers.cpp`.

**Private DirectXTex boundary pattern** (`src/texture/dds_validation.cpp` lines 1-6, 20-39):
```cpp
#include "dds_validation.hpp"

#include <DirectXTex.h>

#include <limits>
```

```cpp
result<dds_validation_metadata> validate_dds(std::span<const std::byte> dds_bytes)
{
    DirectX::TexMetadata metadata{};
    DirectX::ScratchImage image{};
    // DirectXTex validation stays in this private translation unit so public
    // libbsa headers never inherit Windows, DXGI, or texture-library types.
    const auto hr = DirectX::LoadFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
    if (hr < 0) {
        return failure<dds_validation_metadata>({error_code::malformed_archive, "DDS validation failed"});
    }

    dds_validation_metadata result{};
    result.width = checked_metadata_size(metadata.width);
    result.height = checked_metadata_size(metadata.height);
    result.mip_count = checked_metadata_size(metadata.mipLevels);
    result.array_size = checked_metadata_size(metadata.arraySize);
    result.format = dxgi_format{static_cast<std::uint32_t>(metadata.format)};
    result.is_cubemap = metadata.IsCubemap();
    return success(result);
}
```

**Public value-type translation target** (`include/libbsa/ba2.hpp` lines 17-58):
```cpp
struct dxgi_format {
    std::uint32_t value{};
};

struct texture_chunk_metadata {
    std::uint32_t mip_level{};
    std::uint64_t offset{};
    std::uint64_t packed_size{};
    std::uint64_t size{};
    compression_state compression{compression_state::unknown};
};

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

**DX10 chunk payload shape to produce** (`tests/ba2_dds_fixture_helpers.cpp` lines 167-189):
```cpp
for (const auto& texture : textures) {
    append_u32(bytes, texture.name_hash);
    append_magic(bytes, ".dds");
    append_u32(bytes, texture.directory_hash);
    bytes.push_back(std::byte{0});
    bytes.push_back(static_cast<std::byte>(texture.chunks.size()));
    append_u16(bytes, dx10_chunk_header_size);
    append_u16(bytes, texture.height);
    append_u16(bytes, texture.width);
    bytes.push_back(static_cast<std::byte>(texture.mip_count));
    bytes.push_back(static_cast<std::byte>(texture.dxgi_format));
    append_u16(bytes, texture.cubemap ? 6U : texture.array_size);
    for (const auto& chunk_descriptor : texture.chunks) {
        const auto& stored = stored_chunks[chunk_index];
        append_u64(bytes, chunk_offsets[chunk_index]);
        append_u32(bytes, static_cast<std::uint32_t>(stored.size()));
        append_u32(bytes, static_cast<std::uint32_t>(chunk_descriptor.payload.size()));
        append_u16(bytes, chunk_descriptor.start_mip);
        append_u16(bytes, chunk_descriptor.end_mip);
        append_u32(bytes, chunk_tail);
        ++chunk_index;
    }
}
```

**Apply to DDS analyzer:** keep `DirectXTex.h` in `src/texture/dds_analysis.cpp` only; return private libbsa-owned structs containing metadata and plan-owned mip/chunk payload bytes. Use `result<T>` failures for malformed/unsupported DDS and integer narrowing. Do not expose caller-provided native BA2 descriptors.

---

### `tests/ba2_writer_tests.cpp` (test, batch + file-I/O + request-response)

**Analogs:** `tests/bsa_writer_tests.cpp` and `tests/ba2_dds_fixture_helpers.cpp`.

**Imports/helpers pattern** (`tests/bsa_writer_tests.cpp` lines 1-15, 121-147):
```cpp
#include <catch2/catch_test_macros.hpp>

#include <libbsa/bsa.hpp>
#include <libbsa/bsa_writer.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>
```

```cpp
std::vector<std::byte> finalize_to_bytes(const libbsa::bsa_write_plan& plan)
{
    libbsa::memory_sink sink;
    const auto finalized = libbsa::finalize_bsa_write(plan, sink);
    REQUIRE(finalized.has_value());
    return sink.bytes();
}

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

**Round-trip/native layout assertion pattern** (`tests/bsa_writer_tests.cpp` lines 292-331):
```cpp
TEST_CASE("plans and finalizes TES4-family raw BSA archives for every required version", "[unit][bsa-writer][roundtrip]")
{
    const auto entries = tes4_family_entries();

    for (const auto target : {libbsa::bsa_write_target::oblivion_v103,
                              libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                              libbsa::bsa_write_target::skyrim_se_ae_v105}) {
        const auto plan = libbsa::plan_bsa_write(target, std::span<const libbsa::bsa_memory_entry>{entries});
        REQUIRE(plan.has_value());

        const auto bytes = finalize_to_bytes(plan.value());
        REQUIRE(bytes.size() == plan.value().total_size);
        CHECK(le_u32(bytes, 0) == bsa_magic);
        CHECK(le_u32(bytes, 4) == expected_version(target));

        const libbsa::memory_source source{std::span<const std::byte>{bytes}};
        const auto archive = libbsa::open_bsa(source);
        REQUIRE(archive.has_value());
        CHECK(archive.value().summary().version == expected_version(target));

        for (const auto& entry : entries) {
            const auto metadata = archive.value().entry(entry.path);
            REQUIRE(metadata.has_value());
            const auto& planned = find_planned_entry(plan.value(), entry.path);
            CHECK(metadata.value().offset == planned.offset);
            CHECK(metadata.value().stored_size == planned.stored_size);
            CHECK(extract_bytes(bytes, entry.path) == entry.payload);
        }
    }
}
```

**Disk planning/no-reopen pattern** (`tests/bsa_writer_tests.cpp` lines 644-681):
```cpp
TEST_CASE("disk-backed and memory-backed BSA inputs read back equivalently", "[unit][bsa-writer][disk][roundtrip]")
{
    const std::vector memory_entries{bsa_entry("meshes/from-disk.nif", ascii_bytes("disk mesh"), libbsa::compression_policy::force_raw),
                                     bsa_entry("textures/from-disk.dds", ascii_bytes("disk texture"), libbsa::compression_policy::force_raw)};
    const auto mesh_path = unique_temp_file("mesh");
    const auto texture_path = unique_temp_file("texture");
    write_temp_file(mesh_path, memory_entries[0].payload);
    write_temp_file(texture_path, memory_entries[1].payload);
    const std::vector disk_entries{disk_entry(mesh_path, memory_entries[0].path, memory_entries[0].compression),
                                   disk_entry(texture_path, memory_entries[1].path, memory_entries[1].compression)};

    const auto memory_plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                                   std::span<const libbsa::bsa_memory_entry>{memory_entries});
    const auto disk_plan = libbsa::plan_bsa_write_from_disk(libbsa::bsa_write_target::fo3_fnv_skyrim_le_v104,
                                                           std::span<const libbsa::bsa_disk_entry>{disk_entries});

    REQUIRE(memory_plan.has_value());
    REQUIRE(disk_plan.has_value());
    require_archives_equivalent(finalize_to_bytes(memory_plan.value()), finalize_to_bytes(disk_plan.value()), memory_entries);
}
```

**Failure/no sink write pattern** (`tests/bsa_writer_tests.cpp` lines 683-725):
```cpp
TEST_CASE("duplicate normalized BSA paths fail during planning", "[unit][bsa-writer][failure]")
{
    const std::vector entries{bsa_entry("Meshes/Armor/Iron.NIF", ascii_bytes("first")),
                              bsa_entry("meshes\\armor\\iron.nif", ascii_bytes("second"))};
    libbsa::memory_sink sink;

    const auto plan = libbsa::plan_bsa_write(libbsa::bsa_write_target::skyrim_se_ae_v105,
                                             std::span<const libbsa::bsa_memory_entry>{entries});

    REQUIRE_FALSE(plan.has_value());
    CHECK(plan.error().code == libbsa::error_code::malformed_archive);
    CHECK(sink.bytes().empty());
}
```

**DDS fixture builder style** (`tests/ba2_dds_fixture_helpers.cpp` lines 127-197): use source-built fixtures with little-endian helper functions, generated name table, computed offsets, compressed stored chunks, and semantic assertions rather than checked-in binary blobs.

---

### `tests/public_header_smoke.cpp` (smoke test, request-response)

**Analog:** existing `tests/public_header_smoke.cpp`.

**Include list pattern** (lines 1-12):
```cpp
#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/ba2.hpp>
#include <libbsa/bsa.hpp>
#include <libbsa/bsa_writer.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/detect.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>
#include <libbsa/writer.hpp>
```

**Consumer-style writer helper pattern** (lines 22-50):
```cpp
bool can_write_and_reopen_bsa(libbsa::bsa_write_target target, const std::string& path)
{
    libbsa::bsa_memory_entry entry{};
    entry.path = path;
    entry.payload = {std::byte{0x41}, std::byte{0x42}, std::byte{0x43}};
    entry.compression = libbsa::compression_policy::force_raw;
    std::vector entries{entry};

    const auto plan = libbsa::plan_bsa_write(target, std::span<const libbsa::bsa_memory_entry>{entries});
    if (!plan.has_value()) {
        return false;
    }

    libbsa::memory_sink archive_sink;
    const auto finalized = libbsa::finalize_bsa_write(plan.value(), archive_sink);
    if (!finalized.has_value()) {
        return false;
    }

    const libbsa::memory_source archive_source{std::span<const std::byte>{archive_sink.bytes()}};
    const auto archive = libbsa::open_bsa(archive_source);
    if (!archive.has_value() || !archive.value().contains(path)) {
        return false;
    }

    libbsa::memory_sink extracted;
    const auto extracted_result = libbsa::extract_bsa_entry(archive.value(), archive_source, path, extracted);
    return extracted_result.has_value() && extracted.bytes() == entry.payload;
}
```

**Function pointer/return gate pattern** (lines 156-183):
```cpp
const auto plan_writer_fn = &libbsa::plan_archive_write;
const auto finalize_writer_fn = &libbsa::finalize_archive_write;
const auto plan_bsa_write_fn = &libbsa::plan_bsa_write;
const auto plan_bsa_write_from_disk_fn = &libbsa::plan_bsa_write_from_disk;
const auto finalize_bsa_write_fn = &libbsa::finalize_bsa_write;
...
return ok.has_value() && source.size() == 2 && write.has_value() && sink.bytes().size() == 2 && codec.has_value() &&
        ...
        plan_writer_fn != nullptr && finalize_writer_fn != nullptr && plan_bsa_write_fn != nullptr &&
        plan_bsa_write_from_disk_fn != nullptr && finalize_bsa_write_fn != nullptr && bsa_writer_smoke_ok
    ? 0
    : 1;
```

**Apply to BA2:** add `#include <libbsa/ba2_writer.hpp>`, consumer-style GNRL and DDS memory entry construction, plan/finalize/reopen/extract checks, and function-pointer references for all public BA2 writer entry points. This file must compile without private headers or dependency tokens.

---

### `CMakeLists.txt` (build config, batch)

**Analog:** existing explicit source/header/test wiring in `CMakeLists.txt`.

**Library dependency and target setup pattern** (lines 9-12, 40-47):
```cmake
find_package(libdeflate CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(DirectXTex CONFIG REQUIRED)

include(CTest)
```

```cmake
add_library(libbsa)
add_library(libbsa::libbsa ALIAS libbsa)

target_compile_features(libbsa PUBLIC cxx_std_20)

# TES5Edit/ is read-only reference material; do not compile, link, vendor,
# or add it to libbsa source lists.
target_sources(libbsa
```

**Explicit source/header wiring pattern** (lines 47-80):
```cmake
target_sources(libbsa
  PRIVATE
    src/libbsa.cpp
    src/detect.cpp
    src/archive_path.cpp
    src/archive_view.cpp
    src/ba2_reader.cpp
    src/bsa_reader.cpp
    src/bsa_writer.cpp
    src/compression.cpp
    src/compression/deflate_codec.cpp
    src/compression/lz4_block_codec.cpp
    src/compression/lz4_frame_codec.cpp
    src/hash.cpp
    src/io.cpp
    src/writer.cpp
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
      include/libbsa/bsa_writer.hpp
      include/libbsa/compression.hpp
      include/libbsa/detect.hpp
      include/libbsa/io.hpp
      include/libbsa/writer.hpp)
```

**Test target pattern** (lines 182-209):
```cmake
add_executable(libbsa_bsa_writer_tests tests/bsa_writer_tests.cpp)
target_link_libraries(libbsa_bsa_writer_tests
  PRIVATE
    libbsa::libbsa
    Catch2::Catch2WithMain)

catch_discover_tests(libbsa_bsa_writer_tests TEST_PREFIX "libbsa_bsa_writer_tests." ADD_TAGS_AS_LABELS PROPERTIES LABELS "unit;fixture;codec;roundtrip")

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

**Apply to BA2:** add `src/ba2_writer.cpp`, `src/texture/dds_analysis.cpp`, public `include/libbsa/ba2_writer.hpp`, and a `libbsa_ba2_writer_tests` target. Add `target_include_directories(... PRIVATE src)` if tests include private DDS helpers. Do not add any `TES5Edit/` path.

---

### `src/texture/dds_reconstruction.cpp` (possible format support extension, transform + validation)

**Analogs:** `src/ba2_reader.cpp` DDS extraction and `src/texture/dds_validation.cpp`.

**No-partial DDS extraction/validation pattern** (`src/ba2_reader.cpp` lines 638-681):
```cpp
result<void> extract_ba2_texture_entry(const ba2_archive& archive, const byte_source& source, std::string path, byte_sink& sink)
{
    auto texture = archive.texture_metadata(std::move(path));
    if (!texture.has_value()) {
        return failure<void>(texture.error());
    }

    std::vector<std::byte> image_payload;
    for (const auto& chunk : texture.value().chunks) {
        auto payload = read_bytes(source, chunk.offset, chunk.packed_size);
        if (!payload.has_value()) {
            return failure<void>({error_code::malformed_archive, "BA2 DX10 chunk range exceeds source size"});
        }
        ...
        image_payload.insert(image_payload.end(), decompressed.value().begin(), decompressed.value().end());
    }

    auto dds = detail::reconstruct_dds(texture.value(), std::span<const std::byte>{image_payload});
    if (!dds.has_value()) {
        return failure<void>(dds.error());
    }
    auto validated = detail::validate_dds(std::span<const std::byte>{dds.value()});
    if (!validated.has_value()) {
        return failure<void>(validated.error());
    }

    // The sink is intentionally touched only after all chunk decoding,
    // reconstruction, and validation succeeds, preserving no-partial-write behavior.
    return sink.write(std::span<const std::byte>{dds.value()});
}
```

**Apply to reconstruction support:** if Phase 10 needs more DDS formats than current reconstruction supports, preserve this no-partial-output contract and return `result<T>` structured failures before sink writes.

## Shared Patterns

### Public headers expose libbsa-owned types only
**Source:** `include/libbsa/ba2.hpp` lines 17-58 and `src/texture/dds_validation.cpp` lines 20-27  
**Apply to:** `include/libbsa/ba2_writer.hpp`, public smoke tests
```cpp
/// Libbsa-owned representation of a DDS DXGI format value.
struct dxgi_format {
    std::uint32_t value{};
};
```
```cpp
// DirectXTex validation stays in this private translation unit so public
// libbsa headers never inherit Windows, DXGI, or texture-library types.
const auto hr = DirectX::LoadFromDDSMemory(dds_bytes.data(), dds_bytes.size(), DirectX::DDS_FLAGS_NONE, &metadata, image);
```

### Structured `result<T>` error propagation
**Source:** `src/bsa_writer.cpp` lines 327-341, 744-757  
**Apply to:** all BA2 planning/finalization functions
```cpp
auto normalized = normalize_entries(entries);
if (!normalized.has_value()) {
    return failure<bsa_write_plan>(normalized.error());
}
...
auto written = write_chunk(sink, std::span<const std::byte>{plan.table_bytes});
if (!written.has_value()) {
    return failure<void>(written.error());
}
```

### Archive path normalization before file I/O
**Source:** `src/bsa_writer.cpp` lines 209-232 and 719-741  
**Apply to:** BA2 GNRL/DDS disk planning
```cpp
auto paths_validated = validate_disk_entry_archive_paths(entries);
if (!paths_validated.has_value()) {
    return failure<bsa_write_plan>(paths_validated.error());
}
...
std::ifstream stream{entry.host_path, std::ios::binary};
if (!stream) {
    return failure<bsa_write_plan>({error_code::io_failure, "failed to open BSA writer input file"});
}
```

### Compression routing stays explicit and private
**Source:** `src/compression.cpp` lines 45-82, 84-107, 131-145  
**Apply to:** BA2 GNRL payloads and DDS chunks
```cpp
result<compression_algorithm> resolve_payload_codec(const payload_codec_request& request)
{
    switch (request.entry_state) {
    case compression_state::raw:
        return success(compression_algorithm::none);
    case compression_state::deflate:
        if (supports_deflate(request.format)) {
            return success(compression_algorithm::deflate);
        }
        return unsupported_route();
    case compression_state::lz4_block:
        if (is_starfield_ba2(request.format) && request.compression_method == 3) {
            return success(compression_algorithm::lz4_block);
        }
        return unsupported_route();
    ...
    }
}
```

### BA2 raw-size semantics differ by subtype
**Source:** `src/ba2_reader.cpp` lines 122-155, 217-220  
**Apply to:** `src/ba2_writer.cpp`
```cpp
compression_state compression_for_record(std::uint32_t version, std::uint32_t compression_method, std::uint32_t packed_size) noexcept
{
    if (packed_size == 0) {
        return compression_state::raw;
    }
    ...
}

compression_state compression_for_dx10_chunk(std::uint32_t version,
                                             std::uint32_t compression_method,
                                             std::uint32_t packed_size,
                                             std::uint32_t size) noexcept
{
    if (packed_size == size) {
        return compression_state::raw;
    }
    ...
}

std::uint64_t stored_size_for_record(const ba2_record& record) noexcept
{
    return record.packed_size == 0 ? record.size : record.packed_size;
}
```

### Name table must end at first payload offset
**Source:** `src/ba2_reader.cpp` lines 285-297 and 445-454  
**Apply to:** GNRL and DX10 layout planning
```cpp
if (!records.empty()) {
    auto first_payload_offset = records.front().offset;
    for (const auto& record : records) {
        if (record.offset < first_payload_offset) {
            first_payload_offset = record.offset;
        }
    }
    if (names.value().end_offset != first_payload_offset) {
        return failure<ba2_archive>({error_code::malformed_archive, "truncated BA2 table"});
    }
}
```

### Tests prove generated bytes through readers/extractors
**Source:** `tests/bsa_writer_tests.cpp` lines 313-330 and `src/ba2_reader.cpp` lines 684-721  
**Apply to:** `tests/ba2_writer_tests.cpp`
```cpp
const libbsa::memory_source source{std::span<const std::byte>{bytes}};
const auto archive = libbsa::open_bsa(source);
REQUIRE(archive.has_value());
...
CHECK(extract_bytes(bytes, normalized) == entry.payload);
```

## No Analog Found

All proposed files have close analogs. The weakest match is `src/texture/dds_analysis.*` because the codebase currently validates/reconstructs DDS data but does not yet analyze caller-supplied DDS bytes into writer-owned mip/chunk payload plans. Use `src/texture/dds_validation.cpp` for the DirectXTex boundary and `tests/ba2_dds_fixture_helpers.cpp` / `src/ba2_reader.cpp` for the BA2 DX10 chunk shape.

## Metadata

**Analog search scope:** `include/libbsa/*writer*.hpp`, `src/*writer*.cpp`, `src/texture/dds_*.cpp`, `tests/*writer*_tests.cpp`, `tests/*smoke*.cpp`, canonical BA2 reader/test/build files from context.  
**Files scanned/read:** 12  
**Pattern extraction date:** 2026-05-07
