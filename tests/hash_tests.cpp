#include "tes4_hash.hpp"
#include "tes3_hash.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

namespace {

struct HashVector {
    const char* name;
    const char* extension;
    std::uint64_t expected;
};

} // namespace

TEST_CASE("TES4 hash matches BSArchPro vectors")
{
    const HashVector vectors[] = {
        {"meshes\\actors", "", 0xBB1653B16D0D7273ull},
        {"skeleton", ".nif", 0x875CFA7E7308EF6Eull},
        {"idle", ".kf", 0x1711E44D69046CE5ull},
        {"diffuse", ".dds", 0xD5C78F116407F3E5ull},
        {"sound", ".wav", 0x97A2EB64F3056E64ull},
        {"readme", ".txt", 0xC7EDDCEA72066D65ull},
    };

    for (const auto& vector : vectors) {
        INFO(vector.name << vector.extension);
        CHECK(libbsa::detail::hash_tes4(vector.name, vector.extension) == vector.expected);
    }
}

TEST_CASE("TES4 file hash splits the final extension")
{
    CHECK(libbsa::detail::hash_tes4_file("textures\\stone.dds") == 0xEAE77943740EEEE5ull);
    CHECK(libbsa::detail::hash_tes4_file("stone.dds") == 0x8E4FC6C07305EEE5ull);
}

TEST_CASE("TES3 hash matches BSArchPro vectors")
{
    const HashVector vectors[] = {
        {"meshes\\x", ".nif", 0x68731608D4B7713Eull},
        {"textures\\stone", ".dds", 0x071D175DE4DA09E2ull},
        {"sound\\fx\\hit", ".wav", 0x16133317C1D8A6A5ull},
        {"a", "", 0x0000000080000030ull},
        {"ab", "", 0x0000006180000018ull},
        {"abc", "", 0x0000006180006318ull},
        {"MESHES\\X", ".NIF", 0x68731608D4B7713Eull},
    };

    for (const auto& vector : vectors) {
        const std::string filename = std::string(vector.name) + vector.extension;
        INFO(filename);
        CHECK(libbsa::detail::hash_tes3(filename) == vector.expected);
    }
}

TEST_CASE("TES3 hash lowercases ASCII bytes only")
{
    std::string non_ascii;
    non_ascii.push_back(static_cast<char>(0xC0));
    non_ascii.push_back('A');
    non_ascii.push_back('Z');

    std::string lower_non_ascii;
    lower_non_ascii.push_back(static_cast<char>(0xC0));
    lower_non_ascii.push_back('a');
    lower_non_ascii.push_back('z');

    CHECK(libbsa::detail::hash_tes3(non_ascii) == 0x000000C080007A30ull);
    CHECK(libbsa::detail::hash_tes3(non_ascii) == libbsa::detail::hash_tes3(lower_non_ascii));
}
