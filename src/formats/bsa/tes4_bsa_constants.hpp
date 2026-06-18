#pragma once

#include <cstddef>
#include <cstdint>

namespace libbsa::formats::bsa {

inline constexpr std::uint32_t tes4_bsa_magic = 0x0041'5342U;

inline constexpr std::uint32_t tes4_bsa_oblivion_version = 0x67U;
inline constexpr std::uint32_t tes4_bsa_fallout3_version = 0x68U;
inline constexpr std::uint32_t tes4_bsa_skyrim_se_version = 0x69U;

inline constexpr std::size_t tes4_bsa_header_size = 36U;
inline constexpr std::size_t tes4_bsa_legacy_folder_record_size = 16U;
inline constexpr std::size_t tes4_bsa_sse_folder_record_size = 24U;
inline constexpr std::size_t tes4_bsa_file_record_size = 16U;

// Reader and writer paths depend on name tables being present to expose stable
// public archive paths.
inline constexpr std::uint32_t tes4_bsa_archive_include_directory_names = 0x0001U;
inline constexpr std::uint32_t tes4_bsa_archive_include_file_names = 0x0002U;

// This archive flag sets the default; the per-file size flag below XORs that
// default for one entry.
inline constexpr std::uint32_t tes4_bsa_archive_compress_by_default = 0x0004U;

// Embedded names are payload prefixes and are intentionally suppressed for
// Oblivion-compatible archives.
inline constexpr std::uint32_t tes4_bsa_archive_embed_names = 0x0100U;

inline constexpr std::uint32_t tes4_bsa_file_flag_meshes = 0x0001U;
inline constexpr std::uint32_t tes4_bsa_file_flag_textures = 0x0002U;
inline constexpr std::uint32_t tes4_bsa_file_flag_sounds = 0x0004U;
inline constexpr std::uint32_t tes4_bsa_file_flag_scripts = 0x0008U;
inline constexpr std::uint32_t tes4_bsa_file_flag_menus = 0x0010U;
inline constexpr std::uint32_t tes4_bsa_file_flag_misc = 0x0100U;

// The high size bit is not part of the stored payload length; it toggles
// compression from the archive default.
inline constexpr std::uint32_t tes4_bsa_file_size_compression_toggle = 0x4000'0000U;

}  // namespace libbsa::formats::bsa
