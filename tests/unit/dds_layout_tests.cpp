#include "texture/dds_layout.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
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

}  // namespace

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

TEST_CASE("dds_layout accepts exact BSArch aggregate cubemap tails",
          "[unit][dds_layout][bsarch_cubemap]") {
    const libbsa::texture::dds_texture_layout layout{8U, 8U, 2U, 71U, 1U, true};
    // Each BC1 face contains a 32-byte top mip and an 8-byte tail. BSArch's
    // final chunk preserves the DDS remainder, including the other five faces.
    SECTION("one chunk contains all six complete faces") {
        const std::array chunks{chunk(0, 1, 240)};
        const auto result = libbsa::texture::validate_and_order_chunks(layout, chunks);
        REQUIRE(result.has_value());
        REQUIRE(result.value().size() == 1U);
        CHECK(result.value()[0].source_chunk_index == 0U);
    }
    SECTION("a leading mip is followed by the aggregate face tail") {
        const std::array chunks{chunk(0, 0, 32), chunk(1, 1, 208)};
        const auto result = libbsa::texture::validate_and_order_chunks(layout, chunks);
        REQUIRE(result.has_value());
        REQUIRE(result.value().size() == 2U);
        CHECK(result.value()[0].source_chunk_index == 0U);
        CHECK(result.value()[1].source_chunk_index == 1U);
    }
}

TEST_CASE("dds_layout rejects malformed BSArch aggregate cubemap tails",
          "[unit][malformed][dds_layout][bsarch_cubemap]") {
    const libbsa::texture::dds_texture_layout layout{8U, 8U, 2U, 71U, 1U, true};
    for (const auto size : {239U, 241U, 200U, 280U}) {
        const std::array chunks{chunk(0, 1, size)};
        const auto result = libbsa::texture::validate_and_order_chunks(layout, chunks);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::error_code::format_error);
    }
    for (const auto& chunks : std::array{
             std::array{chunk(0, 0, 32), chunk(0, 1, 208)},
             std::array{chunk(0, 0, 32), chunk(1, 1, 207)},
             std::array{chunk(0, 1, 240), chunk(0, 1, 40)}}) {
        const auto result = libbsa::texture::validate_and_order_chunks(layout, chunks);
        REQUIRE_FALSE(result.has_value());
    }
    SECTION("ordinary textures and cube arrays cannot borrow aggregate semantics") {
        const std::array chunks{chunk(0, 1, 240)};
        auto incompatible = layout;
        incompatible.is_cubemap = false;
        REQUIRE_FALSE(libbsa::texture::validate_and_order_chunks(incompatible, chunks).has_value());
        incompatible.is_cubemap = true;
        incompatible.array_size = 2;
        REQUIRE_FALSE(libbsa::texture::validate_and_order_chunks(incompatible, chunks).has_value());
    }
}

TEST_CASE("dds_layout computes locked DX10 mip byte sizes", "[unit][dds_layout]") {
    struct format_case {
        std::uint32_t dxgi_format;
        const char* name;
        std::uint32_t width;
        std::uint32_t height;
        std::uint64_t expected_bytes;
    };

    const std::array cases{
        // DXGI_FORMAT_BC1_UNORM=71 and DXGI_FORMAT_BC1_UNORM_SRGB=72: 4x4 blocks,
        // 8 bytes.
        format_case{71U, "BC1_UNORM", 4U, 4U, 8U},
        format_case{72U, "BC1_UNORM_SRGB", 2U, 2U, 8U},
        // DXGI_FORMAT_BC3_UNORM=77: 4x4 blocks, 16 bytes.
        format_case{77U, "BC3_UNORM", 4U, 4U, 16U},
        // DXGI_FORMAT_BC4_UNORM=80: 4x4 blocks, 8 bytes.
        format_case{80U, "BC4_UNORM", 4U, 4U, 8U},
        // DXGI_FORMAT_BC5_UNORM=83 and DXGI_FORMAT_BC5_SNORM=84: 4x4 blocks, 16
        // bytes.
        format_case{83U, "BC5_UNORM", 4U, 4U, 16U},
        format_case{84U, "BC5_SNORM", 4U, 4U, 16U},
        // DXGI_FORMAT_BC6H_UF16/BC6H_SF16=95/96 and
        // DXGI_FORMAT_BC7_UNORM/BC7_UNORM_SRGB=98/99:
        // 4x4 blocks, 16 bytes.
        format_case{95U, "BC6H_UF16", 4U, 4U, 16U},
        format_case{96U, "BC6H_SF16", 4U, 4U, 16U},
        format_case{98U, "BC7_UNORM", 4U, 4U, 16U},
        format_case{99U, "BC7_UNORM_SRGB", 4U, 4U, 16U},
        // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB=29, B8G8R8A8_UNORM=87, and
        // R8G8B8A8_SNORM=31: 4 bytes/pixel.
        format_case{29U, "R8G8B8A8_UNORM_SRGB", 4U, 4U, 64U},
        format_case{87U, "B8G8R8A8_UNORM", 4U, 4U, 64U},
        format_case{31U, "R8G8B8A8_SNORM", 4U, 4U, 64U},
        // DXGI_FORMAT_R8_UNORM=61: 1 byte/pixel.
        format_case{61U, "R8_UNORM", 4U, 4U, 16U},
    };

    for (const auto& test_case : cases) {
        CAPTURE(test_case.name);
        const libbsa::texture::dds_texture_layout layout{
            .width = test_case.width,
            .height = test_case.height,
            .mip_count = 1,
            .dxgi_format = test_case.dxgi_format,
            .array_size = 1,
            .is_cubemap = false,
        };

        const auto size = libbsa::texture::mip_size_for_format(layout, 0U);

        REQUIRE(size.has_value());
        CHECK(size.value() == test_case.expected_bytes);
    }
}

TEST_CASE("dds_layout fails closed for hostile block-compressed dimensions", "[unit][dds_layout]") {
    const auto hostile_dimension = std::numeric_limits<std::uint32_t>::max();
    const libbsa::texture::dds_texture_layout hostile_bc1{
        .width = hostile_dimension,
        .height = hostile_dimension,
        .mip_count = 1,
        .dxgi_format = 71,
        .array_size = 1,
        .is_cubemap = false,
    };

    SECTION("BC1 UINT32_MAX dimensions compute the full rounded uint64 byte size") {
        const auto size = libbsa::texture::mip_size_for_format(hostile_bc1, 0U);

        REQUIRE(size.has_value());
        CHECK(size.value() == 9223372036854775808ULL);
    }

    SECTION("wrapped BC1 raw-size metadata is rejected") {
        const std::array chunks{chunk(0, 0, 8)};
        const auto result = libbsa::texture::validate_and_order_chunks(hostile_bc1, chunks);

        REQUIRE_FALSE(result.has_value());
        CHECK(result.error().code == libbsa::error_code::format_error);
    }

    SECTION("BC7 UINT32_MAX dimensions fail closed when byte size overflows") {
        const libbsa::texture::dds_texture_layout hostile_bc7{
            .width = hostile_dimension,
            .height = hostile_dimension,
            .mip_count = 1,
            .dxgi_format = 98,
            .array_size = 1,
            .is_cubemap = false,
        };

        const auto size = libbsa::texture::mip_size_for_format(hostile_bc7, 0U);

        REQUIRE_FALSE(size.has_value());
        CHECK(size.error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("dds_layout plans DX10 chunks from default policy and byte caps", "[unit][dds_layout]") {
    const libbsa::texture::dds_texture_layout layout{
        .width = 1024,
        .height = 1024,
        .mip_count = 5,
        .dxgi_format = 98,  // DXGI_FORMAT_BC7_UNORM=98.
        .array_size = 1,
        .is_cubemap = false,
    };

    SECTION("reference default max_decoded_chunk_bytes groups contiguous mips") {
        const auto chunks = libbsa::texture::plan_dx10_chunks(layout, 0U);

        REQUIRE(chunks.has_value());
        REQUIRE(chunks.value().size() == 3U);
        CHECK(chunks.value()[0].start_mip == 0U);
        CHECK(chunks.value()[0].end_mip == 0U);
        CHECK(chunks.value()[1].start_mip == 1U);
        CHECK(chunks.value()[1].end_mip == 1U);
        CHECK(chunks.value()[2].start_mip == 2U);
        CHECK(chunks.value()[2].end_mip == 4U);
    }

    SECTION("explicit max_decoded_chunk_bytes splits only at mip boundaries") {
        const auto chunks = libbsa::texture::plan_dx10_chunks(layout, 1048576U);

        REQUIRE(chunks.has_value());
        REQUIRE(chunks.value().size() == 2U);
        CHECK(chunks.value()[0].start_mip == 0U);
        CHECK(chunks.value()[0].end_mip == 0U);
        CHECK(chunks.value()[1].start_mip == 1U);
        CHECK(chunks.value()[1].end_mip == 4U);
    }

    SECTION("impossible max_decoded_chunk_bytes fails closed") {
        const auto chunks = libbsa::texture::plan_dx10_chunks(layout, 1U);

        REQUIRE_FALSE(chunks.has_value());
        CHECK(chunks.error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("dds_layout repeats DX10 chunk plans for arrays and cubemaps", "[unit][dds_layout]") {
    SECTION("array slices repeat the same mip split") {
        const libbsa::texture::dds_texture_layout layout{
            .width = 4,
            .height = 4,
            .mip_count = 2,
            .dxgi_format = 71,
            .array_size = 2,
            .is_cubemap = false,
        };

        const auto chunks = libbsa::texture::plan_dx10_chunks(layout, 8U);

        REQUIRE(chunks.has_value());
        REQUIRE(chunks.value().size() == 4U);
        CHECK(chunks.value()[0].array_index == 0U);
        CHECK(chunks.value()[0].start_mip == 0U);
        CHECK(chunks.value()[1].array_index == 0U);
        CHECK(chunks.value()[1].start_mip == 1U);
        CHECK(chunks.value()[2].array_index == 1U);
        CHECK(chunks.value()[2].start_mip == 0U);
        CHECK(chunks.value()[3].array_index == 1U);
        CHECK(chunks.value()[3].start_mip == 1U);
    }

    SECTION("cubemap faces repeat the same mip split") {
        const libbsa::texture::dds_texture_layout layout{
            .width = 4,
            .height = 4,
            .mip_count = 1,
            .dxgi_format = 71,
            .array_size = 1,
            .is_cubemap = true,
        };

        const auto chunks = libbsa::texture::plan_dx10_chunks(layout, 0U);

        REQUIRE(chunks.has_value());
        REQUIRE(chunks.value().size() == 6U);
        for (std::uint32_t face = 0; face < 6U; ++face) {
            CHECK(chunks.value()[face].array_index == 0U);
            CHECK(chunks.value()[face].face_index == face);
            CHECK(chunks.value()[face].start_mip == 0U);
            CHECK(chunks.value()[face].end_mip == 0U);
        }
    }
}

TEST_CASE(
    "dds_layout rejects gaps, duplicate coverage, impossible sizes, and "
    "unsupported formats",
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
