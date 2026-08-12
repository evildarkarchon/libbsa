#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <libbsa/writer.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>

namespace {

/// One row of the expected BA2 fixed-header layout table.
struct header_layout_case {
    std::uint32_t version;
    std::size_t header_size;
    libbsa::archive_variant variant;
    bool has_starfield_unknown_fields;
    bool has_compression_method;
};

}  // namespace

TEST_CASE("ba2_header_layout_for_version pins every supported header width",
          "[unit][ba2_profile][fixed_header]") {
    // Widths verified against retail archives by locating the BAADF00D record
    // sentinel, and against wbBSArchive.pas, which groups versions 1, 7 and 8
    // under baFO4 and reads no trailing header fields for them. Versions 7 and
    // 8 sort above 2 and 3, so an ordered comparison would hand v7 the 36-byte
    // Starfield v3 width and misparse every record.
    const auto layouts = std::to_array<header_layout_case>({
        {libbsa::formats::ba2::ba2_fallout4_version, libbsa::formats::ba2::ba2_common_header_size,
         libbsa::archive_variant::fallout4, false, false},
        {libbsa::formats::ba2::ba2_starfield_v2_version,
         libbsa::formats::ba2::ba2_starfield_v2_header_size, libbsa::archive_variant::starfield,
         true, false},
        {libbsa::formats::ba2::ba2_starfield_v3_version,
         libbsa::formats::ba2::ba2_starfield_v3_header_size, libbsa::archive_variant::starfield,
         true, true},
        {libbsa::formats::ba2::ba2_fallout4_ng_v7_version,
         libbsa::formats::ba2::ba2_common_header_size, libbsa::archive_variant::fallout4, false,
         false},
        {libbsa::formats::ba2::ba2_fallout4_ng_v8_version,
         libbsa::formats::ba2::ba2_common_header_size, libbsa::archive_variant::fallout4, false,
         false},
    });

    for (const auto& expected : layouts) {
        INFO("BA2 header version " << expected.version);
        auto layout = libbsa::formats::ba2::ba2_header_layout_for_version(expected.version);

        REQUIRE(layout.has_value());
        CHECK(layout.value().header_size == expected.header_size);
        CHECK(layout.value().variant == expected.variant);
        CHECK(layout.value().has_starfield_unknown_fields == expected.has_starfield_unknown_fields);
        CHECK(layout.value().has_compression_method == expected.has_compression_method);
    }
}

TEST_CASE("ba2_header_layout_for_version rejects versions outside the supported set",
          "[unit][ba2_profile][fixed_header]") {
    // Versions adjacent to the supported tags must not be absorbed into a
    // neighbouring layout; each has to fail loudly until it is traced.
    constexpr auto unsupported =
        std::to_array<std::uint32_t>({0U, 4U, 5U, 6U, 9U, 0x67U, 0x68U, 0x69U, 0xFFFF'FFFFU});

    for (const auto version : unsupported) {
        INFO("BA2 header version " << version);
        auto layout = libbsa::formats::ba2::ba2_header_layout_for_version(version);

        REQUIRE_FALSE(layout.has_value());
        CHECK(layout.error().code == libbsa::error_code::unsupported);
        CHECK(layout.error().message == "BA2 header version is not supported");
    }
}

TEST_CASE("ba2_profile maps Fallout 4 next-gen versions to Fallout 4 deflate profiles",
          "[unit][ba2_profile][fallout4]") {
    // Retail Fallout 4 ships v7 DX10 and v8 GNRL/DX10 archives. Neither header
    // carries a CompressionMethod field, so compressed payloads are deflate and
    // the profile must not consult Starfield metadata to decide that.
    constexpr auto versions =
        std::to_array<std::uint32_t>({libbsa::formats::ba2::ba2_fallout4_ng_v7_version,
                                      libbsa::formats::ba2::ba2_fallout4_ng_v8_version});

    for (const auto version : versions) {
        INFO("BA2 header version " << version);
        auto gnrl = libbsa::formats::ba2::make_ba2_profile_from_header(
            version, libbsa::formats::ba2::ba2_subtype::gnrl, {});

        REQUIRE(gnrl.has_value());
        CHECK(gnrl.value().variant() == libbsa::archive_variant::fallout4);
        CHECK(gnrl.value().is_gnrl());
        CHECK(gnrl.value().version() == version);
        CHECK(gnrl.value().header_size() == libbsa::formats::ba2::ba2_common_header_size);
        CHECK(gnrl.value().default_compression() == libbsa::entry_compression::deflate);
        CHECK(gnrl.value().compressed_payload_method() == libbsa::detail::compression_method::zlib);

        auto dx10 = libbsa::formats::ba2::make_ba2_profile_from_header(
            version, libbsa::formats::ba2::ba2_subtype::dx10, {});

        REQUIRE(dx10.has_value());
        CHECK(dx10.value().variant() == libbsa::archive_variant::fallout4);
        CHECK(dx10.value().is_dx10());
        CHECK(dx10.value().header_size() == libbsa::formats::ba2::ba2_common_header_size);
        CHECK(dx10.value().compressed_payload_method() == libbsa::detail::compression_method::zlib);
    }
}

TEST_CASE("ba2_profile maps detected header versions to profile facts", "[unit][ba2_profile]") {
    auto fallout4 = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_fallout4_version, libbsa::formats::ba2::ba2_subtype::gnrl, {});

    REQUIRE(fallout4.has_value());
    CHECK(fallout4.value().variant() == libbsa::archive_variant::fallout4);
    CHECK(fallout4.value().is_gnrl());
    CHECK(fallout4.value().version() == libbsa::formats::ba2::ba2_fallout4_version);
    CHECK(fallout4.value().header_size() == libbsa::formats::ba2::ba2_common_header_size);
    CHECK(fallout4.value().default_compression() == libbsa::entry_compression::deflate);
    CHECK(fallout4.value().compressed_payload_method() == libbsa::detail::compression_method::zlib);

    libbsa::ba2_archive_metadata starfield_v2_metadata;
    starfield_v2_metadata.starfield_unknown1 = 7U;
    starfield_v2_metadata.starfield_unknown2 = 9U;
    auto starfield_v2 = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_starfield_v2_version, libbsa::formats::ba2::ba2_subtype::gnrl,
        starfield_v2_metadata);

    REQUIRE(starfield_v2.has_value());
    CHECK(starfield_v2.value().variant() == libbsa::archive_variant::starfield);
    CHECK(starfield_v2.value().header_size() == libbsa::formats::ba2::ba2_starfield_v2_header_size);
    CHECK(starfield_v2.value().compressed_payload_method() ==
          libbsa::detail::compression_method::zlib);
}

TEST_CASE("ba2_profile maps Starfield v3 compression methods", "[unit][ba2_profile][starfield]") {
    libbsa::ba2_archive_metadata deflate_metadata;
    deflate_metadata.starfield_unknown1 = 1U;
    deflate_metadata.starfield_unknown2 = 0U;
    deflate_metadata.compression_method = libbsa::formats::ba2::ba2_starfield_compression_deflate;

    auto deflate = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_starfield_v3_version, libbsa::formats::ba2::ba2_subtype::dx10,
        deflate_metadata);

    REQUIRE(deflate.has_value());
    CHECK(deflate.value().is_dx10());
    CHECK(deflate.value().header_size() == libbsa::formats::ba2::ba2_starfield_v3_header_size);
    CHECK(deflate.value().default_compression() == libbsa::entry_compression::deflate);
    CHECK(deflate.value().compressed_payload_method() == libbsa::detail::compression_method::zlib);

    auto lz4_metadata = deflate_metadata;
    lz4_metadata.compression_method = libbsa::formats::ba2::ba2_starfield_compression_lz4_block;
    auto lz4 = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_starfield_v3_version, libbsa::formats::ba2::ba2_subtype::dx10,
        lz4_metadata);

    REQUIRE(lz4.has_value());
    CHECK(lz4.value().default_compression() == libbsa::entry_compression::lz4_block);
    CHECK(lz4.value().compressed_payload_method() == libbsa::detail::compression_method::lz4_block);

    auto unsupported_metadata = deflate_metadata;
    unsupported_metadata.compression_method = 99U;
    auto unsupported = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_starfield_v3_version, libbsa::formats::ba2::ba2_subtype::dx10,
        unsupported_metadata);

    REQUIRE_FALSE(unsupported.has_value());
    CHECK(unsupported.error().code == libbsa::error_code::unsupported);
    CHECK(unsupported.error().message == "Starfield BA2 v3 CompressionMethod is unsupported");
}

TEST_CASE("ba2_profile writer factories preserve subtype-specific diagnostics",
          "[unit][ba2_profile][writer]") {
    libbsa::ba2_gnrl_writer_options gnrl_options;
    gnrl_options.starfield_compression_method = 99U;
    auto gnrl = libbsa::formats::ba2::make_ba2_profile_for_gnrl_writer(
        libbsa::ba2_gnrl_target::starfield_v3, gnrl_options);

    REQUIRE_FALSE(gnrl.has_value());
    CHECK(gnrl.error().code == libbsa::error_code::unsupported);
    CHECK(gnrl.error().message == "BA2 GNRL Starfield v3 compression method is unsupported");

    libbsa::ba2_dx10_writer_options dx10_options;
    dx10_options.starfield_compression_method = 99U;
    auto dx10 = libbsa::formats::ba2::make_ba2_profile_for_dx10_writer(
        libbsa::ba2_dx10_target::starfield_v3, dx10_options);

    REQUIRE_FALSE(dx10.has_value());
    CHECK(dx10.error().code == libbsa::error_code::unsupported);
    CHECK(dx10.error().message == "BA2 DX10 Starfield v3 compression method is unsupported");
}

TEST_CASE("ba2_profile builds Starfield v2 DX10 writers with fixed deflate semantics",
          "[unit][ba2_profile][writer][starfield]") {
    libbsa::ba2_dx10_writer_options options;
    options.starfield_compression_method = 99U;

    auto profile = libbsa::formats::ba2::make_ba2_profile_for_dx10_writer(
        libbsa::ba2_dx10_target::starfield_v2, options);

    REQUIRE(profile.has_value());
    CHECK(profile.value().variant() == libbsa::archive_variant::starfield);
    CHECK(profile.value().is_dx10());
    CHECK(profile.value().version() == libbsa::formats::ba2::ba2_starfield_v2_version);
    CHECK(profile.value().header_size() == libbsa::formats::ba2::ba2_starfield_v2_header_size);
    CHECK(profile.value().default_compression() == libbsa::entry_compression::deflate);
    CHECK(profile.value().compressed_payload_method() == libbsa::detail::compression_method::zlib);
}

TEST_CASE("ba2_profile maps public BA2 compression metadata to codec methods",
          "[unit][ba2_profile][reader]") {
    auto deflate = libbsa::formats::ba2::ba2_compressed_payload_method(
        libbsa::formats::ba2::ba2_subtype::gnrl, libbsa::entry_compression::deflate);
    auto lz4 = libbsa::formats::ba2::ba2_compressed_payload_method(
        libbsa::formats::ba2::ba2_subtype::dx10, libbsa::entry_compression::lz4_block);

    REQUIRE(deflate.has_value());
    CHECK(deflate.value() == libbsa::detail::compression_method::zlib);
    REQUIRE(lz4.has_value());
    CHECK(lz4.value() == libbsa::detail::compression_method::lz4_block);

    auto raw_gnrl = libbsa::formats::ba2::ba2_compressed_payload_method(
        libbsa::formats::ba2::ba2_subtype::gnrl, libbsa::entry_compression::none);
    REQUIRE_FALSE(raw_gnrl.has_value());
    CHECK(raw_gnrl.error().message == "BA2 GNRL raw entries must not enter decompression routing");

    auto frame_dx10 = libbsa::formats::ba2::ba2_compressed_payload_method(
        libbsa::formats::ba2::ba2_subtype::dx10, libbsa::entry_compression::lz4_frame);
    REQUIRE_FALSE(frame_dx10.has_value());
    CHECK(frame_dx10.error().message == "BA2 DX10 does not support lz4_frame chunk payloads");
}

TEST_CASE("ba2_profile rejects unsupported subtype magic", "[unit][ba2_profile]") {
    auto subtype = libbsa::formats::ba2::ba2_subtype_from_magic(0xFFFFFFFFU);

    REQUIRE_FALSE(subtype.has_value());
    CHECK(subtype.error().code == libbsa::error_code::unsupported);
    CHECK(subtype.error().message == "BA2 subtype is not GNRL or DX10");
}
