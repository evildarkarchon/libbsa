#include "formats/ba2/ba2_archive_header.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <detail/binary_io.hpp>
#include <detail/parser_primitives.hpp>

#include <string>
#include <utility>

namespace libbsa::formats::ba2 {
namespace {

/// Reads one required 32-bit field with a fixed-header-specific truncation error.
result<std::uint32_t> read_required_u32(detail::binary_reader& reader,
                                        std::string_view field_name) {
    auto value = reader.read_u32_le();
    if (!value) {
        return error{error_code::format_error,
                     "BA2 header is truncated before " + std::string{field_name}};
    }
    return value.value();
}

/// Reads one required 64-bit field with a fixed-header-specific truncation error.
result<std::uint64_t> read_required_u64(detail::binary_reader& reader,
                                        std::string_view field_name) {
    auto value = reader.read_u64_le();
    if (!value) {
        return error{error_code::format_error,
                     "BA2 header is truncated before " + std::string{field_name}};
    }
    return value.value();
}

}  // namespace

ba2_archive_header::ba2_archive_header(ba2_profile profile, std::uint32_t file_count,
                                       std::uint64_t filename_table_offset,
                                       ba2_archive_metadata stored_metadata)
    : profile_{std::move(profile)},
      file_count_{file_count},
      filename_table_offset_{filename_table_offset},
      stored_metadata_{std::move(stored_metadata)} {}

const ba2_profile& ba2_archive_header::profile() const noexcept { return profile_; }

std::uint32_t ba2_archive_header::file_count() const noexcept { return file_count_; }

std::uint64_t ba2_archive_header::filename_table_offset() const noexcept {
    return filename_table_offset_;
}

const ba2_archive_metadata& ba2_archive_header::stored_metadata() const noexcept {
    return stored_metadata_;
}

archive_metadata ba2_archive_header::materialize_metadata() const {
    return archive_metadata{archive_type::ba2,  profile_.variant(),
                            profile_.version(), 0U,
                            file_count_,        profile_.default_compression(),
                            stored_metadata_};
}

result<ba2_archive_header> decode_ba2_archive_header(std::span<const std::byte> bytes,
                                                     std::uint64_t archive_size) {
    detail::binary_reader reader{bytes};
    auto magic = read_required_u32(reader, "BTDX magic");
    if (!magic) {
        return magic.error();
    }
    if (magic.value() != ba2_btdx_magic) {
        return error{error_code::unsupported, "archive bytes do not start with BTDX magic"};
    }

    auto version = read_required_u32(reader, "version");
    if (!version) {
        return version.error();
    }
    auto subtype_magic = read_required_u32(reader, "subtype");
    if (!subtype_magic) {
        return subtype_magic.error();
    }
    auto subtype = ba2_subtype_from_magic(subtype_magic.value());
    if (!subtype) {
        return subtype.error();
    }
    auto file_count = read_required_u32(reader, "file count");
    if (!file_count) {
        return file_count.error();
    }
    auto filename_table_offset = read_required_u64(reader, "filename table offset");
    if (!filename_table_offset) {
        return filename_table_offset.error();
    }

    ba2_archive_metadata stored_metadata{};
    switch (version.value()) {
        case ba2_fallout4_version:
            break;
        case ba2_starfield_v2_version:
        case ba2_starfield_v3_version: {
            auto unknown1 = read_required_u32(reader, "Starfield Unknown1");
            if (!unknown1) {
                return unknown1.error();
            }
            auto unknown2 = read_required_u32(reader, "Starfield Unknown2");
            if (!unknown2) {
                return unknown2.error();
            }
            stored_metadata.starfield_unknown1 = unknown1.value();
            stored_metadata.starfield_unknown2 = unknown2.value();
            if (version.value() == ba2_starfield_v3_version) {
                auto compression_method = read_required_u32(reader, "Starfield CompressionMethod");
                if (!compression_method) {
                    return compression_method.error();
                }
                stored_metadata.compression_method = compression_method.value();
            }
            break;
        }
        default:
            return error{error_code::unsupported, "BA2 header version is not supported"};
    }

    auto profile = make_ba2_profile_from_header(version.value(), subtype.value(), stored_metadata);
    if (!profile) {
        return profile.error();
    }
    auto count_limit = detail::validate_metadata_count(
        file_count.value(), detail::metadata_entry_count_limit, "BA2 file count");
    if (!count_limit) {
        return count_limit.error();
    }
    if (filename_table_offset.value() < profile.value().header_size() ||
        filename_table_offset.value() > archive_size) {
        return error{error_code::format_error, "BA2 filename table offset is outside the archive"};
    }

    return ba2_archive_header{std::move(profile).value(), file_count.value(),
                              filename_table_offset.value(), std::move(stored_metadata)};
}

}  // namespace libbsa::formats::ba2
