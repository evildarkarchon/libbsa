#include "texture/dds_layout.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

constexpr std::uint32_t dds_magic = 0x20534444U;
constexpr std::uint32_t dds_header_size = 124U;
constexpr std::uint32_t dds_pixel_format_size = 32U;
constexpr std::uint32_t dds_fourcc_dx10 = 0x30315844U;
constexpr std::uint32_t d3d_resource_dimension_texture2d = 3U;
constexpr std::uint32_t d3d_resource_misc_texturecube = 0x4U;
constexpr std::size_t dds_dxt10_file_header_size = 148U;

std::uint32_t read_u32_le(const std::vector<std::byte>& bytes, const std::size_t offset) {
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

libbsa::texture_chunk_metadata chunk(const std::uint16_t start_mip, const std::uint16_t end_mip,
                                     const std::uint32_t raw_size) {
  return libbsa::texture_chunk_metadata{
      .payload_offset = 0,
      .stored_size = raw_size,
      .raw_size = raw_size,
      .start_mip = start_mip,
      .end_mip = end_mip,
      .compression = libbsa::entry_compression::none,
  };
}

} // namespace

TEST_CASE("dds_layout builds deterministic DDS DXT10 headers", "[unit][dds_layout]") {
  const libbsa::texture::dds_texture_layout layout{
      .width = 4,
      .height = 4,
      .mip_count = 1,
      .dxgi_format = 71,
      .array_size = 1,
      .is_cubemap = false,
  };

  const auto header = libbsa::texture::build_dds_dxt10_header(layout);

  REQUIRE(header.has_value());
  REQUIRE(header.value().size() == dds_dxt10_file_header_size);
  CHECK(read_u32_le(header.value(), 0) == dds_magic);
  CHECK(read_u32_le(header.value(), 4) == dds_header_size);
  CHECK(read_u32_le(header.value(), 76) == dds_pixel_format_size);
  CHECK(read_u32_le(header.value(), 84) == dds_fourcc_dx10);
  CHECK(read_u32_le(header.value(), 128) == layout.dxgi_format);
  CHECK(read_u32_le(header.value(), 132) == d3d_resource_dimension_texture2d);
  CHECK(read_u32_le(header.value(), 136) == 0U);
  CHECK(read_u32_le(header.value(), 140) == layout.array_size);
  CHECK(read_u32_le(header.value(), 144) == 0U);
}

TEST_CASE("dds_layout computes cubemap DDS face order with source chunk identity",
          "[unit][dds_layout]") {
  const libbsa::texture::dds_texture_layout layout{
      .width = 4,
      .height = 4,
      .mip_count = 1,
      .dxgi_format = 71,
      .array_size = 1,
      .is_cubemap = true,
  };
  const std::array<libbsa::texture_chunk_metadata, 6> chunks{
      chunk(0, 0, 8), chunk(0, 0, 8), chunk(0, 0, 8),
      chunk(0, 0, 8), chunk(0, 0, 8), chunk(0, 0, 8),
  };

  const auto header = libbsa::texture::build_dds_dxt10_header(layout);
  const auto segments = libbsa::texture::validate_and_order_chunks(layout, chunks);

  REQUIRE(header.has_value());
  CHECK(read_u32_le(header.value(), 136) == d3d_resource_misc_texturecube);
  REQUIRE(segments.has_value());
  REQUIRE(segments.value().size() == chunks.size());

  for (std::size_t i = 0; i < segments.value().size(); ++i) {
    const auto& segment = segments.value()[i];
    CHECK(segment.array_index == 0U);
    CHECK(segment.face_index == i);
    CHECK(segment.start_mip == 0U);
    CHECK(segment.end_mip == 0U);
    CHECK(segment.source_chunk_index == i);
  }
}

TEST_CASE("dds_layout allows repeated mip ranges across different array slices",
          "[unit][dds_layout]") {
  const libbsa::texture::dds_texture_layout layout{
      .width = 4,
      .height = 4,
      .mip_count = 1,
      .dxgi_format = 28,
      .array_size = 2,
      .is_cubemap = false,
  };
  const std::array<libbsa::texture_chunk_metadata, 2> chunks{
      chunk(0, 0, 64),
      chunk(0, 0, 64),
  };

  const auto segments = libbsa::texture::validate_and_order_chunks(layout, chunks);

  REQUIRE(segments.has_value());
  REQUIRE(segments.value().size() == chunks.size());
  CHECK(segments.value()[0].array_index == 0U);
  CHECK(segments.value()[0].face_index == 0U);
  CHECK(segments.value()[0].source_chunk_index == 0U);
  CHECK(segments.value()[1].array_index == 1U);
  CHECK(segments.value()[1].face_index == 0U);
  CHECK(segments.value()[1].source_chunk_index == 1U);
}

TEST_CASE("dds_layout rejects gaps, duplicate coverage, impossible sizes, and unsupported formats",
          "[unit][dds_layout]") {
  const libbsa::texture::dds_texture_layout bc1_two_mips{
      .width = 4,
      .height = 4,
      .mip_count = 2,
      .dxgi_format = 71,
      .array_size = 1,
      .is_cubemap = false,
  };
  const libbsa::texture::dds_texture_layout unsupported_format{
      .width = 4,
      .height = 4,
      .mip_count = 1,
      .dxgi_format = 999,
      .array_size = 1,
      .is_cubemap = false,
  };

  SECTION("zero chunks are rejected") {
    const std::array<libbsa::texture_chunk_metadata, 0> chunks{};
    const auto result = libbsa::texture::validate_and_order_chunks(bc1_two_mips, chunks);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::format_error);
  }

  SECTION("mip gaps are rejected") {
    const std::array chunks{chunk(1, 1, 8)};
    const auto result = libbsa::texture::validate_and_order_chunks(bc1_two_mips, chunks);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::format_error);
  }

  SECTION("duplicate coverage within the same face or slice is rejected") {
    const std::array chunks{chunk(0, 0, 8), chunk(0, 0, 8)};
    const auto result = libbsa::texture::validate_and_order_chunks(bc1_two_mips, chunks);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::format_error);
  }

  SECTION("start mip after end mip is rejected") {
    const std::array chunks{chunk(1, 0, 8)};
    const auto result = libbsa::texture::validate_and_order_chunks(bc1_two_mips, chunks);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::format_error);
  }

  SECTION("unknown unsupported DXGI fixture formats fail closed") {
    const std::array chunks{chunk(0, 0, 64)};
    const auto result = libbsa::texture::validate_and_order_chunks(unsupported_format, chunks);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::format_error);
  }

  SECTION("impossible raw byte totals are rejected") {
    const libbsa::texture::dds_texture_layout layout{
        .width = 4,
        .height = 4,
        .mip_count = 1,
        .dxgi_format = 71,
        .array_size = 1,
        .is_cubemap = false,
    };
    const std::array chunks{chunk(0, 0, 7)};
    const auto result = libbsa::texture::validate_and_order_chunks(layout, chunks);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::format_error);
  }

  SECTION("chunk sequences that contradict researched BA2 order are rejected") {
    const std::array chunks{chunk(1, 1, 8), chunk(0, 0, 8)};
    const auto result = libbsa::texture::validate_and_order_chunks(bc1_two_mips, chunks);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error().code == libbsa::error_code::format_error);
  }
}
