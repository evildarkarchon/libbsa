#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace libbsa::detail {

result<std::vector<std::byte>> lz4_frame_decompress(std::span<const std::byte> packed, std::uint64_t expected_size);

result<std::vector<std::byte>> lz4_frame_compress(std::span<const std::byte> unpacked);

} // namespace libbsa::detail
