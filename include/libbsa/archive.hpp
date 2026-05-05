#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace libbsa {

/// Identifies the supported archive family and variant detected by libbsa.
enum class archive_format {
    tes3_bsa,
    tes4_bsa,
    fo3_bsa,
    sse_bsa,
    fo4_ba2_gnrl,
    fo4_ba2_dds,
    starfield_ba2_gnrl,
    starfield_ba2_dds,
};

/// Describes how an entry's payload is stored relative to archive defaults.
enum class compression_state {
    none,
    raw,
    deflate,
    lz4_frame,
    lz4_block,
    archive_default,
    unknown,
};

/// Metadata safely available from bounded archive header detection.
///
/// Optional fields are populated only when the detector has read the required
/// header bytes for that archive family; this type does not imply full table
/// parsing or payload validation.
struct archive_summary {
    archive_format format{};
    std::optional<std::uint32_t> version;
    std::optional<std::uint32_t> subtype;
    std::optional<std::uint32_t> flags;
    std::optional<std::uint32_t> folder_count;
    std::optional<std::uint32_t> file_count;
    std::optional<std::uint64_t> file_table_offset;
    std::optional<std::uint32_t> compression_method;
};

/// Copied metadata for a single archive entry.
///
/// The `path` value is expected to be a normalized archive-virtual path when it
/// is stored in lookup views; hashes are retained for compatibility-specific
/// readers that need family-native lookup data.
struct entry_metadata {
    std::string path;
    std::uint64_t size{};
    std::uint64_t packed_size{};
    std::uint64_t offset{};
    std::uint64_t name_hash{};
    std::uint64_t directory_hash{};
    compression_state compression{compression_state::unknown};
};

} // namespace libbsa
