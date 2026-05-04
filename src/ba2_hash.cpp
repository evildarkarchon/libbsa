#include "ba2_hash.hpp"

#include <array>

namespace libbsa::detail {
namespace {

constexpr std::uint32_t kCrc32Polynomial = 0xEDB88320u;

constexpr std::array<std::uint32_t, 256U> make_crc32_table() noexcept
{
    std::array<std::uint32_t, 256U> table{};
    for (std::uint32_t index = 0; index < table.size(); ++index) {
        auto value = index;
        for (int bit = 0; bit < 8; ++bit) {
            value = (value & 1U) != 0U ? (value >> 1U) ^ kCrc32Polynomial : value >> 1U;
        }
        table[index] = value;
    }

    return table;
}

constexpr auto kCrc32Table = make_crc32_table();

constexpr unsigned char normalized_hash_byte(char value) noexcept
{
    auto byte = static_cast<unsigned char>(value);
    if (byte == '/') {
        byte = '\\';
    }
    if (byte >= static_cast<unsigned char>('A') && byte <= static_cast<unsigned char>('Z')) {
        byte = static_cast<unsigned char>(byte + ('a' - 'A'));
    }

    return byte;
}

} // namespace

std::uint32_t create_hash_fo4(std::string_view value) noexcept
{
    std::uint32_t hash = 0;
    for (const auto character : value) {
        const auto byte = normalized_hash_byte(character);
        if (byte > 127U) {
            continue;
        }

        hash = (hash >> 8U) ^ kCrc32Table[(hash ^ byte) & 0xFFU];
    }

    return hash;
}

} // namespace libbsa::detail
