#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>

#include <string>
#include <vector>

namespace {

libbsa::entry_metadata entry(std::string path, std::uint64_t size, std::uint64_t packed_size, std::uint64_t offset,
    std::uint64_t name_hash, libbsa::compression_state compression)
{
    libbsa::entry_metadata metadata{};
    metadata.path = std::move(path);
    metadata.size = size;
    metadata.packed_size = packed_size;
    metadata.offset = offset;
    metadata.name_hash = name_hash;
    metadata.directory_hash = 0xfeedfaceULL;
    metadata.compression = compression;
    return metadata;
}

libbsa::archive_summary summary()
{
    libbsa::archive_summary result{};
    result.format = libbsa::archive_format::sse_bsa;
    result.version = 0x69;
    result.file_count = 2;
    return result;
}

} // namespace

TEST_CASE("archive view lists normalized paths in deterministic order", "[unit]")
{
    std::vector entries{
        entry("Meshes\\Actors\\Hero.NIF", 10, 8, 100, 0x02, libbsa::compression_state::lz4_frame),
        entry("Textures//Actors\\Hero.DDS", 20, 12, 200, 0x01, libbsa::compression_state::archive_default),
    };

    const libbsa::archive_view view{summary(), entries};
    auto paths = view.paths();

    REQUIRE(paths.size() == 2);
    CHECK(paths[0].string() == "meshes/actors/hero.nif");
    CHECK(paths[1].string() == "textures/actors/hero.dds");
}

TEST_CASE("archive view contains normalizes query path", "[unit]")
{
    std::vector entries{entry("textures/actors/hero.dds", 20, 12, 200, 0x01, libbsa::compression_state::archive_default)};
    const libbsa::archive_view view{summary(), entries};

    CHECK(view.contains("Textures\\Actors\\Hero.DDS"));
    CHECK_FALSE(view.contains("textures/actors/missing.dds"));
    CHECK_FALSE(view.contains("C:\\Data\\hero.dds"));
}

TEST_CASE("archive view returns copied entry metadata", "[unit]")
{
    std::vector entries{entry("textures/actors/hero.dds", 20, 12, 200, 0x99, libbsa::compression_state::deflate)};
    const libbsa::archive_view view{summary(), entries};

    auto found = view.entry("textures/actors/hero.dds");

    REQUIRE(found.has_value());
    CHECK(found.value().path == "textures/actors/hero.dds");
    CHECK(found.value().size == 20);
    CHECK(found.value().packed_size == 12);
    CHECK(found.value().offset == 200);
    CHECK(found.value().name_hash == 0x99);
    CHECK(found.value().directory_hash == 0xfeedfaceULL);
    CHECK(found.value().compression == libbsa::compression_state::deflate);
}

TEST_CASE("archive view reports missing and invalid lookup paths distinctly", "[unit]")
{
    std::vector entries{entry("textures/actors/hero.dds", 20, 12, 200, 0x99, libbsa::compression_state::deflate)};
    const libbsa::archive_view view{summary(), entries};

    auto missing = view.entry("textures/actors/missing.dds");
    auto invalid = view.entry("../hero.dds");

    REQUIRE_FALSE(missing.has_value());
    CHECK(missing.error().code == libbsa::error_code::malformed_archive);
    CHECK(missing.error().message == "archive entry not found");
    REQUIRE_FALSE(invalid.has_value());
    CHECK(invalid.error().code == libbsa::error_code::malformed_archive);
    CHECK(invalid.error().message == "invalid archive path");
}

TEST_CASE("archive view owns metadata after construction", "[unit]")
{
    std::vector entries{entry("textures/actors/hero.dds", 20, 12, 200, 0x99, libbsa::compression_state::deflate)};
    const libbsa::archive_view view{summary(), entries};

    entries[0].path = "textures/actors/mutated.dds";
    entries[0].size = 999;

    REQUIRE(view.contains("textures/actors/hero.dds"));
    auto found = view.entry("textures/actors/hero.dds");
    REQUIRE(found.has_value());
    CHECK(found.value().size == 20);
}
