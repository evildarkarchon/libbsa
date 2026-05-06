#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <string>
#include <vector>

namespace libbsa {

/// Owns parsed metadata for a BA2-family archive.
///
/// The archive object is a copied metadata view only: payload bytes remain owned
/// by caller-provided `byte_source` objects passed to `open_ba2` and extraction
/// calls. This keeps BA2 archive lifetimes independent from source lifetimes.
class ba2_archive {
public:
    /// Builds an archive metadata view from a parsed summary and copied entries.
    ba2_archive(archive_summary summary, std::vector<entry_metadata> entries);

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

/// Opens a BA2-family archive and parses its metadata tables.
///
/// The source is read through bounded random-access calls; payload data remains
/// in the caller-owned source until extraction receives the source again.
[[nodiscard]] result<ba2_archive> open_ba2(const byte_source& source);

/// Extracts a single BA2 entry to a caller-owned sink.
///
/// `path` is normalized with the same rules as metadata lookup. The caller must
/// provide the source that owns payload bytes for the duration of the operation.
[[nodiscard]] result<void> extract_ba2_entry(const ba2_archive& archive,
                                            const byte_source& source,
                                            std::string path,
                                            byte_sink& sink);

} // namespace libbsa
