#include "formats/ba2/ba2_archive_opening.hpp"

#include "formats/ba2/ba2_archive_header.hpp"
#include "formats/ba2/ba2_archive_source.hpp"
#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_dx10_parser.hpp"
#include "formats/ba2/ba2_gnrl_parser.hpp"

#include <detail/host_file.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

detail::host_file_context ba2_archive_session_context() noexcept {
    return {"failed to open BA2 archive read session",
            "failed to determine BA2 archive read session size",
            "failed while reading BA2 archive metadata",
            "BA2 archive changed during metadata reading", "BA2 archive metadata"};
}

/// Adapts the shared stable host-file session to BA2 format-range semantics.
class ba2_stable_archive_source final : public ba2_archive_source {
   public:
    /// Opens one stable archive observation through the shared host-file owner.
    static result<ba2_stable_archive_source> open(const detail::host_file_path& host_path) {
        auto session =
            detail::stable_host_file_session::open(host_path, ba2_archive_session_context());
        if (!session) {
            return session.error();
        }
        return ba2_stable_archive_source{std::move(session).value()};
    }

    /// Returns the size observed by the shared stable session.
    [[nodiscard]] std::uint64_t size() const noexcept override { return session_.size(); }

    /// Reads exactly `count` bytes at a checked archive-absolute offset.
    result<std::vector<std::byte>> read_exact(std::uint64_t offset, std::size_t count,
                                              std::string_view description) const override {
        if (offset > session_.size() ||
            static_cast<std::uint64_t>(count) > session_.size() - offset) {
            return error{error_code::format_error,
                         std::string{description} + " is outside the BA2 archive"};
        }
        auto bytes = session_.read_exact_at(offset, count, description);
        if (!bytes) {
            return bytes.error();
        }
        return std::move(bytes).value();
    }

   private:
    /// Takes ownership of the shared session after a successful open.
    explicit ba2_stable_archive_source(detail::stable_host_file_session session) noexcept
        : session_{std::move(session)} {}

    detail::stable_host_file_session session_;
};

/// Reads the bounded fixed-header prefix needed by every supported BA2 version.
result<ba2_archive_header> read_authoritative_header(const ba2_archive_source& session) {
    const auto header_bytes_to_read = static_cast<std::size_t>(
        std::min<std::uint64_t>(session.size(), ba2_starfield_v3_header_size));
    auto bytes = session.read_exact(0U, header_bytes_to_read, "BA2 fixed header");
    if (!bytes) {
        return bytes.error();
    }
    return decode_ba2_archive_header(bytes.value(), session.size());
}

}  // namespace

result<opened_ba2_archive> open_ba2_archive(const detail::host_file_path& host_path) {
    auto session = ba2_stable_archive_source::open(host_path);
    if (!session) {
        return session.error();
    }
    auto header = read_authoritative_header(session.value());
    if (!header) {
        return header.error();
    }

    if (header.value().profile().is_dx10()) {
        return materialize_ba2_dx10_archive(session.value(), header.value());
    }

    return materialize_ba2_gnrl_archive(session.value(), header.value());
}

}  // namespace libbsa::formats::ba2
