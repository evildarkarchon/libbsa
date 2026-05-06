#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/bsa.hpp>
#include <libbsa/io.hpp>

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
