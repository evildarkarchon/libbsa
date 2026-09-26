#include "formats/bsa/tes3_bsa_prepare.hpp"

#include <detail/archive_path.hpp>
#include <detail/bethesda_hash.hpp>
#include <detail/host_file.hpp>

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <utility>

namespace libbsa::formats::bsa {

namespace {

constexpr detail::host_file_context tes3_prepare_source_context{
    .open_error = "TES3 BSA writer failed to open disk source",
    .inspect_error = "TES3 BSA writer failed to inspect disk source",
    .read_error = "TES3 BSA writer failed while reading disk source",
    .changed_error = "TES3 BSA disk source changed during finalization",
    .allocation_description = "TES3 BSA disk source"};

/// Returns the spelling a TES3 name-table entry is serialized and hashed with.
///
/// Morrowind resolves a file by hashing the stored name, and `hash_tes3` folds
/// ASCII case but not path separators: 11089 of the 11090 names in vanilla
/// `Morrowind.bsa` hash differently after a `\` to `/` swap. Bethesda stores
/// backslashes in all 11090, so a forward-slash name is one the game cannot
/// find. libbsa used to normalize the other way, which made its TES3 output
/// unresolvable regardless of the hash word order (issue #54).
///
/// The reference rejects a forward slash rather than converting it
/// (`TAsset.IsValidAssetName`, `wbAssets.pas:302`). libbsa normalizes instead, so
/// callers may pass either spelling and neither is an error.
///
/// Casing is deliberately left alone. The reference lowercases stored names
/// (`wbBSArchive.pas:1367`), but `CreateHashTES3` folds case, so preserving the
/// caller's spelling costs no lookup compatibility and keeps the display path
/// faithful to what the caller asked for.
std::string preserved_archive_path(std::string_view archive_path) {
    std::string preserved{archive_path};
    std::replace(preserved.begin(), preserved.end(), '/', '\\');
    return preserved;
}

result<std::uint32_t> checked_u32(std::uint64_t value, std::string_view description) {
    if (value > std::numeric_limits<std::uint32_t>::max()) {
        return error{error_code::format_error,
                     std::string{description} + " exceeds uint32_t limits"};
    }
    return static_cast<std::uint32_t>(value);
}

result<detail::host_file_path> resolve_tes3_source_path(std::string_view host_path) {
    return detail::resolve_host_file_path(host_path);
}

result<std::uint32_t> disk_payload_size(const detail::host_file_path& host_path) {
    auto size = detail::inspect_host_file_size(host_path, tes3_prepare_source_context);
    if (!size) {
        return size.error();
    }

    return checked_u32(size.value(), "TES3 BSA disk source size");
}

}  // namespace

result<tes3_writer_entry> tes3_make_writer_entry(std::string_view archive_path) {
    if (archive_path.find('\0') != std::string_view::npos) {
        return error{error_code::invalid_argument,
                     "TES3 BSA archive path must not contain NUL bytes"};
    }

    auto canonical = detail::normalize_archive_path(archive_path);
    if (!canonical) {
        return canonical.error();
    }

    tes3_writer_entry entry;
    entry.archive_path_original = preserved_archive_path(archive_path);
    entry.archive_path_canonical = std::move(canonical.value().value);
    return entry;
}

result<void> tes3_validate_host_path(std::string_view host_path, std::string_view description) {
    if (host_path.empty()) {
        return error{error_code::invalid_argument, std::string{description} + " must not be empty"};
    }
    if (host_path.find('\0') != std::string_view::npos) {
        return error{error_code::invalid_argument,
                     std::string{description} + " must not contain NUL bytes"};
    }
    return {};
}

result<void> tes3_validate_entries(std::span<const tes3_writer_entry> entries) {
    if (entries.empty()) {
        return error{error_code::invalid_argument,
                     "TES3 BSA writer requires at least one file entry"};
    }

    std::unordered_set<std::string> canonical_paths;
    for (const auto& entry : entries) {
        if (!canonical_paths.insert(entry.archive_path_canonical).second) {
            return error{error_code::format_error,
                         "TES3 BSA writer has duplicate canonical archive paths"};
        }
    }

    return {};
}

result<std::vector<tes3_prepared_entry>> tes3_prepare_entries(
    std::span<const tes3_writer_entry> entries) {
    std::vector<tes3_prepared_entry> prepared;
    prepared.reserve(entries.size());

    for (const auto& entry : entries) {
        tes3_prepared_entry prepared_entry;
        prepared_entry.archive_path_original = entry.archive_path_original;
        prepared_entry.hash = detail::hash_tes3(entry.archive_path_original);
        prepared_entry.from_memory = entry.from_memory;
        if (entry.from_memory) {
            auto payload_size = checked_u32(entry.memory_bytes.size(), "TES3 BSA payload size");
            if (!payload_size) {
                return payload_size.error();
            }
            prepared_entry.payload = entry.memory_bytes;
            prepared_entry.payload_size = payload_size.value();
        } else {
            auto source_path = resolve_tes3_source_path(entry.host_path);
            if (!source_path) {
                return source_path.error();
            }
            auto payload_size = disk_payload_size(source_path.value());
            if (!payload_size) {
                return payload_size.error();
            }
            prepared_entry.host_path = entry.host_path;
            prepared_entry.resolved_host_path = std::move(source_path).value();
            prepared_entry.payload_size = payload_size.value();
        }

        // TES3 hashes are computed from the preserved serialized name, not the
        // canonical lowercase lookup key.
        prepared.push_back(std::move(prepared_entry));
    }

    std::sort(prepared.begin(), prepared.end(),
              [](const tes3_prepared_entry& lhs, const tes3_prepared_entry& rhs) {
                  // Retail record order compares the two stored hash words in the
                  // order they appear on disk -- first-half sum, then second-half
                  // sum -- which is ascending `hash_tes3` value. libbsa used to
                  // compare low32 first, an order no retail archive uses; all
                  // 11090 records of vanilla `Morrowind.bsa` are sorted the way
                  // this comparator now sorts them (issue #46). The reference
                  // sorts the same way: `HashPairSort` compares `DirHash` then
                  // `FileHash` (`wbBSArchive.pas:1251-1265`), and the TES3 path
                  // only ever populates `FileHash` with `CreateHashTES3`
                  // (`wbBSArchive.pas:1355-1360`), leaving `DirHash` zero.
                  return lhs.hash < rhs.hash;
              });
    return prepared;
}

}  // namespace libbsa::formats::bsa
