#pragma once

#include <detail/host_file.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace libbsa::formats::bsa {

/// Owns the stable host-file adapter used throughout BSA Archive Opening.
///
/// Parsers borrow this source only while materializing metadata. Its observed
/// size and identity belong to the same native session, never separate path probes.
class bsa_archive_source final {
   public:
    /// Opens and sizes a stable observation, returning io_error on host failure.
    static result<bsa_archive_source> open(const detail::host_file_path& host_path);

    [[nodiscard]] std::uint64_t size() const noexcept { return session_.size(); }

    /// Reads a metadata range from the owned observation.
    ///
    /// Archive-declared out-of-range spans return format_error; native read
    /// failures return io_error. Allocation failures use the shared result contract.
    result<std::vector<std::byte>> read_exact(std::uint64_t offset, std::size_t count,
                                              std::string_view description) const;

   private:
    /// Takes exclusive ownership of an already-open stable session.
    explicit bsa_archive_source(detail::stable_host_file_session session) noexcept;

    detail::stable_host_file_session session_;
};

}  // namespace libbsa::formats::bsa
