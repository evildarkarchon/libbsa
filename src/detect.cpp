#include <libbsa/detect.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa {
namespace {

constexpr std::uint32_t magic_tes3 = 0x00000100U;
constexpr std::uint32_t magic_bsa = 0x00415342U;
constexpr std::uint32_t magic_btdx = 0x58445442U;
constexpr std::uint32_t magic_gnrl = 0x4c524e47U;
constexpr std::uint32_t magic_dx10 = 0x30315844U;

constexpr std::uint32_t version_tes4 = 0x67U;
constexpr std::uint32_t version_fo3 = 0x68U;
constexpr std::uint32_t version_sse = 0x69U;
constexpr std::uint32_t version_fo4_v1 = 0x01U;
constexpr std::uint32_t version_starfield_v2 = 0x02U;
constexpr std::uint32_t version_starfield_v3 = 0x03U;
constexpr std::uint32_t version_fo4_v7 = 0x07U;
constexpr std::uint32_t version_fo4_v8 = 0x08U;

result<std::array<std::byte, 36>> read_header(const byte_source& source, std::uint64_t count)
{
    std::array<std::byte, 36> bytes{};
    auto read = source.read_at(0, std::span<std::byte>{bytes}.first(static_cast<std::size_t>(count)));
    if (!read.has_value()) {
        return failure<std::array<std::byte, 36>>({error_code::malformed_archive, "truncated archive header"});
    }

    return success(bytes);
}

std::uint32_t read_u32(const std::array<std::byte, 36>& bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

std::uint64_t read_u64(const std::array<std::byte, 36>& bytes, std::size_t offset)
{
    return static_cast<std::uint64_t>(read_u32(bytes, offset)) |
        (static_cast<std::uint64_t>(read_u32(bytes, offset + 4)) << 32U);
}

result<archive_summary> unsupported_version()
{
    return failure<archive_summary>({error_code::unsupported_format, "unsupported archive version"});
}

result<archive_summary> detect_tes3(const byte_source& source)
{
    auto header = read_header(source, 12);
    if (!header.has_value()) {
        return failure<archive_summary>(header.error());
    }

    archive_summary summary{};
    summary.format = archive_format::tes3_bsa;
    summary.file_table_offset = read_u32(header.value(), 4);
    summary.file_count = read_u32(header.value(), 8);
    return success(summary);
}

result<archive_summary> detect_bsa(const byte_source& source)
{
    auto header = read_header(source, 36);
    if (!header.has_value()) {
        return failure<archive_summary>(header.error());
    }

    const auto version = read_u32(header.value(), 4);
    archive_summary summary{};
    summary.version = version;
    switch (version) {
    case version_tes4:
        summary.format = archive_format::tes4_bsa;
        break;
    case version_fo3:
        summary.format = archive_format::fo3_bsa;
        break;
    case version_sse:
        summary.format = archive_format::sse_bsa;
        break;
    default:
        return unsupported_version();
    }

    summary.file_table_offset = read_u32(header.value(), 8);
    summary.flags = read_u32(header.value(), 12);
    summary.folder_count = read_u32(header.value(), 16);
    summary.file_count = read_u32(header.value(), 20);
    return success(summary);
}

result<archive_summary> detect_ba2(const byte_source& source)
{
    auto base = read_header(source, 12);
    if (!base.has_value()) {
        return failure<archive_summary>(base.error());
    }

    const auto version = read_u32(base.value(), 4);
    std::uint64_t required_size = 24;
    if (version == version_starfield_v2) {
        required_size = 32;
    } else if (version == version_starfield_v3) {
        required_size = 36;
    }

    auto header = read_header(source, required_size);
    if (!header.has_value()) {
        return failure<archive_summary>(header.error());
    }

    archive_summary summary{};
    summary.version = version;
    summary.subtype = read_u32(header.value(), 8);
    summary.file_count = read_u32(header.value(), 12);
    summary.file_table_offset = read_u64(header.value(), 16);
    if (version == version_starfield_v3) {
        summary.compression_method = read_u32(header.value(), 32);
    }

    const bool is_gnrl = *summary.subtype == magic_gnrl;
    const bool is_dx10 = *summary.subtype == magic_dx10;
    if (!is_gnrl && !is_dx10) {
        return failure<archive_summary>({error_code::unsupported_format, "unsupported BA2 subtype"});
    }

    switch (version) {
    case version_fo4_v1:
    case version_fo4_v7:
    case version_fo4_v8:
        summary.format = is_gnrl ? archive_format::fo4_ba2_gnrl : archive_format::fo4_ba2_dds;
        return success(summary);
    case version_starfield_v2:
    case version_starfield_v3:
        summary.format = is_gnrl ? archive_format::starfield_ba2_gnrl : archive_format::starfield_ba2_dds;
        return success(summary);
    default:
        return unsupported_version();
    }
}

} // namespace

result<archive_summary> detect_archive(const byte_source& source)
{
    auto magic = read_header(source, 4);
    if (!magic.has_value()) {
        return failure<archive_summary>(magic.error());
    }

    switch (read_u32(magic.value(), 0)) {
    case magic_tes3:
        return detect_tes3(source);
    case magic_bsa:
        return detect_bsa(source);
    case magic_btdx:
        return detect_ba2(source);
    default:
        return failure<archive_summary>({error_code::unsupported_format, "unknown archive magic"});
    }
}

} // namespace libbsa
