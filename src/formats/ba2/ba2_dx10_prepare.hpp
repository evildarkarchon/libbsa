#pragma once

#include "formats/ba2/ba2_profile.hpp"
#include "formats/ba2/ba2_dx10_writer.hpp"

#include <detail/compression_router.hpp>
#include <detail/stored_payload.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::texture {
struct planned_texture_chunk;
}

namespace libbsa::formats::ba2 {

/// Retains one prepared DX10 chunk's decode facts and move-only Stored Payload.
///
/// The Stored Payload owns the exact post-compression bytes. Archive placement
/// and sharing belong exclusively to the later BA2 DX10 Placement Plan.
struct ba2_dx10_prepared_chunk {
    std::uint32_t packed_size{};
    std::uint32_t raw_size{};
    std::uint16_t start_mip{};
    std::uint16_t end_mip{};
    detail::compression_method compression{};
    detail::stored_payload payload;
};

struct ba2_dx10_prepared_entry {
    std::string archive_path_original;
    std::string archive_path_canonical;
    std::array<std::byte, 4> extension{};
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    std::uint8_t unknown_tex{0};
    std::uint8_t chunk_count{};
    std::uint16_t height{};
    std::uint16_t width{};
    std::uint8_t mip_count{};
    std::uint8_t dxgi_format{};
    std::uint16_t cube_maps_raw{};
    std::vector<ba2_dx10_prepared_chunk> chunks;
};

/// Validates target-specific DDS DXGI format compatibility before archive
/// serialization.
result<void> ba2_dx10_validate_texture_format_for_target(ba2_dx10_target target,
                                                         std::uint32_t dxgi_format);

/// Ensures the writer has a temporary snapshot directory for add-time DDS
/// ownership.
result<void> ba2_dx10_ensure_snapshot_directory(std::filesystem::path& snapshot_dir_path);

/// Builds a BA2 DX10 writer entry by parsing DDS metadata and snapshotting
/// subresource bytes.
result<ba2_dx10_writer_entry> ba2_dx10_make_writer_entry(std::string_view archive_path,
                                                         std::string_view dds_host_path,
                                                         ba2_dx10_target target,
                                                         const std::filesystem::path& snapshot_dir,
                                                         std::size_t entry_index);

/// Validates BA2 DX10 writer entries before chunk preparation.
result<void> ba2_dx10_validate_entries(ba2_dx10_target target,
                                       std::span<const ba2_dx10_writer_entry> entries);

/// Prepares a single BA2 DX10 texture chunk from planned DDS subresources.
result<ba2_dx10_prepared_chunk> ba2_dx10_prepare_chunk(
    const ba2_profile& profile, const ba2_dx10_writer_options& options,
    const ba2_dx10_writer_entry& source, const texture::planned_texture_chunk& planned);

/// Prepares BA2 DX10 entries by planning chunks, compressing payloads, hashing
/// names, and sorting records.
result<std::vector<ba2_dx10_prepared_entry>> ba2_dx10_prepare_entries(
    const ba2_profile& profile, const ba2_dx10_writer_options& options,
    std::span<const ba2_dx10_writer_entry> entries, std::uint32_t worker_count);

}  // namespace libbsa::formats::ba2
