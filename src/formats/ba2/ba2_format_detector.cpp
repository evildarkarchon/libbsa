#include "formats/ba2/ba2_format_detector.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/binary_io.hpp>

#include <utility>

namespace libbsa::formats::ba2 {

result<detected_ba2_format> detect_ba2_format(std::span<const std::byte> bytes) {
    detail::binary_reader reader{bytes};
    const auto magic = reader.read_u32_le();
    if (!magic) {
        return magic.error();
    }
    if (magic.value() != ba2_btdx_magic) {
        return error{error_code::unsupported, "archive bytes do not start with BTDX magic"};
    }

    const auto version = reader.read_u32_le();
    if (!version) {
        return error{error_code::format_error, "BA2 header is truncated before version"};
    }

    const auto subtype = reader.read_u32_le();
    if (!subtype) {
        return error{error_code::format_error, "BA2 header is truncated before subtype"};
    }
    auto ba2_subtype = ba2_subtype_from_magic(subtype.value());
    if (!ba2_subtype) {
        return ba2_subtype.error();
    }

    const auto file_count = reader.read_u32_le();
    if (!file_count) {
        return error{error_code::format_error, "BA2 header is truncated before file count"};
    }
    const auto file_table_offset = reader.read_u64_le();
    if (!file_table_offset) {
        return error{error_code::format_error, "BA2 header is truncated before file table offset"};
    }

    ba2_archive_metadata ba2{};
    switch (version.value()) {
        case ba2_fallout4_version:
            break;
        case ba2_starfield_v2_version: {
            const auto unknown1 = reader.read_u32_le();
            const auto unknown2 = reader.read_u32_le();
            if (!unknown1 || !unknown2) {
                return error{error_code::format_error,
                             "Starfield BA2 v2 header is truncated before Unknown fields"};
            }
            ba2.starfield_unknown1 = unknown1.value();
            ba2.starfield_unknown2 = unknown2.value();
            break;
        }
        case ba2_starfield_v3_version: {
            const auto unknown1 = reader.read_u32_le();
            const auto unknown2 = reader.read_u32_le();
            const auto compression_method = reader.read_u32_le();
            if (!unknown1 || !unknown2 || !compression_method) {
                return error{error_code::format_error,
                             "Starfield BA2 v3 header is truncated before CompressionMethod"};
            }
            ba2.starfield_unknown1 = unknown1.value();
            ba2.starfield_unknown2 = unknown2.value();
            ba2.compression_method = compression_method.value();
            break;
        }
        default:
            return error{error_code::unsupported, "BA2 header version is not supported"};
    }

    auto profile = make_ba2_profile_from_header(version.value(), ba2_subtype.value(), ba2);
    if (!profile) {
        return profile.error();
    }
    return detected_ba2_format{profile.value(), file_count.value(), std::move(ba2)};
}

}  // namespace libbsa::formats::ba2
