#pragma once

#include <cstdint>
#include <string_view>

namespace libbsa::detail {

std::uint64_t hash_tes4(std::string_view name, std::string_view extension) noexcept;
std::uint64_t hash_tes4_file(std::string_view file_name) noexcept;

} // namespace libbsa::detail
