#include <detail/bethesda_hash.hpp>

#include <array>
#include <cstddef>

namespace libbsa::detail {
namespace {

std::uint8_t lower_byte(char value) noexcept {
  const auto byte = static_cast<unsigned char>(value);
  if (byte >= static_cast<unsigned char>('A') && byte <= static_cast<unsigned char>('Z')) {
    return static_cast<std::uint8_t>(byte + static_cast<unsigned char>('a') - static_cast<unsigned char>('A'));
  }
  return static_cast<std::uint8_t>(byte);
}

std::uint32_t rotate_right(std::uint32_t value, std::uint32_t amount) noexcept {
  const auto shift = amount & 31U;
  if (shift == 0) {
    return value;
  }
  return (value >> shift) | (value << (32U - shift));
}

std::uint32_t extension_magic(std::string_view extension) noexcept {
  std::uint32_t value = 0;
  const auto count = extension.size() < 4 ? extension.size() : 4;
  for (std::size_t index = 0; index < count; ++index) {
    value |= static_cast<std::uint32_t>(lower_byte(extension[index])) << (index * 8U);
  }
  return value;
}

std::uint32_t crc32_entry(std::uint32_t index) noexcept {
  std::uint32_t value = index;
  for (int bit = 0; bit < 8; ++bit) {
    value = (value & 1U) != 0U ? (value >> 1U) ^ 0xEDB88320U : (value >> 1U);
  }
  return value;
}

std::uint32_t crc32_lookup(std::uint32_t index) noexcept {
  static const auto table = [] {
    std::array<std::uint32_t, 256> values{};
    for (std::uint32_t i = 0; i < values.size(); ++i) {
      values[i] = crc32_entry(i);
    }
    return values;
  }();
  return table[index & 0xFFU];
}

} // namespace

std::uint64_t hash_tes3(std::string_view archive_path) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashTES3 halves the byte string, lowercases
  // ASCII only via LowerByte, then XORs shifted bytes with a rotate in the low half.
  const auto half = archive_path.size() >> 1U;
  std::uint32_t sum = 0;
  std::uint32_t offset = 0;
  for (std::size_t index = 0; index < half; ++index) {
    const auto temp = static_cast<std::uint32_t>(lower_byte(archive_path[index])) << (offset & 31U);
    sum ^= temp;
    offset += 8U;
  }

  std::uint64_t result = static_cast<std::uint64_t>(sum) << 32U;
  sum = 0;
  offset = 0;
  for (std::size_t index = half; index < archive_path.size(); ++index) {
    const auto temp = static_cast<std::uint32_t>(lower_byte(archive_path[index])) << (offset & 31U);
    sum ^= temp;
    sum = rotate_right(sum, temp & 31U);
    offset += 8U;
  }
  return result | sum;
}

std::uint64_t hash_tes4(std::string_view name_or_path) {
  const auto dot = name_or_path.find_last_of('.');
  if (dot == std::string_view::npos) {
    return hash_tes4(name_or_path, {});
  }
  return hash_tes4(name_or_path.substr(0, dot), name_or_path.substr(dot));
}

std::uint64_t hash_tes4(std::string_view name_without_extension, std::string_view extension_with_dot) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashTES4 encodes name edge bytes and
  // extension special bits for .kf/.nif/.dds/.wav, then adds sdbm accumulators.
  const auto length = name_without_extension.size();
  if (length == 0) {
    return 0;
  }

  std::uint64_t result = lower_byte(name_without_extension[length - 1]);
  if (length > 2) {
    result |= static_cast<std::uint64_t>(lower_byte(name_without_extension[length - 2])) << 8U;
  }
  result |= static_cast<std::uint64_t>(static_cast<std::uint32_t>(length) << 16U);
  result |= static_cast<std::uint64_t>(lower_byte(name_without_extension[0])) << 24U;

  switch (extension_magic(extension_with_dot)) {
  case 0x00666B2EU: // .kf
    result |= 0x80U;
    break;
  case 0x66696E2EU: // .nif
    result |= 0x8000U;
    break;
  case 0x7364642EU: // .dds
    result |= 0x8080U;
    break;
  case 0x7661772EU: // .wav
    result |= 0x80000000U;
    break;
  default:
    break;
  }

  std::uint32_t hash = 0;
  for (std::size_t index = 1; index + 2 < length; ++index) {
    hash = lower_byte(name_without_extension[index]) + (hash << 6U) + (hash << 16U) - hash;
  }
  result += static_cast<std::uint64_t>(hash) << 32U;

  hash = 0;
  for (const char ch : extension_with_dot) {
    hash = lower_byte(ch) + (hash << 6U) + (hash << 16U) - hash;
  }
  result += static_cast<std::uint64_t>(hash) << 32U;
  return result;
}

std::uint32_t hash_fo4(std::string_view archive_path) {
  // TES5Edit/Core/wbBSArchive.pas CreateHashFO4 uses CRC32 with initial value 0,
  // ASCII LowerByte folding, skips bytes >127, and treats '/' as '\\'.
  std::uint32_t result = 0;
  for (char ch : archive_path) {
    auto byte = static_cast<unsigned char>(ch);
    if (byte > 127U) {
      continue;
    }
    if (ch == '/') {
      ch = '\\';
    }
    result = (result >> 8U) ^ crc32_lookup((result ^ lower_byte(ch)) & 0xFFU);
  }
  return result;
}

} // namespace libbsa::detail
