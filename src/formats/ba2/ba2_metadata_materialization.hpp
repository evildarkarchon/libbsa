#pragma once

#include "formats/ba2/ba2_archive_header.hpp"
#include "formats/ba2/ba2_archive_source.hpp"

#include <libbsa/result.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
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
};

/// Archive-absolute extent of the encoded filename table: `[offset, end)`.
///
/// `end` is the byte after the last encoded name; trailing padding after the
/// final count-delimited name is deliberately not part of the extent.
struct ba2_filename_table_extent {
    std::uint64_t offset;
    std::uint64_t end;
};

/// BA2 Metadata Materialization for one BA2 Archive Opening.
///
/// This is the subtype-independent part of opening a BA2 archive (see
/// `GLOSSARY.md`). Both the GNRL and DX10 parsers create exactly one instance per
/// opening, after decoding their record table, and keep their own record
/// decoding and texture interpretation.
///
/// Creation reads the filename table. The object then holds the archive size,
/// the end of the record table, the filename-table extent and the names.
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
    ///         conversion report under the subtype's read contexts. Failed
    ///         reservations are returned as results, not thrown, so callers need
    ///         no allocation guard around creation.
    [[nodiscard]] static result<ba2_metadata_materialization> create(
        const ba2_archive_source& source, const ba2_archive_header& header,
        std::uint64_t records_end, const ba2_metadata_wording& wording);

    /// Returns the filename-table names in record order.
    ///
    /// Transitional: exposed only until record naming moves into this module,
    /// at which point the accessor is removed.
    [[nodiscard]] std::span<const std::string> names() const noexcept;

    /// Returns the measured archive-absolute extent of the filename table.
    ///
    /// Transitional: exposed only until payload span admission moves into this
    /// module, at which point the accessor is removed.
    [[nodiscard]] ba2_filename_table_extent filename_table_extent() const noexcept;

   private:
    /// Takes ownership of the already-read filename table.
    ba2_metadata_materialization(std::uint64_t archive_size, std::uint64_t records_end,
                                 ba2_filename_table_extent filename_table_extent,
                                 std::vector<std::string> names,
                                 const ba2_metadata_wording& wording) noexcept;

    // The archive size, the record-table end and the wording are held for the
    // per-record operations (payload span admission and record naming) that join
    // this module after the filename-table reader; nothing reads them yet.
    std::uint64_t archive_size_;
    std::uint64_t records_end_;
    ba2_filename_table_extent filename_table_extent_;
    std::vector<std::string> names_;
    // Copied by value; the views themselves refer to the subtype's static strings.
    ba2_metadata_wording wording_;
};

}  // namespace libbsa::formats::ba2
