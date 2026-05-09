#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <span>

namespace libbsa::texture {

/// Loads DDS metadata through the private texture analyzer boundary.
///
/// The returned value is immediately translated into libbsa-owned texture metadata so internal
/// callers and future writer phases do not pass third-party metadata objects across module seams.
result<texture_metadata> analyze_dds_metadata(std::span<const std::byte> dds_bytes);

} // namespace libbsa::texture
