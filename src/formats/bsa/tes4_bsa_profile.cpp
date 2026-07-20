#include "formats/bsa/tes4_bsa_profile.hpp"

#include "formats/bsa/tes4_bsa_constants.hpp"

#include <libbsa/writer.hpp>

namespace libbsa::formats::bsa {
namespace {

constexpr std::uint32_t dxgi_format_bc1_unorm = 71U;  // DXGI_FORMAT_BC1_UNORM.
constexpr std::uint32_t dxgi_format_bc2_unorm = 74U;  // DXGI_FORMAT_BC2_UNORM.
constexpr std::uint32_t dxgi_format_bc3_unorm = 77U;  // DXGI_FORMAT_BC3_UNORM.
constexpr std::uint32_t dxgi_format_bc4_unorm = 80U;  // DXGI_FORMAT_BC4_UNORM.
constexpr std::uint32_t dxgi_format_bc5_unorm = 83U;  // DXGI_FORMAT_BC5_UNORM.
constexpr std::uint32_t dxgi_format_bc7_unorm = 98U;  // DXGI_FORMAT_BC7_UNORM.

std::string_view extension_for_path(std::string_view archive_path) noexcept {
    const auto separator = archive_path.find_last_of("/\\");
    const auto dot = archive_path.find_last_of('.');
    if (dot == std::string_view::npos || (separator != std::string_view::npos && dot < separator)) {
        return {};
    }

    return archive_path.substr(dot);
}

bool extension_is(std::string_view extension, std::string_view expected_lowercase) noexcept {
    if (extension.size() != expected_lowercase.size()) {
        return false;
    }
    for (std::size_t index = 0U; index < extension.size(); ++index) {
        auto character = extension[index];
        if (character >= 'A' && character <= 'Z') {
            character = static_cast<char>(character - 'A' + 'a');
        }
        if (character != expected_lowercase[index]) {
            return false;
        }
    }
    return true;
}

bool is_legacy_bsa_texture_format(std::uint32_t dxgi_format) noexcept {
    // Oblivion, Fallout 3/New Vegas, and classic Skyrim BSA textures are
    // limited to the DXT1-DXT5 family represented by BC1, BC2, and BC3.
    switch (dxgi_format) {
        case dxgi_format_bc1_unorm:  // DXT1.
        case dxgi_format_bc2_unorm:  // DXT2/DXT3.
        case dxgi_format_bc3_unorm:  // DXT4/DXT5.
            return true;
        default:
            return false;
    }
}

bool is_sse_bsa_texture_format(std::uint32_t dxgi_format) noexcept {
    // Skyrim SE retains the legacy DXT family and adds the established BC4,
    // BC5, and BC7 formats without widening the public API to DirectX types.
    if (is_legacy_bsa_texture_format(dxgi_format)) {
        return true;
    }
    switch (dxgi_format) {
        case dxgi_format_bc4_unorm:
        case dxgi_format_bc5_unorm:
        case dxgi_format_bc7_unorm:
            return true;
        default:
            return false;
    }
}

}  // namespace

tes4_bsa_profile::tes4_bsa_profile(profile_facts facts)
    : version_(facts.version),
      folder_shape_(facts.folder_shape),
      folder_record_size_(facts.folder_record_size),
      compressed_entry_metadata_(facts.compressed_entry_metadata),
      compressed_payload_method_(facts.compressed_payload_method),
      target_default_compressed_(facts.target_default_compressed),
      supports_embedded_names_(facts.supports_embedded_names) {}

archive_variant tes4_bsa_profile::variant() const noexcept { return archive_variant::tes4; }

std::uint32_t tes4_bsa_profile::version() const noexcept { return version_; }

tes4_folder_record_shape tes4_bsa_profile::folder_record_shape() const noexcept {
    return folder_shape_;
}

std::size_t tes4_bsa_profile::folder_record_size() const noexcept { return folder_record_size_; }

entry_compression tes4_bsa_profile::compressed_entry_metadata() const noexcept {
    return compressed_entry_metadata_;
}

detail::compression_method tes4_bsa_profile::compressed_payload_method() const noexcept {
    return compressed_payload_method_;
}

bool tes4_bsa_profile::archive_default_compressed(
    archive_compression_policy policy) const noexcept {
    switch (policy) {
        case archive_compression_policy::target_default:
            return target_default_compressed_;
        case archive_compression_policy::all_raw:
            return false;
        case archive_compression_policy::all_compressed:
            return true;
    }
    return false;
}

tes4_writer_compression_decision tes4_bsa_profile::writer_entry_compression(
    archive_compression_policy archive_policy, entry_compression_policy entry_policy,
    std::uint64_t raw_size) const noexcept {
    const bool archive_default = archive_default_compressed(archive_policy);
    bool requested_compression = false;
    switch (entry_policy) {
        case entry_compression_policy::inherit:
            requested_compression = archive_default;
            break;
        case entry_compression_policy::raw:
            requested_compression = false;
            break;
        case entry_compression_policy::compressed:
            requested_compression = true;
            break;
    }

    // Empty compressed entries would advertise a decoded-size prefix that has
    // no bytes, so established writer behavior forces them raw and toggles a
    // compressed archive default back off for that record.
    const bool effective_compression = requested_compression && raw_size != 0U;
    const auto compression =
        effective_compression ? compressed_entry_metadata_ : entry_compression::none;
    const auto record_flags =
        archive_default != effective_compression ? tes4_bsa_file_size_compression_toggle : 0U;
    return tes4_writer_compression_decision{compression, record_flags};
}

entry_compression tes4_bsa_profile::reader_entry_compression(
    std::uint32_t archive_flags, std::uint32_t record_flags) const noexcept {
    const bool archive_default = (archive_flags & tes4_bsa_archive_compress_by_default) != 0U;
    const bool record_toggle = (record_flags & tes4_bsa_file_size_compression_toggle) != 0U;
    return archive_default ^ record_toggle ? compressed_entry_metadata_ : entry_compression::none;
}

bool tes4_bsa_profile::reader_has_embedded_names(std::uint32_t archive_flags) const noexcept {
    // BSArchPro suppresses v103 payload prefixes even when the archive flag is
    // present, so support must be checked in addition to the per-archive bit.
    return supports_embedded_names_ && (archive_flags & tes4_bsa_archive_embed_names) != 0U;
}

bool tes4_bsa_profile::writer_emits_embedded_names(
    const tes4_bsa_writer_options& options) const noexcept {
    return supports_embedded_names_ && options.embed_file_names;
}

std::uint32_t tes4_bsa_profile::file_flag_for_path(std::string_view archive_path) const noexcept {
    const auto extension = extension_for_path(archive_path);
    if (extension_is(extension, ".nif") || extension_is(extension, ".kf")) {
        return tes4_bsa_file_flag_meshes;
    }
    if (extension_is(extension, ".dds")) {
        return tes4_bsa_file_flag_textures;
    }
    if (extension_is(extension, ".wav")) {
        return tes4_bsa_file_flag_sounds;
    }
    if (extension_is(extension, ".pex") || extension_is(extension, ".psc")) {
        return tes4_bsa_file_flag_scripts;
    }
    // These XML and miscellaneous masks intentionally preserve libbsa's
    // established emitted bytes; they are narrower than xEdit's broader
    // post-processed file-classification table.
    if (extension_is(extension, ".xml")) {
        return version_ == tes4_bsa_oblivion_version ? tes4_bsa_file_flag_menus : 0U;
    }
    if (extension_is(extension, ".txt") || extension_is(extension, ".html") ||
        extension_is(extension, ".bat") || extension_is(extension, ".scc")) {
        return version_ == tes4_bsa_skyrim_se_version ? 0U : tes4_bsa_file_flag_misc;
    }
    return 0U;
}

result<void> tes4_bsa_profile::validate_texture_metadata(const texture_metadata& metadata) const {
    if (folder_shape_ == tes4_folder_record_shape::sse_64_bit_offset) {
        if (!is_sse_bsa_texture_format(metadata.dxgi_format)) {
            return error{error_code::format_error,
                         "Skyrim SE BSA target supports the same DDS texture format set as "
                         "Fallout 4"};
        }
        return {};
    }

    if (!is_legacy_bsa_texture_format(metadata.dxgi_format)) {
        return error{error_code::format_error,
                     "TES4-family BSA target supports only DX9 DDS texture formats before "
                     "Skyrim SE"};
    }
    return {};
}

result<tes4_bsa_profile> make_tes4_bsa_profile_from_header(std::uint32_t version) {
    // Keep every compatibility dimension in one complete row: v103/v104 use
    // legacy 32-bit-offset folder records and deflate, while v105 changes both
    // the record shape and codec to SSE's 64-bit-offset/LZ4-frame contract.
    // Only v103's established writer target default is raw.
    switch (version) {
        case tes4_bsa_oblivion_version:
            return tes4_bsa_profile{tes4_bsa_profile::profile_facts{
                .version = version,
                .folder_shape = tes4_folder_record_shape::legacy_32_bit_offset,
                .folder_record_size = tes4_bsa_legacy_folder_record_size,
                .compressed_entry_metadata = entry_compression::deflate,
                .compressed_payload_method = detail::compression_method::deflate,
                .target_default_compressed = false,
                .supports_embedded_names = false}};
        case tes4_bsa_fallout3_version:
            return tes4_bsa_profile{tes4_bsa_profile::profile_facts{
                .version = version,
                .folder_shape = tes4_folder_record_shape::legacy_32_bit_offset,
                .folder_record_size = tes4_bsa_legacy_folder_record_size,
                .compressed_entry_metadata = entry_compression::deflate,
                .compressed_payload_method = detail::compression_method::deflate,
                .target_default_compressed = true,
                .supports_embedded_names = true}};
        case tes4_bsa_skyrim_se_version:
            return tes4_bsa_profile{tes4_bsa_profile::profile_facts{
                .version = version,
                .folder_shape = tes4_folder_record_shape::sse_64_bit_offset,
                .folder_record_size = tes4_bsa_sse_folder_record_size,
                .compressed_entry_metadata = entry_compression::lz4_frame,
                .compressed_payload_method = detail::compression_method::lz4_frame,
                .target_default_compressed = true,
                .supports_embedded_names = true}};
        default:
            return error{error_code::unsupported, "BSA header version is not supported"};
    }
}

result<tes4_bsa_profile> make_tes4_bsa_profile_for_writer(tes4_bsa_target target) {
    switch (target) {
        case tes4_bsa_target::oblivion:
            return make_tes4_bsa_profile_from_header(tes4_bsa_oblivion_version);
        case tes4_bsa_target::fallout3:
            return make_tes4_bsa_profile_from_header(tes4_bsa_fallout3_version);
        case tes4_bsa_target::skyrim_se:
            return make_tes4_bsa_profile_from_header(tes4_bsa_skyrim_se_version);
    }
    return error{error_code::invalid_argument, "TES4 BSA writer target profile is not supported"};
}

}  // namespace libbsa::formats::bsa
