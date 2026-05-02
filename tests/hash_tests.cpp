#include "tes4_hash.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

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
