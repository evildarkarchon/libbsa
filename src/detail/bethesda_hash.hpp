#pragma once

#include <cstdint>
#include <string_view>

namespace libbsa::detail {

/// Returns the TES3 BSA hash for an archive path using TES5Edit-compatible byte rules.
std::uint64_t hash_tes3(std::string_view archive_path);

/// Returns the low 32 bits used as the primary TES3 BSA hash sort key.
std::uint32_t tes3_hash_low32(std::uint64_t hash) noexcept;

/// Returns the high 32 bits used as the secondary TES3 BSA hash sort key.
std::uint32_t tes3_hash_high32(std::uint64_t hash) noexcept;

/// Returns a lexicographically comparable TES3 BSA hash sort key: low32 then high32.
std::uint64_t tes3_hash_sort_key(std::uint64_t hash) noexcept;

/// Returns the TES4-family BSA hash after splitting `name_or_path` at its final dot.
std::uint64_t hash_tes4(std::string_view name_or_path);

/// Returns the TES4-family BSA hash for an explicitly split name and extension.
std::uint64_t hash_tes4(std::string_view name_without_extension, std::string_view extension_with_dot);

/// Returns the FO4/BA2 CRC32-style hash for an archive path.
std::uint32_t hash_fo4(std::string_view archive_path);

} // namespace libbsa::detail
