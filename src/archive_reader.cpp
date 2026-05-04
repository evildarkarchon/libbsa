#include <libbsa/archive.hpp>

#include "ba2_gnrl_archive.hpp"
#include "tes3_archive.hpp"
#include "tes4_archive.hpp"

#include <array>
#include <fstream>
#include <ios>
#include <memory>
#include <new>
#include <ostream>
#include <string>

namespace libbsa {
namespace {

constexpr std::uint32_t kMagicTes3 = 0x00000100;
constexpr std::uint32_t kMagicBsa = 0x00415342;
constexpr std::uint32_t kMagicBtdx = 0x58445442;

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
    if (magic.value() == kMagicBtdx) {
        return detail::parse_ba2_gnrl_archive(path);
    }
    if (magic.value() == kMagicBsa) {
        return detail::parse_tes4_archive(path);
    }

    return unsupported("archive magic is not a supported BSA or BA2 value");
}

std::string lookup_key_for(const detail::ParsedArchive& archive, std::string_view archive_path)
{
    if (archive.metadata.format == ArchiveFormat::tes3) {
        return detail::lookup_key_for_tes3_archive_path(archive_path);
    }
    if (archive.metadata.format == ArchiveFormat::fo4 || archive.metadata.format == ArchiveFormat::starfield) {
        return detail::lookup_key_for_ba2_archive_path(archive_path);
    }

    return detail::lookup_key_for_archive_path(archive_path);
}

const ArchiveMetadata& empty_metadata() noexcept
{
    // Reference-returning APIs cannot use Result, so moved-from readers expose stable empty views.
    static const ArchiveMetadata metadata{};
    return metadata;
}

const std::vector<ArchiveEntry>& empty_entries() noexcept
{
    static const std::vector<ArchiveEntry> entries;
    return entries;
}

Error moved_from_reader_error()
{
    return {ErrorCode::invalid_state, "archive reader no longer has archive state after move"};
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

    try {
        // Keep post-parse reader construction inside open()'s Result error contract.
        auto impl = std::make_unique<Impl>();
        impl->archive = std::move(parsed).value();
        return ArchiveReader(std::move(impl));
    } catch (const std::bad_alloc&) {
        return io_error("failed to allocate archive reader state");
    }
}

const ArchiveMetadata& ArchiveReader::metadata() const noexcept
{
    if (!impl_) {
        return empty_metadata();
    }

    return impl_->archive.metadata;
}

const std::vector<ArchiveEntry>& ArchiveReader::entries() const noexcept
{
    if (!impl_) {
        return empty_entries();
    }

    return impl_->archive.entries;
}

bool ArchiveReader::contains(std::string_view archive_path) const
{
    if (!impl_) {
        return false;
    }

    const auto key = lookup_key_for(impl_->archive, archive_path);
    return impl_->archive.lookup.find(key) != impl_->archive.lookup.end();
}

Result<ArchiveEntry> ArchiveReader::entry(std::string_view archive_path) const
{
    if (!impl_) {
        return moved_from_reader_error();
    }

    const auto normalized = detail::normalize_archive_path(archive_path);
    const auto found = impl_->archive.lookup.find(lookup_key_for(impl_->archive, normalized));
    if (found == impl_->archive.lookup.end()) {
        return Error{ErrorCode::missing_file, "archive path was not found: " + normalized};
    }

    return impl_->archive.entries[found->second];
}

Result<std::vector<std::uint8_t>> ArchiveReader::extract(std::string_view archive_path) const
{
    if (!impl_) {
        return moved_from_reader_error();
    }

    const auto normalized = detail::normalize_archive_path(archive_path);
    const auto found = impl_->archive.lookup.find(lookup_key_for(impl_->archive, normalized));
    if (found == impl_->archive.lookup.end()) {
        return Error{ErrorCode::missing_file, "archive path was not found: " + normalized};
    }

    if (impl_->archive.metadata.format == ArchiveFormat::tes3) {
        return detail::extract_tes3_entry(impl_->archive, impl_->archive.entries[found->second]);
    }
    if (impl_->archive.metadata.format == ArchiveFormat::fo4 || impl_->archive.metadata.format == ArchiveFormat::starfield) {
        return detail::extract_ba2_gnrl_entry(impl_->archive, impl_->archive.entries[found->second]);
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
    try {
        output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    } catch (const std::ios_base::failure&) {
        return Error{ErrorCode::io_error, "failed to write extracted bytes to output stream"};
    }
    if (!output) {
        return Error{ErrorCode::io_error, "failed to write extracted bytes to output stream"};
    }

    return {};
}

} // namespace libbsa
