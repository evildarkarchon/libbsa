#include <libbsa/bsa.hpp>

#include "bsa_reader.hpp"

#include <utility>

namespace libbsa::detail {

bool bsa_is_supported_version(std::uint32_t version) noexcept
{
    return version == version_tes4 || version == version_fo3 || version == version_sse;
}

std::string bsa_join_path(std::string folder, std::string file)
{
    if (!folder.empty() && folder.back() != '/' && folder.back() != '\\') {
        folder.push_back('/');
    }
    folder += file;
    return folder;
}

} // namespace libbsa::detail

namespace libbsa {

bsa_archive::bsa_archive(archive_summary summary, std::vector<entry_metadata> entries)
    : view_(std::move(summary), std::move(entries))
{
}

const archive_summary& bsa_archive::summary() const noexcept
{
    return view_.summary();
}

std::vector<archive_path> bsa_archive::paths() const
{
    return view_.paths();
}

bool bsa_archive::contains(std::string path) const
{
    return view_.contains(std::move(path));
}

result<entry_metadata> bsa_archive::entry(std::string path) const
{
    return view_.entry(std::move(path));
}

result<bsa_archive> open_bsa(const byte_source& source)
{
    (void)source;
    return failure<bsa_archive>({error_code::unsupported_format, "unsupported BSA version"});
}

result<void> extract_bsa_entry(const bsa_archive& archive, const byte_source& source, std::string path, byte_sink& sink)
{
    (void)archive;
    (void)source;
    (void)path;
    (void)sink;
    return failure<void>({error_code::unsupported_format, "unsupported BSA version"});
}

} // namespace libbsa
