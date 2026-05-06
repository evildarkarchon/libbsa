#pragma once

#include <libbsa/archive.hpp>
#include <libbsa/archive_path.hpp>
#include <libbsa/archive_view.hpp>
#include <libbsa/io.hpp>
#include <libbsa/result.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa {

/// Libbsa-owned representation of a DDS DXGI format value.
///
/// The raw numeric value is preserved even when libbsa does not know a friendly
/// name for the format, so callers can inspect unsupported textures without
/// depending on platform or texture-library declarations.
struct dxgi_format {
    std::uint32_t value{};
};

/// Returns a stable libbsa-owned label for a known DXGI format.
///
/// Unknown numeric values are reported as `"UNKNOWN"` while remaining available
/// through `dxgi_format::value`.
[[nodiscard]] std::string_view dxgi_format_name(dxgi_format format) noexcept;

/// Describes one stored BA2 DDS texture chunk.
///
/// Offsets are archive-absolute payload offsets. `packed_size` is the stored
/// payload byte count, while `size` is the reconstructed uncompressed chunk byte
/// count expected after codec routing.
struct texture_chunk_metadata {
    std::uint32_t mip_level{};
    std::uint64_t offset{};
    std::uint64_t packed_size{};
    std::uint64_t size{};
    compression_state compression{compression_state::unknown};
};

/// Copied public metadata for a BA2 DDS texture entry.
///
/// The value is independent of parser storage and contains only libbsa-owned
/// types so consumers can inspect texture layout without private dependencies.
struct texture_metadata {
    std::string path;
    dxgi_format format{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t mip_count{};
    std::uint32_t array_size{};
    bool is_cubemap{};
    std::vector<texture_chunk_metadata> chunks;
};

/// Owns parsed metadata for a BA2-family archive.
///
/// The archive object is a copied metadata view only: payload bytes remain owned
/// by caller-provided `byte_source` objects passed to `open_ba2` and extraction
/// calls. This keeps BA2 archive lifetimes independent from source lifetimes.
class ba2_archive {
public:
    /// Builds an archive metadata view from a parsed summary and copied entries.
    ba2_archive(archive_summary summary, std::vector<entry_metadata> entries);

    /// Builds an archive metadata view with copied BA2 texture metadata.
    ba2_archive(archive_summary summary,
                std::vector<entry_metadata> entries,
                std::vector<libbsa::texture_metadata> textures);

    /// Returns the parsed archive summary.
    [[nodiscard]] const archive_summary& summary() const noexcept;

    /// Returns normalized archive paths in deterministic sorted order.
    [[nodiscard]] std::vector<archive_path> paths() const;

    /// Returns true when `path` normalizes to an entry in this archive.
    [[nodiscard]] bool contains(std::string path) const;

    /// Returns copied entry metadata for `path`, or a structured lookup failure.
    [[nodiscard]] result<entry_metadata> entry(std::string path) const;

    /// Returns copied BA2 texture metadata for `path`, or a structured lookup failure.
    [[nodiscard]] result<libbsa::texture_metadata> texture_metadata(std::string path) const;

private:
    archive_view view_;
    std::map<std::string, libbsa::texture_metadata> textures_;
};

/// Opens a BA2-family archive and parses its metadata tables.
///
/// The source is read through bounded random-access calls; payload data remains
/// in the caller-owned source until extraction receives the source again.
[[nodiscard]] result<ba2_archive> open_ba2(const byte_source& source);

/// Extracts a single BA2 entry to a caller-owned sink.
///
/// `path` is normalized with the same rules as metadata lookup. The caller must
/// provide the source that owns payload bytes for the duration of the operation.
[[nodiscard]] result<void> extract_ba2_entry(const ba2_archive& archive,
                                            const byte_source& source,
                                            std::string path,
                                            byte_sink& sink);

} // namespace libbsa
