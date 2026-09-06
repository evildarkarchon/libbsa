#pragma once

#include "formats/ba2/ba2_archive_header.hpp"
#include "formats/ba2/ba2_archive_opening.hpp"
#include "formats/ba2/ba2_archive_source.hpp"

#include <libbsa/result.hpp>

namespace libbsa::formats::ba2 {

/// Materializes checked BA2 DX10 records, chunks, count-delimited names, and metadata.
///
/// The validated header and source must describe the same stable BA2 Archive
/// Opening observation. Parsing begins at the texture record table and never
/// reopens the host path or decodes the common fixed header.
result<opened_ba2_archive> materialize_ba2_dx10_archive(const ba2_archive_source& source,
                                                        const ba2_archive_header& header);

}  // namespace libbsa::formats::ba2
