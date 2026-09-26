#pragma once

#include "formats/ba2/ba2_subtype.hpp"

#include <detail/host_file_path.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <vector>

namespace libbsa::formats::ba2 {

/// Normalized metadata result produced for every supported BA2 subtype.
struct opened_ba2_archive {
    archive_metadata metadata;
    std::vector<entry_metadata> entries;
    ba2_subtype subtype;
};

/// Opens and materializes one BA2 archive from a coherent native read session.
///
/// The session permits concurrent readers while denying writer and delete
/// sharing until all metadata and entries have been materialized. The native
/// handle and authoritative fixed header are released before the result escapes.
result<opened_ba2_archive> open_ba2_archive(const detail::host_file_path& host_path);

}  // namespace libbsa::formats::ba2
