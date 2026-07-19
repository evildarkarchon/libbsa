#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace libbsa::formats::ba2 {

/// Provides bounded random-access reads from one stable BA2 archive observation.
///
/// Implementations keep the observed file identity and size stable for the
/// complete metadata-opening lifetime. This contract is internal to BA2 Archive
/// Opening and is never retained by public reader state.
class ba2_archive_source {
   public:
    virtual ~ba2_archive_source() = default;

    /// Returns the archive size observed by this source.
    [[nodiscard]] virtual std::uint64_t size() const noexcept = 0;

    /// Reads exactly `count` bytes at a checked archive-absolute offset.
    ///
    /// `description` identifies the requested metadata in allocation, bounds,
    /// truncation, and native-read diagnostics. Returns `format_error` for an
    /// invalid or truncated span, `io_error` for a source read failure, and a
    /// result-based allocation error when the output buffer cannot be created.
    virtual result<std::vector<std::byte>> read_exact(std::uint64_t offset, std::size_t count,
                                                      std::string_view description) const = 0;
};

}  // namespace libbsa::formats::ba2
