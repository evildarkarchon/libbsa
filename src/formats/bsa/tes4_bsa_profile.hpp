#pragma once

#include <detail/compression_router.hpp>

#include <libbsa/archive.hpp>
#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace libbsa {

enum class archive_compression_policy;
enum class entry_compression_policy;
enum class tes4_bsa_target;
struct tes4_bsa_writer_options;

}  // namespace libbsa

namespace libbsa::formats::bsa {

/// Physical folder-record representation selected by a TES4 BSA Profile.
enum class tes4_folder_record_shape {
    legacy_32_bit_offset,
    sse_64_bit_offset,
};

/// Effective writer compression and the corresponding serialized record bit.
struct tes4_writer_compression_decision {
    entry_compression compression;
    std::uint32_t record_flags;
};

/// Immutable TES4-family version policy shared by reader and writer phases.
///
/// A profile contains only version-intrinsic behavior. Archive flags, writer
/// options, and per-entry policy remain inputs to its pure operations.
class tes4_bsa_profile {
   public:
    /// Returns the public archive family represented by this profile.
    [[nodiscard]] archive_variant variant() const noexcept;

    /// Returns the serialized and publicly reported BSA version.
    [[nodiscard]] std::uint32_t version() const noexcept;

    /// Returns the folder-record representation used by this version.
    [[nodiscard]] tes4_folder_record_shape folder_record_shape() const noexcept;

    /// Returns the serialized folder-record size used by this version.
    [[nodiscard]] std::size_t folder_record_size() const noexcept;

    /// Returns public entry metadata for compressed payloads in this version.
    [[nodiscard]] entry_compression compressed_entry_metadata() const noexcept;

    /// Returns the internal codec route for compressed payloads in this version.
    [[nodiscard]] detail::compression_method compressed_payload_method() const noexcept;

    /// Interprets an archive-wide writer compression policy for this version.
    [[nodiscard]] bool archive_default_compressed(archive_compression_policy policy) const noexcept;

    /// Resolves effective writer compression and the per-record XOR toggle.
    ///
    /// Zero-byte entries are always raw so readers never expect a missing
    /// decoded-size prefix, even when the archive default is compressed.
    [[nodiscard]] tes4_writer_compression_decision writer_entry_compression(
        archive_compression_policy archive_policy, entry_compression_policy entry_policy,
        std::uint64_t raw_size) const noexcept;

    /// Interprets the archive default bit and per-record toggle for a reader.
    [[nodiscard]] entry_compression reader_entry_compression(
        std::uint32_t archive_flags, std::uint32_t record_flags) const noexcept;

    /// Interprets the embedded-name archive flag for a reader.
    [[nodiscard]] bool reader_has_embedded_names(std::uint32_t archive_flags) const noexcept;

    /// Interprets writer embedded-name options for this version.
    [[nodiscard]] bool writer_emits_embedded_names(
        const tes4_bsa_writer_options& options) const noexcept;

    /// Classifies an archive path into the serialized TES4 file-flag mask.
    [[nodiscard]] std::uint32_t file_flag_for_path(std::string_view archive_path) const noexcept;

    /// Checks already-analyzed, libbsa-native DDS metadata against this
    /// version's established texture format allowlist.
    ///
    /// Returns `format_error` with the established target diagnostic when the
    /// numeric DDS format is outside the allowlist.
    [[nodiscard]] result<void> validate_texture_metadata(const texture_metadata& metadata) const;

   private:
    /// Complete version-intrinsic policy row used only by checked factories.
    struct profile_facts {
        std::uint32_t version;
        tes4_folder_record_shape folder_shape;
        std::size_t folder_record_size;
        entry_compression compressed_entry_metadata;
        detail::compression_method compressed_payload_method;
        bool target_default_compressed;
        bool supports_embedded_names;
    };

    /// Creates a fully resolved profile; callers must use one of the checked
    /// header-version or writer-target factories below.
    explicit tes4_bsa_profile(profile_facts facts);

    std::uint32_t version_{0U};
    tes4_folder_record_shape folder_shape_{tes4_folder_record_shape::legacy_32_bit_offset};
    std::size_t folder_record_size_{0U};
    entry_compression compressed_entry_metadata_{entry_compression::deflate};
    detail::compression_method compressed_payload_method_{detail::compression_method::deflate};
    bool target_default_compressed_{false};
    bool supports_embedded_names_{false};

    friend result<tes4_bsa_profile> make_tes4_bsa_profile_from_header(std::uint32_t version);
    friend result<tes4_bsa_profile> make_tes4_bsa_profile_for_writer(tes4_bsa_target target);
};

/// Resolves a complete TES4 BSA Profile from a parsed header version.
///
/// Unsupported versions return the established `unsupported` diagnostic.
result<tes4_bsa_profile> make_tes4_bsa_profile_from_header(std::uint32_t version);

/// Resolves a complete TES4 BSA Profile from a public writer target.
///
/// Invalid enum values return the established `invalid_argument` diagnostic.
result<tes4_bsa_profile> make_tes4_bsa_profile_for_writer(tes4_bsa_target target);

}  // namespace libbsa::formats::bsa
