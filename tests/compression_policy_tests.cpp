#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/compression.hpp>
#include <libbsa/result.hpp>

#include <cstdint>
#include <optional>

namespace {

libbsa::payload_codec_request request(libbsa::archive_format format,
                                      libbsa::compression_state state,
                                      std::optional<std::uint32_t> compression_method = std::nullopt)
{
    return libbsa::payload_codec_request{format, state, compression_method};
}

} // namespace

TEST_CASE("payload codec routing is explicit for archive formats and entry state", "[unit][codec]")
{
    const auto sse_frame = libbsa::resolve_payload_codec(
        request(libbsa::archive_format::sse_bsa, libbsa::compression_state::lz4_frame));
    REQUIRE(sse_frame.has_value());
    CHECK(sse_frame.value() == libbsa::compression_algorithm::lz4_frame);

    const auto starfield_lz4 = libbsa::resolve_payload_codec(
        request(libbsa::archive_format::starfield_ba2_gnrl,
                libbsa::compression_state::archive_default,
                3));
    REQUIRE(starfield_lz4.has_value());
    CHECK(starfield_lz4.value() == libbsa::compression_algorithm::lz4_block);

    const auto fo4_deflate = libbsa::resolve_payload_codec(
        request(libbsa::archive_format::fo4_ba2_gnrl, libbsa::compression_state::deflate));
    REQUIRE(fo4_deflate.has_value());
    CHECK(fo4_deflate.value() == libbsa::compression_algorithm::deflate);

    const auto starfield_deflate = libbsa::resolve_payload_codec(
        request(libbsa::archive_format::starfield_ba2_gnrl,
                libbsa::compression_state::archive_default,
                0));
    REQUIRE(starfield_deflate.has_value());
    CHECK(starfield_deflate.value() == libbsa::compression_algorithm::deflate);
}

TEST_CASE("unsupported payload routes return structured failures", "[unit][codec]")
{
    const auto routed = libbsa::resolve_payload_codec(
        request(libbsa::archive_format::fo4_ba2_gnrl, libbsa::compression_state::lz4_block));

    REQUIRE_FALSE(routed.has_value());
    CHECK(routed.error().code == libbsa::error_code::unsupported_format);
    CHECK(routed.error().message == "unsupported compression route");
}

TEST_CASE("writer compression policy resolves to archive-specific states", "[unit][codec]")
{
    const auto sse_compressed = libbsa::resolve_write_compression(
        libbsa::archive_format::sse_bsa, libbsa::compression_policy::force_compressed, false);
    REQUIRE(sse_compressed.has_value());
    CHECK(sse_compressed.value() == libbsa::compression_state::lz4_frame);

    const auto fo4_raw = libbsa::resolve_write_compression(
        libbsa::archive_format::fo4_ba2_gnrl, libbsa::compression_policy::force_raw, true);
    REQUIRE(fo4_raw.has_value());
    CHECK(fo4_raw.value() == libbsa::compression_state::raw);
}
