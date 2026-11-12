#include "formats/ba2/ba2_metadata_materialization.hpp"

#include <detail/binary_io.hpp>
#include <detail/parser_primitives.hpp>

#include <utility>

namespace libbsa::formats::ba2 {
namespace {

/// Reads exactly `file_count` length-prefixed names through the stable source.
///
/// Every failure carries the subtype's wording verbatim. `table_end` receives the
/// archive-absolute byte after the last encoded name only on success.
result<std::vector<std::string>> read_filename_table(const ba2_archive_source& source,
                                                     std::uint64_t file_table_offset,
                                                     std::uint32_t file_count,
                                                     const ba2_metadata_wording& wording,
                                                     std::uint64_t& table_end) {
    const auto archive_size = source.size();

    std::vector<std::string> names;
    auto reserved =
        detail::reserve_metadata_vector(names, file_count, wording.filename_table_entries);
    if (!reserved) {
        return reserved.error();
    }

    std::uint64_t cursor = file_table_offset;
    for (std::uint32_t index = 0; index < file_count; ++index) {
        if (!detail::span_fits_u64(cursor, 2U, archive_size)) {
            return error{error_code::format_error, std::string{wording.filename_length_truncated}};
        }

        auto length_bytes = source.read_exact(cursor, 2U, wording.filename_length_read);
        if (!length_bytes) {
            return length_bytes.error();
        }

        detail::binary_reader length_reader{length_bytes.value()};
        const auto length = length_reader.read_u16_le();
        if (!length) {
            return error{error_code::format_error, std::string{wording.filename_length_truncated}};
        }
        cursor += 2U;
        if (!detail::span_fits_u64(cursor, length.value(), archive_size)) {
            return error{error_code::format_error, std::string{wording.filename_bytes_truncated}};
        }

        auto name_bytes = source.read_exact(cursor, length.value(), wording.filename_bytes_read);
        if (!name_bytes) {
            return name_bytes.error();
        }
        if (name_bytes.value().empty()) {
            return error{error_code::format_error, std::string{wording.empty_filename}};
        }
        auto name =
            detail::archive_string_from_bytes(name_bytes.value(), wording.filename_bytes_read);
        if (!name) {
            return name.error();
        }
        // Cannot reallocate: the vector was reserved for `file_count` names above,
        // so the only allocation failure on this path is the reported one.
        names.push_back(std::move(name.value()));
        cursor += length.value();
    }
    table_end = cursor;
    return names;
}

}  // namespace

result<ba2_metadata_materialization> ba2_metadata_materialization::create(
    const ba2_archive_source& source, const ba2_archive_header& header, std::uint64_t records_end,
    const ba2_metadata_wording& wording) {
    std::uint64_t table_end = 0U;
    auto names = read_filename_table(source, header.filename_table_offset(), header.file_count(),
                                     wording, table_end);
    if (!names) {
        return names.error();
    }
    return ba2_metadata_materialization{
        source.size(), records_end,
        filename_table_extent{header.filename_table_offset(), table_end}, std::move(names).value(),
        wording};
}

std::span<const std::string> ba2_metadata_materialization::names() const noexcept { return names_; }

result<void> ba2_metadata_materialization::admit_payload_span(std::uint64_t offset,
                                                              std::uint64_t stored_size) {
    if (!detail::span_fits_u64(offset, stored_size, archive_size_)) {
        return error{error_code::format_error, std::string{wording_.payload_span_outside_archive}};
    }
    // BSArchPro writes BA2 payloads after the fixed header and records; spans
    // into this prefix would later extract archive metadata bytes as if they
    // were file payload.
    if (detail::spans_overlap_u64(offset, stored_size, 0U, records_end_)) {
        return error{error_code::format_error,
                     std::string{wording_.payload_span_intersects_records}};
    }
    if (detail::spans_overlap_u64(offset, stored_size, filename_table_.offset,
                                  filename_table_.end - filename_table_.offset)) {
        return error{error_code::format_error,
                     std::string{wording_.filename_table_intersects_payload}};
    }
    if (stored_size == 0U) {
        // An empty span overlaps nothing, so it is accepted without joining the
        // exclusivity set. GNRL can reach this with a zero-size entry; DX10
        // rejects every empty chunk before admission.
        return {};
    }

    // Writer dedupe can intentionally publish exact duplicate stored spans;
    // partial sharing would make two entries read ambiguous bytes from each
    // other's payload ranges, and for DX10 would make texture chunk extraction
    // ambiguous because logical DDS segments would read bytes from each other's
    // ranges. Exclusivity runs last, after the bounds and metadata-intersect
    // checks, because which diagnostic a multi-defect archive reports is
    // observable behavior.
    if (!accepted_payload_spans_) {
        accepted_payload_spans_ = std::make_unique<detail::payload_span_exclusivity>();
    }
    return accepted_payload_spans_->insert(offset, stored_size,
                                           wording_.payload_spans_partially_overlap);
}

ba2_metadata_materialization::ba2_metadata_materialization(
    std::uint64_t archive_size, std::uint64_t records_end, filename_table_extent filename_table,
    std::vector<std::string> names, const ba2_metadata_wording& wording) noexcept
    : archive_size_{archive_size},
      records_end_{records_end},
      filename_table_{filename_table},
      names_{std::move(names)},
      wording_{wording} {}

}  // namespace libbsa::formats::ba2
