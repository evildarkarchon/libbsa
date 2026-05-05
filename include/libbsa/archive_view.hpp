#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/result.hpp>

#include <map>
#include <string>
#include <vector>

namespace libbsa {

/// Metadata-only lookup view over copied archive entries.
///
/// The view owns normalized keys and copied metadata. It does not retain caller
/// vectors, byte sources, sinks, spans, or archive payload lifetimes.
class archive_view {
public:
    /// Builds a lookup view from a summary and caller-provided metadata entries.
    archive_view(archive_summary summary, std::vector<entry_metadata> entries);

    /// Returns the copied archive summary associated with this view.
    [[nodiscard]] const archive_summary& summary() const noexcept;

    /// Returns normalized archive paths in deterministic sorted order.
    [[nodiscard]] std::vector<archive_path> paths() const;

    /// Returns true when `path` normalizes to an entry stored in the view.
    [[nodiscard]] bool contains(std::string path) const;

    /// Returns copied metadata for `path`, or a structured lookup failure.
    [[nodiscard]] result<entry_metadata> entry(std::string path) const;

private:
    archive_summary summary_;
    std::map<std::string, entry_metadata> entries_;
};

} // namespace libbsa
