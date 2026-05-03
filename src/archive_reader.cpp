#include <libbsa/archive.hpp>

#include "tes3_archive.hpp"
#include "tes4_archive.hpp"

#include <array>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>

namespace libbsa {
namespace {

constexpr std::uint32_t kMagicTes3 = 0x00000100;

Error io_error(std::string message)
{
    return {ErrorCode::io_error, std::move(message)};
}

Error unsupported(std::string message)
{
    return {ErrorCode::unsupported_format, std::move(message)};
}

Result<std::uint32_t> read_archive_magic(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return io_error("failed to open archive: " + path.string());
    }

    std::array<std::uint8_t, 4U> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (input.gcount() != static_cast<std::streamsize>(bytes.size())) {
        if (input.bad()) {
            return io_error("failed to read archive magic: " + path.string());
        }
        return unsupported("archive is too small to contain a BSA magic value");
    }

    std::uint32_t magic = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        magic |= static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(shift / 8)]) << shift;
    }
    return magic;
}

Result<detail::ParsedArchive> parse_archive(const std::filesystem::path& path)
{
    auto magic = read_archive_magic(path);
    if (!magic) {
        return magic.error();
    }

    if (magic.value() == kMagicTes3) {
        return detail::parse_tes3_archive(path);
    }

    return detail::parse_tes4_archive(path);
}

std::string lookup_key_for(const detail::ParsedArchive& archive, std::string_view archive_path)
{
    if (archive.metadata.format == ArchiveFormat::tes3) {
        return detail::lookup_key_for_tes3_archive_path(archive_path);
    }

    return detail::lookup_key_for_archive_path(archive_path);
}

} // namespace

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
    auto parsed = parse_archive(path);
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
    const auto key = lookup_key_for(impl_->archive, archive_path);
    return impl_->archive.lookup.find(key) != impl_->archive.lookup.end();
}

Result<ArchiveEntry> ArchiveReader::entry(std::string_view archive_path) const
{
    const auto normalized = detail::normalize_archive_path(archive_path);
    const auto found = impl_->archive.lookup.find(lookup_key_for(impl_->archive, normalized));
    if (found == impl_->archive.lookup.end()) {
        return Error{ErrorCode::missing_file, "archive path was not found: " + normalized};
    }

    return impl_->archive.entries[found->second];
}

Result<std::vector<std::uint8_t>> ArchiveReader::extract(std::string_view archive_path) const
{
    const auto normalized = detail::normalize_archive_path(archive_path);
    const auto found = impl_->archive.lookup.find(lookup_key_for(impl_->archive, normalized));
    if (found == impl_->archive.lookup.end()) {
        return Error{ErrorCode::missing_file, "archive path was not found: " + normalized};
    }

    if (impl_->archive.metadata.format == ArchiveFormat::tes3) {
        return detail::extract_tes3_entry(impl_->archive, impl_->archive.entries[found->second]);
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
