#pragma once

#include "formats/bsa/tes4_bsa_profile.hpp"

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace libbsa::formats::bsa {

/// Checked TES4-family BSA header fields consumed by private parser seams.
struct tes4_bsa_header_fields {
    std::uint32_t version;
    std::uint32_t folder_offset;
    std::uint32_t archive_flags;
    std::uint32_t folder_count;
    std::uint32_t file_count;
    std::uint32_t total_folder_name_length;
    std::uint32_t total_file_name_length;
    std::uint32_t file_flags;
};

/// Raw TES4 folder table record before public path materialization.
struct tes4_bsa_folder_record {
    std::uint64_t hash;
    std::uint32_t file_count;
    std::uint64_t offset;
};

/// Raw TES4 file table record before payload descriptor and path
/// materialization.
struct tes4_bsa_file_record {
    std::uint64_t hash;
    std::uint32_t size_flags;
    std::uint32_t offset;
};

/// Raw TES4 folder block containing stored folder spelling and file records
/// only.
struct tes4_bsa_folder_block {
    std::string name;
    std::vector<tes4_bsa_file_record> files;
};

/// Private raw table seam output. It intentionally excludes canonical paths and
/// entry metadata.
struct tes4_bsa_raw_table {
    tes4_bsa_header_fields header;

    /// Where the metadata table actually ends, taken from the parser's position
    /// after the walk.
    ///
    /// The folder-name block contributes the bytes that were walked, not what
    /// `TotalFolderNameLength` declared, so a wrong value there cannot inflate
    /// this and make the payload/metadata overlap check reject legal payloads.
    /// `TotalFileNameLength` is still taken at its word, because the file-name
    /// table is the last thing before the payload region and retail archives
    /// place their first payload after the whole declared table, padding
    /// included -- true even of `Fallout - Voices1.bsa`, whose last 105 declared
    /// bytes no name consumes.
    std::size_t metadata_table_size;
    std::vector<tes4_bsa_folder_record> folder_records;
    std::vector<tes4_bsa_folder_block> folder_blocks;
    std::vector<std::string> file_names;

    /// True when TotalFileNameLength declared more bytes than the file names
    /// actually consumed. The surplus is ignored during parsing and surfaced as a
    /// public compatibility warning; see `read_tes4_bsa_raw_table`.
    bool file_name_table_has_trailing_bytes{false};

    /// True when TotalFolderNameLength disagrees with the folder-name bytes the
    /// walk actually consumed. Nothing in parsing depends on the header value, so
    /// this is a pure cross-check surfaced as a public compatibility warning.
    bool folder_name_table_length_mismatch{false};
};

/// Returns how many leading archive bytes a caller must hold to be sure it has
/// the whole TES4 metadata table.
///
/// This is an upper bound, not the table's true size, and it deliberately does
/// not derive the folder-name block from `TotalFolderNameLength`. The reference
/// never reads that field back -- it appears only on the write path
/// (`wbBSArchive.pas:1393`, `:1469`) and `TwbBSArchive.LoadFromFile` walks folder
/// names sequentially instead -- so an archive may carry a wrong value and still
/// be perfectly readable. The block is bounded by `tes4_bsa_max_folder_name_block_entry_size`
/// per folder and the result is clamped to `archive_size`.
/// `read_tes4_bsa_raw_table` reports the table's true size, measured by walking.
result<std::size_t> tes4_bsa_metadata_table_read_bound(const tes4_bsa_header_fields& header,
                                                       std::size_t folder_record_size,
                                                       std::size_t archive_size);

/// Reads the fixed TES4 header for callers that must size a file-backed
/// metadata table before loading it.
result<tes4_bsa_header_fields> read_tes4_bsa_header(std::span<const std::byte> header_bytes);

/// Reads and validates TES4 header, folder records/blocks, and file names
/// through one resolved TES4 BSA Profile without materializing entries.
result<tes4_bsa_raw_table> read_tes4_bsa_raw_table(std::span<const std::byte> table_bytes,
                                                   std::size_t archive_size,
                                                   const tes4_bsa_profile& profile);

}  // namespace libbsa::formats::bsa
