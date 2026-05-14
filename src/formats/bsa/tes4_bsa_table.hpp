#pragma once

#include "formats/bsa/bsa_format_detector.hpp"

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

/// Raw TES4 file table record before payload descriptor and path materialization.
struct tes4_bsa_file_record {
  std::uint64_t hash;
  std::uint32_t size_flags;
  std::uint32_t offset;
};

/// Raw TES4 folder block containing stored folder spelling and file records only.
struct tes4_bsa_folder_block {
  std::string name;
  std::vector<tes4_bsa_file_record> files;
};

/// Private raw table seam output. It intentionally excludes canonical paths and entry metadata.
struct tes4_bsa_raw_table {
  tes4_bsa_header_fields header;
  std::size_t metadata_table_size;
  std::vector<tes4_bsa_folder_record> folder_records;
  std::vector<tes4_bsa_folder_block> folder_blocks;
  std::vector<std::string> file_names;
};

/// Computes the checked TES4 metadata table byte size for an already-read header.
result<std::size_t> tes4_bsa_metadata_table_size(const tes4_bsa_header_fields& header,
                                                 std::size_t folder_record_size,
                                                 std::size_t archive_size);

/// Reads the fixed TES4 header for callers that must size a file-backed metadata table before loading it.
result<tes4_bsa_header_fields> read_tes4_bsa_header(std::span<const std::byte> header_bytes);

/// Reads and validates TES4 header, folder records/blocks, and file names without materializing entries.
result<tes4_bsa_raw_table> read_tes4_bsa_raw_table(std::span<const std::byte> table_bytes,
                                                   std::size_t archive_size,
                                                   detected_bsa_format detected);

} // namespace libbsa::formats::bsa
