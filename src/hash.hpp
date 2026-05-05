#pragma once

#include <cstdint>
#include <string_view>

namespace libbsa::detail {

[[nodiscard]] std::uint64_t hash_tes3_path(std::string_view path);
[[nodiscard]] std::uint64_t hash_tes4_name(std::string_view name, std::string_view ext);
[[nodiscard]] std::uint64_t hash_tes4_path(std::string_view path);
[[nodiscard]] std::uint32_t hash_fo4_path_part(std::string_view path);
[[nodiscard]] std::uint32_t fo4_extension_magic(std::string_view extension_without_dot);

} // namespace libbsa::detail
