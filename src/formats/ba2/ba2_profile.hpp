#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include "formats/ba2/ba2_subtype.hpp"

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

/// Version-determined shape of the BA2 fixed header.
///
/// BA2 header versions are tags, not an ordered capability ladder: Fallout 4's
/// next-gen versions 7 and 8 sit numerically above Starfield's 2 and 3 while
/// reusing the original 24-byte Fallout 4 header with no trailing fields. This
/// struct exists so every header-shape decision resolves through one explicit
/// per-version table instead of a `>=` comparison that would misread 7 and 8.
struct ba2_header_layout {
    /// Game family reported through public archive metadata.
    archive_variant variant{archive_variant::fallout4};
    /// Serialized width of the fixed header, including any trailing fields.
    std::size_t header_size{0U};
    /// True when the header stores the Starfield Unknown1/Unknown2 pair.
    bool has_starfield_unknown_fields{false};
    /// True when the header stores the Starfield v3 CompressionMethod field.
    bool has_compression_method{false};
};

/// Resolves the fixed-header layout for a reader-supported BA2 header version.
///
/// Returns `error_code::unsupported` for any version outside the supported set,
/// which is what keeps an unrecognized future version from being absorbed into
/// an existing layout.
result<ba2_header_layout> ba2_header_layout_for_version(std::uint32_t version);

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

    /// Returns true when this version's header stores Starfield Unknown1/Unknown2.
    [[nodiscard]] bool has_starfield_unknown_fields() const noexcept;

    /// Returns true when this version's header stores a CompressionMethod field.
    [[nodiscard]] bool has_compression_method_field() const noexcept;

    /// Returns the public default compression metadata for compressed entries.
    [[nodiscard]] entry_compression default_compression() const noexcept;

    /// Returns the internal codec used when an entry or chunk is compressed.
    [[nodiscard]] detail::compression_method compressed_payload_method() const noexcept;

    /// Returns true when this profile is for BA2 GNRL archives.
    [[nodiscard]] bool is_gnrl() const noexcept;

    /// Returns true when this profile is for BA2 DX10 archives.
    [[nodiscard]] bool is_dx10() const noexcept;

   private:
    ba2_profile(ba2_header_layout layout, ba2_subtype subtype, std::uint32_t version,
                entry_compression default_compression,
                detail::compression_method compressed_method);

    ba2_header_layout layout_{};
    ba2_subtype subtype_{ba2_subtype::gnrl};
    std::uint32_t version_{0U};
    entry_compression default_compression_{entry_compression::deflate};
    detail::compression_method compressed_method_{detail::compression_method::zlib};

    friend result<ba2_profile> make_ba2_profile_from_header(std::uint32_t version,
                                                            ba2_subtype subtype,
                                                            ba2_archive_metadata metadata);
    friend result<ba2_profile> make_ba2_profile_for_gnrl_writer(
        ba2_gnrl_target target, const ba2_gnrl_writer_options& options);
    friend result<ba2_profile> make_ba2_profile_for_dx10_writer(
        ba2_dx10_target target, const ba2_dx10_writer_options& options);
    friend result<ba2_profile> make_profile(std::uint32_t version, ba2_subtype subtype,
                                            detail::compression_method method);
};

/// Converts a BA2 subtype magic value into a supported subtype.
result<ba2_subtype> ba2_subtype_from_magic(std::uint32_t subtype_magic);

/// Builds reusable BA2 family and compression semantics from parsed header fields.
///
/// Version-specific raw metadata is consulted to resolve compression but remains
/// owned by the archive header rather than the resulting profile.
result<ba2_profile> make_ba2_profile_from_header(std::uint32_t version, ba2_subtype subtype,
                                                 ba2_archive_metadata metadata);

/// Builds a BA2 GNRL writer profile from public target options.
result<ba2_profile> make_ba2_profile_for_gnrl_writer(ba2_gnrl_target target,
                                                     const ba2_gnrl_writer_options& options);

/// Builds a BA2 DX10 writer profile from public target options.
result<ba2_profile> make_ba2_profile_for_dx10_writer(ba2_dx10_target target,
                                                     const ba2_dx10_writer_options& options);

/// Maps public BA2 entry compression metadata to the internal codec method.
result<detail::compression_method> ba2_compressed_payload_method(ba2_subtype subtype,
                                                                 entry_compression compression);

}  // namespace libbsa::formats::ba2
