#include <catch2/catch_test_macros.hpp>

#include <libbsa/ba2.hpp>
#include <libbsa/io.hpp>

#include "ba2_dds_fixture_helpers.hpp"
#include "texture/dds_validation.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t MAGIC_DX10 = 0x30315844;

std::uint64_t dx10_file_table_offset(std::uint32_t version, std::uint32_t chunk_count)
{
    const auto header_size = version == libbsa::test::VERSION_STARFIELD_DX10_V3 ? 36U : 24U;
    return header_size + 24U + (static_cast<std::uint64_t>(chunk_count) * 24U);
}

libbsa::result<libbsa::ba2_archive> open_fixture(const libbsa::test::ba2_dds_fixture& fixture)
{
    const libbsa::memory_source source{std::span<const std::byte>{fixture.bytes}};
    return libbsa::open_ba2(source);
}

void assert_fo4_dx10_fixture(const libbsa::test::ba2_dds_fixture& fixture,
                             std::uint32_t version,
                             std::string path)
{
    const auto opened = open_fixture(fixture);
    REQUIRE(opened.has_value());
    CHECK(opened.value().summary().format == libbsa::archive_format::fo4_ba2_dds);
    REQUIRE(opened.value().summary().version.has_value());
    CHECK(*opened.value().summary().version == version);
    REQUIRE(opened.value().summary().subtype.has_value());
    CHECK(*opened.value().summary().subtype == MAGIC_DX10);
    REQUIRE(opened.value().summary().file_count.has_value());
    CHECK(*opened.value().summary().file_count == 1);
    REQUIRE(opened.value().summary().file_table_offset.has_value());
    CHECK(*opened.value().summary().file_table_offset == dx10_file_table_offset(version, 1));

    const auto paths = opened.value().paths();
    REQUIRE(paths.size() == 1);
    CHECK(paths.front().string() == path);
    CHECK(opened.value().contains(path));

    const auto entry = opened.value().entry(path);
    REQUIRE(entry.has_value());
    CHECK(entry.value().path == path);
    CHECK(entry.value().compression == libbsa::compression_state::raw);

    const auto texture = opened.value().texture_metadata(path);
    REQUIRE(texture.has_value());
    CHECK(texture.value().path == path);
    CHECK(texture.value().width == fixture.textures.front().width);
    CHECK(texture.value().height == fixture.textures.front().height);
    CHECK(texture.value().mip_count == fixture.textures.front().mip_count);
    CHECK(texture.value().array_size == fixture.textures.front().array_size);
    CHECK(texture.value().is_cubemap == fixture.textures.front().cubemap);
    CHECK(texture.value().format.value == fixture.textures.front().dxgi_format);
    REQUIRE(texture.value().chunks.size() == 1);
    CHECK(texture.value().chunks.front().mip_level == fixture.textures.front().chunks.front().start_mip);
    CHECK(texture.value().chunks.front().size == fixture.textures.front().chunks.front().payload.size());
    CHECK(texture.value().chunks.front().packed_size == fixture.textures.front().chunks.front().payload.size());
    CHECK(texture.value().chunks.front().compression == libbsa::compression_state::raw);
}

std::vector<std::byte> extract_fixture(const libbsa::test::ba2_dds_fixture& fixture, std::string path)
{
    const libbsa::memory_source source{std::span<const std::byte>{fixture.bytes}};
    const auto opened = libbsa::open_ba2(source);
    REQUIRE(opened.has_value());
    libbsa::memory_sink sink;
    const auto extracted = libbsa::extract_ba2_entry(opened.value(), source, std::move(path), sink);
    REQUIRE(extracted.has_value());
    return sink.bytes();
}

void assert_validated_dds(const std::vector<std::byte>& bytes,
                          std::uint32_t width,
                          std::uint32_t height,
                          std::uint32_t mip_count,
                          std::uint32_t array_size,
                          bool cubemap)
{
    // The private validation helper wraps DirectXTex LoadFromDDSMemory for these extraction tests.
    const auto validated = libbsa::detail::validate_dds(std::span<const std::byte>{bytes});
    REQUIRE(validated.has_value());
    CHECK(validated.value().width == width);
    CHECK(validated.value().height == height);
    CHECK(validated.value().mip_count == mip_count);
    CHECK(validated.value().array_size == array_size);
    CHECK(validated.value().is_cubemap == cubemap);
}

} // namespace

TEST_CASE("generated BA2 DDS fixtures expose stable BTDX DX10 identity bytes", "[fixture]")
{
    const auto fixture = libbsa::test::fo4_dx10_v1_one_mip_fixture();

    libbsa::test::require_ba2_dds_identity(fixture);
    REQUIRE(fixture.textures.size() == 1);
    CHECK(fixture.textures.front().path == "textures/generated/one_mip.dds");
}

TEST_CASE("open_ba2 reads Fallout 4 DX10 v1 texture_metadata and chunk summary", "[fixture]")
{
    assert_fo4_dx10_fixture(libbsa::test::fo4_dx10_v1_one_mip_fixture(),
                            libbsa::test::VERSION_FO4_DX10_V1,
                            "textures/generated/one_mip.dds");
}

TEST_CASE("open_ba2 reads Fallout 4 DX10 v7 texture_metadata and chunk summary", "[fixture]")
{
    assert_fo4_dx10_fixture(libbsa::test::fo4_dx10_v7_one_mip_fixture(),
                            libbsa::test::VERSION_FO4_DX10_V7,
                            "textures/generated/one_mip.dds");
}

TEST_CASE("open_ba2 reads Fallout 4 DX10 v8 texture_metadata and chunk summary", "[fixture]")
{
    assert_fo4_dx10_fixture(libbsa::test::fo4_dx10_v8_one_mip_fixture(),
                            libbsa::test::VERSION_FO4_DX10_V8,
                            "textures/generated/one_mip.dds");
}

TEST_CASE("open_ba2 reads multi-chunk DX10 texture_metadata chunk ranges", "[fixture]")
{
    const auto fixture = libbsa::test::multi_mip_fixture(libbsa::test::VERSION_FO4_DX10_V8);
    const auto opened = open_fixture(fixture);

    REQUIRE(opened.has_value());
    const auto texture = opened.value().texture_metadata("textures/generated/multi_mip.dds");
    REQUIRE(texture.has_value());
    CHECK(texture.value().mip_count == 3);
    REQUIRE(texture.value().chunks.size() == 2);
    CHECK(texture.value().chunks[0].mip_level == 0);
    CHECK(texture.value().chunks[0].size == fixture.textures.front().chunks[0].payload.size());
    CHECK(texture.value().chunks[1].mip_level == 2);
    CHECK(texture.value().chunks[1].size == fixture.textures.front().chunks[1].payload.size());
}

TEST_CASE("open_ba2 reads Starfield DX10 v3 texture_metadata and lz4_block chunk routing", "[fixture][codec]")
{
    const auto fixture = libbsa::test::starfield_dx10_v3_lz4_block_fixture();
    const auto opened = open_fixture(fixture);

    REQUIRE(opened.has_value());
    CHECK(opened.value().summary().format == libbsa::archive_format::starfield_ba2_dds);
    REQUIRE(opened.value().summary().version.has_value());
    CHECK(*opened.value().summary().version == libbsa::test::VERSION_STARFIELD_DX10_V3);
    REQUIRE(opened.value().summary().subtype.has_value());
    CHECK(*opened.value().summary().subtype == MAGIC_DX10);
    REQUIRE(opened.value().summary().compression_method.has_value());
    CHECK(*opened.value().summary().compression_method == 3);

    const auto texture = opened.value().texture_metadata("textures/generated/lz4_block.dds");
    REQUIRE(texture.has_value());
    REQUIRE(texture.value().chunks.size() == 1);
    CHECK(texture.value().chunks.front().compression == libbsa::compression_state::lz4_block);
    CHECK(texture.value().chunks.front().size == fixture.textures.front().chunks.front().payload.size());
    CHECK(texture.value().chunks.front().packed_size != texture.value().chunks.front().size);
}

TEST_CASE("extract_ba2_entry writes FO4 deflate DX10 texture as DDS", "[fixture][codec]")
{
    const auto fixture = libbsa::test::deflate_chunk_fixture(libbsa::test::VERSION_FO4_DX10_V1);

    const auto bytes = extract_fixture(fixture, "textures/generated/deflate.dds");

    assert_validated_dds(bytes, 4, 4, 1, 1, false);
}

TEST_CASE("extract_ba2_entry writes Starfield LZ4 DX10 texture as DDS", "[fixture][codec]")
{
    const auto fixture = libbsa::test::starfield_dx10_v3_lz4_block_fixture();

    const auto bytes = extract_fixture(fixture, "textures/generated/lz4_block.dds");

    assert_validated_dds(bytes, 4, 4, 1, 1, false);
}

TEST_CASE("extract_ba2_entry writes raw chunk DX10 texture as DDS", "[fixture]")
{
    const auto fixture = libbsa::test::raw_chunk_fixture(libbsa::test::VERSION_FO4_DX10_V8);

    const auto bytes = extract_fixture(fixture, "textures/generated/one_mip.dds");

    assert_validated_dds(bytes, 4, 4, 1, 1, false);
}

TEST_CASE("extract_ba2_entry writes one-mip and multi-mip DX10 textures as DDS", "[fixture]")
{
    const auto one_mip = extract_fixture(libbsa::test::one_mip_fixture(libbsa::test::VERSION_FO4_DX10_V1),
                                         "textures/generated/one_mip.dds");
    const auto multi_mip = extract_fixture(libbsa::test::multi_mip_fixture(libbsa::test::VERSION_FO4_DX10_V8),
                                           "textures/generated/multi_mip.dds");

    assert_validated_dds(one_mip, 4, 4, 1, 1, false);
    assert_validated_dds(multi_mip, 16, 16, 3, 1, false);
}

TEST_CASE("extract_ba2_entry writes cubemap and array DX10 textures as DDS", "[fixture]")
{
    const auto cubemap = extract_fixture(libbsa::test::cubemap_fixture(libbsa::test::VERSION_FO4_DX10_V1),
                                         "textures/generated/cubemap.dds");
    const auto array = extract_fixture(libbsa::test::array_fixture(libbsa::test::VERSION_FO4_DX10_V8),
                                       "textures/generated/array.dds");

    assert_validated_dds(cubemap, 4, 4, 1, 6, true);
    assert_validated_dds(array, 4, 4, 1, 4, false);
}

TEST_CASE("extract_ba2_entry leaves no partial bytes when DX10 validation fails", "[fixture]")
{
    const auto fixture = libbsa::test::malformed_reconstruction_failure_fixture();
    const libbsa::memory_source source{std::span<const std::byte>{fixture.bytes}};
    const auto opened = libbsa::open_ba2(source);
    REQUIRE(opened.has_value());
    libbsa::memory_sink sink;

    const auto extracted = libbsa::extract_ba2_entry(opened.value(), source, "textures/generated/reconstruction_failure.dds", sink);

    REQUIRE_FALSE(extracted.has_value());
    CHECK(sink.bytes().empty());
}
