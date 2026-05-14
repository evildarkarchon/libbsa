#pragma once

#include "formats/bsa/tes4_bsa_constants.hpp"
#include "formats/bsa/tes4_bsa_table.hpp"

#include <detail/parser_primitives.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace libbsa::formats::bsa {

/// Private TES4 payload facts derived from a raw file record before public entry materialization.
struct tes4_bsa_payload_descriptor {
  std::uint64_t payload_offset;
  std::uint32_t stored_size;
  std::uint32_t embedded_prefix_size;
  std::uint32_t raw_size;
  entry_compression compression;
};

/// Interprets TES4 archive default compression and the per-file toggle bit for a raw file record.
entry_compression tes4_bsa_compression_for(const tes4_bsa_header_fields& header, std::uint32_t size_flags) noexcept;

/// Returns whether TES4 payloads carry embedded filenames for this checked header.
bool tes4_bsa_has_embedded_names(const tes4_bsa_header_fields& header) noexcept;

/// Rejects archive-controlled TES4 payload spans outside the archive or overlapping metadata.
result<std::uint32_t> tes4_bsa_stored_payload_size(const tes4_bsa_file_record& record,
                                                   std::size_t archive_size,
                                                   std::size_t metadata_size);

/// Reads prefix-dependent TES4 payload metadata through the parser's archive/host-file callback.
template <typename PayloadReader>
result<tes4_bsa_payload_descriptor> make_tes4_bsa_payload_descriptor(const tes4_bsa_header_fields& header,
                                                                     const tes4_bsa_file_record& record,
                                                                     std::size_t archive_size,
                                                                     std::size_t metadata_size,
                                                                     PayloadReader& read_payload_bytes) {
  auto stored_size = tes4_bsa_stored_payload_size(record, archive_size, metadata_size);
  if (!stored_size) {
    return stored_size.error();
  }

  const auto compression = tes4_bsa_compression_for(header, record.size_flags);
  std::uint32_t embedded_prefix = 0;
  if (tes4_bsa_has_embedded_names(header)) {
    if (!detail::span_fits(record.offset, 1U, archive_size)) {
      return error{error_code::format_error, "TES4 BSA embedded-name prefix is outside the archive"};
    }
    auto bytes = read_payload_bytes(record.offset, 1U);
    if (!bytes) {
      return bytes.error();
    }
    const auto length = static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[0]));
    embedded_prefix = length + 1U;
    if (embedded_prefix > stored_size.value() || !detail::span_fits(record.offset, embedded_prefix, archive_size)) {
      return error{error_code::format_error, "TES4 BSA embedded-name prefix exceeds stored payload"};
    }
  }

  const auto cursor = static_cast<std::size_t>(record.offset) + embedded_prefix;
  const auto remaining = stored_size.value() - embedded_prefix;
  std::uint32_t raw_size = remaining;
  if (compression != entry_compression::none) {
    if (remaining < 4U || !detail::span_fits(cursor, 4U, archive_size)) {
      return error{error_code::format_error, "TES4 BSA compressed payload size prefix is truncated"};
    }
    auto bytes = read_payload_bytes(cursor, 4U);
    if (!bytes) {
      return bytes.error();
    }
    raw_size = static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[0])) |
               (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[1U])) << 8U) |
               (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[2U])) << 16U) |
               (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes.value()[3U])) << 24U);
  }

  return tes4_bsa_payload_descriptor{record.offset, stored_size.value(), embedded_prefix, raw_size, compression};
}

} // namespace libbsa::formats::bsa
