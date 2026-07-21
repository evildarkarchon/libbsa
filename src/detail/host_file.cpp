#include <detail/host_file.hpp>

#include <detail/byte_vector.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <utility>

namespace libbsa::detail {

namespace {

error io_error(std::string_view message) {
    return error{error_code::io_error, std::string{message}};
}

result<std::size_t> checked_buffer_size(std::uint64_t size, const host_file_context& context) {
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return byte_vector_allocation_error(context.allocation_description);
    }
    return static_cast<std::size_t>(size);
}

result<void> validate_expected_host_file_size(const std::filesystem::path& host_path,
                                              std::uint64_t expected_size,
                                              const host_file_context& context) {
    auto actual_size = inspect_host_file_size(host_path, context);
    if (!actual_size) {
        return actual_size.error();
    }
    if (actual_size.value() != expected_size) {
        // Size changes are surfaced through caller-supplied diagnostics so each
        // archive flow keeps its existing attribution when mutable host files are
        // detected mid-operation.
        return io_error(context.changed_error);
    }
    return {};
}

result<void> read_exact_bytes(std::ifstream& input, std::span<std::byte> output,
                              const host_file_context& context) {
    while (!output.empty()) {
        const auto requested = std::min<std::size_t>(
            output.size(), static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()));
        input.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(requested));
        if (input.bad()) {
            return io_error(context.read_error);
        }
        if (input.gcount() != static_cast<std::streamsize>(requested)) {
            return io_error(context.changed_error);
        }
        output = output.subspan(requested);
    }
    return {};
}

result<void> reject_appended_host_file_byte(std::ifstream& input,
                                            const host_file_context& context) {
    char extra = '\0';
    if (input.get(extra)) {
        return io_error(context.changed_error);
    }
    if (input.bad()) {
        return io_error(context.read_error);
    }
    return {};
}

}  // namespace

result<stable_host_file_session> stable_host_file_session::open(const host_file_path& host_path,
                                                                const host_file_context& context) {
    session_diagnostics diagnostics{std::string{context.read_error},
                                    std::string{context.changed_error},
                                    std::string{context.allocation_description}};

    // FILE_SHARE_READ is deliberately the only sharing permission: Windows
    // applies share compatibility for the handle lifetime, which stabilizes
    // both file contents and path identity without excluding other readers.
    const auto handle = ::CreateFileW(host_path.resolved.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return io_error(context.open_error);
    }

    LARGE_INTEGER size{};
    if (::GetFileSizeEx(handle, &size) == FALSE || size.QuadPart < 0) {
        // Best-effort close must not replace the more useful sizing failure.
        ::CloseHandle(handle);
        return io_error(context.inspect_error);
    }

    return stable_host_file_session{handle, static_cast<std::uint64_t>(size.QuadPart),
                                    std::move(diagnostics)};
}

stable_host_file_session::stable_host_file_session(void* native_handle, std::uint64_t size,
                                                   session_diagnostics diagnostics) noexcept
    : native_handle_{native_handle}, size_{size}, diagnostics_{std::move(diagnostics)} {}

stable_host_file_session::~stable_host_file_session() noexcept { close(); }

stable_host_file_session::stable_host_file_session(stable_host_file_session&& other) noexcept
    : native_handle_{std::exchange(other.native_handle_, nullptr)},
      size_{std::exchange(other.size_, 0U)},
      sequential_offset_{std::exchange(other.sequential_offset_, 0U)},
      diagnostics_{std::move(other.diagnostics_)} {}

stable_host_file_session& stable_host_file_session::operator=(
    stable_host_file_session&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    close();
    native_handle_ = std::exchange(other.native_handle_, nullptr);
    size_ = std::exchange(other.size_, 0U);
    sequential_offset_ = std::exchange(other.sequential_offset_, 0U);
    diagnostics_ = std::move(other.diagnostics_);
    return *this;
}

std::uint64_t stable_host_file_session::size() const noexcept { return size_; }

result<std::vector<std::byte>> stable_host_file_session::read_prefix(std::size_t max_bytes) {
    const auto remaining = sequential_offset_ <= size_ ? size_ - sequential_offset_ : 0U;
    const auto bounded_size = static_cast<std::size_t>(
        std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(max_bytes)));
    auto bytes = make_byte_vector(bounded_size, diagnostics_.allocation_description);
    if (!bytes) {
        return bytes.error();
    }

    std::size_t offset = 0U;
    while (offset < bytes.value().size()) {
        const auto requested = static_cast<DWORD>(
            std::min<std::size_t>(bytes.value().size() - offset,
                                  static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
        auto read = read_at_most(sequential_offset_,
                                 std::span<std::byte>{bytes.value().data() + offset,
                                                      static_cast<std::size_t>(requested)});
        if (!read) {
            return read.error();
        }
        sequential_offset_ += static_cast<std::uint64_t>(read.value());
        offset += read.value();
        if (read.value() != static_cast<std::size_t>(requested)) {
            break;
        }
    }
    bytes.value().resize(offset);
    return std::move(bytes).value();
}

result<std::vector<std::byte>> stable_host_file_session::read_exact(std::uint64_t expected_size) {
    auto expected = validate_expected_size(expected_size);
    if (!expected) {
        return expected.error();
    }
    if (expected_size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        return byte_vector_allocation_error(diagnostics_.allocation_description);
    }
    auto bytes = make_byte_vector(static_cast<std::size_t>(expected_size),
                                  diagnostics_.allocation_description);
    if (!bytes) {
        return bytes.error();
    }
    auto read = read_sequential_exact(bytes.value());
    if (!read) {
        return read.error();
    }
    auto exact = reject_trailing_byte();
    if (!exact) {
        return exact.error();
    }
    return std::move(bytes).value();
}

result<std::vector<std::byte>> stable_host_file_session::read_exact_at(
    std::uint64_t offset, std::size_t count, std::string_view description) const {
    if (offset > size_ || static_cast<std::uint64_t>(count) > size_ - offset) {
        return io_error(diagnostics_.changed_error);
    }

    auto bytes = make_byte_vector(count, description);
    if (!bytes) {
        return bytes.error();
    }

    std::size_t output_offset = 0U;
    std::uint64_t source_offset = offset;
    while (output_offset < count) {
        const auto requested = static_cast<DWORD>(std::min<std::size_t>(
            count - output_offset, static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
        auto read =
            read_at_most(source_offset, std::span<std::byte>{bytes.value().data() + output_offset,
                                                             static_cast<std::size_t>(requested)});
        if (!read) {
            return read.error();
        }
        if (read.value() != static_cast<std::size_t>(requested)) {
            return io_error(diagnostics_.changed_error);
        }
        output_offset += read.value();
        source_offset += static_cast<std::uint64_t>(read.value());
    }
    return std::move(bytes).value();
}

result<void> stable_host_file_session::rewind() {
    // Every read carries an explicit native offset, so the logical cursor stays
    // independent from BA2 random-access reads that may update Windows' file pointer.
    sequential_offset_ = 0U;
    return {};
}

result<void> stable_host_file_session::copy_exact(
    std::uint64_t expected_size,
    const std::function<result<void>(std::span<const std::byte>)>& callback,
    std::size_t chunk_size) {
    if (chunk_size == 0U) {
        return error{error_code::invalid_argument, "stable host-file chunk size must be non-zero"};
    }
    auto expected = validate_expected_size(expected_size);
    if (!expected) {
        return expected.error();
    }

    const auto scratch_size = static_cast<std::size_t>(
        std::min<std::uint64_t>(expected_size, static_cast<std::uint64_t>(chunk_size)));
    auto scratch = make_byte_vector(scratch_size, diagnostics_.allocation_description);
    if (!scratch) {
        return scratch.error();
    }

    std::uint64_t remaining = expected_size;
    while (remaining > 0U) {
        const auto requested = static_cast<std::size_t>(
            std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.value().size())));
        auto read = read_sequential_exact(std::span<std::byte>{scratch.value().data(), requested});
        if (!read) {
            return read.error();
        }
        auto handled = callback(std::span<const std::byte>{scratch.value().data(), requested});
        if (!handled) {
            return handled.error();
        }
        remaining -= static_cast<std::uint64_t>(requested);
    }
    return reject_trailing_byte();
}

void stable_host_file_session::close() noexcept {
    const auto handle = std::exchange(native_handle_, nullptr);
    size_ = 0U;
    sequential_offset_ = 0U;
    if (handle != nullptr) {
        // Once ownership is released there is no safe retry or error channel.
        ::CloseHandle(static_cast<HANDLE>(handle));
    }
}

result<void> stable_host_file_session::validate_expected_size(std::uint64_t expected_size) const {
    if (expected_size != size_) {
        return io_error(diagnostics_.changed_error);
    }
    return {};
}

result<void> stable_host_file_session::read_sequential_exact(std::span<std::byte> output) {
    while (!output.empty()) {
        const auto requested = static_cast<DWORD>(std::min<std::size_t>(
            output.size(), static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
        auto read =
            read_at_most(sequential_offset_, output.first(static_cast<std::size_t>(requested)));
        if (!read) {
            return read.error();
        }
        sequential_offset_ += static_cast<std::uint64_t>(read.value());
        if (read.value() != static_cast<std::size_t>(requested)) {
            return io_error(diagnostics_.changed_error);
        }
        output = output.subspan(read.value());
    }
    return {};
}

result<std::size_t> stable_host_file_session::read_at_most(std::uint64_t offset,
                                                           std::span<std::byte> output) const {
    const auto requested = static_cast<DWORD>(std::min<std::size_t>(
        output.size(), static_cast<std::size_t>((std::numeric_limits<DWORD>::max)())));
    OVERLAPPED position{};
    position.Offset = static_cast<DWORD>(offset & 0xFFFF'FFFFULL);
    position.OffsetHigh = static_cast<DWORD>(offset >> 32U);
    DWORD read_count = 0U;
    if (::ReadFile(static_cast<HANDLE>(native_handle_), output.data(), requested, &read_count,
                   &position) == FALSE) {
        // Explicit-offset reads on synchronous disk handles report an EOF probe
        // as ERROR_HANDLE_EOF rather than a successful zero-byte read.
        if (::GetLastError() == ERROR_HANDLE_EOF) {
            return 0U;
        }
        return io_error(diagnostics_.read_error);
    }
    return static_cast<std::size_t>(read_count);
}

result<void> stable_host_file_session::reject_trailing_byte() {
    std::byte trailing{};
    auto read = read_at_most(sequential_offset_, std::span<std::byte>{&trailing, 1U});
    if (!read) {
        return read.error();
    }
    sequential_offset_ += static_cast<std::uint64_t>(read.value());
    if (read.value() != 0U) {
        return io_error(diagnostics_.changed_error);
    }
    return {};
}

result<std::ifstream> open_host_file(const std::filesystem::path& host_path,
                                     const host_file_context& context) {
    std::ifstream input{host_path, std::ios::binary};
    if (!input) {
        return io_error(context.open_error);
    }
    return input;
}

result<std::ifstream> open_host_file(const host_file_path& host_path,
                                     const host_file_context& context) {
    return open_host_file(host_path.resolved, context);
}

result<std::uint64_t> inspect_host_file_size(const std::filesystem::path& host_path,
                                             const host_file_context& context) {
    std::error_code fs_error;
    const bool regular_file = std::filesystem::is_regular_file(host_path, fs_error);
    if (fs_error || !regular_file) {
        return io_error(context.inspect_error);
    }

    const auto size = std::filesystem::file_size(host_path, fs_error);
    if (fs_error) {
        return io_error(context.inspect_error);
    }
    return size;
}

result<std::uint64_t> inspect_host_file_size(const host_file_path& host_path,
                                             const host_file_context& context) {
    return inspect_host_file_size(host_path.resolved, context);
}

result<std::vector<std::byte>> read_host_file_exact(const std::filesystem::path& host_path,
                                                    std::uint64_t expected_size,
                                                    const host_file_context& context) {
    auto size_valid = validate_expected_host_file_size(host_path, expected_size, context);
    if (!size_valid) {
        return size_valid.error();
    }

    auto size = checked_buffer_size(expected_size, context);
    if (!size) {
        return size.error();
    }
    auto bytes = make_byte_vector(size.value(), context.allocation_description);
    if (!bytes) {
        return bytes.error();
    }

    auto input = open_host_file(host_path, context);
    if (!input) {
        return input.error();
    }
    auto read = read_exact_bytes(input.value(), bytes.value(), context);
    if (!read) {
        return read.error();
    }
    auto stable = reject_appended_host_file_byte(input.value(), context);
    if (!stable) {
        return stable.error();
    }
    return std::move(bytes).value();
}

result<std::vector<std::byte>> read_host_file_exact(const host_file_path& host_path,
                                                    std::uint64_t expected_size,
                                                    const host_file_context& context) {
    return read_host_file_exact(host_path.resolved, expected_size, context);
}

result<std::vector<std::byte>> read_host_file_exact(const std::filesystem::path& host_path,
                                                    const host_file_context& context) {
    auto size = inspect_host_file_size(host_path, context);
    if (!size) {
        return size.error();
    }
    return read_host_file_exact(host_path, size.value(), context);
}

result<std::vector<std::byte>> read_host_file_exact(const host_file_path& host_path,
                                                    const host_file_context& context) {
    return read_host_file_exact(host_path.resolved, context);
}

result<std::vector<std::byte>> read_host_file_prefix(const std::filesystem::path& host_path,
                                                     std::size_t max_bytes,
                                                     const host_file_context& context) {
    auto input = open_host_file(host_path, context);
    if (!input) {
        return input.error();
    }

    auto bytes = make_byte_vector(max_bytes, context.allocation_description);
    if (!bytes) {
        return bytes.error();
    }

    std::size_t offset = 0;
    while (offset < bytes.value().size()) {
        const auto requested = std::min<std::size_t>(
            bytes.value().size() - offset,
            static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()));
        input.value().read(reinterpret_cast<char*>(bytes.value().data() + offset),
                           static_cast<std::streamsize>(requested));
        if (input.value().bad()) {
            return io_error(context.read_error);
        }
        const auto count = input.value().gcount();
        offset += static_cast<std::size_t>(count);
        if (count != static_cast<std::streamsize>(requested)) {
            break;
        }
    }
    bytes.value().resize(offset);
    return std::move(bytes).value();
}

result<std::vector<std::byte>> read_host_file_prefix(const host_file_path& host_path,
                                                     std::size_t max_bytes,
                                                     const host_file_context& context) {
    return read_host_file_prefix(host_path.resolved, max_bytes, context);
}

result<void> for_each_host_file_chunk(
    const std::filesystem::path& host_path, std::uint64_t expected_size,
    const host_file_context& context,
    const std::function<result<void>(std::span<const std::byte>)>& callback,
    std::size_t chunk_size) {
    if (chunk_size == 0U) {
        return error{error_code::invalid_argument, "host file chunk size must be non-zero"};
    }

    auto size_valid = validate_expected_host_file_size(host_path, expected_size, context);
    if (!size_valid) {
        return size_valid.error();
    }

    auto input = open_host_file(host_path, context);
    if (!input) {
        return input.error();
    }

    auto scratch = make_byte_vector(
        static_cast<std::size_t>(std::min<std::uint64_t>(expected_size, chunk_size)),
        context.allocation_description);
    if (!scratch) {
        return scratch.error();
    }

    std::uint64_t remaining = expected_size;
    while (remaining > 0U) {
        const auto requested = static_cast<std::size_t>(
            std::min<std::uint64_t>(remaining, static_cast<std::uint64_t>(scratch.value().size())));
        auto read = read_exact_bytes(
            input.value(), std::span<std::byte>{scratch.value().data(), requested}, context);
        if (!read) {
            return read.error();
        }
        auto handled = callback(std::span<const std::byte>{scratch.value().data(), requested});
        if (!handled) {
            return handled.error();
        }
        remaining -= requested;
    }

    return reject_appended_host_file_byte(input.value(), context);
}

result<void> for_each_host_file_chunk(
    const host_file_path& host_path, std::uint64_t expected_size, const host_file_context& context,
    const std::function<result<void>(std::span<const std::byte>)>& callback,
    std::size_t chunk_size) {
    return for_each_host_file_chunk(host_path.resolved, expected_size, context, callback,
                                    chunk_size);
}

}  // namespace libbsa::detail
