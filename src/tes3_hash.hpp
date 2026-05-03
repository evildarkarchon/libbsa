#pragma once

#include <cstdint>
#include <string_view>

namespace libbsa::detail {

/// Computes the Morrowind/TES3 BSA filename hash used by BSArchPro's CreateHashTES3.
std::uint64_t hash_tes3(std::string_view filename) noexcept;

} // namespace libbsa::detail
