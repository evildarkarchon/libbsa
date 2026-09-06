#pragma once

#include <detail/host_file_path.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <vector>

namespace libbsa::formats::bsa {

/// Materialized metadata and Archive Entry Catalog for one supported BSA archive.
struct opened_bsa_archive {
    archive_metadata metadata;
    std::vector<entry_metadata> entries;
};

/// Resolves and materializes one TES3 or TES4-family BSA through a stable observation.
///
/// Concurrent readers are permitted, but writer and delete access are excluded
/// until metadata materialization completes. The observation is released before
/// returning on success or failure; later extraction observes the host path anew.
/// Returns format/unsupported errors for archive defects and io_error for host failures.
result<opened_bsa_archive> open_bsa_archive(const detail::host_file_path& host_path);

}  // namespace libbsa::formats::bsa
