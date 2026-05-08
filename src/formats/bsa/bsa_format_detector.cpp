#include "formats/bsa/bsa_format_detector.hpp"

#include <detail/binary_io.hpp>

namespace libbsa::formats::bsa {
namespace {

constexpr std::uint32_t tes4_version = 0x67U;
constexpr std::uint32_t fo3_version = 0x68U;
constexpr std::uint32_t sse_version = 0x69U;
constexpr std::uint32_t tes3_magic_version = 0x00000100U;

} // namespace

result<detected_bsa_format> detect_bsa_format(std::span<const std::byte> bytes) {
  detail::binary_reader reader{bytes};
  const auto magic = reader.read_bytes(4);
  if (!magic) {
    return magic.error();
  }

  const auto magic_bytes = magic.value();
  const auto little_endian_magic = static_cast<std::uint32_t>(magic_bytes[0]) |
                                   (static_cast<std::uint32_t>(magic_bytes[1]) << 8U) |
                                   (static_cast<std::uint32_t>(magic_bytes[2]) << 16U) |
                                   (static_cast<std::uint32_t>(magic_bytes[3]) << 24U);
  if (little_endian_magic == tes3_magic_version) {
    return detected_bsa_format{archive_variant::tes3, tes3_magic_version, entry_compression::none};
  }

  if (magic_bytes[0] != std::byte{'B'} || magic_bytes[1] != std::byte{'S'} || magic_bytes[2] != std::byte{'A'} ||
      magic_bytes[3] != std::byte{0}) {
    return error{error_code::unsupported, "archive bytes do not start with BSA magic"};
  }

  const auto version = reader.read_u32_le();
  if (!version) {
    return version.error();
  }

  switch (version.value()) {
  case tes4_version:
  case fo3_version:
    return detected_bsa_format{archive_variant::tes4, version.value(), entry_compression::deflate};
  case sse_version:
    return detected_bsa_format{archive_variant::tes4, version.value(), entry_compression::lz4_frame};
  default:
    return error{error_code::unsupported, "BSA header version is not supported"};
  }
}

} // namespace libbsa::formats::bsa
