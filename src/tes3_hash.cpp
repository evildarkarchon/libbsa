#include "tes3_hash.hpp"

namespace libbsa::detail {
namespace {

std::uint8_t lower_byte(char value) noexcept
{
    const auto byte = static_cast<unsigned char>(value);
    if (byte >= static_cast<unsigned char>('A') && byte <= static_cast<unsigned char>('Z')) {
        return static_cast<std::uint8_t>(byte + ('a' - 'A'));
    }
    return static_cast<std::uint8_t>(byte);
}

std::uint32_t shifted_byte(char value, std::uint32_t offset) noexcept
{
    return static_cast<std::uint32_t>(lower_byte(value)) << (offset & 0x1FU);
}

std::uint32_t rotate_right(std::uint32_t value, std::uint32_t count) noexcept
{
    count &= 0x1FU;
    if (count == 0U) {
        return value;
    }

    return (value >> count) | (value << (32U - count));
}

} // namespace

std::uint64_t hash_tes3(std::string_view filename) noexcept
{
    const auto split = filename.size() / 2U;

    std::uint32_t sum = 0;
    std::uint32_t offset = 0;
    for (std::size_t index = 0; index < split; ++index) {
        sum ^= shifted_byte(filename[index], offset);
        offset += 8U;
    }

    std::uint64_t result = static_cast<std::uint64_t>(sum) << 32U;

    sum = 0;
    offset = 0;
    for (std::size_t index = split; index < filename.size(); ++index) {
        const auto temp = shifted_byte(filename[index], offset);
        sum ^= temp;
        // BSArchPro rotates by bits from the shifted byte value, not by the string byte offset.
        sum = rotate_right(sum, temp & 0x1FU);
        offset += 8U;
    }

    return result | sum;
}

} // namespace libbsa::detail
