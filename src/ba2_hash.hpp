#pragma once

#include <cstdint>
#include <string_view>

namespace libbsa::detail {

/// Computes the BSArchPro-compatible CRC32 hash used by BA2 GNRL directory and file-name records.
/// Bytes are normalized like the reference implementation: ASCII case is folded, `/` becomes `\`, and non-ASCII bytes are skipped.
[[nodiscard]] std::uint32_t create_hash_fo4(std::string_view value) noexcept;

} // namespace libbsa::detail
