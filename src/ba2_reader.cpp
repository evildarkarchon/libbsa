#include <libbsa/ba2.hpp>
#include <libbsa/compression.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace libbsa {
namespace {

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
constexpr std::uint64_t dx10_record_prefix_size = 24U;
constexpr std::uint64_t dx10_chunk_size = 24U;

error truncated_table_error()
{
    return {error_code::malformed_archive, "truncated BA2 table"};
}

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

std::uint16_t le_u16(std::span<const std::byte> bytes, std::size_t offset) noexcept
{
    return static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset])) |
           static_cast<std::uint16_t>(std::to_integer<unsigned char>(bytes[offset + 1]) << 8U);
}

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

bool supported_version(std::uint32_t version) noexcept
{
    return version == version_fo4_v1 || version == version_fo4_v7 || version == version_fo4_v8 ||
           version == version_starfield_v2 || version == version_starfield_v3;
}

bool supported_dx10_version(std::uint32_t version) noexcept
{
    return version == version_fo4_v1 || version == version_fo4_v7 || version == version_fo4_v8 ||
           version == version_starfield_v3;
}

std::uint64_t header_size_for(std::uint32_t version) noexcept
{
    if (version == version_starfield_v3) {
        return starfield_v3_header_size;
    }
    return version == version_starfield_v2 ? starfield_v2_header_size : base_header_size;
}

archive_format format_for_version(std::uint32_t version) noexcept
{
    return version == version_starfield_v2 || version == version_starfield_v3 ? archive_format::starfield_ba2_gnrl
                                                                              : archive_format::fo4_ba2_gnrl;
}

archive_format dx10_format_for_version(std::uint32_t version) noexcept
{
    return version == version_starfield_v3 ? archive_format::starfield_ba2_dds : archive_format::fo4_ba2_dds;
}

compression_state compression_for_record(std::uint32_t version, std::uint32_t compression_method, std::uint32_t packed_size) noexcept
{
    if (packed_size == 0) {
        return compression_state::raw;
    }

    // Starfield BA2 v3 method 3 uses raw LZ4 blocks, not LZ4 frames, and raw entries remain raw.
    if (version == version_starfield_v3) {
        if (compression_method == 3) {
            return compression_state::lz4_block;
        }
        if (compression_method != 0) {
            return compression_state::unknown;
        }
    }

    return compression_state::deflate;
}

compression_state compression_for_dx10_chunk(std::uint32_t version,
                                             std::uint32_t compression_method,
                                             std::uint32_t packed_size,
                                             std::uint32_t size) noexcept
{
    if (packed_size == size) {
        return compression_state::raw;
    }

    if (version == version_starfield_v3) {
        return compression_method == 3 ? compression_state::lz4_block : compression_state::unknown;
    }

    return compression_state::deflate;
}

struct ba2_record {
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    std::uint64_t offset{};
    std::uint32_t packed_size{};
    std::uint32_t size{};
};

struct ba2_texture_chunk_record {
    std::uint64_t offset{};
    std::uint32_t packed_size{};
    std::uint32_t size{};
    std::uint16_t start_mip{};
    std::uint16_t end_mip{};
};

struct ba2_texture_record {
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    std::uint16_t height{};
    std::uint16_t width{};
    std::uint8_t mip_count{};
    std::uint8_t dxgi_format{};
    std::uint16_t cube_maps{};
    std::vector<ba2_texture_chunk_record> chunks;
};

struct ba2_name_table {
    std::vector<std::string> names;
    std::uint64_t end_offset{};
};

result<ba2_name_table> read_name_table(const byte_source& source, std::uint64_t file_table_offset, std::uint32_t file_count)
{
    std::uint64_t cursor = file_table_offset;
    std::vector<std::string> names;
    names.reserve(file_count);
    for (std::uint32_t i = 0; i < file_count; ++i) {
        auto length_bytes = read_bytes(source, cursor, 2);
        if (!length_bytes.has_value()) {
            return failure<ba2_name_table>(length_bytes.error());
        }
        const auto length = le_u16(length_bytes.value(), 0);
        cursor += 2;
        auto name_bytes = read_bytes(source, cursor, length);
        if (!name_bytes.has_value()) {
            return failure<ba2_name_table>(name_bytes.error());
        }
        cursor += length;

        std::string name;
        name.reserve(length);
        for (auto byte : name_bytes.value()) {
            name.push_back(static_cast<char>(std::to_integer<unsigned char>(byte)));
        }
        names.push_back(std::move(name));
    }
    return success(ba2_name_table{std::move(names), cursor});
}

std::uint64_t stored_size_for_record(const ba2_record& record) noexcept
{
    return record.packed_size == 0 ? record.size : record.packed_size;
}

result<ba2_archive> parse_ba2_gnrl(const byte_source& source)
{
    auto base_header = read_bytes(source, 0, base_header_size);
    if (!base_header.has_value()) {
        return failure<ba2_archive>(base_header.error());
    }
    if (le_u32(base_header.value(), 0) != magic_btdx) {
        return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 magic"});
    }

    const auto version = le_u32(base_header.value(), 4);
    if (!supported_version(version)) {
        return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 version"});
    }
    const auto header_size = header_size_for(version);
    auto header = header_size == base_header_size ? std::move(base_header) : read_bytes(source, 0, header_size);
    if (!header.has_value()) {
        return failure<ba2_archive>(header.error());
    }
    const auto subtype = le_u32(header.value(), 8);
    if (subtype != magic_gnrl) {
        return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 subtype"});
    }

    const auto file_count = le_u32(header.value(), 12);
    const auto file_table_offset = le_u64(header.value(), 16);
    const auto compression_method = version == version_starfield_v3 ? le_u32(header.value(), 32) : 0U;
    if (file_table_offset > source.size()) {
        return failure<ba2_archive>(truncated_table_error());
    }
    std::uint64_t record_table_size = 0;
    std::uint64_t record_table_end = 0;
    if (!checked_add(static_cast<std::uint64_t>(file_count) * record_size, 0, record_table_size) ||
        !checked_add(header_size, record_table_size, record_table_end) ||
        file_table_offset < record_table_end || !range_fits(header_size, record_table_size, source.size())) {
        return failure<ba2_archive>(truncated_table_error());
    }

    std::vector<ba2_record> records;
    records.reserve(file_count);
    for (std::uint32_t i = 0; i < file_count; ++i) {
        auto record_bytes = read_bytes(source, header_size + (static_cast<std::uint64_t>(i) * record_size), record_size);
        if (!record_bytes.has_value()) {
            return failure<ba2_archive>(record_bytes.error());
        }
        ba2_record record{};
        record.name_hash = le_u32(record_bytes.value(), 0);
        record.directory_hash = le_u32(record_bytes.value(), 8);
        record.offset = le_u64(record_bytes.value(), 16);
        record.packed_size = le_u32(record_bytes.value(), 24);
        record.size = le_u32(record_bytes.value(), 28);
        records.push_back(record);
    }

    auto names = read_name_table(source, file_table_offset, file_count);
    if (!names.has_value()) {
        return failure<ba2_archive>(names.error());
    }

    if (names.value().names.size() != file_count) {
        return failure<ba2_archive>({error_code::malformed_archive, "truncated BA2 table"});
    }

    if (!records.empty()) {
        auto first_payload_offset = records.front().offset;
        for (const auto& record : records) {
            if (record.offset < first_payload_offset) {
                first_payload_offset = record.offset;
            }
        }
        // BA2 GNRL associates names by record index; accepting extra bytes before the
        // first payload can hide a name-count mismatch and mis-associate unsafe paths.
        if (names.value().end_offset != first_payload_offset) {
            return failure<ba2_archive>({error_code::malformed_archive, "truncated BA2 table"});
        }
    }

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
        entries.push_back(std::move(metadata));
    }

    archive_summary summary{};
    summary.format = format_for_version(version);
    summary.version = version;
    summary.subtype = subtype;
    summary.file_count = file_count;
    summary.file_table_offset = file_table_offset;
    if (version == version_starfield_v3) {
        summary.compression_method = compression_method;
    }
    return success(ba2_archive{summary, std::move(entries)});
}

result<ba2_archive> parse_ba2_dx10(const byte_source& source)
{
    auto base_header = read_bytes(source, 0, base_header_size);
    if (!base_header.has_value()) {
        return failure<ba2_archive>(base_header.error());
    }
    if (le_u32(base_header.value(), 0) != magic_btdx) {
        return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 magic"});
    }

    const auto version = le_u32(base_header.value(), 4);
    if (!supported_dx10_version(version)) {
        return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 version"});
    }
    const auto header_size = header_size_for(version);
    auto header = header_size == base_header_size ? std::move(base_header) : read_bytes(source, 0, header_size);
    if (!header.has_value()) {
        return failure<ba2_archive>(header.error());
    }
    const auto subtype = le_u32(header.value(), 8);
    if (subtype != magic_dx10) {
        return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 subtype"});
    }

    const auto file_count = le_u32(header.value(), 12);
    const auto file_table_offset = le_u64(header.value(), 16);
    const auto compression_method = version == version_starfield_v3 ? le_u32(header.value(), 32) : 0U;
    if (file_table_offset > source.size()) {
        return failure<ba2_archive>(truncated_table_error());
    }

    std::uint64_t cursor = header_size;
    std::vector<ba2_texture_record> records;
    records.reserve(file_count);
    for (std::uint32_t i = 0; i < file_count; ++i) {
        if (!range_fits(cursor, dx10_record_prefix_size, file_table_offset)) {
            return failure<ba2_archive>(truncated_table_error());
        }
        auto record_bytes = read_bytes(source, cursor, dx10_record_prefix_size);
        if (!record_bytes.has_value()) {
            return failure<ba2_archive>(record_bytes.error());
        }

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

        std::uint64_t chunk_table_size = 0;
        std::uint64_t next_record = 0;
        if (chunk_header_size != dx10_chunk_size || !checked_add(static_cast<std::uint64_t>(chunk_count) * dx10_chunk_size, 0, chunk_table_size) ||
            !checked_add(cursor + dx10_record_prefix_size, chunk_table_size, next_record) || next_record > file_table_offset) {
            return failure<ba2_archive>(truncated_table_error());
        }

        // BA2 DX10 texture records store a fixed 24-byte prefix followed by one
        // 24-byte chunk record per mip range, matching the reference reader's
        // field order for height, width, mip count, format, cubemap count, and chunks.
        record.chunks.reserve(chunk_count);
        auto chunk_cursor = cursor + dx10_record_prefix_size;
        for (std::uint32_t chunk_index = 0; chunk_index < chunk_count; ++chunk_index) {
            auto chunk_bytes = read_bytes(source, chunk_cursor, dx10_chunk_size);
            if (!chunk_bytes.has_value()) {
                return failure<ba2_archive>(chunk_bytes.error());
            }
            ba2_texture_chunk_record chunk{};
            chunk.offset = le_u64(chunk_bytes.value(), 0);
            chunk.packed_size = le_u32(chunk_bytes.value(), 8);
            chunk.size = le_u32(chunk_bytes.value(), 12);
            chunk.start_mip = le_u16(chunk_bytes.value(), 16);
            chunk.end_mip = le_u16(chunk_bytes.value(), 18);
            if (chunk.end_mip < chunk.start_mip || record.mip_count <= chunk.end_mip ||
                !range_fits(chunk.offset, chunk.packed_size, source.size())) {
                return failure<ba2_archive>({error_code::malformed_archive, "invalid BA2 DX10 chunk"});
            }
            record.chunks.push_back(chunk);
            chunk_cursor += dx10_chunk_size;
        }

        records.push_back(std::move(record));
        cursor = next_record;
    }

    if (cursor != file_table_offset) {
        return failure<ba2_archive>(truncated_table_error());
    }

    auto names = read_name_table(source, file_table_offset, file_count);
    if (!names.has_value()) {
        return failure<ba2_archive>(names.error());
    }
    if (names.value().names.size() != file_count) {
        return failure<ba2_archive>({error_code::malformed_archive, "truncated BA2 table"});
    }

    if (!records.empty()) {
        std::uint64_t first_payload_offset = std::numeric_limits<std::uint64_t>::max();
        for (const auto& record : records) {
            for (const auto& chunk : record.chunks) {
                first_payload_offset = std::min(first_payload_offset, chunk.offset);
            }
        }
        if (first_payload_offset != std::numeric_limits<std::uint64_t>::max() && names.value().end_offset != first_payload_offset) {
            return failure<ba2_archive>({error_code::malformed_archive, "truncated BA2 table"});
        }
    }

    std::vector<entry_metadata> entries;
    std::vector<libbsa::texture_metadata> textures;
    std::vector<std::string> normalized_paths;
    entries.reserve(file_count);
    textures.reserve(file_count);
    normalized_paths.reserve(file_count);

    for (std::uint32_t i = 0; i < file_count; ++i) {
        const auto& record = records[i];
        auto normalized = normalize_archive_path(names.value().names[i]);
        if (!normalized.has_value()) {
            return failure<ba2_archive>({error_code::malformed_archive, "invalid BA2 name"});
        }
        if (std::find(normalized_paths.begin(), normalized_paths.end(), normalized.value().string()) != normalized_paths.end()) {
            return failure<ba2_archive>({error_code::malformed_archive, "duplicate BA2 name"});
        }
        normalized_paths.push_back(normalized.value().string());

        entry_metadata metadata{};
        metadata.path = normalized.value().string();
        metadata.name_hash = record.name_hash;
        metadata.directory_hash = record.directory_hash;
        metadata.compression = compression_state::raw;

        libbsa::texture_metadata texture{};
        texture.path = normalized.value().string();
        texture.width = record.width;
        texture.height = record.height;
        texture.mip_count = record.mip_count;
        texture.format = dxgi_format{record.dxgi_format};
        texture.array_size = record.cube_maps == 0 ? 1U : record.cube_maps;
        texture.is_cubemap = record.cube_maps == 6U;
        texture.chunks.reserve(record.chunks.size());

        bool first_chunk = true;
        for (const auto& chunk : record.chunks) {
            const auto chunk_compression = compression_for_dx10_chunk(version, compression_method, chunk.packed_size, chunk.size);
            if (chunk_compression == compression_state::unknown) {
                return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 DX10 compression"});
            }

            if (first_chunk || chunk.offset < metadata.offset) {
                metadata.offset = chunk.offset;
                first_chunk = false;
            }
            if (!checked_add(metadata.size, chunk.size, metadata.size) ||
                !checked_add(metadata.packed_size, chunk.packed_size, metadata.packed_size) ||
                !checked_add(metadata.stored_size, chunk.packed_size, metadata.stored_size)) {
                return failure<ba2_archive>({error_code::malformed_archive, "BA2 DX10 chunk size overflow"});
            }
            if (chunk_compression != compression_state::raw) {
                metadata.compression = chunk_compression;
            }

            texture_chunk_metadata chunk_metadata{};
            // Public chunk metadata exposes logical chunk-to-mip association without
            // leaking the raw BA2 record tail; later reconstruction uses the same range.
            chunk_metadata.mip_level = chunk.start_mip;
            chunk_metadata.offset = chunk.offset;
            chunk_metadata.packed_size = chunk.packed_size;
            chunk_metadata.size = chunk.size;
            chunk_metadata.compression = chunk_compression;
            texture.chunks.push_back(chunk_metadata);
        }

        entries.push_back(std::move(metadata));
        textures.push_back(std::move(texture));
    }

    archive_summary summary{};
    summary.format = dx10_format_for_version(version);
    summary.version = version;
    summary.subtype = subtype;
    summary.file_count = file_count;
    summary.file_table_offset = file_table_offset;
    if (version == version_starfield_v3) {
        summary.compression_method = compression_method;
    }

    return success(ba2_archive{summary, std::move(entries), std::move(textures)});
}

} // namespace

ba2_archive::ba2_archive(archive_summary summary, std::vector<entry_metadata> entries)
    : ba2_archive(std::move(summary), std::move(entries), std::vector<libbsa::texture_metadata>{})
{
}

ba2_archive::ba2_archive(archive_summary summary,
                         std::vector<entry_metadata> entries,
                         std::vector<libbsa::texture_metadata> textures)
    : view_(std::move(summary), std::move(entries))
{
    for (auto texture : textures) {
        auto normalized = normalize_archive_path(texture.path);
        if (!normalized.has_value()) {
            continue;
        }
        texture.path = normalized.value().string();
        textures_.insert_or_assign(texture.path, std::move(texture));
    }
}

const archive_summary& ba2_archive::summary() const noexcept
{
    return view_.summary();
}

std::vector<archive_path> ba2_archive::paths() const
{
    return view_.paths();
}

bool ba2_archive::contains(std::string path) const
{
    return view_.contains(std::move(path));
}

result<entry_metadata> ba2_archive::entry(std::string path) const
{
    return view_.entry(std::move(path));
}

result<libbsa::texture_metadata> ba2_archive::texture_metadata(std::string path) const
{
    auto normalized = normalize_archive_path(std::move(path));
    if (!normalized.has_value()) {
        return failure<libbsa::texture_metadata>(normalized.error());
    }

    const auto found = textures_.find(normalized.value().string());
    if (found == textures_.end()) {
        return failure<libbsa::texture_metadata>({error_code::malformed_archive, "BA2 texture metadata not found"});
    }

    return success(found->second);
}

std::string_view dxgi_format_name(dxgi_format format) noexcept
{
    switch (format.value) {
    case 28:
        return "R8G8B8A8_UNORM";
    case 71:
        return "BC1_UNORM";
    case 74:
        return "BC2_UNORM";
    case 77:
        return "BC3_UNORM";
    case 80:
        return "BC4_UNORM";
    case 83:
        return "BC5_UNORM";
    case 87:
        return "B8G8R8A8_UNORM";
    case 95:
        return "BC6H_UF16";
    case 98:
        return "BC7_UNORM";
    default:
        return "UNKNOWN";
    }
}

result<ba2_archive> open_ba2(const byte_source& source)
{
    auto base_header = read_bytes(source, 0, base_header_size);
    if (!base_header.has_value()) {
        return failure<ba2_archive>(base_header.error());
    }
    if (le_u32(base_header.value(), 0) != magic_btdx) {
        return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 magic"});
    }

    const auto subtype = le_u32(base_header.value(), 8);
    if (subtype == magic_gnrl) {
        return parse_ba2_gnrl(source);
    }
    if (subtype == magic_dx10) {
        return parse_ba2_dx10(source);
    }
    return failure<ba2_archive>({error_code::unsupported_format, "unsupported BA2 subtype"});
}

result<void> extract_ba2_entry(const ba2_archive& archive, const byte_source& source, std::string path, byte_sink& sink)
{
    auto metadata = archive.entry(std::move(path));
    if (!metadata.has_value()) {
        return failure<void>(metadata.error());
    }

    if (metadata.value().stored_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
        !range_fits(metadata.value().offset, metadata.value().stored_size, source.size())) {
        return failure<void>({error_code::malformed_archive, "BA2 payload range exceeds source size"});
    }

    auto payload = read_bytes(source, metadata.value().offset, metadata.value().stored_size);
    if (!payload.has_value()) {
        return failure<void>({error_code::malformed_archive, "BA2 payload range exceeds source size"});
    }

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
}

} // namespace libbsa
