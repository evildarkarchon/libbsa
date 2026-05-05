#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive_path.hpp>

#include "../src/hash.hpp"

#include <cstdint>

TEST_CASE("archive paths normalize to stable owned virtual paths", "[unit]")
{
    auto normalized = libbsa::normalize_archive_path("Textures//Actors\\Hero.DDS");

    REQUIRE(normalized.has_value());
    CHECK(normalized.value().string() == "textures/actors/hero.dds");

    auto collapsed = libbsa::normalize_archive_path("./Meshes///Creatures\\Mudcrab.NIF");
    REQUIRE(collapsed.has_value());
    CHECK(collapsed.value().string() == "meshes/creatures/mudcrab.nif");
}

TEST_CASE("archive path normalization rejects invalid path data", "[unit]")
{
    for (const char* path : {"", "C:\\Data\\x.dds", "//server/share/file.dds", "textures/../secret.dds"}) {
        auto normalized = libbsa::normalize_archive_path(path);

        REQUIRE_FALSE(normalized.has_value());
        CHECK(normalized.error().code == libbsa::error_code::malformed_archive);
        CHECK(normalized.error().message == "invalid archive path");
    }
}

TEST_CASE("TES3 path hash matches golden vectors", "[unit][golden]")
{
    // Provenance: TES5Edit/Core/wbBSArchive.pas CreateHashTES3.
    CHECK(libbsa::detail::hash_tes3_path("meshes\\marker.nif") == 0x052f1608f2d7fa5cULL);
    CHECK(libbsa::detail::hash_tes3_path("textures\\tx_boulder_01.dds") == 0x5865633f2479c295ULL);
    CHECK(libbsa::detail::hash_tes3_path("Textures\\Actors\\Hero.DDS") == 0x737e765d521478bbULL);
}

TEST_CASE("TES4 folder and file hashes match golden vectors", "[unit][golden]")
{
    // Provenance: TES5Edit/Core/wbBSArchive.pas CreateHashTES4.
    CHECK(libbsa::detail::hash_tes4_name("textures\\actors", "") == 0x67915cad740f7273ULL);
    CHECK(libbsa::detail::hash_tes4_name("hero", ".dds") == 0x8ddbaa2a6804f2efULL);
    CHECK(libbsa::detail::hash_tes4_name("marker", ".nif") == 0xc30342576d06e572ULL);
    CHECK(libbsa::detail::hash_tes4_name("voice", ".wav") == 0x97a2eb58f6056365ULL);
    CHECK(libbsa::detail::hash_tes4_name("anim", ".kf") == 0x1711e457610469edULL);
    CHECK(libbsa::detail::hash_tes4_path("textures\\actors\\hero.dds") == 0x3e02c5f07414f2efULL);
}

TEST_CASE("FO4 path part hashes match golden vectors", "[unit][golden]")
{
    // Provenance: TES5Edit/Core/wbBSArchive.pas CreateHashFO4.
    CHECK(libbsa::detail::hash_fo4_path_part("textures\\actors") == 0x809b60e0U);
    CHECK(libbsa::detail::hash_fo4_path_part("hero") == 0x708ab19aU);
    CHECK(libbsa::detail::hash_fo4_path_part("dds") == 0x8743edd9U);
    CHECK(libbsa::detail::hash_fo4_path_part("textures/actors") == 0x809b60e0U);
    CHECK(libbsa::detail::hash_fo4_path_part("Textures\\Actors") == 0x809b60e0U);
    CHECK(libbsa::detail::fo4_extension_magic("dds") == 0x00736464U);
}
