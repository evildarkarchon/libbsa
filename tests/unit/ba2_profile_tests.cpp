#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_profile.hpp"

#include <libbsa/writer.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ba2_profile maps detected header versions to profile facts",
          "[unit][ba2_profile]") {
    auto fallout4 =
        libbsa::formats::ba2::make_ba2_profile_from_header(
            libbsa::formats::ba2::ba2_fallout4_version, libbsa::formats::ba2::ba2_subtype::gnrl,
            {});

    REQUIRE(fallout4.has_value());
    CHECK(fallout4.value().variant() == libbsa::archive_variant::fallout4);
    CHECK(fallout4.value().is_gnrl());
    CHECK(fallout4.value().version() == libbsa::formats::ba2::ba2_fallout4_version);
    CHECK(fallout4.value().header_size() == libbsa::formats::ba2::ba2_common_header_size);
    CHECK(fallout4.value().default_compression() == libbsa::entry_compression::deflate);
    CHECK(fallout4.value().compressed_payload_method() ==
          libbsa::detail::compression_method::deflate);

    libbsa::ba2_archive_metadata starfield_v2_metadata;
    starfield_v2_metadata.starfield_unknown1 = 7U;
    starfield_v2_metadata.starfield_unknown2 = 9U;
    auto starfield_v2 = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_starfield_v2_version,
        libbsa::formats::ba2::ba2_subtype::gnrl, starfield_v2_metadata);

    REQUIRE(starfield_v2.has_value());
    CHECK(starfield_v2.value().variant() == libbsa::archive_variant::starfield);
    CHECK(starfield_v2.value().header_size() ==
          libbsa::formats::ba2::ba2_starfield_v2_header_size);
    REQUIRE(starfield_v2.value().ba2_metadata().starfield_unknown1.has_value());
    CHECK(*starfield_v2.value().ba2_metadata().starfield_unknown1 == 7U);
    CHECK(starfield_v2.value().compressed_payload_method() ==
          libbsa::detail::compression_method::deflate);
}

TEST_CASE("ba2_profile maps Starfield v3 compression methods",
          "[unit][ba2_profile][starfield]") {
    libbsa::ba2_archive_metadata deflate_metadata;
    deflate_metadata.starfield_unknown1 = 1U;
    deflate_metadata.starfield_unknown2 = 0U;
    deflate_metadata.compression_method = libbsa::formats::ba2::ba2_starfield_compression_deflate;

    auto deflate = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_starfield_v3_version,
        libbsa::formats::ba2::ba2_subtype::dx10, deflate_metadata);

    REQUIRE(deflate.has_value());
    CHECK(deflate.value().is_dx10());
    CHECK(deflate.value().header_size() ==
          libbsa::formats::ba2::ba2_starfield_v3_header_size);
    CHECK(deflate.value().default_compression() == libbsa::entry_compression::deflate);
    CHECK(deflate.value().compressed_payload_method() ==
          libbsa::detail::compression_method::deflate);

    auto lz4_metadata = deflate_metadata;
    lz4_metadata.compression_method = libbsa::formats::ba2::ba2_starfield_compression_lz4_block;
    auto lz4 = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_starfield_v3_version,
        libbsa::formats::ba2::ba2_subtype::dx10, lz4_metadata);

    REQUIRE(lz4.has_value());
    CHECK(lz4.value().default_compression() == libbsa::entry_compression::lz4_block);
    CHECK(lz4.value().compressed_payload_method() ==
          libbsa::detail::compression_method::lz4_block);

    auto unsupported_metadata = deflate_metadata;
    unsupported_metadata.compression_method = 99U;
    auto unsupported = libbsa::formats::ba2::make_ba2_profile_from_header(
        libbsa::formats::ba2::ba2_starfield_v3_version,
        libbsa::formats::ba2::ba2_subtype::dx10, unsupported_metadata);

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

TEST_CASE("ba2_profile maps public BA2 compression metadata to codec methods",
          "[unit][ba2_profile][reader]") {
    auto deflate = libbsa::formats::ba2::ba2_compressed_payload_method(
        libbsa::formats::ba2::ba2_subtype::gnrl, libbsa::entry_compression::deflate);
    auto lz4 = libbsa::formats::ba2::ba2_compressed_payload_method(
        libbsa::formats::ba2::ba2_subtype::dx10, libbsa::entry_compression::lz4_block);

    REQUIRE(deflate.has_value());
    CHECK(deflate.value() == libbsa::detail::compression_method::deflate);
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
