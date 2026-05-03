#include <libbsa/archive.hpp>

#include "tes4_archive.hpp"

#include <ostream>

namespace libbsa {

struct ArchiveReader::Impl {
    detail::ParsedArchive archive;
};

ArchiveReader::~ArchiveReader() = default;
ArchiveReader::ArchiveReader(ArchiveReader&&) noexcept = default;
ArchiveReader& ArchiveReader::operator=(ArchiveReader&&) noexcept = default;

ArchiveReader::ArchiveReader(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

Result<ArchiveReader> ArchiveReader::open(const std::filesystem::path& path)
{
    auto parsed = detail::parse_tes4_archive(path);
    if (!parsed) {
        return parsed.error();
    }

    auto impl = std::make_unique<Impl>();
    impl->archive = std::move(parsed).value();
    return ArchiveReader(std::move(impl));
}

const ArchiveMetadata& ArchiveReader::metadata() const noexcept
{
    return impl_->archive.metadata;
}

const std::vector<ArchiveEntry>& ArchiveReader::entries() const noexcept
{
    return impl_->archive.entries;
}

bool ArchiveReader::contains(std::string_view archive_path) const
{
    const auto key = detail::lookup_key_for_archive_path(archive_path);
    return impl_->archive.lookup.find(key) != impl_->archive.lookup.end();
}

Result<ArchiveEntry> ArchiveReader::entry(std::string_view archive_path) const
{
    const auto normalized = detail::normalize_archive_path(archive_path);
    const auto found = impl_->archive.lookup.find(detail::lookup_key_for_archive_path(normalized));
    if (found == impl_->archive.lookup.end()) {
        return Error{ErrorCode::missing_file, "archive path was not found: " + normalized};
    }

    return impl_->archive.entries[found->second];
}

Result<std::vector<std::uint8_t>> ArchiveReader::extract(std::string_view archive_path) const
{
    const auto normalized = detail::normalize_archive_path(archive_path);
    const auto found = impl_->archive.lookup.find(detail::lookup_key_for_archive_path(normalized));
    if (found == impl_->archive.lookup.end()) {
        return Error{ErrorCode::missing_file, "archive path was not found: " + normalized};
    }

    return detail::extract_tes4_entry(impl_->archive, impl_->archive.entries[found->second]);
}

Result<void> ArchiveReader::extract_to(std::string_view archive_path, std::ostream& output) const
{
    auto extracted = extract(archive_path);
    if (!extracted) {
        return extracted.error();
    }

    const auto& bytes = extracted.value();
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!output) {
        return Error{ErrorCode::io_error, "failed to write extracted bytes to output stream"};
    }

    return {};
}

} // namespace libbsa
