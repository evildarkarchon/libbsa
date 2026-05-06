#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/ba2.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/io.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::uint32_t VERSION_FO4_V1 = 0x01;
constexpr std::uint32_t VERSION_STARFIELD_V2 = 0x02;
constexpr std::uint32_t VERSION_FO4_V7 = 0x07;
constexpr std::uint32_t VERSION_FO4_V8 = 0x08;
constexpr std::uint32_t RECORD_SIZE = 36;

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

struct ba2_entry_fixture {
    std::string path{"meshes/armor/iron.nif"};
    std::uint32_t name_hash{0x11223344};
    std::uint32_t directory_hash{0xaabbccdd};
    std::uint32_t packed_size{};
    std::uint32_t size{4};
    std::uint64_t offset{};
    std::vector<std::byte> payload{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}};
};

std::uint32_t header_size_for(std::uint32_t version)
{
    return version == VERSION_STARFIELD_V2 ? 32U : 24U;
}

std::vector<std::byte> name_table_bytes(const std::vector<ba2_entry_fixture>& entries)
{
    std::vector<std::byte> names;
    for (const auto& entry : entries) {
        append_u16(names, static_cast<std::uint16_t>(entry.path.size()));
        append_magic(names, entry.path);
    }
    return names;
}

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
    if (version == VERSION_STARFIELD_V2) {
        append_u32(bytes, 0);
        append_u32(bytes, 0);
    } else if (compression_method != 0) {
        append_u32(bytes, 0);
        append_u32(bytes, 0);
        append_u32(bytes, compression_method);
    }

    for (const auto& entry : entries) {
        append_u32(bytes, entry.name_hash);
        append_magic(bytes, ".bin");
        append_u32(bytes, entry.directory_hash);
        append_u32(bytes, 0);
        append_u64(bytes, entry.offset);
        append_u32(bytes, entry.packed_size);
        append_u32(bytes, entry.size);
        append_u32(bytes, 0xbaadf00dU);
    }
    bytes.insert(bytes.end(), names.begin(), names.end());
    for (const auto& entry : entries) {
        if (bytes.size() < entry.offset) {
            bytes.resize(static_cast<std::size_t>(entry.offset), std::byte{0});
        }
        bytes.insert(bytes.end(), entry.payload.begin(), entry.payload.end());
    }
    return bytes;
}

libbsa::result<libbsa::ba2_archive> open_bytes(const std::vector<std::byte>& bytes)
{
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    return libbsa::open_ba2(source);
}

ba2_entry_fixture raw_entry(std::uint32_t version)
{
    ba2_entry_fixture entry;
    entry.path = "meshes/fo4/v" + std::to_string(version) + "/raw.bin";
    entry.name_hash = 0x10000000U + version;
    entry.directory_hash = 0x20000000U + version;
    entry.payload = {std::byte{0xba}, std::byte{0x20}, static_cast<std::byte>(version), std::byte{0x44}};
    entry.size = static_cast<std::uint32_t>(entry.payload.size());
    return entry;
}

ba2_entry_fixture deflate_entry(std::uint32_t version, std::string path)
{
    ba2_entry_fixture entry;
    entry.path = std::move(path);
    entry.name_hash = 0x30000000U + version;
    entry.directory_hash = 0x40000000U + version;
    entry.size = 8;
    const std::vector<std::byte> output{std::byte{0xde}, std::byte{0xf1}, static_cast<std::byte>(version), std::byte{0x04},
                                        std::byte{0x06}, std::byte{0x03}, std::byte{0xba}, std::byte{0x2a}};
    auto compressed = libbsa::compress_payload(libbsa::compression_algorithm::deflate, std::span<const std::byte>{output});
    REQUIRE(compressed.has_value());
    entry.payload = std::move(compressed.value());
    entry.packed_size = static_cast<std::uint32_t>(entry.payload.size());
    return entry;
}

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

std::vector<std::byte> expected_deflate_output(std::uint32_t version)
{
    return {std::byte{0xde}, std::byte{0xf1}, static_cast<std::byte>(version), std::byte{0x04},
            std::byte{0x06}, std::byte{0x03}, std::byte{0xba}, std::byte{0x2a}};
}

void assert_common_gnrl_metadata(std::uint32_t version, libbsa::archive_format expected_format)
{
    ba2_entry_fixture raw;
    raw.path = "Textures\\LUTs\\Color.DDS";
    raw.name_hash = 0x01020304;
    raw.directory_hash = 0x10203040;
    raw.size = 4;
    raw.payload = {std::byte{0xdd}, std::byte{0x05}, std::byte{0x00}, std::byte{0x01}};
    ba2_entry_fixture compressed;
    compressed.path = "meshes/armor/iron.nif";
    compressed.name_hash = 0x55667788;
    compressed.directory_hash = 0x88776655;
    compressed.packed_size = 3;
    compressed.size = 9;
    compressed.payload = {std::byte{0x63}, std::byte{0x62}, std::byte{0x61}};
    const auto bytes = ba2_gnrl_archive_bytes(version, {raw, compressed});

    const auto opened = open_bytes(bytes);

    REQUIRE(opened.has_value());
    CHECK(opened.value().summary().format == expected_format);
    REQUIRE(opened.value().summary().version.has_value());
    CHECK(*opened.value().summary().version == version);
    REQUIRE(opened.value().summary().subtype.has_value());
    CHECK(*opened.value().summary().subtype == 0x4c524e47U);
    REQUIRE(opened.value().summary().file_count.has_value());
    CHECK(*opened.value().summary().file_count == 2);
    REQUIRE(opened.value().summary().file_table_offset.has_value());
    CHECK(*opened.value().summary().file_table_offset == header_size_for(version) + (2U * RECORD_SIZE));

    const auto paths = opened.value().paths();
    REQUIRE(paths.size() == 2);
    CHECK(paths[0].string() == "meshes/armor/iron.nif");
    CHECK(paths[1].string() == "textures/luts/color.dds");
    CHECK(opened.value().contains("Textures/LUTS/color.dds"));
    CHECK(opened.value().contains("textures\\luts\\color.dds"));

    const auto raw_metadata = opened.value().entry("textures/luts/color.dds");
    REQUIRE(raw_metadata.has_value());
    CHECK(raw_metadata.value().path == "textures/luts/color.dds");
    CHECK(raw_metadata.value().name_hash == raw.name_hash);
    CHECK(raw_metadata.value().directory_hash == raw.directory_hash);
    CHECK(raw_metadata.value().size == raw.size);
    CHECK(raw_metadata.value().packed_size == raw.size);
    CHECK(raw_metadata.value().stored_size == raw.size);
    CHECK(raw_metadata.value().compression == libbsa::compression_state::raw);
    CHECK(raw_metadata.value().offset == header_size_for(version) + (2U * RECORD_SIZE) + name_table_bytes({raw, compressed}).size());

    const auto compressed_metadata = opened.value().entry("meshes/armor/iron.nif");
    REQUIRE(compressed_metadata.has_value());
    CHECK(compressed_metadata.value().name_hash == compressed.name_hash);
    CHECK(compressed_metadata.value().directory_hash == compressed.directory_hash);
    CHECK(compressed_metadata.value().size == compressed.size);
    CHECK(compressed_metadata.value().packed_size == compressed.packed_size);
    CHECK(compressed_metadata.value().stored_size == compressed.packed_size);
    CHECK(compressed_metadata.value().compression == libbsa::compression_state::deflate);
}

} // namespace

TEST_CASE("open_ba2 lists Fallout 4 GNRL v1 metadata", "[fixture]")
{
    assert_common_gnrl_metadata(VERSION_FO4_V1, libbsa::archive_format::fo4_ba2_gnrl);
}

TEST_CASE("open_ba2 lists Fallout 4 GNRL v7 metadata", "[fixture]")
{
    assert_common_gnrl_metadata(VERSION_FO4_V7, libbsa::archive_format::fo4_ba2_gnrl);
}

TEST_CASE("open_ba2 lists Fallout 4 GNRL v8 metadata", "[fixture]")
{
    assert_common_gnrl_metadata(VERSION_FO4_V8, libbsa::archive_format::fo4_ba2_gnrl);
}

TEST_CASE("open_ba2 lists Starfield v2 GNRL metadata", "[fixture]")
{
    assert_common_gnrl_metadata(VERSION_STARFIELD_V2, libbsa::archive_format::starfield_ba2_gnrl);
}

TEST_CASE("open_ba2 associates length-prefixed names from FileTableOffset", "[fixture]")
{
    ba2_entry_fixture first;
    first.path = "sounds/fx/chime.wav";
    first.name_hash = 0xabcdef01;
    ba2_entry_fixture second;
    second.path = "interface/icons/map.dds";
    second.name_hash = 0xabcdef02;
    second.offset = 256;
    second.size = 2;
    second.payload = {std::byte{0x42}, std::byte{0x24}};

    const auto opened = open_bytes(ba2_gnrl_archive_bytes(VERSION_FO4_V1, {first, second}));

    REQUIRE(opened.has_value());
    CHECK(opened.value().contains("Sounds\\FX\\Chime.wav"));
    const auto first_metadata = opened.value().entry("sounds/fx/chime.wav");
    REQUIRE(first_metadata.has_value());
    CHECK(first_metadata.value().name_hash == first.name_hash);
    const auto second_metadata = opened.value().entry("interface/icons/map.dds");
    REQUIRE(second_metadata.has_value());
    CHECK(second_metadata.value().name_hash == second.name_hash);
    CHECK(second_metadata.value().offset == second.offset);
}

TEST_CASE("extract_ba2_entry writes raw Fallout 4 v1 bytes", "[fixture]")
{
    const auto entry = raw_entry(VERSION_FO4_V1);
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_FO4_V1, {entry});

    CHECK(extract_ba2_bytes(bytes, entry.path) == entry.payload);
}

TEST_CASE("extract_ba2_entry writes raw Fallout 4 v7 bytes", "[fixture]")
{
    const auto entry = raw_entry(VERSION_FO4_V7);
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_FO4_V7, {entry});

    CHECK(extract_ba2_bytes(bytes, entry.path) == entry.payload);
}

TEST_CASE("extract_ba2_entry writes raw Fallout 4 v8 bytes", "[fixture]")
{
    const auto entry = raw_entry(VERSION_FO4_V8);
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_FO4_V8, {entry});

    CHECK(extract_ba2_bytes(bytes, entry.path) == entry.payload);
}

TEST_CASE("extract_ba2_entry writes deflate Fallout 4 v1 bytes without BSA prefix", "[fixture]")
{
    const auto entry = deflate_entry(VERSION_FO4_V1, "meshes/fo4/v1/packed.bin");
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_FO4_V1, {entry});

    CHECK(extract_ba2_bytes(bytes, entry.path) == expected_deflate_output(VERSION_FO4_V1));
}

TEST_CASE("extract_ba2_entry writes deflate Fallout 4 v7 bytes without BSA prefix", "[fixture]")
{
    const auto entry = deflate_entry(VERSION_FO4_V7, "meshes/fo4/v7/packed.bin");
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_FO4_V7, {entry});

    CHECK(extract_ba2_bytes(bytes, entry.path) == expected_deflate_output(VERSION_FO4_V7));
}

TEST_CASE("extract_ba2_entry writes deflate Fallout 4 v8 bytes without BSA prefix", "[fixture]")
{
    const auto entry = deflate_entry(VERSION_FO4_V8, "meshes/fo4/v8/packed.bin");
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_FO4_V8, {entry});

    CHECK(extract_ba2_bytes(bytes, entry.path) == expected_deflate_output(VERSION_FO4_V8));
}

TEST_CASE("extract_ba2_entry writes Starfield v2 deflate bytes", "[fixture]")
{
    const auto entry = deflate_entry(VERSION_STARFIELD_V2, "data/starfield/v2/packed.bin");
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_STARFIELD_V2, {entry});

    CHECK(extract_ba2_bytes(bytes, entry.path) == expected_deflate_output(VERSION_STARFIELD_V2));
}

TEST_CASE("extract_ba2_entry treats dds names in GNRL as ordinary payloads", "[fixture]")
{
    ba2_entry_fixture entry;
    entry.path = "textures/luts/color.dds";
    entry.payload = {std::byte{0x44}, std::byte{0x44}, std::byte{0x53}, std::byte{0x20}};
    entry.size = static_cast<std::uint32_t>(entry.payload.size());
    const auto bytes = ba2_gnrl_archive_bytes(VERSION_FO4_V1, {entry});

    CHECK(extract_ba2_bytes(bytes, "Textures\\LUTS\\Color.DDS") == entry.payload);
}

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
