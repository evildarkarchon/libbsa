#include <catch2/catch_test_macros.hpp>

#include <detail/archive_path.hpp>

#include <array>
#include <string>
#include <string_view>

TEST_CASE("archive_path normalizes separators and ASCII case", "[unit][archive-path]") {
    auto key = libbsa::detail::normalize_archive_path("Meshes\\Foo/BAR.NIF");

    REQUIRE(key);
    REQUIRE(key.value().value == "meshes/foo/bar.nif");
}

TEST_CASE("archive_path rejects obvious invalid virtual paths", "[unit][archive-path][malformed]") {
    constexpr auto invalid_paths = std::to_array<std::string_view>({
        "",
        "/absolute/path",
        "C:/drive/rooted",
        "textures//bad.dds",
        "textures/./bad.dds",
        "textures/../bad.dds",
    });

    for (const auto input : invalid_paths) {
        auto key = libbsa::detail::normalize_archive_path(input);
        REQUIRE_FALSE(key);
        REQUIRE(key.error().code == libbsa::error_code::invalid_argument);
    }
}

TEST_CASE("archive_path rejects embedded-NUL virtual paths", "[unit][archive-path][malformed]") {
    const std::string input{"Meshes/A.nif\0Suffix", 19U};

    auto key = libbsa::detail::normalize_archive_path(input);

    REQUIRE_FALSE(key.has_value());
    REQUIRE(key.error().code == libbsa::error_code::invalid_argument);
}
