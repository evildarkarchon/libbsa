#include "ba2_dds_fixture_helpers.hpp"

#include <catch2/catch_test_macros.hpp>

#include <libbsa/compression.hpp>

#include <algorithm>
#include <span>
#include <string_view>
#include <utility>

namespace libbsa::test {
namespace {

constexpr std::uint32_t magic_btdx = 0x58445442U;
constexpr std::uint32_t magic_dx10 = 0x30315844U;
constexpr std::uint32_t dx10_record_prefix_size = 24U;
constexpr std::uint32_t dx10_chunk_size = 24U;
constexpr std::uint32_t dx10_chunk_header_size = 24U;
constexpr std::uint32_t starfield_v3_compression_lz4 = 3U;
constexpr std::uint32_t chunk_tail = 0xbaadf00dU;

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

void write_u64(std::vector<std::byte>& bytes, std::size_t offset, std::uint64_t value)
{
    for (int shift = 0; shift < 64; shift += 8) {
        bytes[offset + static_cast<std::size_t>(shift / 8)] = static_cast<std::byte>((value >> shift) & 0xffU);
    }
}

void append_magic(std::vector<std::byte>& bytes, std::string_view magic)
{
    for (char ch : magic) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
}

std::uint32_t header_size_for(std::uint32_t version) noexcept
{
    return version == VERSION_STARFIELD_DX10_V3 ? 36U : 24U;
}

std::uint32_t dx10_record_size(const ba2_dds_texture_descriptor& texture) noexcept
{
    return dx10_record_prefix_size + (static_cast<std::uint32_t>(texture.chunks.size()) * dx10_chunk_size);
}

std::vector<std::byte> name_table_bytes(const std::vector<ba2_dds_texture_descriptor>& textures)
{
    std::vector<std::byte> names;
    for (const auto& texture : textures) {
        append_u16(names, static_cast<std::uint16_t>(texture.path.size()));
        append_magic(names, texture.path);
    }
    return names;
}

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

std::vector<std::byte> deterministic_payload(std::size_t size, std::byte seed)
{
    std::vector<std::byte> payload(size);
    for (std::size_t index = 0; index < payload.size(); ++index) {
        payload[index] = static_cast<std::byte>((std::to_integer<unsigned char>(seed) + index) & 0xffU);
    }
    return payload;
}

ba2_dds_chunk_descriptor chunk(std::uint16_t start_mip,
                               std::uint16_t end_mip,
                               std::vector<std::byte> payload,
                               compression_state compression = compression_state::raw)
{
    ba2_dds_chunk_descriptor result{};
    result.start_mip = start_mip;
    result.end_mip = end_mip;
    result.payload = std::move(payload);
    result.compression = compression;
    return result;
}

ba2_dds_texture_descriptor one_mip_texture(std::string path = "textures/generated/one_mip.dds")
{
    ba2_dds_texture_descriptor texture{};
    texture.path = std::move(path);
    texture.width = 4;
    texture.height = 4;
    texture.mip_count = 1;
    texture.dxgi_format = 71;
    texture.array_size = 1;
    texture.chunks.push_back(chunk(0, 0, deterministic_payload(8, std::byte{0x10})));
    return texture;
}

} // namespace

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
    append_u32(bytes, version);
    append_magic(bytes, "DX10");
    append_u32(bytes, file_count);
    append_u64(bytes, file_table_offset);
    if (version == VERSION_STARFIELD_DX10_V3) {
        append_u32(bytes, 0);
        append_u32(bytes, 0);
        append_u32(bytes, compression_method);
    }

    std::size_t chunk_index = 0;
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

    bytes.insert(bytes.end(), names.begin(), names.end());
    for (const auto& stored : stored_chunks) {
        bytes.insert(bytes.end(), stored.begin(), stored.end());
    }

    return ba2_dds_fixture{version, compression_method, std::move(textures), std::move(bytes)};
}

ba2_dds_fixture fo4_dx10_v1_one_mip_fixture()
{
    return one_mip_fixture(VERSION_FO4_DX10_V1);
}

ba2_dds_fixture fo4_dx10_v7_one_mip_fixture()
{
    return one_mip_fixture(VERSION_FO4_DX10_V7);
}

ba2_dds_fixture fo4_dx10_v8_one_mip_fixture()
{
    return one_mip_fixture(VERSION_FO4_DX10_V8);
}

ba2_dds_fixture starfield_dx10_v3_lz4_block_fixture()
{
    return lz4_block_chunk_fixture();
}

ba2_dds_fixture one_mip_fixture(std::uint32_t version)
{
    return make_ba2_dds_fixture(version, {one_mip_texture()});
}

ba2_dds_fixture multi_mip_fixture(std::uint32_t version)
{
    auto texture = one_mip_texture("textures/generated/multi_mip.dds");
    texture.width = 16;
    texture.height = 16;
    texture.mip_count = 3;
    texture.chunks.clear();
    texture.chunks.push_back(chunk(0, 1, deterministic_payload(32, std::byte{0x20})));
    texture.chunks.push_back(chunk(2, 2, deterministic_payload(8, std::byte{0x40})));
    return make_ba2_dds_fixture(version, {std::move(texture)});
}

ba2_dds_fixture cubemap_fixture(std::uint32_t version)
{
    auto texture = one_mip_texture("textures/generated/cubemap.dds");
    texture.cubemap = true;
    texture.array_size = 6;
    texture.chunks.front().payload = deterministic_payload(48, std::byte{0x60});
    return make_ba2_dds_fixture(version, {std::move(texture)});
}

ba2_dds_fixture array_fixture(std::uint32_t version)
{
    auto texture = one_mip_texture("textures/generated/array.dds");
    texture.array_size = 4;
    texture.chunks.front().payload = deterministic_payload(32, std::byte{0x80});
    return make_ba2_dds_fixture(version, {std::move(texture)});
}

ba2_dds_fixture raw_chunk_fixture(std::uint32_t version)
{
    return one_mip_fixture(version);
}

ba2_dds_fixture deflate_chunk_fixture(std::uint32_t version)
{
    auto texture = one_mip_texture("textures/generated/deflate.dds");
    texture.chunks.front().compression = compression_state::deflate;
    return make_ba2_dds_fixture(version, {std::move(texture)});
}

ba2_dds_fixture lz4_block_chunk_fixture()
{
    auto texture = one_mip_texture("textures/generated/lz4_block.dds");
    texture.chunks.front().compression = compression_state::lz4_block;
    return make_ba2_dds_fixture(VERSION_STARFIELD_DX10_V3, {std::move(texture)}, starfield_v3_compression_lz4);
}

ba2_dds_fixture malformed_truncated_record_fixture()
{
    auto fixture = one_mip_fixture(VERSION_FO4_DX10_V1);
    fixture.bytes.resize(header_size_for(fixture.version) + dx10_record_prefix_size - 1U);
    return fixture;
}

ba2_dds_fixture malformed_invalid_chunk_range_fixture()
{
    auto fixture = one_mip_fixture(VERSION_FO4_DX10_V1);
    write_u64(fixture.bytes, header_size_for(fixture.version) + 24U, static_cast<std::uint64_t>(fixture.bytes.size() + 1024U));
    return fixture;
}

ba2_dds_fixture malformed_duplicate_normalized_names_fixture()
{
    auto first = one_mip_texture("Textures/Generated/Duplicate.DDS");
    auto second = one_mip_texture("textures\\generated\\duplicate.dds");
    second.name_hash = 0x55667788;
    return make_ba2_dds_fixture(VERSION_FO4_DX10_V1, {std::move(first), std::move(second)});
}

ba2_dds_fixture malformed_unsupported_codec_route_fixture()
{
    auto texture = one_mip_texture("textures/generated/unsupported_codec.dds");
    texture.chunks.front().compression = compression_state::lz4_block;
    return make_ba2_dds_fixture(VERSION_STARFIELD_DX10_V3, {std::move(texture)}, 99);
}

ba2_dds_fixture malformed_inconsistent_mip_chunk_mapping_fixture()
{
    auto texture = one_mip_texture("textures/generated/inconsistent_mips.dds");
    texture.mip_count = 1;
    texture.chunks.front().start_mip = 2;
    texture.chunks.front().end_mip = 3;
    return make_ba2_dds_fixture(VERSION_FO4_DX10_V1, {std::move(texture)});
}

ba2_dds_fixture malformed_reconstruction_failure_fixture()
{
    auto texture = one_mip_texture("textures/generated/reconstruction_failure.dds");
    texture.width = 0;
    return make_ba2_dds_fixture(VERSION_FO4_DX10_V1, {std::move(texture)});
}

void require_ba2_dds_identity(const ba2_dds_fixture& fixture)
{
    REQUIRE(fixture.bytes.size() >= 12);
    CHECK(fixture.bytes[0] == std::byte{'B'});
    CHECK(fixture.bytes[1] == std::byte{'T'});
    CHECK(fixture.bytes[2] == std::byte{'D'});
    CHECK(fixture.bytes[3] == std::byte{'X'});
    CHECK(fixture.bytes[8] == std::byte{'D'});
    CHECK(fixture.bytes[9] == std::byte{'X'});
    CHECK(fixture.bytes[10] == std::byte{'1'});
    CHECK(fixture.bytes[11] == std::byte{'0'});
    CHECK(magic_btdx == 0x58445442U);
    CHECK(magic_dx10 == 0x30315844U);
}

} // namespace libbsa::test
