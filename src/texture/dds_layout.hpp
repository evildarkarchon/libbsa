#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace libbsa::texture {

/// Texture dimensions and DDS DXT10 metadata needed to reconstruct a header.
///
/// These values are libbsa-owned so DDS extraction can build deterministic
/// bytes without exposing DirectXTex or platform graphics types across the
/// internal texture boundary.
struct dds_texture_layout {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t mip_count;
    std::uint32_t dxgi_format;
    std::uint32_t array_size;
    bool is_cubemap;
};

/// Identity of a validated texture payload segment in computed DDS output
/// order.
///
/// `source_chunk_index` maps the logical array/face/mip segment back to the
/// parsed archive chunk so extraction can write chunks in validated DDS order
/// instead of blindly streaming archive order.
/// A validated BSArch cubemap tail starts at the recorded first-face mip and
/// also contains all five later faces. Its source chunk is returned once.
struct logical_texture_segment {
    std::uint32_t array_index;
    std::uint32_t face_index;
    std::uint32_t start_mip;
    std::uint32_t end_mip;
    std::size_t source_chunk_index;
};

/// Contiguous mip range selected for one BA2 DX10 texture chunk.
///
/// Array and cubemap textures repeat the same mip split for every slice or face
/// so writer-side serialization preserves the DDS image order without resizing,
/// transcoding, or mip reordering.
struct planned_texture_chunk {
    std::uint32_t array_index;
    std::uint32_t face_index;
    std::uint32_t start_mip;
    std::uint32_t end_mip;
    std::uint64_t raw_size;
};

/// Builds `DDS ` + `DDS_HEADER` + `DDS_HEADER_DXT10` bytes for a BA2 texture
/// layout.
///
/// Returns `format_error` for impossible dimensions or unsupported
/// fixture-backed DXGI formats.
[[nodiscard]] result<std::vector<std::byte>> build_dds_dxt10_header(
    const dds_texture_layout& layout);

/// Computes the raw byte size of one mip level for the locked DX10 writer
/// format set.
///
/// Returns `format_error` for unsupported DXGI format IDs or arithmetic
/// overflow.
[[nodiscard]] result<std::uint64_t> mip_size_for_format(const dds_texture_layout& layout,
                                                        std::uint32_t mip);

/// Computes the raw byte size of an inclusive contiguous mip range.
///
/// Returns `format_error` when the range is invalid, the format is unsupported,
/// or the total overflows.
[[nodiscard]] result<std::uint64_t> mip_range_size(const dds_texture_layout& layout,
                                                   std::uint32_t start_mip, std::uint32_t end_mip);

/// Plans BA2 DX10 chunks as contiguous mip ranges for every array slice or
/// cubemap face.
///
/// `max_decoded_chunk_bytes == 0` selects the reference-derived default: split
/// large mips individually, then group the remaining smaller mips. Nonzero caps
/// greedily group contiguous mips without exceeding the archive-wide
/// decoded-byte cap and fail closed if one mip cannot fit.
[[nodiscard]] result<std::vector<planned_texture_chunk>> plan_dx10_chunks(
    const dds_texture_layout& layout, std::uint32_t max_decoded_chunk_bytes);

/// Validates BA2 texture chunks and returns their logical DDS segment
/// identities.
///
/// The accepted order is array slice ascending, cubemap face order `+X, -X, +Y,
/// -Y, +Z, -Z`, then ascending mip ranges within each face/slice. Gaps,
/// overlaps, duplicates, unsupported formats, and impossible raw byte totals
/// return `format_error` before extraction reads payload bytes.
/// Also accepts BSArch's single-cubemap layout: individual leading mips from
/// face zero followed by one exact-size tail containing the remaining DDS
/// bytes of all six faces. That aggregate tail remains one extraction segment.
[[nodiscard]] result<std::vector<logical_texture_segment>> validate_and_order_chunks(
    const dds_texture_layout& layout, std::span<const texture_chunk_metadata> chunks);

}  // namespace libbsa::texture
