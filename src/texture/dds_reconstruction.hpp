#pragma once

#include <libbsa/ba2.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace libbsa::detail {

/// Reconstructs a complete DDS byte stream from texture metadata and decompressed image bytes.
///
/// The returned buffer contains DDS magic, a legacy DDS header, a DX10 extension header,
/// and the supplied image payload. Unsupported or internally inconsistent metadata returns
/// a structured failure without producing partial bytes.
[[nodiscard]] result<std::vector<std::byte>> reconstruct_dds(const texture_metadata& metadata,
                                                             std::span<const std::byte> image_payload);

} // namespace libbsa::detail
