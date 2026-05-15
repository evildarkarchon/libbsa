#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <detail/host_file_path.hpp>

#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2
{

    /// Returns stable BA2 DX10 entry metadata copies sorted by canonical archive path.
    result<std::vector<entry_metadata>> ba2_dx10_entries(std::span<const entry_metadata> entries);

    /// Looks up one BA2 DX10 entry using public archive-path normalization semantics.
    result<std::optional<entry_metadata>> find_ba2_dx10_entry(std::span<const entry_metadata> entries, std::string_view path);

    /// Reports BA2 DX10 entry presence using the same normalization and errors as find.
    result<bool> contains_ba2_dx10_entry(std::span<const entry_metadata> entries, std::string_view path);

    /// Extracts one BA2 DX10 entry as a reconstructed DDS byte stream by reopening the resolved host archive.
    ///
    /// DDS extraction writes the reconstructed DXT10 header first, then each decoded texture chunk in
    /// parser-validated DDS order. Chunk codec routing comes only from parsed texture metadata and exact
    /// decoded-size validation is applied to every compressed chunk.
    result<void> extract_ba2_dx10_payload(const detail::host_file_path &host_path,
                                          const entry_metadata &entry,
                                          payload_sink &sink);

} // namespace libbsa::formats::ba2
