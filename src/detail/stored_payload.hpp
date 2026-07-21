#pragma once

#include <libbsa/result.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace libbsa::detail {

class finalization_workspace;
class stable_host_file_session;

/// Owns one exact post-prefix, post-compression archive payload byte sequence.
///
/// Stored Payload deliberately contains no archive location or format policy;
/// layout remains responsible for assigning and sharing physical placements.
class stored_payload final {
   public:
    /// Takes ownership of contiguous bytes already in final stored form.
    static stored_payload from_owned_bytes(std::vector<std::byte> bytes) noexcept;

    /// Copies one stable source into a deterministic workspace snapshot.
    ///
    /// The logical sequence is the owned `prefix` followed immediately by the
    /// snapshot body. The source is rewound before one bounded copy that also
    /// accumulates the complete cached fingerprint. The returned payload keeps
    /// no workspace lease; the caller must use it within the workspace lifetime.
    static result<stored_payload> from_workspace_snapshot(std::vector<std::byte> prefix,
                                                          stable_host_file_session source,
                                                          const finalization_workspace& workspace,
                                                          std::size_t stable_preparation_identity,
                                                          std::size_t copy_chunk_size = 64U *
                                                                                        1024U);

    /// Stored Payload values cannot share ownership of their logical bytes.
    stored_payload(const stored_payload&) = delete;

    /// Stored Payload values cannot copy-assign byte ownership.
    stored_payload& operator=(const stored_payload&) = delete;

    /// Releases owned memory without deleting workspace-owned snapshot files.
    ~stored_payload() noexcept = default;

    /// Transfers ownership of the logical byte sequence from `other`.
    stored_payload(stored_payload&& other) noexcept;

    /// Replaces this payload by transferring ownership from `other`.
    stored_payload& operator=(stored_payload&& other) noexcept;

    /// Returns the immutable logical stored-byte count.
    [[nodiscard]] std::uint64_t size() const noexcept;

    /// Returns the cached narrowing fingerprint for the logical byte sequence.
    ///
    /// Owned bytes are scanned only on the first request so callers that skip
    /// deduplication do not pay for unused fingerprint work.
    [[nodiscard]] std::uint64_t fingerprint() const noexcept;

    /// Compares authoritative logical bytes with bounded scratch memory.
    ///
    /// Fingerprints intentionally do not participate here: matching size and
    /// fingerprint may select a layout candidate but cannot establish equality.
    result<bool> exactly_equals(const stored_payload& other,
                                std::size_t comparison_chunk_size = 64U * 1024U) const;

    /// Emits the complete logical sequence to `output` in bounded writes.
    ///
    /// Snapshot-backed emission reads only the workspace snapshot. Any read or
    /// output failure returns a structured error and never reports success.
    result<void> emit(std::ostream& output, std::size_t emission_chunk_size = 64U * 1024U) const;

   private:
    struct owned_bytes_adapter {
        std::vector<std::byte> bytes;
    };

    struct workspace_snapshot_adapter {
        std::vector<std::byte> prefix;
        std::filesystem::path body_path;
        std::uint64_t body_size;
        std::uint64_t snapshot_fingerprint;
    };

    using storage_adapter = std::variant<owned_bytes_adapter, workspace_snapshot_adapter>;

    class logical_reader;
    class owned_logical_reader;
    class snapshot_logical_reader;

    stored_payload(storage_adapter storage, std::uint64_t stored_size,
                   std::optional<std::uint64_t> cached_fingerprint) noexcept;

    /// Opens one bounded logical reader without exposing the storage adapter.
    result<std::unique_ptr<logical_reader>> open_logical_reader() const;

    storage_adapter storage_;
    std::uint64_t stored_size_;
    mutable std::optional<std::uint64_t> cached_fingerprint_;
};

}  // namespace libbsa::detail
