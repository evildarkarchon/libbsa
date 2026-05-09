#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace libbsa::texture {

/// Loads DDS metadata through the private texture analyzer boundary.
///
/// The returned value is immediately translated into libbsa-owned texture metadata so internal
/// callers and future writer phases do not pass third-party metadata objects across module seams.
result<texture_metadata> analyze_dds_metadata(std::span<const std::byte> dds_bytes);

/// Copied image bytes for one validated DDS source subresource.
///
/// The indices are libbsa-owned identities used by later writer planning so third-party image
/// pointers and platform metadata never escape this private texture boundary.
struct dds_source_subresource {
  std::uint32_t array_index;
  std::uint32_t face_index;
  std::uint32_t mip;
  std::vector<std::byte> bytes;
};

/// Validated DDS source bytes and metadata captured for BA2 DX10 writer add-time state.
///
/// `dds_bytes` preserves the caller-provided host file bytes, while `image_payload_bytes` and
/// `subresources` copy validated image memory so later source-file mutations cannot
/// affect archive output planning.
struct dds_source_analysis {
  texture_metadata metadata;
  std::vector<std::byte> dds_bytes;
  std::vector<std::byte> image_payload_bytes;
  std::vector<dds_source_subresource> subresources;
};

/// Loads and snapshots DDS source bytes through the private texture-analysis boundary.
///
/// Returns `format_error` when the bytes are malformed or use a DXGI format outside the locked
/// BA2 DX10 writer source set.
result<dds_source_analysis> analyze_dds_source(std::span<const std::byte> dds_bytes);

} // namespace libbsa::texture
