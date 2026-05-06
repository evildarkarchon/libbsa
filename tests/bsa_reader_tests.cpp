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

namespace {

constexpr std::uint32_t BSA_MAGIC = 0x00415342;
constexpr std::uint32_t VERSION_TES4 = 0x67;
constexpr std::uint32_t VERSION_FO3 = 0x68;
constexpr std::uint32_t VERSION_SSE = 0x69;
constexpr std::uint32_t ARCHIVE_PATHNAMES = 0x0001;
constexpr std::uint32_t ARCHIVE_FILENAMES = 0x0002;
constexpr std::uint32_t ARCHIVE_COMPRESS = 0x0004;
constexpr std::uint32_t ARCHIVE_EMBEDNAME = 0x0100;
constexpr std::uint32_t FILE_SIZE_COMPRESS = 0x40000000;

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

std::uint32_t read_u32(const std::vector<std::byte>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset])) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 1])) << 8U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 2])) << 16U) |
           (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset + 3])) << 24U);
}

void append_cstring(std::vector<std::byte>& bytes, std::string_view value)
{
    for (char ch : value) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    bytes.push_back(std::byte{0});
}

void append_len8_string(std::vector<std::byte>& bytes, std::string_view value)
{
    append_u8(bytes, static_cast<std::uint8_t>(value.size() + 1));
    append_cstring(bytes, value);
}

std::vector<std::byte> bsa_header(std::uint32_t version, std::uint32_t folders, std::uint32_t files)
{
    std::vector<std::byte> bytes;
    append_u32(bytes, BSA_MAGIC);
    append_u32(bytes, version);
    append_u32(bytes, 36);
    append_u32(bytes, ARCHIVE_PATHNAMES | ARCHIVE_FILENAMES);
    append_u32(bytes, folders);
    append_u32(bytes, files);
    append_u32(bytes, 0);
    append_u32(bytes, 0);
    append_u32(bytes, 0);
    return bytes;
}

struct packed_entry {
    std::string folder{"meshes/armor"};
    std::string file{"iron.nif"};
    std::vector<std::byte> output{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}};
    libbsa::compression_algorithm algorithm{libbsa::compression_algorithm::none};
    bool file_compress_flag{};
    bool embedded_name{};
    std::uint32_t stored_size_override{};
    std::uint32_t payload_offset_override{};
};

std::vector<std::byte> make_payload(const packed_entry& entry)
{
    std::vector<std::byte> payload;
    if (entry.embedded_name) {
        constexpr std::string_view embedded = "meshes\\armor\\iron.nif";
        append_u8(payload, static_cast<std::uint8_t>(embedded.size()));
        for (char ch : embedded) {
            payload.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
        }
    }

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

    std::vector<std::byte> bytes;
    append_u32(bytes, BSA_MAGIC);
    append_u32(bytes, version);
    append_u32(bytes, 36);
    append_u32(bytes, flags | ARCHIVE_PATHNAMES | ARCHIVE_FILENAMES);
    append_u32(bytes, 1);
    append_u32(bytes, 1);
    append_u32(bytes, static_cast<std::uint32_t>(entry.folder.size() + 2));
    append_u32(bytes, static_cast<std::uint32_t>(entry.file.size() + 1));
    append_u32(bytes, 0);
    append_u64(bytes, 0);
    append_u32(bytes, 1);
    if (version == VERSION_SSE) {
        append_u32(bytes, 0);
        append_u64(bytes, 36 + folder_record_size);
    } else {
        append_u32(bytes, 36 + folder_record_size);
    }
    bytes.insert(bytes.end(), folder_block.begin(), folder_block.end());
    append_cstring(bytes, entry.file);
    if (bytes.size() < payload_offset) {
        bytes.resize(payload_offset, std::byte{0});
    }
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

libbsa::result<libbsa::bsa_archive> open_bytes(const std::vector<std::byte>& bytes)
{
    const libbsa::memory_source source{std::span<const std::byte>{bytes}};
    return libbsa::open_bsa(source);
}

libbsa::entry_metadata entry(std::string path, std::uint64_t size, std::uint64_t packed_size, std::uint64_t offset)
{
    libbsa::entry_metadata metadata{};
    metadata.path = std::move(path);
    metadata.size = size;
    metadata.packed_size = packed_size;
    metadata.stored_size = packed_size;
    metadata.offset = offset;
    metadata.compression = libbsa::compression_state::raw;
    return metadata;
}

libbsa::archive_summary summary()
{
    libbsa::archive_summary result{};
    result.format = libbsa::archive_format::sse_bsa;
    result.version = VERSION_SSE;
    result.file_count = 2;
    return result;
}

} // namespace

TEST_CASE("bsa archive lists normalized paths in deterministic order", "[unit]")
{
    const libbsa::bsa_archive archive{summary(), std::vector{
                                                   entry("Textures//Actors\\Hero.DDS", 20, 12, 200),
                                                   entry("Meshes\\Actors\\Hero.NIF", 10, 8, 100),
                                               }};

    const auto paths = archive.paths();

    REQUIRE(paths.size() == 2);
    CHECK(paths[0].string() == "meshes/actors/hero.nif");
    CHECK(paths[1].string() == "textures/actors/hero.dds");
}

TEST_CASE("bsa archive lookup uses archive path normalization", "[unit]")
{
    const libbsa::bsa_archive archive{summary(), std::vector{entry("Meshes\\Actors\\Hero.NIF", 10, 8, 100)}};

    CHECK(archive.contains("Meshes\\Actors\\Hero.NIF"));
    const auto found = archive.entry("meshes/actors/hero.nif");

    REQUIRE(found.has_value());
    CHECK(found.value().path == "meshes/actors/hero.nif");
}

TEST_CASE("bsa fixture helpers emit supported family headers", "[fixture]")
{
    for (const auto version : {VERSION_TES4, VERSION_FO3, VERSION_SSE}) {
        const auto bytes = bsa_header(version, 1, 2);

        REQUIRE(bytes.size() == 36);
        CHECK(static_cast<std::uint32_t>(bytes[0]) == 0x42U);
        CHECK(static_cast<std::uint32_t>(bytes[1]) == 0x53U);
        CHECK(static_cast<std::uint32_t>(bytes[2]) == 0x41U);
        CHECK(static_cast<std::uint32_t>(bytes[3]) == 0x00U);
        CHECK(static_cast<std::uint32_t>(bytes[4]) == (version & 0xffU));
    }
}

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

TEST_CASE("open_bsa lists FO3 v104 metadata with XOR compression", "[fixture]")
{
    packed_entry entry;
    entry.algorithm = libbsa::compression_algorithm::deflate;
    const auto opened = open_bytes(bsa_archive_bytes(VERSION_FO3, ARCHIVE_COMPRESS, entry));

    REQUIRE(opened.has_value());
    CHECK(opened.value().summary().format == libbsa::archive_format::fo3_bsa);
    CHECK(opened.value().summary().version == VERSION_FO3);
    const auto metadata = opened.value().entry("meshes/armor/iron.nif");
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().compression == libbsa::compression_state::deflate);
}

TEST_CASE("open_bsa parses SSE v105 folder records", "[fixture]")
{
    packed_entry entry;
    entry.folder = "textures/actors";
    entry.file = "hero.dds";
    entry.algorithm = libbsa::compression_algorithm::lz4_frame;
    const auto opened = open_bytes(bsa_archive_bytes(VERSION_SSE, ARCHIVE_COMPRESS, entry));

    REQUIRE(opened.has_value());
    CHECK(opened.value().summary().format == libbsa::archive_format::sse_bsa);
    CHECK(opened.value().summary().version == VERSION_SSE);
    const auto paths = opened.value().paths();
    REQUIRE(paths.size() == 1);
    CHECK(paths[0].string() == "textures/actors/hero.dds");
    const auto metadata = opened.value().entry("textures/actors/hero.dds");
    REQUIRE(metadata.has_value());
    CHECK(metadata.value().compression == libbsa::compression_state::lz4_frame);
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

TEST_CASE("extract_bsa_entry writes raw TES4 bytes", "[fixture]")
{
    const auto bytes = bsa_archive_bytes(VERSION_TES4, 0, {});

    CHECK(extract_bytes(bytes, "meshes/armor/iron.nif") ==
          std::vector{std::byte{0x10}, std::byte{0x20}, std::byte{0x30}, std::byte{0x40}});
}

TEST_CASE("extract_bsa_entry inflates deflate v104 payloads", "[fixture]")
{
    packed_entry entry;
    entry.algorithm = libbsa::compression_algorithm::deflate;
    const auto bytes = bsa_archive_bytes(VERSION_FO3, ARCHIVE_COMPRESS, entry);

    CHECK(extract_bytes(bytes, "meshes/armor/iron.nif") == entry.output);
}

TEST_CASE("extract_bsa_entry inflates LZ4-frame v105 payloads", "[fixture]")
{
    packed_entry entry;
    entry.algorithm = libbsa::compression_algorithm::lz4_frame;
    const auto bytes = bsa_archive_bytes(VERSION_SSE, ARCHIVE_COMPRESS, entry);

    CHECK(extract_bytes(bytes, "meshes/armor/iron.nif") == entry.output);
}

TEST_CASE("extract_bsa_entry skips embedded name prefixes", "[fixture]")
{
    packed_entry entry;
    entry.embedded_name = true;
    const auto bytes = bsa_archive_bytes(VERSION_FO3, ARCHIVE_EMBEDNAME, entry);

    CHECK(extract_bytes(bytes, "meshes/armor/iron.nif") == entry.output);
}
