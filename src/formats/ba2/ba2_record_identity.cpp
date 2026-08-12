#include "formats/ba2/ba2_record_identity.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

namespace libbsa::formats::ba2 {
namespace {

std::string label_for(ba2_subtype subtype) {
    switch (subtype) {
        case ba2_subtype::gnrl:
            return "BA2 GNRL";
        case ba2_subtype::dx10:
            return "BA2 DX10";
    }
    return "BA2";
}

libbsa::error_code diagnostic_code_for(ba2_record_identity_source source) noexcept {
    return source == ba2_record_identity_source::filename_table ? error_code::format_error
                                                                : error_code::invalid_argument;
}

std::string invalid_path_message(ba2_subtype subtype) {
    return label_for(subtype) + " filename table contains an invalid archive path";
}

std::string missing_gnrl_file_name_message(ba2_record_identity_source source) {
    if (source == ba2_record_identity_source::filename_table) {
        return "BA2 GNRL filename table contains an invalid archive path";
    }
    return "BA2 GNRL archive path must include a file name";
}

std::string missing_dx10_stem_extension_message(ba2_record_identity_source source) {
    if (source == ba2_record_identity_source::filename_table) {
        return "BA2 DX10 filename table must include a file stem and extension";
    }
    return "BA2 DX10 archive path must include a file stem and extension";
}

std::string non_printable_extension_message(ba2_subtype subtype,
                                            ba2_record_identity_source source) {
    if (source == ba2_record_identity_source::filename_table) {
        return label_for(subtype) + " filename table extension must contain printable ASCII bytes";
    }
    return label_for(subtype) + " extension must contain printable ASCII bytes";
}

std::pair<std::string_view, std::string_view> split_directory_file(
    std::string_view archive_path) noexcept {
    const auto slash = archive_path.find_last_of('/');
    if (slash == std::string_view::npos) {
        return {{}, archive_path};
    }
    return {archive_path.substr(0U, slash), archive_path.substr(slash + 1U)};
}

std::pair<std::string_view, std::string_view> split_stem_extension(
    std::string_view file_name) noexcept {
    const auto dot = file_name.find_last_of('.');
    if (dot == std::string_view::npos || dot == 0U || dot + 1U == file_name.size()) {
        return {{}, {}};
    }
    return {file_name.substr(0U, dot), file_name.substr(dot + 1U)};
}

bool is_ascii_extension_byte(unsigned char value) noexcept {
    return value > 0x20U && value <= 0x7EU;
}

std::byte ascii_lower_byte(std::byte byte) noexcept {
    auto value = std::to_integer<unsigned char>(byte);
    if (value >= 'A' && value <= 'Z') {
        value = static_cast<unsigned char>(value - 'A' + 'a');
    }
    return static_cast<std::byte>(value);
}

bool extension_fourcc_matches(const std::array<std::byte, 4>& stored,
                              const std::array<std::byte, 4>& expected) noexcept {
    for (std::size_t index = 0; index < stored.size(); ++index) {
        if (ascii_lower_byte(stored[index]) != ascii_lower_byte(expected[index])) {
            return false;
        }
    }
    return true;
}

result<std::array<std::byte, 4>> extension_fourcc_for(ba2_subtype subtype,
                                                      std::string_view extension,
                                                      ba2_record_identity_source source) {
    // The record's Ext field is a fixed FourCC, and TES5Edit's String2Magic
    // copies at most four characters and silently discards the rest -- it has no
    // error path. Retail Fallout 4 relies on this: Interface.ba2 stores
    // .STRINGS, .ILSTRINGS, and .DLSTRINGS files as 'stri', 'ilst', and 'dlst'.
    // Rejecting the longer extension would refuse archives the game itself
    // ships, so truncate on both the parser and writer paths.
    extension = extension.substr(0U, std::min<std::size_t>(extension.size(), 4U));

    std::array<std::byte, 4> fourcc{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
    for (std::size_t index = 0; index < extension.size(); ++index) {
        const auto value = static_cast<unsigned char>(extension[index]);
        if (!is_ascii_extension_byte(value)) {
            return error{diagnostic_code_for(source),
                         non_printable_extension_message(subtype, source)};
        }
        fourcc[index] = static_cast<std::byte>(value);
    }
    return fourcc;
}

result<ba2_record_identity> make_gnrl_identity(ba2_record_path path,
                                               ba2_record_identity_source source) {
    const auto [directory, file_name] = split_directory_file(path.canonical_path);
    const auto [display_directory, display_file_name] = split_directory_file(path.display_path);
    (void)display_directory;
    if (file_name.empty()) {
        return error{diagnostic_code_for(source), missing_gnrl_file_name_message(source)};
    }

    // GNRL records hash the extension-stripped stem and carry the extension
    // separately as a FourCC lookup field, exactly as DX10 does. TES5Edit's
    // TwbBSArchive.FindFileRecordFO4 is one shared lookup path for baFO4,
    // baFO4dds, baSF, and baSFdds: it calls SplitNameExt and hashes `name`, the
    // stem, never the filename with its extension. Verified against retail
    // archives: 35,602 GNRL records across Fallout 4 v1/v8 and Starfield v2 all
    // match the stem hash and none match the full-filename hash.
    //
    // Writer derivation keeps the caller's extension casing for byte-stable
    // output; parser validation still compares stored and expected extension
    // bytes case-insensitively. The hash basis is always the canonical stem,
    // since hash_fo4 lowercases ASCII anyway.
    const auto extension_file_name =
        source == ba2_record_identity_source::writer_entry ? display_file_name : file_name;
    // An absent extension is ordinary, not malformed. TES5Edit's SplitNameExt
    // yields an empty Ext when the file name carries no dot or ends in one, and
    // String2Magic('') yields #0#0#0#0; both the FO4 writer
    // (wbBSArchive.pas:1538) and FindFileRecordFO4 (wbBSArchive.pas:960) go
    // through that path. Retail Fallout4 - Meshes.ba2 and
    // Starfield - Animations.ba2 both ship extensionless GNRL records, so
    // requiring an extension rejected whole archives (issue #43).
    const auto dot = extension_file_name.find_last_of('.');
    const auto extension_text = dot == std::string_view::npos
                                    ? std::string_view{}
                                    : extension_file_name.substr(dot + 1U);

    auto extension = extension_fourcc_for(ba2_subtype::gnrl, extension_text, source);
    if (!extension) {
        return extension.error();
    }

    const auto canonical_dot = file_name.find_last_of('.');
    const auto stem =
        canonical_dot == std::string_view::npos ? file_name : file_name.substr(0U, canonical_dot);
    const auto name_hash = detail::hash_fo4(stem);
    const auto directory_hash = detail::hash_fo4(directory);
    return ba2_record_identity{std::move(path.display_path), std::move(path.canonical_path),
                               extension.value(), name_hash, directory_hash};
}

result<ba2_record_identity> make_dx10_identity(ba2_record_path path,
                                               ba2_record_identity_source source) {
    const auto [directory, file_name] = split_directory_file(path.canonical_path);
    const auto [stem, extension_text] = split_stem_extension(file_name);
    if (stem.empty() || extension_text.empty()) {
        return error{diagnostic_code_for(source), missing_dx10_stem_extension_message(source)};
    }

    // DX10 records hash the canonical texture stem separately from its
    // directory; extension bytes are independent record metadata, so hashing the
    // full filename would not match Bethesda texture lookup semantics.
    auto extension = extension_fourcc_for(ba2_subtype::dx10, extension_text, source);
    if (!extension) {
        return extension.error();
    }

    const auto name_hash = detail::hash_fo4(stem);
    const auto directory_hash = detail::hash_fo4(directory);
    return ba2_record_identity{std::move(path.display_path), std::move(path.canonical_path),
                               extension.value(), name_hash, directory_hash};
}

}  // namespace

result<ba2_record_path> resolve_ba2_record_path(ba2_subtype subtype, std::string_view archive_path,
                                                ba2_record_identity_source source) {
    std::string display_path{archive_path};
    std::replace(display_path.begin(), display_path.end(), '\\', '/');

    auto canonical = detail::normalize_archive_path(display_path);
    if (!canonical) {
        if (source == ba2_record_identity_source::filename_table) {
            return error{error_code::format_error, invalid_path_message(subtype)};
        }
        return canonical.error();
    }

    return ba2_record_path{std::move(display_path), std::move(canonical.value().value)};
}

result<ba2_record_identity> make_ba2_record_identity(ba2_subtype subtype,
                                                     std::string_view archive_path,
                                                     ba2_record_identity_source source) {
    auto path = resolve_ba2_record_path(subtype, archive_path, source);
    if (!path) {
        return path.error();
    }
    return make_ba2_record_identity(subtype, std::move(path.value()), source);
}

result<ba2_record_identity> make_ba2_record_identity(ba2_subtype subtype, ba2_record_path path,
                                                     ba2_record_identity_source source) {
    switch (subtype) {
        case ba2_subtype::gnrl:
            return make_gnrl_identity(std::move(path), source);
        case ba2_subtype::dx10:
            return make_dx10_identity(std::move(path), source);
    }
    return error{error_code::invalid_argument, "BA2 subtype is not GNRL or DX10"};
}

ba2_record_identity_mismatch compare_ba2_record_identity(
    const ba2_stored_record_identity& stored, const ba2_record_identity& expected) noexcept {
    return ba2_record_identity_mismatch{
        stored.name_hash != expected.name_hash,
        stored.directory_hash != expected.directory_hash,
        !extension_fourcc_matches(stored.extension, expected.extension)};
}

}  // namespace libbsa::formats::ba2
