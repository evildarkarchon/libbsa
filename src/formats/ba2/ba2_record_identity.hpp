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

/// BA2 archive path spelling after display-separator preservation and canonical
/// lookup normalization.
struct ba2_record_path {
    std::string display_path;
    std::string canonical_path;
};

/// Subtype-specific lookup facts that bind a BA2 path to its stored record
/// fields.
struct ba2_record_identity {
    std::string display_path;
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

/// Validates all stored BA2 record identity fields while preserving specific
/// mismatch diagnostics.
result<void> validate_ba2_record_identity(ba2_subtype subtype,
                                          const ba2_stored_record_identity& stored,
                                          const ba2_record_identity& expected);

}  // namespace libbsa::formats::ba2
