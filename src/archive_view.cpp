#include <libbsa/archive_view.hpp>

#include <utility>

namespace libbsa {

archive_view::archive_view(archive_summary summary, std::vector<entry_metadata> entries) : summary_(std::move(summary))
{
    for (auto metadata : entries) {
        auto normalized = normalize_archive_path(metadata.path);
        if (!normalized.has_value()) {
            continue;
        }
        metadata.path = normalized.value().string();
        entries_.insert_or_assign(metadata.path, std::move(metadata));
    }
}

const archive_summary& archive_view::summary() const noexcept
{
    return summary_;
}

std::vector<archive_path> archive_view::paths() const
{
    std::vector<archive_path> result;
    result.reserve(entries_.size());
    for (const auto& [path, metadata] : entries_) {
        (void)metadata;
        auto normalized = normalize_archive_path(path);
        if (normalized.has_value()) {
            result.push_back(std::move(normalized.value()));
        }
    }
    return result;
}

bool archive_view::contains(std::string path) const
{
    auto normalized = normalize_archive_path(std::move(path));
    return normalized.has_value() && entries_.contains(normalized.value().string());
}

result<entry_metadata> archive_view::entry(std::string path) const
{
    auto normalized = normalize_archive_path(std::move(path));
    if (!normalized.has_value()) {
        return failure<entry_metadata>(normalized.error());
    }

    const auto found = entries_.find(normalized.value().string());
    if (found == entries_.end()) {
        return failure<entry_metadata>({error_code::malformed_archive, "archive entry not found"});
    }

    return success(found->second);
}

} // namespace libbsa
