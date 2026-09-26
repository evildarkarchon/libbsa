#pragma once

#include <libbsa/writer.hpp>

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

struct tes4_writer_entry {
    std::string archive_path_original;
    std::string archive_path_canonical;
    std::string host_path;
    std::vector<std::byte> memory_bytes;
    bool from_memory{false};
    entry_compression_policy compression{entry_compression_policy::inherit};
};

/// Finalizes staged TES4 writer entries into a newly published archive.
///
/// Output-path and entry validation intentionally precede resolving `target`
/// into the single TES4 BSA Profile used by downstream writer policy.
///
/// \param target Public target resolved only after earlier finalization validation.
/// \param options Per-archive policy and publication options.
/// \param entries Read-only staged entries prepared during this call.
/// \param output_host_path UTF-8 destination path for the published archive.
/// \param worker_count Positive number of parallel preparation workers.
/// \return Success after publication, or the first validation, format,
/// compression, or I/O error.
result<void> write_tes4_bsa_archive(tes4_bsa_target target, const tes4_bsa_writer_options& options,
                                    std::span<const tes4_writer_entry> entries,
                                    std::string_view output_host_path, std::uint32_t worker_count);

}  // namespace libbsa::formats::bsa
