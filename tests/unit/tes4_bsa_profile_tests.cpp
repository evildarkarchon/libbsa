#include "formats/bsa/tes4_bsa_constants.hpp"
#include "formats/bsa/tes4_bsa_profile.hpp"

#include <libbsa/writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace {

using libbsa::formats::bsa::tes4_bsa_profile;

struct profile_expectation {
    std::uint32_t version;
    libbsa::tes4_bsa_target target;
    libbsa::formats::bsa::tes4_folder_record_shape folder_shape;
    std::size_t folder_record_size;
    libbsa::entry_compression compressed_entry_metadata;
    libbsa::detail::compression_method compressed_payload_method;
    bool target_default_compressed;
    bool supports_embedded_names;
};

constexpr std::array profile_matrix{
    profile_expectation{libbsa::formats::bsa::tes4_bsa_oblivion_version,
                        libbsa::tes4_bsa_target::oblivion,
                        libbsa::formats::bsa::tes4_folder_record_shape::legacy_32_bit_offset,
                        libbsa::formats::bsa::tes4_bsa_legacy_folder_record_size,
                        libbsa::entry_compression::deflate,
                        libbsa::detail::compression_method::deflate, false, false},
    profile_expectation{libbsa::formats::bsa::tes4_bsa_fallout3_version,
                        libbsa::tes4_bsa_target::fallout3,
                        libbsa::formats::bsa::tes4_folder_record_shape::legacy_32_bit_offset,
                        libbsa::formats::bsa::tes4_bsa_legacy_folder_record_size,
                        libbsa::entry_compression::deflate,
                        libbsa::detail::compression_method::deflate, true, true},
    profile_expectation{
        libbsa::formats::bsa::tes4_bsa_skyrim_se_version, libbsa::tes4_bsa_target::skyrim_se,
        libbsa::formats::bsa::tes4_folder_record_shape::sse_64_bit_offset,
        libbsa::formats::bsa::tes4_bsa_sse_folder_record_size, libbsa::entry_compression::lz4_frame,
        libbsa::detail::compression_method::lz4_frame, true, true},
};

libbsa::texture_metadata texture_with_format(std::uint32_t dxgi_format) {
    return libbsa::texture_metadata{.width = 1U,
                                    .height = 1U,
                                    .mip_count = 1U,
                                    .dxgi_format = dxgi_format,
                                    .array_size = 1U,
                                    .is_cubemap = false,
                                    .unknown_tex = 0U,
                                    .cube_maps_raw = 0U,
                                    .chunks = {}};
}

}  // namespace

static_assert(!std::is_default_constructible_v<tes4_bsa_profile>);
static_assert(!std::is_aggregate_v<tes4_bsa_profile>);

TEST_CASE("tes4_bsa_profile resolves every supported header version", "[unit][tes4_bsa_profile]") {
    for (const auto& expected : profile_matrix) {
        auto profile = libbsa::formats::bsa::make_tes4_bsa_profile_from_header(expected.version);

        REQUIRE(profile.has_value());
        CHECK(profile.value().variant() == libbsa::archive_variant::tes4);
        CHECK(profile.value().version() == expected.version);
        CHECK(profile.value().folder_record_shape() == expected.folder_shape);
        CHECK(profile.value().folder_record_size() == expected.folder_record_size);
        CHECK(profile.value().compressed_entry_metadata() == expected.compressed_entry_metadata);
        CHECK(profile.value().compressed_payload_method() == expected.compressed_payload_method);
    }
}

TEST_CASE("tes4_bsa_profile resolves every public writer target",
          "[unit][tes4_bsa_profile][writer]") {
    for (const auto& expected : profile_matrix) {
        auto profile = libbsa::formats::bsa::make_tes4_bsa_profile_for_writer(expected.target);

        REQUIRE(profile.has_value());
        CHECK(profile.value().version() == expected.version);
        CHECK(profile.value().folder_record_shape() == expected.folder_shape);
        CHECK(profile.value().folder_record_size() == expected.folder_record_size);
        CHECK(profile.value().compressed_entry_metadata() == expected.compressed_entry_metadata);
        CHECK(profile.value().compressed_payload_method() == expected.compressed_payload_method);
        CHECK(profile.value().archive_default_compressed(
                  libbsa::archive_compression_policy::target_default) ==
              expected.target_default_compressed);
    }
}

TEST_CASE("tes4_bsa_profile preserves invalid resolution diagnostics", "[unit][tes4_bsa_profile]") {
    auto unsupported = libbsa::formats::bsa::make_tes4_bsa_profile_from_header(106U);
    REQUIRE_FALSE(unsupported.has_value());
    CHECK(unsupported.error().code == libbsa::error_code::unsupported);
    CHECK(unsupported.error().message == "BSA header version is not supported");

    auto invalid_target = libbsa::formats::bsa::make_tes4_bsa_profile_for_writer(
        static_cast<libbsa::tes4_bsa_target>(999));
    REQUIRE_FALSE(invalid_target.has_value());
    CHECK(invalid_target.error().code == libbsa::error_code::invalid_argument);
    CHECK(invalid_target.error().message == "TES4 BSA writer target profile is not supported");
}

TEST_CASE("tes4_bsa_profile interprets archive and entry compression policies",
          "[unit][tes4_bsa_profile][compression]") {
    struct archive_policy_expectation {
        libbsa::archive_compression_policy policy;
        bool oblivion_default;
        bool later_default;
    };
    constexpr std::array archive_policies{
        archive_policy_expectation{libbsa::archive_compression_policy::target_default, false, true},
        archive_policy_expectation{libbsa::archive_compression_policy::all_raw, false, false},
        archive_policy_expectation{libbsa::archive_compression_policy::all_compressed, true, true},
    };

    for (const auto& expected : profile_matrix) {
        auto resolved = libbsa::formats::bsa::make_tes4_bsa_profile_from_header(expected.version);
        REQUIRE(resolved.has_value());
        const auto& profile = resolved.value();

        for (const auto& archive : archive_policies) {
            const bool expected_default =
                expected.version == libbsa::formats::bsa::tes4_bsa_oblivion_version
                    ? archive.oblivion_default
                    : archive.later_default;
            CHECK(profile.archive_default_compressed(archive.policy) == expected_default);

            const auto inherited = profile.writer_entry_compression(
                archive.policy, libbsa::entry_compression_policy::inherit, 8U);
            CHECK(inherited.compression == (expected_default ? expected.compressed_entry_metadata
                                                             : libbsa::entry_compression::none));
            CHECK(inherited.record_flags == 0U);

            const auto raw = profile.writer_entry_compression(
                archive.policy, libbsa::entry_compression_policy::raw, 8U);
            CHECK(raw.compression == libbsa::entry_compression::none);
            CHECK(raw.record_flags ==
                  (expected_default ? libbsa::formats::bsa::tes4_bsa_file_size_compression_toggle
                                    : 0U));

            const auto compressed = profile.writer_entry_compression(
                archive.policy, libbsa::entry_compression_policy::compressed, 8U);
            CHECK(compressed.compression == expected.compressed_entry_metadata);
            CHECK(compressed.record_flags ==
                  (expected_default ? 0U
                                    : libbsa::formats::bsa::tes4_bsa_file_size_compression_toggle));

            const auto empty = profile.writer_entry_compression(
                archive.policy, libbsa::entry_compression_policy::compressed, 0U);
            CHECK(empty.compression == libbsa::entry_compression::none);
            CHECK(empty.record_flags ==
                  (expected_default ? libbsa::formats::bsa::tes4_bsa_file_size_compression_toggle
                                    : 0U));
        }
    }
}

TEST_CASE("tes4_bsa_profile owns reader XOR and writer toggle semantics",
          "[unit][tes4_bsa_profile][compression]") {
    for (const auto& expected : profile_matrix) {
        auto resolved = libbsa::formats::bsa::make_tes4_bsa_profile_from_header(expected.version);
        REQUIRE(resolved.has_value());
        const auto& profile = resolved.value();

        CHECK(profile.reader_entry_compression(0x8000U, 0x2000U) ==
              libbsa::entry_compression::none);
        CHECK(profile.reader_entry_compression(
                  libbsa::formats::bsa::tes4_bsa_archive_compress_by_default | 0x8000U, 0x2000U) ==
              expected.compressed_entry_metadata);
        CHECK(profile.reader_entry_compression(
                  0x8000U, libbsa::formats::bsa::tes4_bsa_file_size_compression_toggle | 0x2000U) ==
              expected.compressed_entry_metadata);
        CHECK(profile.reader_entry_compression(
                  libbsa::formats::bsa::tes4_bsa_archive_compress_by_default | 0x8000U,
                  libbsa::formats::bsa::tes4_bsa_file_size_compression_toggle | 0x2000U) ==
              libbsa::entry_compression::none);

        const auto empty =
            profile.writer_entry_compression(libbsa::archive_compression_policy::all_compressed,
                                             libbsa::entry_compression_policy::inherit, 0U);
        CHECK(empty.compression == libbsa::entry_compression::none);
        CHECK(empty.record_flags == libbsa::formats::bsa::tes4_bsa_file_size_compression_toggle);
    }
}

TEST_CASE("tes4_bsa_profile applies effective embedded-name policy",
          "[unit][tes4_bsa_profile][embedded-names]") {
    for (const auto& expected : profile_matrix) {
        auto resolved = libbsa::formats::bsa::make_tes4_bsa_profile_from_header(expected.version);
        REQUIRE(resolved.has_value());
        const auto& profile = resolved.value();

        CHECK(
            profile.reader_has_embedded_names(libbsa::formats::bsa::tes4_bsa_archive_embed_names) ==
            expected.supports_embedded_names);
        CHECK_FALSE(profile.reader_has_embedded_names(0U));
        libbsa::tes4_bsa_writer_options requested;
        requested.embed_file_names = true;
        CHECK(profile.writer_emits_embedded_names(requested) == expected.supports_embedded_names);
        libbsa::tes4_bsa_writer_options disabled;
        disabled.embed_file_names = false;
        CHECK_FALSE(profile.writer_emits_embedded_names(disabled));
    }
}

TEST_CASE("tes4_bsa_profile classifies file flags by version",
          "[unit][tes4_bsa_profile][file-flags]") {
    for (const auto& expected : profile_matrix) {
        auto resolved = libbsa::formats::bsa::make_tes4_bsa_profile_from_header(expected.version);
        REQUIRE(resolved.has_value());
        const auto& profile = resolved.value();

        CHECK(profile.file_flag_for_path("Meshes\\Armor.NIF") ==
              libbsa::formats::bsa::tes4_bsa_file_flag_meshes);
        CHECK(profile.file_flag_for_path("Meshes\\Animation.KF") ==
              libbsa::formats::bsa::tes4_bsa_file_flag_meshes);
        CHECK(profile.file_flag_for_path("Textures/A.DDS") ==
              libbsa::formats::bsa::tes4_bsa_file_flag_textures);
        CHECK(profile.file_flag_for_path("Sound/A.WAV") ==
              libbsa::formats::bsa::tes4_bsa_file_flag_sounds);
        CHECK(profile.file_flag_for_path("Scripts/A.PEX") ==
              libbsa::formats::bsa::tes4_bsa_file_flag_scripts);
        CHECK(profile.file_flag_for_path("Scripts/A.PSC") ==
              libbsa::formats::bsa::tes4_bsa_file_flag_scripts);
        CHECK(profile.file_flag_for_path("Menus/A.XML") ==
              (expected.version == libbsa::formats::bsa::tes4_bsa_oblivion_version
                   ? libbsa::formats::bsa::tes4_bsa_file_flag_menus
                   : 0U));
        CHECK(profile.file_flag_for_path("Docs/A.TXT") ==
              (expected.version == libbsa::formats::bsa::tes4_bsa_skyrim_se_version
                   ? 0U
                   : libbsa::formats::bsa::tes4_bsa_file_flag_misc));
        for (const std::string_view path : {"Docs/A.HTML", "Docs/A.BAT", "Docs/A.SCC"}) {
            CHECK(profile.file_flag_for_path(path) ==
                  (expected.version == libbsa::formats::bsa::tes4_bsa_skyrim_se_version
                       ? 0U
                       : libbsa::formats::bsa::tes4_bsa_file_flag_misc));
        }
        CHECK(profile.file_flag_for_path("Folder.with.dot/A") == 0U);
        CHECK(profile.file_flag_for_path("Unknown/A.BIN") == 0U);
    }
}

TEST_CASE("tes4_bsa_profile validates native DDS metadata against target allowlists",
          "[unit][tes4_bsa_profile][dds]") {
    constexpr std::array legacy_dxt_formats{71U, 74U, 77U};
    constexpr std::array analyzable_but_disallowed_formats{28U, 61U, 65U, 87U, 88U};
    constexpr std::array skyrim_se_added_formats{80U, 83U, 98U};

    for (const auto& expected : profile_matrix) {
        auto resolved = libbsa::formats::bsa::make_tes4_bsa_profile_from_header(expected.version);
        REQUIRE(resolved.has_value());
        const auto& profile = resolved.value();

        for (const auto format : legacy_dxt_formats) {
            CHECK(profile.validate_texture_metadata(texture_with_format(format)).has_value());
        }
        for (const auto format : analyzable_but_disallowed_formats) {
            CHECK_FALSE(profile.validate_texture_metadata(texture_with_format(format)).has_value());
        }
        for (const auto format : skyrim_se_added_formats) {
            const auto validation = profile.validate_texture_metadata(texture_with_format(format));
            CHECK(validation.has_value() ==
                  (expected.version == libbsa::formats::bsa::tes4_bsa_skyrim_se_version));
        }

        auto unsupported = profile.validate_texture_metadata(texture_with_format(99U));
        REQUIRE_FALSE(unsupported.has_value());
        CHECK(unsupported.error().code == libbsa::error_code::format_error);
        CHECK(unsupported.error().message ==
              (expected.version == libbsa::formats::bsa::tes4_bsa_skyrim_se_version
                   ? "Skyrim SE BSA target supports the same DDS texture format set as Fallout 4"
                   : "TES4-family BSA target supports only DX9 DDS texture formats before Skyrim "
                     "SE"));
    }
}
