#include "tes4_hash.hpp"

#include <algorithm>

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

std::uint32_t mix_hash(std::uint32_t hash, char value) noexcept
{
    return static_cast<std::uint32_t>(
        lower_byte(value) + (hash << 6U) + (hash << 16U) - hash);
}

} // namespace

std::uint64_t hash_tes4(std::string_view name, std::string_view extension) noexcept
{
    const auto length = name.size();
    if (length == 0U) {
        return 0;
    }

    std::uint64_t result = lower_byte(name[length - 1U]);
    if (length > 2U) {
        result |= static_cast<std::uint64_t>(lower_byte(name[length - 2U])) << 8U;
    }
    result |= static_cast<std::uint64_t>(length) << 16U;
    result |= static_cast<std::uint64_t>(lower_byte(name.front())) << 24U;

    std::uint32_t extension_marker = 0;
    const auto marker_count = std::min<std::size_t>(extension.size(), 4U);
    for (std::size_t i = 0; i < marker_count; ++i) {
        extension_marker |= static_cast<std::uint32_t>(lower_byte(extension[i])) << (8U * i);
    }

    switch (extension_marker) {
    case 0x00666B2E: // .kf
        result |= 0x80U;
        break;
    case 0x66696E2E: // .nif
        result |= 0x8000U;
        break;
    case 0x7364642E: // .dds
        result |= 0x8080U;
        break;
    case 0x7661772E: // .wav
        result |= 0x80000000U;
        break;
    default:
        break;
    }

    std::uint32_t hash = 0;
    for (std::size_t i = 1; i + 2U < length; ++i) {
        hash = mix_hash(hash, name[i]);
    }
    result += static_cast<std::uint64_t>(hash) << 32U;

    hash = 0;
    for (const auto value : extension) {
        hash = mix_hash(hash, value);
    }
    result += static_cast<std::uint64_t>(hash) << 32U;

    return result;
}

std::uint64_t hash_tes4_file(std::string_view file_name) noexcept
{
    const auto dot = file_name.find_last_of('.');
    if (dot == std::string_view::npos) {
        return hash_tes4(file_name, {});
    }

    return hash_tes4(file_name.substr(0, dot), file_name.substr(dot));
}

} // namespace libbsa::detail
