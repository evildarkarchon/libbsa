#pragma once

#include "formats/bsa/bsa_archive_opening.hpp"
#include "formats/bsa/bsa_archive_source.hpp"
#include "formats/bsa/tes4_bsa_profile.hpp"

namespace libbsa::formats::bsa {

/// Materializes TES4 tables, payload prefixes, and entries from one borrowed source.
/// The profile must come from that observation; validation precedence is preserved
/// and no source ownership escapes with the resulting Archive Entry Catalog.
result<opened_bsa_archive> materialize_tes4_bsa_archive(const bsa_archive_source& source,
                                                        const tes4_bsa_profile& profile);

}  // namespace libbsa::formats::bsa
