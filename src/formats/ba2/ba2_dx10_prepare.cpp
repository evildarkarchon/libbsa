#include "formats/ba2/ba2_dx10_prepare.hpp"

#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_dx10_chunk_assembler.hpp"
#include "formats/ba2/ba2_dx10_snapshot_builder.hpp"
#include "formats/ba2/ba2_record_identity.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace libbsa::formats::ba2 {

result<ba2_dx10_writer_entry> ba2_dx10_make_writer_entry(std::string_view archive_path,
                                                         std::string_view dds_host_path,
                                                         ba2_dx10_target target,
                                                         const std::filesystem::path& snapshot_dir,
                                                         std::size_t entry_index) {
    // Add-time snapshots become the source of truth for later writes; source DDS
    // files may change or disappear.
    return ba2_dx10_build_writer_entry_snapshot(archive_path, dds_host_path, target, snapshot_dir,
                                                entry_index);
}

result<void> ba2_dx10_validate_entries(ba2_dx10_target target,
                                       std::span<const ba2_dx10_writer_entry> entries) {
    if (entries.empty()) {
        return error{error_code::invalid_argument,
                     "BA2 DX10 writer requires at least one texture entry"};
    }

    std::unordered_set<std::string> canonical_paths;
    for (const auto& entry : entries) {
        if (!canonical_paths.insert(entry.archive_path_canonical).second) {
            return error{error_code::format_error,
                         "BA2 DX10 writer has duplicate canonical archive paths"};
        }
        auto target_format =
            ba2_dx10_validate_texture_format_for_target(target, entry.metadata.dxgi_format);
        if (!target_format) {
            return target_format.error();
        }
        auto identity = make_ba2_record_identity(
            ba2_subtype::dx10,
            ba2_record_path{entry.archive_path_original, entry.archive_path_canonical},
            ba2_record_identity_source::writer_entry);
        if (!identity) {
            return identity.error();
        }
        if (entry.subresources.empty()) {
            return error{error_code::format_error,
                         "BA2 DX10 writer entry has no DDS image payload"};
        }
    }
    return {};
}

result<ba2_dx10_prepared_chunk> ba2_dx10_prepare_chunk(
    const ba2_profile& profile, const ba2_dx10_writer_options& options,
    const ba2_dx10_writer_entry& source, const texture::planned_texture_chunk& planned) {
    return ba2_dx10_assemble_chunk(profile, options, source, planned);
}

result<std::vector<ba2_dx10_prepared_entry>> ba2_dx10_prepare_entries(
    const ba2_profile& profile, const ba2_dx10_writer_options& options,
    std::span<const ba2_dx10_writer_entry> entries, std::uint32_t worker_count) {
    if (!profile.is_dx10()) {
        return error{error_code::invalid_argument, "BA2 DX10 writer profile is not DX10"};
    }

    std::vector<ba2_dx10_prepared_entry> prepared;
    prepared.reserve(entries.size());
    for (const auto& entry : entries) {
        // The chunk seam still owns detail::run_indexed_work over planned chunk
        // indices; this coordinator only serializes entry preparation and final
        // canonical-path sorting.
        auto next = ba2_dx10_assemble_planned_entry(profile, options, entry, worker_count);
        if (!next) {
            return next.error();
        }
        prepared.push_back(std::move(next.value()));
    }

    // Sorting happens only after each entry has been fully prepared, so indexed
    // chunk work remains entry-local.
    std::sort(prepared.begin(), prepared.end(),
              [](const ba2_dx10_prepared_entry& lhs, const ba2_dx10_prepared_entry& rhs) {
                  return lhs.archive_path_canonical < rhs.archive_path_canonical;
              });
    return prepared;
}

}  // namespace libbsa::formats::ba2
