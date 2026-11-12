#pragma once

#include "formats/ba2/ba2_archive_header.hpp"
#include "formats/ba2/ba2_archive_source.hpp"
#include "formats/ba2/ba2_record_identity.hpp"
#include "formats/ba2/ba2_subtype.hpp"

#include <detail/payload_span_exclusivity.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace libbsa::formats::ba2 {

/// Complete diagnostic strings one BA2 subtype supplies to BA2 Metadata Materialization.
///
/// Each field is a whole, verbatim message or read context, never a fragment:
/// the module does not build wording from a subtype label at run time, so every
/// subtype keeps the exact strings its callers and tests observe. This mirrors
/// `detail::host_file_context`. Instances are expected to be constants with
/// static storage duration, because the module keeps the views for the life of
/// one BA2 Archive Opening.
struct ba2_metadata_wording {
    /// Allocation description used when the filename-table name vector cannot be
    /// reserved, for example "BA2 GNRL filename table entries".
    std::string_view filename_table_entries;

    /// Format error reported when fewer than two bytes remain for a UInt16 name length.
    std::string_view filename_length_truncated;

    /// Format error reported when the declared name length runs past the archive.
    std::string_view filename_bytes_truncated;

    /// Format error reported when a filename-table entry declares a zero-length name.
    std::string_view empty_filename;

    /// Read context passed to the archive source for each UInt16 name length.
    std::string_view filename_length_read;

    /// Read context passed to the archive source, and to string conversion, for name bytes.
    std::string_view filename_bytes_read;

    /// Format error reported when a payload span runs past the archive, for
    /// example "BA2 GNRL entry payload span is outside the archive".
    std::string_view payload_span_outside_archive;

    /// Format error reported when a payload span reaches into the header or the
    /// record table.
    std::string_view payload_span_intersects_records;

    /// Format error reported when a payload span reaches into the filename table.
    std::string_view filename_table_intersects_payload;

    /// Format error reported when a payload span violates Payload Span Exclusivity.
    std::string_view payload_spans_partially_overlap;

    /// Format error reported when two records resolve to the same canonical
    /// archive path, for example "BA2 GNRL contains duplicate canonical archive paths".
    std::string_view duplicate_canonical_paths;

    /// Allocation description used when the seen-canonical-path set cannot be
    /// reserved, for example "BA2 GNRL canonical path set".
    std::string_view canonical_path_set;
};

/// One record bound to its filename-table name by BA2 Metadata Materialization.
struct ba2_named_record {
    /// Lookup key the archive is indexed by.
    std::string canonical_path;

    /// Stored spelling with separators converted for display.
    std::string display_path;

    /// True when any stored lookup field disagrees with the filename-table name
    /// (the issue #43 tolerance: recorded, never enforced).
    bool record_identity_mismatch{false};
};

/// BA2 Metadata Materialization for one BA2 Archive Opening.
///
/// This is the subtype-independent part of opening a BA2 archive (see
/// `GLOSSARY.md`). Both the GNRL and DX10 parsers create exactly one instance per
/// opening, after decoding their record table, and keep their own record
/// decoding and texture interpretation.
///
/// Creation reads the filename table. The object then holds the archive size,
/// the end of the record table, the filename-table extent, the names, the set
/// of payload spans accepted so far and the set of canonical paths named so far.
///
/// Its two per-record operations are admitting a payload span and naming a
/// record. Each subtype owns its own loop and the order it calls them in: GNRL
/// names a record and then admits its span, record by record, while DX10 admits
/// every chunk span before naming any record. That difference is observable
/// diagnostic precedence (ADR-0002), so the module never runs a loop itself.
///
/// The per-record operations may throw `std::bad_alloc`; the module owns no
/// allocation boundary. Each subtype calls them inside its own allocation guard
/// so the failure keeps that subtype's wording.
class ba2_metadata_materialization final {
   public:
    /// Reads the filename table and creates the materialization for one opening.
    ///
    /// Reads exactly `header.file_count()` length-prefixed names (UInt16
    /// little-endian length, then that many name bytes) starting at
    /// `header.filename_table_offset()`, through `source`. The table is
    /// count-delimited, so the archive size is only a truncation guard; bounding
    /// it by the first payload offset instead would reject the reference layout,
    /// where BSArchPro writes the filename table after every payload. Overlap
    /// between the names and the payload area is checked afterwards against the
    /// measured extent.
    ///
    /// The filename-table offset is not re-checked against the archive size:
    /// `decode_ba2_archive_header` has already rejected an offset beyond the
    /// archive ("BA2 filename table offset is outside the archive"). The
    /// subtype-specific lower-bound check against the record table stays with
    /// each subtype, which is why `records_end` is supplied rather than derived.
    ///
    /// @param source Stable archive source of the same observation as `header`.
    /// @param header Validated BA2 Archive Header of this opening.
    /// @param records_end Archive-absolute byte after the subtype record table.
    /// @param wording The subtype's constant wording table; its views must
    ///        outlive the returned object.
    /// @return The materialization, or a `format_error` carrying the subtype's
    ///         truncation or empty-name wording, an allocation `format_error`
    ///         worded with `wording.filename_table_entries` when the name vector
    ///         cannot be reserved, or any error `source.read_exact` and string
    ///         conversion report under the subtype's read contexts. A failed
    ///         name-vector reservation is returned as a result rather than
    ///         thrown; building an error message can still throw
    ///         `std::bad_alloc`.
    [[nodiscard]] static result<ba2_metadata_materialization> create(
        const ba2_archive_source& source, const ba2_archive_header& header,
        std::uint64_t records_end, const ba2_metadata_wording& wording);

    /// Admits one stored payload span, `[offset, offset + stored_size)`.
    ///
    /// Runs four checks in this fixed order and reports the first failure with
    /// the subtype's wording: the span lies inside the archive; it is clear of
    /// the header and record table; it is clear of the filename table; it
    /// satisfies Payload Span Exclusivity against every span admitted before it.
    /// The order is observable diagnostic precedence (ADR-0002). An empty span
    /// still runs the first three checks but is not added to the accepted set.
    ///
    /// Callers own the order in which spans are admitted relative to their other
    /// checks; this only fixes the order within one span.
    ///
    /// @param offset Archive-absolute offset of the stored payload.
    /// @param stored_size Stored (packed when compressed) byte length.
    /// @return Success, or a `format_error` carrying one of the four span strings
    ///         from the wording table.
    /// @throws std::bad_alloc When the accepted-span set cannot be created or
    ///         grown. Callers translate it inside their allocation guard.
    [[nodiscard]] result<void> admit_payload_span(std::uint64_t offset, std::uint64_t stored_size);

    /// Binds record `index` to its filename-table name.
    ///
    /// Runs four steps in this fixed order: builds the BA2 Record Identity from
    /// the name for the subtype the BA2 Archive Header resolved; rejects a
    /// canonical path already named; compares `stored` against the identity to
    /// produce the mismatch flag, which is recorded and never enforced; and
    /// converts the stored path's separators for display.
    ///
    /// The first call reserves the seen-canonical-path set for every record of
    /// the opening, so callers must make it after reserving their own entry
    /// vector to keep the reservation diagnostics in their established order.
    ///
    /// @param index Zero-based record index; it selects the filename-table name
    ///        written at the same position. Must be below the header's file count.
    /// @param stored Lookup fields the record table stores for this record.
    /// @return The named record; a `format_error` carrying any BA2 Record
    ///         Identity path error or `wording.duplicate_canonical_paths`; or an
    ///         allocation `format_error` worded with `wording.canonical_path_set`
    ///         when the set cannot be reserved.
    /// @throws std::bad_alloc When the set cannot be created or grown, or a
    ///         path cannot be copied. Callers translate it inside their
    ///         allocation guard.
    /// @throws std::out_of_range When `index` is not below the file count, a
    ///         programmer precondition violation.
    [[nodiscard]] result<ba2_named_record> name_record(std::size_t index,
                                                       const ba2_stored_record_identity& stored);

   private:
    /// Archive-absolute extent of the encoded filename table: `[offset, end)`.
    ///
    /// `end` is the byte after the last encoded name; trailing padding after the
    /// final count-delimited name is deliberately not part of the extent.
    struct filename_table_extent {
        std::uint64_t offset;
        std::uint64_t end;
    };

    /// Takes ownership of the already-read filename table.
    ba2_metadata_materialization(ba2_subtype subtype, std::uint64_t archive_size,
                                 std::uint64_t records_end, filename_table_extent filename_table,
                                 std::vector<std::string> names,
                                 const ba2_metadata_wording& wording) noexcept;

    ba2_subtype subtype_;
    std::uint64_t archive_size_;
    std::uint64_t records_end_;
    filename_table_extent filename_table_;
    std::vector<std::string> names_;
    // Copied by value; the views themselves refer to the subtype's static strings.
    ba2_metadata_wording wording_;
    // Created on the first non-empty admission, never in `create()` or the
    // constructor. MSVC's `std::set` allocates its sentinel node when it is
    // default-constructed and again when it is moved from, so holding the
    // collection by value would put a possible `std::bad_alloc` in the noexcept
    // constructor and in every move of this object, outside any subtype
    // allocation guard. Behind a pointer, moves stay noexcept and the only
    // allocations happen inside `admit_payload_span`, which callers guard.
    std::unique_ptr<detail::payload_span_exclusivity> accepted_payload_spans_;
    // Created and reserved on the first `name_record` call, for the same reason:
    // MSVC's `std::unordered_set` also allocates when default-constructed and
    // when moved from. Deferring it to the first naming call also keeps the
    // reservation after each subtype's entry-vector reservation, which is the
    // order the two allocation diagnostics have always been reported in.
    std::unique_ptr<std::unordered_set<std::string>> seen_canonical_paths_;
};

}  // namespace libbsa::formats::ba2
