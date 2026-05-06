#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace libbsa::detail {

inline constexpr std::uint32_t bsa_magic = 0x00415342;
inline constexpr std::uint32_t magic_tes3 = 0x00000100;
inline constexpr std::uint32_t version_tes4 = 0x67;
inline constexpr std::uint32_t version_fo3 = 0x68;
inline constexpr std::uint32_t version_sse = 0x69;
inline constexpr std::uint64_t header_size = 36;
inline constexpr std::uint64_t tes3_header_size = 12;
inline constexpr std::uint64_t tes3_file_record_size = 8;
inline constexpr std::uint64_t tes3_name_offset_size = 4;
inline constexpr std::uint64_t tes3_hash_size = 8;
inline constexpr std::uint32_t archive_pathnames = 0x0001;
inline constexpr std::uint32_t archive_filenames = 0x0002;
inline constexpr std::uint32_t archive_compress = 0x0004;
inline constexpr std::uint32_t archive_embed_name = 0x0100;
inline constexpr std::uint32_t file_size_compress = 0x40000000;
inline constexpr std::uint64_t folder_record_size_legacy = 16;
inline constexpr std::uint64_t folder_record_size_sse = 24;

struct bsa_header {
    std::uint32_t version{};
    std::uint32_t folders_offset{};
    std::uint32_t flags{};
    std::uint32_t folder_count{};
    std::uint32_t file_count{};
    std::uint32_t total_folder_name_length{};
    std::uint32_t total_file_name_length{};
    std::uint32_t file_flags{};
};

[[nodiscard]] bool bsa_is_supported_version(std::uint32_t version) noexcept;
[[nodiscard]] std::string bsa_join_path(std::string folder, std::string file);

} // namespace libbsa::detail
