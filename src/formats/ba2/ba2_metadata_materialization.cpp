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
        header.profile().subtype(),
        source.size(),
        records_end,
        filename_table_extent{header.filename_table_offset(), table_end},
        std::move(names).value(),
        wording};
}

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

result<ba2_named_record> ba2_metadata_materialization::name_record(
    std::size_t index, const ba2_stored_record_identity& stored) {
    if (!seen_canonical_paths_) {
        // Construction may throw into the caller's guard, exactly as the
        // subtype-local set did; only the reservation reports its own wording.
        auto seen = std::make_unique<std::unordered_set<std::string>>();
        auto reserved =
            detail::reserve_metadata_set(*seen, names_.size(), wording_.canonical_path_set);
        if (!reserved) {
            return reserved.error();
        }
        seen_canonical_paths_ = std::move(seen);
    }

    auto identity = make_ba2_record_identity(subtype_, names_.at(index),
                                             ba2_record_identity_source::filename_table);
    if (!identity) {
        return identity.error();
    }
    if (!seen_canonical_paths_->insert(identity.value().canonical_path).second) {
        return error{error_code::format_error, std::string{wording_.duplicate_canonical_paths}};
    }

    // Stored lookup fields are recorded, not enforced: BSArchPro never
    // compares them against the filename table, and retail archives ship
    // records whose stored NameHash no reconstruction reproduces.
    // Fallout4 - Voices.ba2 has three in 116,101, and failing the archive
    // over them rejected every good entry with them (issue #43). All
    // three carry non-ASCII names, which hash_fo4 skips byte-for-byte as
    // the reference does, so the cause is an unresolved encoding
    // question in Bethesda's packer rather than proven corruption.
    // BSArchPro reads these fields verbatim and only ever recomputes hashes
    // from a query path.
    const auto identity_mismatch = compare_ba2_record_identity(stored, identity.value());

    // BA2 stores `/` in its filename table; the display spelling libbsa
    // reports is `\` on every format. Convert here rather than in
    // `resolve_ba2_record_path`, which the BA2 writers share for the
    // bytes they serialize.
    auto display_path = std::move(identity.value().stored_path);
    detail::normalize_display_separators(display_path);

    return ba2_named_record{std::move(identity.value().canonical_path), std::move(display_path),
                            identity_mismatch.any()};
}

ba2_metadata_materialization::ba2_metadata_materialization(
    ba2_subtype subtype, std::uint64_t archive_size, std::uint64_t records_end,
    filename_table_extent filename_table, std::vector<std::string> names,
    const ba2_metadata_wording& wording) noexcept
    : subtype_{subtype},
      archive_size_{archive_size},
      records_end_{records_end},
      filename_table_{filename_table},
      names_{std::move(names)},
      wording_{wording} {}

}  // namespace libbsa::formats::ba2
