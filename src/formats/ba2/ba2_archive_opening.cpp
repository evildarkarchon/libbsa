#include "formats/ba2/ba2_archive_opening.hpp"

#include "formats/ba2/ba2_archive_header.hpp"
#include "formats/ba2/ba2_archive_source.hpp"
#include "formats/ba2/ba2_constants.hpp"
#include "formats/ba2/ba2_dx10_parser.hpp"
#include "formats/ba2/ba2_gnrl_parser.hpp"

#include <detail/byte_vector.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libbsa::formats::ba2 {
namespace {

inline constexpr std::size_t ba2_native_read_chunk_size = 64U * 1024U;

/// Owns the native handle that stabilizes one BA2 metadata observation.
class ba2_native_read_session final : public ba2_archive_source {
   public:
    /// Acquires a native read handle that permits only other readers to share it.
    static result<ba2_native_read_session> open(const detail::host_file_path& host_path) {
        // Excluding write and delete sharing prevents both byte mutation and path
        // replacement while subtype metadata is materialized through this handle.
        const auto handle = ::CreateFileW(host_path.resolved.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                          nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle == INVALID_HANDLE_VALUE) {
            return error{error_code::io_error, "failed to open BA2 archive read session"};
        }

        LARGE_INTEGER size{};
        if (::GetFileSizeEx(handle, &size) == FALSE || size.QuadPart < 0) {
            ::CloseHandle(handle);
            return error{error_code::io_error, "failed to determine BA2 archive read session size"};
        }
        return ba2_native_read_session{handle, static_cast<std::uint64_t>(size.QuadPart)};
    }

    /// Releases the stabilizing native handle.
    ~ba2_native_read_session() {
        if (handle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle_);
        }
    }

    ba2_native_read_session(const ba2_native_read_session&) = delete;
    ba2_native_read_session& operator=(const ba2_native_read_session&) = delete;

    /// Transfers ownership of the stabilizing native handle.
    ba2_native_read_session(ba2_native_read_session&& other) noexcept
        : handle_{std::exchange(other.handle_, INVALID_HANDLE_VALUE)}, size_{other.size_} {}

    /// Transfers ownership after releasing any currently owned handle.
    ba2_native_read_session& operator=(ba2_native_read_session&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        if (handle_ != INVALID_HANDLE_VALUE) {
            ::CloseHandle(handle_);
        }
        handle_ = std::exchange(other.handle_, INVALID_HANDLE_VALUE);
        size_ = other.size_;
        return *this;
    }

    /// Returns the file size observed through this session's handle.
    [[nodiscard]] std::uint64_t size() const noexcept override { return size_; }

    /// Reads exactly `count` bytes at a checked archive-absolute offset.
    ///
    /// Native calls are deliberately chunked so subtype record and name tables
    /// can reuse this source without introducing single-call DWORD limits.
    result<std::vector<std::byte>> read_exact(std::uint64_t offset, std::size_t count,
                                              std::string_view description) const override {
        if (offset > size_ || static_cast<std::uint64_t>(count) > size_ - offset) {
            return error{error_code::format_error,
                         std::string{description} + " is outside the BA2 archive"};
        }

        auto bytes = detail::make_byte_vector(count, description);
        if (!bytes) {
            return bytes.error();
        }

        std::size_t output_offset = 0U;
        std::uint64_t archive_offset = offset;
        while (output_offset < count) {
            const auto requested = static_cast<DWORD>(std::min<std::size_t>(
                count - output_offset,
                std::min<std::size_t>(
                    ba2_native_read_chunk_size,
                    static_cast<std::size_t>((std::numeric_limits<DWORD>::max)()))));
            OVERLAPPED position{};
            position.Offset = static_cast<DWORD>(archive_offset & 0xFFFF'FFFFULL);
            position.OffsetHigh = static_cast<DWORD>(archive_offset >> 32U);
            DWORD read_count = 0U;
            if (::ReadFile(handle_, bytes.value().data() + output_offset, requested, &read_count,
                           &position) == FALSE) {
                return error{error_code::io_error, "failed while reading BA2 archive metadata"};
            }
            if (read_count != requested) {
                return error{error_code::format_error, std::string{description} + " is truncated"};
            }
            output_offset += requested;
            archive_offset += requested;
        }
        return std::move(bytes).value();
    }

   private:
    /// Takes ownership of a successfully sized native archive handle.
    ba2_native_read_session(HANDLE handle, std::uint64_t size) noexcept
        : handle_{handle}, size_{size} {}

    HANDLE handle_{INVALID_HANDLE_VALUE};
    std::uint64_t size_{0U};
};

/// Reads the bounded fixed-header prefix needed by every supported BA2 version.
result<ba2_archive_header> read_authoritative_header(const ba2_native_read_session& session) {
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
    auto session = ba2_native_read_session::open(host_path);
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
