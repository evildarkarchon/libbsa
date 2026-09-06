#pragma once

#include "formats/bsa/bsa_archive_opening.hpp"
#include "formats/bsa/bsa_archive_source.hpp"

namespace libbsa::formats::bsa {

/// Materializes TES3 tables and entries while borrowing the opening's stable source.
/// Preserves header, hash, and payload validation order and retains no source ownership.
result<opened_bsa_archive> materialize_tes3_bsa_archive(const bsa_archive_source& source);

}  // namespace libbsa::formats::bsa
