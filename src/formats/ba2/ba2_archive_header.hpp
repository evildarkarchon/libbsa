#pragma once

#include "formats/ba2/ba2_profile.hpp"

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace libbsa::formats::ba2 {

/// Validated immutable fixed-header facts for one BA2 archive instance.
class ba2_archive_header final {
   public:
    /// Returns the reusable family and compression interpretation of the header.
    [[nodiscard]] const ba2_profile& profile() const noexcept;

    /// Returns the number of subtype records declared by this archive.
    [[nodiscard]] std::uint32_t file_count() const noexcept;

    /// Returns the archive-absolute offset of the count-delimited filename table.
    [[nodiscard]] std::uint64_t filename_table_offset() const noexcept;

    /// Returns version-specific raw fields stored by this archive instance.
    [[nodiscard]] const ba2_archive_metadata& stored_metadata() const noexcept;

    /// Returns normalized public archive metadata derived from this validated header.
    [[nodiscard]] archive_metadata materialize_metadata() const;

   private:
    /// Constructs a header only after the decoder has validated every fixed field.
    ba2_archive_header(ba2_profile profile, std::uint32_t file_count,
                       std::uint64_t filename_table_offset, ba2_archive_metadata stored_metadata);

    ba2_profile profile_;
    std::uint32_t file_count_;
    std::uint64_t filename_table_offset_;
    ba2_archive_metadata stored_metadata_;

    friend result<ba2_archive_header> decode_ba2_archive_header(std::span<const std::byte> bytes,
                                                                std::uint64_t archive_size);
};

/// Decodes and validates one complete version-specific BA2 fixed header.
///
/// `archive_size` must come from the same stable read session as `bytes`; it is
/// used to reject archive-instance offsets that cannot address that observation.
result<ba2_archive_header> decode_ba2_archive_header(std::span<const std::byte> bytes,
                                                     std::uint64_t archive_size);

}  // namespace libbsa::formats::ba2
