#include "formats/ba2/ba2_dx10_serialize.hpp"
#include "formats/ba2/ba2_gnrl_serialize.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <catch2/catch_test_macros.hpp>
#include <libbsa/writer.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<std::byte> literal_bytes(std::initializer_list<std::uint8_t> values) {
    std::vector<std::byte> bytes;
    bytes.reserve(values.size());
    for (const auto value : values) {
        bytes.push_back(static_cast<std::byte>(value));
    }
    return bytes;
}

std::filesystem::path serialization_output_path(std::string name) {
    auto directory =
        std::filesystem::temp_directory_path() / "libbsa_ba2_archive_serialization_tests";
    std::filesystem::create_directories(directory);
    return directory / std::move(name);
}

std::vector<std::byte> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    REQUIRE(input.good());

    std::vector<std::byte> bytes;
    for (char ch = 0; input.get(ch);) {
        bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    REQUIRE_FALSE(input.bad());
    return bytes;
}

libbsa::formats::ba2::ba2_profile require_gnrl_profile(
    libbsa::ba2_gnrl_target target, const libbsa::ba2_gnrl_writer_options& options = {}) {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_gnrl_writer(target, options);
    REQUIRE(profile.has_value());
    return profile.value();
}

libbsa::formats::ba2::ba2_profile require_dx10_profile(
    libbsa::ba2_dx10_target target, const libbsa::ba2_dx10_writer_options& options = {}) {
    auto profile = libbsa::formats::ba2::make_ba2_profile_for_dx10_writer(target, options);
    REQUIRE(profile.has_value());
    return profile.value();
}

}  // namespace

TEST_CASE("BA2 Archive Serialization emits complete GNRL bytes in Placement Plan order",
          "[unit][ba2_archive_serialization][gnrl][successful-output]") {
    libbsa::formats::ba2::ba2_gnrl_placement_plan plan;
    plan.records = {
        {"Meshes/A.bin",
         {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}},
         0x1122'3344U,
         0x5566'7788U,
         0x99AA'BBCCU,
         3U,
         7U,
         1U},
        {"Textures/B.dds",
         {std::byte{0x64}, std::byte{0x64}, std::byte{0x73}, std::byte{0x00}},
         0x0102'0304U,
         0xA0B0'C0D0U,
         0x1020'3040U,
         0U,
         2U,
         0U},
        {"Shared/C.bin",
         {std::byte{0x62}, std::byte{0x69}, std::byte{0x6E}, std::byte{0x00}},
         0x0A0B'0C0DU,
         0x1021'3243U,
         0U,
         3U,
         3U,
         1U},
    };
    plan.payloads.push_back(libbsa::formats::ba2::ba2_gnrl_payload_placement{
        144U, 2U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xA1, 0xA2}))});
    plan.payloads.push_back(libbsa::formats::ba2::ba2_gnrl_payload_placement{
        146U, 3U,
        libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xB1, 0xB2, 0xB3}))});
    plan.filename_table_offset = 149U;

    const auto profile = require_gnrl_profile(libbsa::ba2_gnrl_target::starfield_v3);
    const auto output = serialization_output_path("gnrl-complete-bytes.ba2");
    auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
        profile, libbsa::ba2_gnrl_writer_options{}, plan, output);

    REQUIRE(serialized.has_value());
    const auto expected = literal_bytes({
        // Starfield v3 fixed header, including the profile-gated 1/0/3 defaults.
        0x42,
        0x54,
        0x44,
        0x58,
        0x03,
        0x00,
        0x00,
        0x00,
        0x47,
        0x4E,
        0x52,
        0x4C,
        0x03,
        0x00,
        0x00,
        0x00,
        0x95,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x03,
        0x00,
        0x00,
        0x00,
        // Three complete GNRL records. Records one and three retain their shared
        // reference to payload index one even though payload zero is emitted first.
        0x44,
        0x33,
        0x22,
        0x11,
        0x62,
        0x69,
        0x6E,
        0x00,
        0x88,
        0x77,
        0x66,
        0x55,
        0xCC,
        0xBB,
        0xAA,
        0x99,
        0x92,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x03,
        0x00,
        0x00,
        0x00,
        0x07,
        0x00,
        0x00,
        0x00,
        0x0D,
        0xF0,
        0xAD,
        0xBA,
        0x04,
        0x03,
        0x02,
        0x01,
        0x64,
        0x64,
        0x73,
        0x00,
        0xD0,
        0xC0,
        0xB0,
        0xA0,
        0x40,
        0x30,
        0x20,
        0x10,
        0x90,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x0D,
        0xF0,
        0xAD,
        0xBA,
        0x0D,
        0x0C,
        0x0B,
        0x0A,
        0x62,
        0x69,
        0x6E,
        0x00,
        0x43,
        0x32,
        0x21,
        0x10,
        0x00,
        0x00,
        0x00,
        0x00,
        0x92,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x03,
        0x00,
        0x00,
        0x00,
        0x03,
        0x00,
        0x00,
        0x00,
        0x0D,
        0xF0,
        0xAD,
        0xBA,
        // Unique Stored Payloads in Placement Plan order, followed by all names.
        0xA1,
        0xA2,
        0xB1,
        0xB2,
        0xB3,
        0x0C,
        0x00,
        0x4D,
        0x65,
        0x73,
        0x68,
        0x65,
        0x73,
        0x2F,
        0x41,
        0x2E,
        0x62,
        0x69,
        0x6E,
        0x0E,
        0x00,
        0x54,
        0x65,
        0x78,
        0x74,
        0x75,
        0x72,
        0x65,
        0x73,
        0x2F,
        0x42,
        0x2E,
        0x64,
        0x64,
        0x73,
        0x0C,
        0x00,
        0x53,
        0x68,
        0x61,
        0x72,
        0x65,
        0x64,
        0x2F,
        0x43,
        0x2E,
        0x62,
        0x69,
        0x6E,
    });
    CHECK(read_binary_file(output) == expected);
}

TEST_CASE("BA2 Archive Serialization emits complete DX10 bytes in Placement Plan order",
          "[unit][ba2_archive_serialization][dx10][successful-output]") {
    libbsa::formats::ba2::ba2_dx10_placement_plan plan;
    plan.records = {
        {"Textures/A.dds",
         {std::byte{0x64}, std::byte{0x64}, std::byte{0x73}, std::byte{0x00}},
         0x1122'3344U,
         0x5566'7788U,
         0x9AU,
         2U,
         64U,
         128U,
         3U,
         28U,
         2048U,
         {{2U, 5U, 0U, 1U, libbsa::detail::compression_method::zlib, 1U},
          {0U, 3U, 2U, 2U, libbsa::detail::compression_method::zlib, 0U}}},
        {"Textures/B.dds",
         {std::byte{0x64}, std::byte{0x64}, std::byte{0x73}, std::byte{0x00}},
         0x0102'0304U,
         0xA0B0'C0D0U,
         0U,
         1U,
         32U,
         32U,
         1U,
         71U,
         2049U,
         {{2U, 5U, 0U, 0U, libbsa::detail::compression_method::zlib, 1U}}},
    };
    plan.payloads.push_back(libbsa::formats::ba2::ba2_dx10_payload_placement{
        156U, 3U,
        libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xC1, 0xC2, 0xC3}))});
    plan.payloads.push_back(libbsa::formats::ba2::ba2_dx10_payload_placement{
        159U, 2U, libbsa::detail::stored_payload::from_owned_bytes(literal_bytes({0xD1, 0xD2}))});
    plan.filename_table_offset = 161U;

    const auto profile = require_dx10_profile(libbsa::ba2_dx10_target::starfield_v3);
    const auto output = serialization_output_path("dx10-complete-bytes.ba2");
    auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
        profile, libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan, output);

    REQUIRE(serialized.has_value());
    const auto expected = literal_bytes({
        // Starfield v3 fixed header, including the profile-gated 1/0/3 defaults.
        0x42,
        0x54,
        0x44,
        0x58,
        0x03,
        0x00,
        0x00,
        0x00,
        0x44,
        0x58,
        0x31,
        0x30,
        0x02,
        0x00,
        0x00,
        0x00,
        0xA1,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x03,
        0x00,
        0x00,
        0x00,
        // First texture record and both of its complete chunk-table entries.
        0x44,
        0x33,
        0x22,
        0x11,
        0x64,
        0x64,
        0x73,
        0x00,
        0x88,
        0x77,
        0x66,
        0x55,
        0x9A,
        0x02,
        0x18,
        0x00,
        0x40,
        0x00,
        0x80,
        0x00,
        0x03,
        0x1C,
        0x00,
        0x08,
        0x9F,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x05,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x0D,
        0xF0,
        0xAD,
        0xBA,
        0x9C,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x03,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x02,
        0x00,
        0x0D,
        0xF0,
        0xAD,
        0xBA,
        // Second texture record and its shared reference to payload index one.
        0x04,
        0x03,
        0x02,
        0x01,
        0x64,
        0x64,
        0x73,
        0x00,
        0xD0,
        0xC0,
        0xB0,
        0xA0,
        0x00,
        0x01,
        0x18,
        0x00,
        0x20,
        0x00,
        0x20,
        0x00,
        0x01,
        0x47,
        0x01,
        0x08,
        0x9F,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x05,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x0D,
        0xF0,
        0xAD,
        0xBA,
        // Unique Stored Payloads in Placement Plan order, followed by all names.
        0xC1,
        0xC2,
        0xC3,
        0xD1,
        0xD2,
        0x0E,
        0x00,
        0x54,
        0x65,
        0x78,
        0x74,
        0x75,
        0x72,
        0x65,
        0x73,
        0x2F,
        0x41,
        0x2E,
        0x64,
        0x64,
        0x73,
        0x0E,
        0x00,
        0x54,
        0x65,
        0x78,
        0x74,
        0x75,
        0x72,
        0x65,
        0x73,
        0x2F,
        0x42,
        0x2E,
        0x64,
        0x64,
        0x73,
    });
    CHECK(read_binary_file(output) == expected);
}

TEST_CASE("BA2 Archive Serialization gates GNRL stored header defaults by profile",
          "[unit][ba2_archive_serialization][gnrl][successful-output]") {
    struct header_case {
        libbsa::ba2_gnrl_target target;
        std::uint64_t filename_table_offset;
        std::vector<std::byte> expected;
        std::string output_name;
    };
    const std::array cases{
        header_case{
            libbsa::ba2_gnrl_target::fallout4, 24U,
            literal_bytes({0x42, 0x54, 0x44, 0x58, 0x01, 0x00, 0x00, 0x00, 0x47, 0x4E, 0x52, 0x4C,
                           0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}),
            "gnrl-fallout4-header.ba2"},
        header_case{libbsa::ba2_gnrl_target::starfield_v2, 32U,
                    literal_bytes({0x42, 0x54, 0x44, 0x58, 0x02, 0x00, 0x00, 0x00, 0x47, 0x4E, 0x52,
                                   0x4C, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}),
                    "gnrl-starfield-v2-header.ba2"},
        header_case{
            libbsa::ba2_gnrl_target::starfield_v3, 36U,
            literal_bytes({0x42, 0x54, 0x44, 0x58, 0x03, 0x00, 0x00, 0x00, 0x47, 0x4E, 0x52, 0x4C,
                           0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00}),
            "gnrl-starfield-v3-header.ba2"},
    };

    for (const auto& test_case : cases) {
        libbsa::formats::ba2::ba2_gnrl_placement_plan plan;
        plan.filename_table_offset = test_case.filename_table_offset;
        const auto profile = require_gnrl_profile(test_case.target);
        const auto output = serialization_output_path(test_case.output_name);

        auto serialized = libbsa::formats::ba2::ba2_gnrl_write_archive_bytes(
            profile, libbsa::ba2_gnrl_writer_options{}, plan, output);

        REQUIRE(serialized.has_value());
        CHECK(read_binary_file(output) == test_case.expected);
    }
}

TEST_CASE("BA2 Archive Serialization gates DX10 stored header defaults by profile",
          "[unit][ba2_archive_serialization][dx10][successful-output]") {
    struct header_case {
        libbsa::ba2_dx10_target target;
        std::uint64_t filename_table_offset;
        std::vector<std::byte> expected;
        std::string output_name;
    };
    const std::array cases{
        header_case{
            libbsa::ba2_dx10_target::fallout4, 24U,
            literal_bytes({0x42, 0x54, 0x44, 0x58, 0x01, 0x00, 0x00, 0x00, 0x44, 0x58, 0x31, 0x30,
                           0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}),
            "dx10-fallout4-header.ba2"},
        header_case{libbsa::ba2_dx10_target::starfield_v2, 32U,
                    literal_bytes({0x42, 0x54, 0x44, 0x58, 0x02, 0x00, 0x00, 0x00, 0x44, 0x58, 0x31,
                                   0x30, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}),
                    "dx10-starfield-v2-header.ba2"},
        header_case{
            libbsa::ba2_dx10_target::starfield_v3, 36U,
            literal_bytes({0x42, 0x54, 0x44, 0x58, 0x03, 0x00, 0x00, 0x00, 0x44, 0x58, 0x31, 0x30,
                           0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00}),
            "dx10-starfield-v3-header.ba2"},
    };

    for (const auto& test_case : cases) {
        libbsa::formats::ba2::ba2_dx10_placement_plan plan;
        plan.filename_table_offset = test_case.filename_table_offset;
        const auto profile = require_dx10_profile(test_case.target);
        const auto output = serialization_output_path(test_case.output_name);

        auto serialized = libbsa::formats::ba2::ba2_dx10_write_archive_bytes(
            profile, libbsa::formats::ba2::ba2_dx10_stored_header_options{}, plan, output);

        REQUIRE(serialized.has_value());
        CHECK(read_binary_file(output) == test_case.expected);
    }
}
