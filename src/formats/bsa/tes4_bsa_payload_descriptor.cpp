#include "formats/bsa/tes4_bsa_payload_descriptor.hpp"

namespace libbsa::formats::bsa {
namespace {

bool non_empty_span_intersects_prefix(std::size_t start, std::size_t length, std::size_t prefix_size) noexcept {
  return length != 0U && start < prefix_size;
}

} // namespace

entry_compression tes4_bsa_compression_for(const tes4_bsa_header_fields& header, std::uint32_t size_flags) noexcept {
  const bool default_compressed = (header.archive_flags & tes4_bsa_archive_compress_by_default) != 0U;
  const bool toggled = (size_flags & tes4_bsa_file_size_compression_toggle) != 0U;
  if (!(default_compressed ^ toggled)) {
    return entry_compression::none;
  }
  return header.version == tes4_bsa_skyrim_se_version ? entry_compression::lz4_frame : entry_compression::deflate;
}

bool tes4_bsa_has_embedded_names(const tes4_bsa_header_fields& header) noexcept {
  return header.version != tes4_bsa_oblivion_version && (header.archive_flags & tes4_bsa_archive_embed_names) != 0U;
}

result<std::uint32_t> tes4_bsa_stored_payload_size(const tes4_bsa_file_record& record,
                                                   std::size_t archive_size,
                                                   std::size_t metadata_size) {
  const auto stored_size = record.size_flags & ~tes4_bsa_file_size_compression_toggle;
  if (!detail::span_fits(record.offset, stored_size, archive_size)) {
    return error{error_code::format_error, "TES4 BSA entry payload span is outside the archive"};
  }
  // Payload offsets are archive-controlled; non-empty file bytes must not point back into the header or name tables.
  if (non_empty_span_intersects_prefix(record.offset, stored_size, metadata_size)) {
    return error{error_code::format_error, "TES4 BSA entry payload span overlaps metadata"};
  }
  return stored_size;
}

} // namespace libbsa::formats::bsa
