#include "formats/ba2/ba2_profile.hpp"

#include "formats/ba2/ba2_constants.hpp"

#include <libbsa/writer.hpp>

#include <string>
#include <string_view>
#include <utility>

namespace libbsa::formats::ba2 {
namespace {

std::size_t header_size_for_version(std::uint32_t version) noexcept {
    if (version >= ba2_starfield_v3_version) {
        return ba2_starfield_v3_header_size;
    }
    if (version >= ba2_starfield_v2_version) {
        return ba2_starfield_v2_header_size;
    }
    return ba2_common_header_size;
}

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
        case ba2_dx10_target::starfield_v3:
            return ba2_starfield_v3_version;
    }
    return 0U;
}

result<detail::compression_method> method_for_starfield_v3(std::uint32_t compression_method,
                                                           std::string_view error_message) {
    if (compression_method == ba2_starfield_compression_deflate) {
        return detail::compression_method::deflate;
    }
    if (compression_method == ba2_starfield_compression_lz4_block) {
        return detail::compression_method::lz4_block;
    }
    return error{error_code::unsupported, std::string{error_message}};
}

entry_compression public_compression_for(detail::compression_method method) noexcept {
    switch (method) {
        case detail::compression_method::deflate:
            return entry_compression::deflate;
        case detail::compression_method::lz4_block:
            return entry_compression::lz4_block;
        case detail::compression_method::lz4_frame:
            return entry_compression::lz4_frame;
    }
    return entry_compression::deflate;
}

}  // namespace

result<ba2_profile> make_profile(std::uint32_t version, ba2_subtype subtype,
                                 ba2_archive_metadata metadata,
                                 detail::compression_method method) {
    const auto variant =
        version == ba2_fallout4_version ? archive_variant::fallout4 : archive_variant::starfield;
    return ba2_profile{variant, subtype, version, header_size_for_version(version),
                       public_compression_for(method), std::move(metadata), method};
}

ba2_profile::ba2_profile(archive_variant variant, ba2_subtype subtype, std::uint32_t version,
                         std::size_t header_size, entry_compression default_compression,
                         ba2_archive_metadata metadata,
                         detail::compression_method compressed_method)
    : variant_(variant),
      subtype_(subtype),
      version_(version),
      header_size_(header_size),
      default_compression_(default_compression),
      metadata_(std::move(metadata)),
      compressed_method_(compressed_method) {}

archive_variant ba2_profile::variant() const noexcept { return variant_; }

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

std::size_t ba2_profile::header_size() const noexcept { return header_size_; }

entry_compression ba2_profile::default_compression() const noexcept {
    return default_compression_;
}

const ba2_archive_metadata& ba2_profile::ba2_metadata() const noexcept { return metadata_; }

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
    switch (version) {
        case ba2_fallout4_version:
            return make_profile(version, subtype, std::move(metadata),
                                detail::compression_method::deflate);
        case ba2_starfield_v2_version:
            return make_profile(version, subtype, std::move(metadata),
                                detail::compression_method::deflate);
        case ba2_starfield_v3_version: {
            if (!metadata.compression_method.has_value()) {
                return error{error_code::format_error,
                             "Starfield BA2 v3 CompressionMethod is truncated"};
            }
            auto method =
                method_for_starfield_v3(*metadata.compression_method,
                                        "Starfield BA2 v3 CompressionMethod is unsupported");
            if (!method) {
                return method.error();
            }
            return make_profile(version, subtype, std::move(metadata), method.value());
        }
        default:
            return error{error_code::unsupported, "BA2 header version is not supported"};
    }
}

result<ba2_profile> make_ba2_profile_for_gnrl_writer(
    ba2_gnrl_target target, const ba2_gnrl_writer_options& options) {
    ba2_archive_metadata metadata{};
    switch (target) {
        case ba2_gnrl_target::fallout4:
            return make_profile(version_for(target), ba2_subtype::gnrl, metadata,
                                detail::compression_method::deflate);
        case ba2_gnrl_target::starfield_v2:
            metadata.starfield_unknown1 = options.starfield_unknown1;
            metadata.starfield_unknown2 = options.starfield_unknown2;
            return make_profile(version_for(target), ba2_subtype::gnrl, metadata,
                                detail::compression_method::deflate);
        case ba2_gnrl_target::starfield_v3: {
            metadata.starfield_unknown1 = options.starfield_unknown1;
            metadata.starfield_unknown2 = options.starfield_unknown2;
            metadata.compression_method = options.starfield_compression_method;
            auto method = method_for_starfield_v3(
                options.starfield_compression_method,
                "BA2 GNRL Starfield v3 compression method is unsupported");
            if (!method) {
                return method.error();
            }
            return make_profile(version_for(target), ba2_subtype::gnrl, metadata, method.value());
        }
    }
    return error{error_code::invalid_argument, "BA2 GNRL writer target profile is not supported"};
}

result<ba2_profile> make_ba2_profile_for_dx10_writer(
    ba2_dx10_target target, const ba2_dx10_writer_options& options) {
    ba2_archive_metadata metadata{};
    switch (target) {
        case ba2_dx10_target::fallout4:
            return make_profile(version_for(target), ba2_subtype::dx10, metadata,
                                detail::compression_method::deflate);
        case ba2_dx10_target::starfield_v3: {
            metadata.starfield_unknown1 = options.starfield_unknown1;
            metadata.starfield_unknown2 = options.starfield_unknown2;
            metadata.compression_method = options.starfield_compression_method;
            auto method = method_for_starfield_v3(
                options.starfield_compression_method,
                "BA2 DX10 Starfield v3 compression method is unsupported");
            if (!method) {
                return method.error();
            }
            return make_profile(version_for(target), ba2_subtype::dx10, metadata, method.value());
        }
    }
    return error{error_code::invalid_argument, "BA2 DX10 writer target profile is not supported"};
}

result<detail::compression_method> ba2_compressed_payload_method(
    ba2_subtype subtype, entry_compression compression) {
    switch (compression) {
        case entry_compression::deflate:
            return detail::compression_method::deflate;
        case entry_compression::lz4_block:
            return detail::compression_method::lz4_block;
        case entry_compression::none:
            return error{
                error_code::format_error,
                subtype == ba2_subtype::gnrl
                    ? "BA2 GNRL raw entries must not enter decompression routing"
                    : "BA2 DX10 raw chunks must not enter decompression routing"};
        case entry_compression::lz4_frame:
            return error{error_code::format_error,
                         subtype == ba2_subtype::gnrl
                             ? "BA2 GNRL does not support LZ4 frame payloads"
                             : "BA2 DX10 does not support lz4_frame chunk payloads"};
    }
    return error{error_code::format_error,
                 subtype == ba2_subtype::gnrl
                     ? "BA2 GNRL entry has unknown compression metadata"
                     : "BA2 DX10 chunk has unknown compression metadata"};
}

}  // namespace libbsa::formats::ba2
