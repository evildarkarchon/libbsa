#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <string>
#include <vector>

namespace libbsa {

/// Owns parsed metadata for a BSA-family archive.
///
/// The archive object is a metadata view only: payload bytes remain owned by the
/// caller-provided `byte_source` passed to `open_bsa` and extraction calls.
class bsa_archive {
public:
    /// Builds an archive metadata view from a parsed summary and copied entries.
    bsa_archive(archive_summary summary, std::vector<entry_metadata> entries);

    /// Returns the parsed archive summary.
    [[nodiscard]] const archive_summary& summary() const noexcept;

    /// Returns normalized archive paths in deterministic sorted order.
    [[nodiscard]] std::vector<archive_path> paths() const;

    /// Returns true when `path` normalizes to an entry in this archive.
    [[nodiscard]] bool contains(std::string path) const;

    /// Returns copied entry metadata for `path`, or a structured lookup failure.
    [[nodiscard]] result<entry_metadata> entry(std::string path) const;

private:
    archive_view view_;
};

/// Opens a BSA-family archive and parses its metadata tables.
///
/// The source is read through bounded random-access calls; payload data remains
/// in the source until a caller extracts a selected entry.
[[nodiscard]] result<bsa_archive> open_bsa(const byte_source& source);

/// Extracts a single BSA entry to a caller-owned sink.
///
/// `path` is normalized with the same rules as metadata lookup. The function
/// validates the stored payload range before writing bytes to `sink`.
[[nodiscard]] result<void> extract_bsa_entry(const bsa_archive& archive,
                                            const byte_source& source,
                                            std::string path,
                                            byte_sink& sink);

} // namespace libbsa
