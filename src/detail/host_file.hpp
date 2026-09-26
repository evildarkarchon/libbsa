#pragma once

#include <libbsa/result.hpp>

#include <detail/host_file_path.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libbsa::detail {

/// Diagnostics supplied by archive-family code for shared host-file reads.
struct host_file_context {
    std::string_view open_error;
    std::string_view inspect_error;
    std::string_view read_error;
    std::string_view changed_error;
    std::string_view allocation_description;
};

/// Default scratch-buffer size used when internal code streams host-file bytes.
inline constexpr std::size_t host_file_chunk_size = 64U * 1024U;

/// Owns one stable Windows observation of a resolved host file.
///
/// The session opens with read sharing only, so concurrent readers remain
/// compatible while writers and delete-capable handles are excluded for the
/// entire observation. Sequential operations share one cursor; callers must
/// explicitly rewind after a probe before reading or copying the whole source.
class stable_host_file_session final {
   public:
    /// Opens and sizes one resolved host path without a second path observation.
    static result<stable_host_file_session> open(const host_file_path& host_path,
                                                 const host_file_context& context);

    /// Releases the owned Windows handle exactly once.
    ~stable_host_file_session() noexcept;

    /// Stable observations cannot share ownership of one native handle.
    stable_host_file_session(const stable_host_file_session&) = delete;

    /// Stable observations cannot copy-assign native handle ownership.
    stable_host_file_session& operator=(const stable_host_file_session&) = delete;

    /// Transfers the stable observation from `other`.
    stable_host_file_session(stable_host_file_session&& other) noexcept;

    /// Releases the current observation, then transfers `other` into this session.
    stable_host_file_session& operator=(stable_host_file_session&& other) noexcept;

    /// Returns the byte size observed through the owned handle at open time.
    [[nodiscard]] std::uint64_t size() const noexcept;

    /// Reads at most `max_bytes` from the current sequential cursor.
    result<std::vector<std::byte>> read_prefix(std::size_t max_bytes);

    /// Reads one complete source of `expected_size` from the current cursor.
    ///
    /// A size mismatch, short read, or trailing byte is reported through the
    /// caller's `changed_error` diagnostic.
    result<std::vector<std::byte>> read_exact(std::uint64_t expected_size);

    /// Reads an exact checked range without changing the sequential cursor.
    ///
    /// `description` identifies allocation failures. Out-of-range and short
    /// reads use the caller's source-change diagnostic so archive adapters can
    /// perform their own format-range classification before delegating.
    result<std::vector<std::byte>> read_exact_at(std::uint64_t offset, std::size_t count,
                                                 std::string_view description) const;

    /// Moves the shared sequential cursor back to the beginning of the source.
    result<void> rewind();

    /// Copies exactly `expected_size` sequential bytes through bounded chunks.
    ///
    /// Callback errors are returned unchanged. `chunk_size` must be non-zero,
    /// and successful completion also proves that no trailing byte was present.
    result<void> copy_exact(std::uint64_t expected_size,
                            const std::function<result<void>(std::span<const std::byte>)>& callback,
                            std::size_t chunk_size = host_file_chunk_size);

   private:
    /// Owns the diagnostics needed after the caller's context leaves scope.
    struct session_diagnostics {
        std::string read_error;
        std::string changed_error;
        std::string allocation_description;
    };

    /// Takes ownership of an already-opened and sized Windows handle.
    stable_host_file_session(void* native_handle, std::uint64_t size,
                             session_diagnostics diagnostics) noexcept;

    /// Releases the current handle, if any, and leaves the session moved-from.
    ///
    /// A close failure is intentionally ignored because ownership cannot be
    /// restored and destructors or move replacement have no useful recovery.
    void close() noexcept;

    /// Rejects an expected size that differs from the handle observation.
    result<void> validate_expected_size(std::uint64_t expected_size) const;

    /// Fills one buffer from the current sequential cursor.
    result<void> read_sequential_exact(std::span<std::byte> output);

    /// Reads at most one native-call-sized span at an explicit source offset.
    result<std::size_t> read_at_most(std::uint64_t offset, std::span<std::byte> output) const;

    /// Proves that the current sequential cursor is exactly at end of source.
    result<void> reject_trailing_byte();

    void* native_handle_{nullptr};
    std::uint64_t size_{0U};
    std::uint64_t sequential_offset_{0U};
    session_diagnostics diagnostics_;
};

/// Opens a host file through the shared filesystem boundary using caller-owned
/// diagnostics.
result<std::ifstream> open_host_file(const std::filesystem::path& host_path,
                                     const host_file_context& context);

/// Opens a resolved host file while preserving the caller's original UTF-8 text
/// for diagnostics only.
result<std::ifstream> open_host_file(const host_file_path& host_path,
                                     const host_file_context& context);

/// Returns the current byte size of a regular host file using caller-owned
/// diagnostics.
result<std::uint64_t> inspect_host_file_size(const std::filesystem::path& host_path,
                                             const host_file_context& context);

/// Returns the current byte size of a resolved host file without reopening from
/// raw caller text.
result<std::uint64_t> inspect_host_file_size(const host_file_path& host_path,
                                             const host_file_context& context);

/// Reads exactly `expected_size` bytes from `host_path` and rejects host-file
/// size changes.
result<std::vector<std::byte>> read_host_file_exact(const std::filesystem::path& host_path,
                                                    std::uint64_t expected_size,
                                                    const host_file_context& context);

/// Reads exactly `expected_size` bytes from a resolved host file and preserves
/// original UTF-8 diagnostics.
result<std::vector<std::byte>> read_host_file_exact(const host_file_path& host_path,
                                                    std::uint64_t expected_size,
                                                    const host_file_context& context);

/// Sizes and reads an entire host file without byte-at-a-time vector growth.
result<std::vector<std::byte>> read_host_file_exact(const std::filesystem::path& host_path,
                                                    const host_file_context& context);

/// Sizes and reads an entire resolved host file while keeping diagnostics tied
/// to the original UTF-8 text.
result<std::vector<std::byte>> read_host_file_exact(const host_file_path& host_path,
                                                    const host_file_context& context);

/// Reads at most `max_bytes` from `host_path`, returning a shorter prefix when
/// the file is shorter.
result<std::vector<std::byte>> read_host_file_prefix(const std::filesystem::path& host_path,
                                                     std::size_t max_bytes,
                                                     const host_file_context& context);

/// Reads at most `max_bytes` from a resolved host file while preserving
/// original UTF-8 diagnostics text.
result<std::vector<std::byte>> read_host_file_prefix(const host_file_path& host_path,
                                                     std::size_t max_bytes,
                                                     const host_file_context& context);

/// Iterates exactly `expected_size` host-file bytes through bounded chunks and
/// rejects source changes.
result<void> for_each_host_file_chunk(
    const std::filesystem::path& host_path, std::uint64_t expected_size,
    const host_file_context& context,
    const std::function<result<void>(std::span<const std::byte>)>& callback,
    std::size_t chunk_size = host_file_chunk_size);

/// Iterates a resolved host file through bounded chunks without reinterpreting
/// caller text at each reopen.
result<void> for_each_host_file_chunk(
    const host_file_path& host_path, std::uint64_t expected_size, const host_file_context& context,
    const std::function<result<void>(std::span<const std::byte>)>& callback,
    std::size_t chunk_size = host_file_chunk_size);

}  // namespace libbsa::detail
