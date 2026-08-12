#include "formats/ba2/ba2_profile.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <libbsa/writer.hpp>

#include <string>
#include <string_view>

namespace libbsa::formats::ba2 {
namespace {

std::uint32_t version_for(ba2_gnrl_target target) noexcept {
    switch (target) {
        case ba2_gnrl_target::fallout4:
            return ba2_fallout4_version;
        case ba2_gnrl_target::starfield_v2:
            return ba2_starfield_v2_version;
        case ba2_gnrl_target::starfield_v3:
            return ba2_starfield_v3_version;
    }
    return 0U;
}

std::uint32_t version_for(ba2_dx10_target target) noexcept {
    switch (target) {
        case ba2_dx10_target::fallout4:
            return ba2_fallout4_version;
        case ba2_dx10_target::starfield_v2:
            return ba2_starfield_v2_version;
        case ba2_dx10_target::starfield_v3:
            return ba2_starfield_v3_version;
    }
    return 0U;
}

result<detail::compression_method> method_for_starfield_v3(std::uint32_t compression_method,
                                                           std::string_view error_message) {
    if (compression_method == ba2_starfield_compression_deflate) {
        return detail::compression_method::zlib;
    }
    if (compression_method == ba2_starfield_compression_lz4_block) {
        return detail::compression_method::lz4_block;
    }
    return error{error_code::unsupported, std::string{error_message}};
}

entry_compression public_compression_for(detail::compression_method method) noexcept {
    switch (method) {
        case detail::compression_method::zlib:
            return entry_compression::deflate;
        case detail::compression_method::lz4_block:
            return entry_compression::lz4_block;
        case detail::compression_method::lz4_frame:
            return entry_compression::lz4_frame;
    }
    return entry_compression::deflate;
}

}  // namespace

result<ba2_header_layout> ba2_header_layout_for_version(std::uint32_t version) {
    switch (version) {
        case ba2_fallout4_version:
        case ba2_fallout4_ng_v7_version:
        case ba2_fallout4_ng_v8_version:
            // Verified against retail Fallout 4 archives: the BAADF00D sentinel
            // sits at byte 56 for GNRL and byte 68 for DX10 in v1, v7, and v8
            // alike, so all three share the 24-byte header with no trailing
            // fields. wbBSArchive.pas agrees: TwbBSArchive.LoadFromFile maps
            // HEADER_VERSION_FO4v1, HEADER_VERSION_FO4NGv7, and
            // HEADER_VERSION_FO4NGv8 to baFO4 with the default zlib/deflate
            // compression type, and reads no version-specific header fields for
            // them.
            return ba2_header_layout{archive_variant::fallout4, ba2_common_header_size, false,
                                     false};
        case ba2_starfield_v2_version:
            return ba2_header_layout{archive_variant::starfield, ba2_starfield_v2_header_size, true,
                                     false};
        case ba2_starfield_v3_version:
            return ba2_header_layout{archive_variant::starfield, ba2_starfield_v3_header_size, true,
                                     true};
        default:
            return error{error_code::unsupported, "BA2 header version is not supported"};
    }
}

result<ba2_profile> make_profile(std::uint32_t version, ba2_subtype subtype,
                                 detail::compression_method method) {
    auto layout = ba2_header_layout_for_version(version);
    if (!layout) {
        return layout.error();
    }
    return ba2_profile{std::move(layout).value(), subtype, version, public_compression_for(method),
                       method};
}

ba2_profile::ba2_profile(ba2_header_layout layout, ba2_subtype subtype, std::uint32_t version,
                         entry_compression default_compression,
                         detail::compression_method compressed_method)
    : layout_(layout),
      subtype_(subtype),
      version_(version),
      default_compression_(default_compression),
      compressed_method_(compressed_method) {}

archive_variant ba2_profile::variant() const noexcept { return layout_.variant; }

ba2_subtype ba2_profile::subtype() const noexcept { return subtype_; }

std::uint32_t ba2_profile::subtype_magic() const noexcept {
    switch (subtype_) {
        case ba2_subtype::gnrl:
            return ba2_gnrl_magic;
        case ba2_subtype::dx10:
            return ba2_dx10_magic;
    }
    return ba2_gnrl_magic;
}

std::uint32_t ba2_profile::version() const noexcept { return version_; }

std::size_t ba2_profile::header_size() const noexcept { return layout_.header_size; }

bool ba2_profile::has_starfield_unknown_fields() const noexcept {
    return layout_.has_starfield_unknown_fields;
}

bool ba2_profile::has_compression_method_field() const noexcept {
    return layout_.has_compression_method;
}

entry_compression ba2_profile::default_compression() const noexcept { return default_compression_; }

detail::compression_method ba2_profile::compressed_payload_method() const noexcept {
    return compressed_method_;
}

bool ba2_profile::is_gnrl() const noexcept { return subtype_ == ba2_subtype::gnrl; }

bool ba2_profile::is_dx10() const noexcept { return subtype_ == ba2_subtype::dx10; }

result<ba2_subtype> ba2_subtype_from_magic(std::uint32_t subtype_magic) {
    if (subtype_magic == ba2_gnrl_magic) {
        return ba2_subtype::gnrl;
    }
    if (subtype_magic == ba2_dx10_magic) {
        return ba2_subtype::dx10;
    }
    return error{error_code::unsupported, "BA2 subtype is not GNRL or DX10"};
}

result<ba2_profile> make_ba2_profile_from_header(std::uint32_t version, ba2_subtype subtype,
                                                 ba2_archive_metadata metadata) {
    auto layout = ba2_header_layout_for_version(version);
    if (!layout) {
        return layout.error();
    }
    // Only the v3 CompressionMethod field can move a header off the deflate
    // default. Fallout 4 v1/v7/v8 and Starfield v2 have no such field, so their
    // compressed payloads are always deflate.
    if (!layout.value().has_compression_method) {
        return make_profile(version, subtype, detail::compression_method::zlib);
    }
    if (!metadata.compression_method.has_value()) {
        return error{error_code::format_error, "Starfield BA2 v3 CompressionMethod is truncated"};
    }
    auto method = method_for_starfield_v3(*metadata.compression_method,
                                          "Starfield BA2 v3 CompressionMethod is unsupported");
    if (!method) {
        return method.error();
    }
    return make_profile(version, subtype, method.value());
}

result<ba2_profile> make_ba2_profile_for_gnrl_writer(ba2_gnrl_target target,
                                                     const ba2_gnrl_writer_options& options) {
    switch (target) {
        case ba2_gnrl_target::fallout4:
            return make_profile(version_for(target), ba2_subtype::gnrl,
                                detail::compression_method::zlib);
        case ba2_gnrl_target::starfield_v2:
            return make_profile(version_for(target), ba2_subtype::gnrl,
                                detail::compression_method::zlib);
        case ba2_gnrl_target::starfield_v3: {
            auto method =
                method_for_starfield_v3(options.starfield_compression_method,
                                        "BA2 GNRL Starfield v3 compression method is unsupported");
            if (!method) {
                return method.error();
            }
            return make_profile(version_for(target), ba2_subtype::gnrl, method.value());
        }
    }
    return error{error_code::invalid_argument, "BA2 GNRL writer target profile is not supported"};
}

result<ba2_profile> make_ba2_profile_for_dx10_writer(ba2_dx10_target target,
                                                     const ba2_dx10_writer_options& options) {
    switch (target) {
        case ba2_dx10_target::fallout4:
            return make_profile(version_for(target), ba2_subtype::dx10,
                                detail::compression_method::zlib);
        case ba2_dx10_target::starfield_v2:
            return make_profile(version_for(target), ba2_subtype::dx10,
                                detail::compression_method::zlib);
        case ba2_dx10_target::starfield_v3: {
            auto method =
                method_for_starfield_v3(options.starfield_compression_method,
                                        "BA2 DX10 Starfield v3 compression method is unsupported");
            if (!method) {
                return method.error();
            }
            return make_profile(version_for(target), ba2_subtype::dx10, method.value());
        }
    }
    return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
}

result<detail::compression_method> ba2_compressed_payload_method(ba2_subtype subtype,
                                                                 entry_compression compression) {
    switch (compression) {
        case entry_compression::deflate:
            return detail::compression_method::zlib;
        case entry_compression::lz4_block:
            return detail::compression_method::lz4_block;
        case entry_compression::none:
            return error{error_code::format_error,
                         subtype == ba2_subtype::gnrl
                             ? "BA2 GNRL raw entries must not enter decompression routing"
                             : "BA2 DX10 raw chunks must not enter decompression routing"};
        case entry_compression::lz4_frame:
            return error{error_code::format_error,
                         subtype == ba2_subtype::gnrl
                             ? "BA2 GNRL does not support LZ4 frame payloads"
                             : "BA2 DX10 does not support lz4_frame chunk payloads"};
    }
    return error{error_code::format_error, subtype == ba2_subtype::gnrl
                                               ? "BA2 GNRL entry has unknown compression metadata"
                                               : "BA2 DX10 chunk has unknown compression metadata"};
}

}  // namespace libbsa::formats::ba2
