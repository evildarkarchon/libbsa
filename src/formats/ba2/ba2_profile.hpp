#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <detail/compression_router.hpp>

#include <cstddef>
#include <cstdint>

namespace libbsa {

enum class ba2_gnrl_target;
enum class ba2_dx10_target;
struct ba2_gnrl_writer_options;
struct ba2_dx10_writer_options;

}  // namespace libbsa

namespace libbsa::formats::ba2 {

/// BA2 archive subtype selected by the `BTDX` subtype field.
enum class ba2_subtype {
    gnrl,
    dx10,
};

/// Immutable interpretation of BA2 version, subtype, header, and compression
/// semantics.
class ba2_profile {
   public:
    /// Returns the game-family variant represented by this BA2 profile.
    [[nodiscard]] archive_variant variant() const noexcept;

    /// Returns the BA2 subtype represented by this profile.
    [[nodiscard]] ba2_subtype subtype() const noexcept;

    /// Returns the serialized BA2 subtype magic for this profile.
    [[nodiscard]] std::uint32_t subtype_magic() const noexcept;

    /// Returns the BA2 header version represented by this profile.
    [[nodiscard]] std::uint32_t version() const noexcept;

    /// Returns the fixed BA2 header size for this profile.
    [[nodiscard]] std::size_t header_size() const noexcept;

    /// Returns the public default compression metadata for compressed entries.
    [[nodiscard]] entry_compression default_compression() const noexcept;

    /// Returns raw BA2 archive metadata exposed through the public reader.
    [[nodiscard]] const ba2_archive_metadata& ba2_metadata() const noexcept;

    /// Returns the internal codec used when an entry or chunk is compressed.
    [[nodiscard]] detail::compression_method compressed_payload_method() const noexcept;

    /// Returns true when this profile is for BA2 GNRL archives.
    [[nodiscard]] bool is_gnrl() const noexcept;

    /// Returns true when this profile is for BA2 DX10 archives.
    [[nodiscard]] bool is_dx10() const noexcept;

   private:
    ba2_profile(archive_variant variant, ba2_subtype subtype, std::uint32_t version,
                std::size_t header_size, entry_compression default_compression,
                ba2_archive_metadata metadata, detail::compression_method compressed_method);

    archive_variant variant_{archive_variant::fallout4};
    ba2_subtype subtype_{ba2_subtype::gnrl};
    std::uint32_t version_{0U};
    std::size_t header_size_{0U};
    entry_compression default_compression_{entry_compression::deflate};
    ba2_archive_metadata metadata_{};
    detail::compression_method compressed_method_{detail::compression_method::deflate};

    friend result<ba2_profile> make_ba2_profile_from_header(
        std::uint32_t version, ba2_subtype subtype, ba2_archive_metadata metadata);
    friend result<ba2_profile> make_ba2_profile_for_gnrl_writer(
        ba2_gnrl_target target, const ba2_gnrl_writer_options& options);
    friend result<ba2_profile> make_ba2_profile_for_dx10_writer(
        ba2_dx10_target target, const ba2_dx10_writer_options& options);
    friend result<ba2_profile> make_profile(std::uint32_t version, ba2_subtype subtype,
                                            ba2_archive_metadata metadata,
                                            detail::compression_method method);
};

/// Converts a BA2 subtype magic value into a supported subtype.
result<ba2_subtype> ba2_subtype_from_magic(std::uint32_t subtype_magic);

/// Builds a BA2 profile from parsed header fields.
result<ba2_profile> make_ba2_profile_from_header(std::uint32_t version, ba2_subtype subtype,
                                                 ba2_archive_metadata metadata);

/// Builds a BA2 GNRL writer profile from public target options.
result<ba2_profile> make_ba2_profile_for_gnrl_writer(ba2_gnrl_target target,
                                                     const ba2_gnrl_writer_options& options);

/// Builds a BA2 DX10 writer profile from public target options.
result<ba2_profile> make_ba2_profile_for_dx10_writer(ba2_dx10_target target,
                                                     const ba2_dx10_writer_options& options);

/// Maps public BA2 entry compression metadata to the internal codec method.
result<detail::compression_method> ba2_compressed_payload_method(
    ba2_subtype subtype, entry_compression compression);

}  // namespace libbsa::formats::ba2
