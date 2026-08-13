#pragma once

#include "formats/ba2/ba2_subtype.hpp"

#include <libbsa/result.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace libbsa::formats::ba2 {

/// Identifies which BA2 boundary supplied path text so diagnostics can preserve
/// parser and writer wording.
enum class ba2_record_identity_source {
    filename_table,
    writer_entry,
};

/// BA2 archive path spelling as the archive stores it, plus its canonical key.
///
/// `stored_path` is the BA2 filename-table spelling: case preserved, separators
/// normalized to `/` because that is what Bethesda's own packer writes
/// (`wbBSArchive.pas:1539-1540`, "archive2.exe uses /"). It is the value BA2
/// writers serialize verbatim, so it must not be repointed at the display
/// separator -- readers convert it for display separately, via
/// `detail::normalize_display_separators`. BA2 hashes fold separators
/// (`CreateHashFO4`), so the stored spelling carries no lookup constraint.
struct ba2_record_path {
    std::string stored_path;
    std::string canonical_path;
};

/// Subtype-specific lookup facts that bind a BA2 path to its stored record
/// fields.
struct ba2_record_identity {
    std::string stored_path;
    std::string canonical_path;
    std::array<std::byte, 4> extension{};
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
};

/// Stored BA2 record identity fields read from a GNRL or DX10 record table.
struct ba2_stored_record_identity {
    std::uint32_t name_hash{};
    std::uint32_t directory_hash{};
    std::array<std::byte, 4> extension{};
};

/// Stored BA2 record lookup fields that disagree with the filename-table path.
///
/// The reference never cross-checks these fields: `TwbBSArchive.LoadFromFile`
/// reads `NameHash`, `Ext`, and `DirHash` straight into the record array, and
/// `FindFileRecordFO4` recomputes hashes from the *query* path before scanning
/// for a match. A record whose stored fields disagree with its own name is
/// simply unreachable by name, which is why retail archives ship a handful of
/// them (issue #43). Disagreement is therefore per-entry compatibility
/// evidence, not a structural invariant.
struct ba2_record_identity_mismatch {
    bool name_hash{false};
    bool directory_hash{false};
    bool extension{false};

    /// Returns true when any stored lookup field disagrees with the path.
    [[nodiscard]] constexpr bool any() const noexcept {
        return name_hash || directory_hash || extension;
    }
};

/// Resolves BA2 path spelling without deriving hashes or extension metadata.
result<ba2_record_path> resolve_ba2_record_path(ba2_subtype subtype,
                                                std::string_view archive_path,
                                                ba2_record_identity_source source);

/// Derives subtype-specific BA2 record identity from a raw archive path.
result<ba2_record_identity> make_ba2_record_identity(ba2_subtype subtype,
                                                     std::string_view archive_path,
                                                     ba2_record_identity_source source);

/// Derives subtype-specific BA2 record identity from an already resolved path.
result<ba2_record_identity> make_ba2_record_identity(ba2_subtype subtype, ba2_record_path path,
                                                     ba2_record_identity_source source);

/// Compares stored BA2 record lookup fields against the filename-table path.
///
/// Comparison is non-fatal by design; callers surface the result as per-entry
/// compatibility evidence. Extension bytes compare case-insensitively because
/// the reference lowercases on write but retail archives are not uniform.
[[nodiscard]] ba2_record_identity_mismatch compare_ba2_record_identity(
    const ba2_stored_record_identity& stored, const ba2_record_identity& expected) noexcept;

}  // namespace libbsa::formats::ba2
