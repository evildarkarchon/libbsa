#pragma once

#include <cstdint>
#include <string_view>

namespace libbsa::detail {

/// Returns the TES3 BSA hash for an archive path using TES5Edit-compatible byte
/// rules.
std::uint64_t hash_tes3(std::string_view archive_path);

/// Returns the low 32 bits of a TES3 BSA hash: the second-half byte sum.
///
/// This is the *second* of the two `u32` values a TES3 hash record stores on
/// disk. See `tes3_hash_high32` for the word order the format uses.
std::uint32_t tes3_hash_low32(std::uint64_t hash) noexcept;

/// Returns the high 32 bits of a TES3 BSA hash: the first-half byte sum.
///
/// This is the *first* of the two `u32` values a TES3 hash record stores on
/// disk, so a record serializes `high32` then `low32`. Reading the eight bytes
/// as one little-endian `u64` therefore yields the halves transposed relative to
/// `hash_tes3`, which is why the format's word order is spelled out explicitly at
/// every serialization site rather than implied by a `u64` read (issue #46).
std::uint32_t tes3_hash_high32(std::uint64_t hash) noexcept;

/// Returns the TES4-family BSA hash after splitting `name_or_path` at its final
/// dot.
std::uint64_t hash_tes4(std::string_view name_or_path);

/// Returns the TES4-family BSA hash for an explicitly split name and extension.
std::uint64_t hash_tes4(std::string_view name_without_extension,
                        std::string_view extension_with_dot);

/// Returns the FO4/BA2 CRC32-style hash for an archive path.
std::uint32_t hash_fo4(std::string_view archive_path);

}  // namespace libbsa::detail
