#include <libbsa/ba2.hpp>

#include <utility>

namespace libbsa {

ba2_archive::ba2_archive(archive_summary summary, std::vector<entry_metadata> entries)
    : view_(std::move(summary), std::move(entries))
{
}

const archive_summary& ba2_archive::summary() const noexcept
{
    return view_.summary();
}

std::vector<archive_path> ba2_archive::paths() const
{
    return view_.paths();
}

bool ba2_archive::contains(std::string path) const
{
    return view_.contains(std::move(path));
}

result<entry_metadata> ba2_archive::entry(std::string path) const
{
    return view_.entry(std::move(path));
}

result<ba2_archive> open_ba2(const byte_source& source)
{
    static_cast<void>(source);
    return failure<ba2_archive>({error_code::unsupported_format, "BA2 GNRL parsing is not implemented yet"});
}

result<void> extract_ba2_entry(const ba2_archive& archive, const byte_source& source, std::string path, byte_sink& sink)
{
    static_cast<void>(archive);
    static_cast<void>(source);
    static_cast<void>(path);
    static_cast<void>(sink);
    return failure<void>({error_code::unsupported_format, "BA2 GNRL extraction is not implemented yet"});
}

} // namespace libbsa
