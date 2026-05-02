#pragma once

#include <libbsa/export.hpp>

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace libbsa {

/// Identifies the archive family detected by the milestone 1 reader.
enum class ArchiveFormat {
    unknown,
    tes4,
    fo3,
    sse,
};

/// Describes the compression method used by an extracted entry.
enum class CompressionMethod {
    none,
    zlib,
    lz4_frame,
};

/// Classifies ordinary archive-opening and extraction failures.
enum class ErrorCode {
    io_error,
    unsupported_format,
    malformed_archive,
    missing_file,
    decompression_failed,
};

/// Carries a typed failure plus a short diagnostic suitable for logs or UI.
struct Error {
    ErrorCode code = ErrorCode::malformed_archive;
    std::string message;
};

/// Minimal C++20 result type used for non-throwing archive operations.
template <typename T>
class Result {
public:
    /// Constructs a successful result from a value.
    Result(T value) : storage_(std::move(value)) {}

    /// Constructs a failed result from a typed error.
    Result(Error error) : storage_(std::move(error)) {}

    /// Returns true when the result contains a value.
    [[nodiscard]] bool has_value() const noexcept
    {
        return std::holds_alternative<T>(storage_);
    }

    /// Returns true when the result contains a value.
    explicit operator bool() const noexcept
    {
        return has_value();
    }

    /// Returns the contained value. Calling this on an error is a programmer error.
    [[nodiscard]] T& value() &
    {
        return std::get<T>(storage_);
    }

    /// Returns the contained value. Calling this on an error is a programmer error.
    [[nodiscard]] const T& value() const&
    {
        return std::get<T>(storage_);
    }

    /// Moves out the contained value. Calling this on an error is a programmer error.
    [[nodiscard]] T&& value() &&
    {
        return std::get<T>(std::move(storage_));
    }

    /// Returns the typed error. Calling this on a successful result is a programmer error.
    [[nodiscard]] const Error& error() const&
    {
        return std::get<Error>(storage_);
    }

private:
    std::variant<T, Error> storage_;
};

/// Result specialization for operations that only need success or a typed error.
template <>
class Result<void> {
public:
    /// Constructs a successful void result.
    Result() = default;

    /// Constructs a failed result from a typed error.
    Result(Error error) : storage_(std::move(error)) {}

    /// Returns true when the operation succeeded.
    [[nodiscard]] bool has_value() const noexcept
    {
        return !storage_.has_value();
    }

    /// Returns true when the operation succeeded.
    explicit operator bool() const noexcept
    {
        return has_value();
    }

    /// Returns the typed error. Calling this on a successful result is a programmer error.
    [[nodiscard]] const Error& error() const&
    {
        return *storage_;
    }

private:
    std::optional<Error> storage_;
};

/// Summarizes archive-level metadata parsed from a supported BSA header.
struct ArchiveMetadata {
    ArchiveFormat format = ArchiveFormat::unknown;
    std::uint32_t version = 0;
    std::uint32_t archive_flags = 0;
    std::uint32_t file_flags = 0;
    std::uint32_t folder_count = 0;
    std::uint32_t file_count = 0;
};

/// Describes one file record in a TES4-family BSA index.
struct ArchiveEntry {
    std::string path;
    std::uint64_t folder_hash = 0;
    std::uint64_t file_hash = 0;
    std::uint64_t folder_offset = 0;
    std::uint64_t data_offset = 0;
    std::uint64_t stored_size = 0;
    std::uint64_t packed_size = 0;
    std::uint64_t uncompressed_size = 0;
    CompressionMethod compression = CompressionMethod::none;
    bool compressed = false;
};

/// Owns a parsed archive index and provides random-access extraction by path.
class ArchiveReader {
public:
    LIBBSA_API ArchiveReader();
    LIBBSA_API ~ArchiveReader();

    LIBBSA_API ArchiveReader(ArchiveReader&&) noexcept;
    LIBBSA_API ArchiveReader& operator=(ArchiveReader&&) noexcept;

    ArchiveReader(const ArchiveReader&) = delete;
    ArchiveReader& operator=(const ArchiveReader&) = delete;

    /// Opens a TES4-family BSA from disk and returns typed errors for ordinary failures.
    [[nodiscard]] LIBBSA_API static Result<ArchiveReader> open(const std::filesystem::path& path);

    /// Returns archive-level metadata. The reference remains valid for this reader's lifetime.
    [[nodiscard]] LIBBSA_API const ArchiveMetadata& metadata() const noexcept;

    /// Returns the parsed file entries in archive order.
    [[nodiscard]] LIBBSA_API const std::vector<ArchiveEntry>& entries() const noexcept;

    /// Returns true if a normalized, hash-based lookup finds the archive-relative path.
    [[nodiscard]] LIBBSA_API bool contains(std::string_view archive_path) const;

    /// Looks up metadata for an archive-relative file path.
    [[nodiscard]] LIBBSA_API Result<ArchiveEntry> entry(std::string_view archive_path) const;

    /// Extracts a file to memory, transparently handling embedded names and compression.
    [[nodiscard]] LIBBSA_API Result<std::vector<std::uint8_t>> extract(std::string_view archive_path) const;

    /// Extracts a file and writes the resulting bytes to the caller-provided stream.
    [[nodiscard]] LIBBSA_API Result<void> extract_to(std::string_view archive_path, std::ostream& output) const;

private:
    struct Impl;

    explicit ArchiveReader(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};

} // namespace libbsa
