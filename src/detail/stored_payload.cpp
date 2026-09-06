#include <detail/stored_payload.hpp>

#include <detail/byte_vector.hpp>
#include <detail/host_file.hpp>
#include <detail/host_file_path.hpp>
#include <detail/writer_publish.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace libbsa::detail {

namespace {

constexpr std::uint64_t fnv1a_offset_basis = 14695981039346656037ULL;
constexpr std::uint64_t fnv1a_prime = 1099511628211ULL;

void fingerprint_bytes(std::uint64_t& hash, std::span<const std::byte> bytes) noexcept {
    for (const auto byte : bytes) {
        hash ^= std::to_integer<std::uint8_t>(byte);
        hash *= fnv1a_prime;
    }
}

error snapshot_io_error(std::string_view message) {
    return error{error_code::io_error, std::string{message}};
}

constexpr host_file_context snapshot_read_context{
    "Stored Payload failed to open workspace snapshot",
    "Stored Payload failed to inspect workspace snapshot",
    "Stored Payload failed while reading workspace snapshot",
    "Stored Payload workspace snapshot changed", "Stored Payload comparison scratch"};

}  // namespace

class stored_payload::logical_reader {
   public:
    virtual ~logical_reader() noexcept = default;

    /// Returns the next non-owning logical-byte chunk, bounded by `max_bytes`.
    virtual result<std::span<const std::byte>> next(std::size_t max_bytes) = 0;

    /// Reports whether every logical byte has been returned.
    [[nodiscard]] virtual bool finished() const noexcept = 0;
};

class stored_payload::owned_logical_reader final : public logical_reader {
   public:
    explicit owned_logical_reader(std::span<const std::byte> bytes) noexcept : bytes_(bytes) {}

    result<std::span<const std::byte>> next(std::size_t max_bytes) override {
        const auto count = std::min(max_bytes, bytes_.size() - offset_);
        const auto chunk = bytes_.subspan(offset_, count);
        offset_ += count;
        return chunk;
    }

    [[nodiscard]] bool finished() const noexcept override { return offset_ == bytes_.size(); }

   private:
    std::span<const std::byte> bytes_;
    std::size_t offset_{0U};
};

class stored_payload::snapshot_logical_reader final : public logical_reader {
   public:
    snapshot_logical_reader(std::span<const std::byte> prefix, stable_host_file_session body,
                            std::uint64_t body_size) noexcept
        : prefix_(prefix), body_(std::move(body)), body_remaining_(body_size) {}

    result<std::span<const std::byte>> next(std::size_t max_bytes) override {
        if (prefix_offset_ != prefix_.size()) {
            const auto count = std::min(max_bytes, prefix_.size() - prefix_offset_);
            const auto chunk = prefix_.subspan(prefix_offset_, count);
            prefix_offset_ += count;
            return chunk;
        }
        if (body_remaining_ == 0U) {
            return std::span<const std::byte>{};
        }

        const auto requested = static_cast<std::size_t>(
            std::min<std::uint64_t>(body_remaining_, static_cast<std::uint64_t>(max_bytes)));
        auto read = body_.read_prefix(requested);
        if (!read) {
            return read.error();
        }
        if (read.value().size() != requested) {
            return snapshot_io_error("Stored Payload workspace snapshot changed");
        }
        body_remaining_ -= static_cast<std::uint64_t>(requested);
        body_chunk_ = std::move(read.value());
        return std::span<const std::byte>{body_chunk_};
    }

    [[nodiscard]] bool finished() const noexcept override {
        return prefix_offset_ == prefix_.size() && body_remaining_ == 0U;
    }

   private:
    std::span<const std::byte> prefix_;
    stable_host_file_session body_;
    std::uint64_t body_remaining_;
    std::size_t prefix_offset_{0U};
    std::vector<std::byte> body_chunk_;
};

stored_payload stored_payload::from_owned_bytes(std::vector<std::byte> bytes) noexcept {
    const auto stored_size = static_cast<std::uint64_t>(bytes.size());
    return stored_payload{owned_bytes_adapter{std::move(bytes)}, stored_size, std::nullopt};
}

stored_payload::stored_payload(stored_payload&& other) noexcept
    : storage_(std::move(other.storage_)),
      stored_size_(std::exchange(other.stored_size_, 0U)),
      cached_fingerprint_(std::move(other.cached_fingerprint_)) {
    other.storage_.emplace<owned_bytes_adapter>();
    other.cached_fingerprint_.reset();
}

stored_payload& stored_payload::operator=(stored_payload&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    storage_ = std::move(other.storage_);
    stored_size_ = std::exchange(other.stored_size_, 0U);
    cached_fingerprint_ = std::move(other.cached_fingerprint_);
    other.storage_.emplace<owned_bytes_adapter>();
    other.cached_fingerprint_.reset();
    return *this;
}

result<stored_payload> stored_payload::from_workspace_snapshot(
    std::vector<std::byte> prefix, stable_host_file_session source,
    const finalization_workspace& workspace, std::size_t stable_preparation_identity,
    std::size_t copy_chunk_size) {
    if (copy_chunk_size > std::vector<std::byte>{}.max_size()) {
        return byte_vector_allocation_error("Stored Payload snapshot scratch");
    }
    if (source.size() > std::numeric_limits<std::uint64_t>::max() - prefix.size()) {
        return error{error_code::format_error, "Stored Payload size overflows uint64_t"};
    }
    const auto stored_size = source.size() + static_cast<std::uint64_t>(prefix.size());

    auto rewound = source.rewind();
    if (!rewound) {
        return rewound.error();
    }

    try {
        auto snapshot_path = workspace.snapshot_path(stable_preparation_identity);
        std::ofstream output{snapshot_path, std::ios::binary | std::ios::trunc};
        if (!output.good()) {
            return snapshot_io_error("Stored Payload failed to create workspace snapshot");
        }

        std::uint64_t fingerprint = fnv1a_offset_basis;
        // Prefix bytes participate in identity even though only the body is
        // copied to disk; TES4 embedded names are part of stored payload bytes.
        fingerprint_bytes(fingerprint, prefix);
        auto copied = source.copy_exact(
            source.size(),
            [&](std::span<const std::byte> chunk) -> result<void> {
                output.write(reinterpret_cast<const char*>(chunk.data()),
                             static_cast<std::streamsize>(chunk.size()));
                if (!output.good()) {
                    return snapshot_io_error(
                        "Stored Payload failed while writing workspace snapshot");
                }
                // The mandatory stable-session copy is the only body pass needed
                // to seed snapshot identity before later dedupe candidate lookup.
                fingerprint_bytes(fingerprint, chunk);
                return {};
            },
            copy_chunk_size);
        if (!copied) {
            return copied.error();
        }

        output.flush();
        if (!output.good()) {
            return snapshot_io_error("Stored Payload failed to flush workspace snapshot");
        }
        output.close();
        if (output.fail()) {
            return snapshot_io_error("Stored Payload failed to close workspace snapshot");
        }

        return stored_payload{
            workspace_snapshot_adapter{std::move(prefix), std::move(snapshot_path), source.size(),
                                       fingerprint},
            stored_size, fingerprint};
    } catch (const std::bad_alloc&) {
        return byte_vector_allocation_error("Stored Payload snapshot storage");
    } catch (const std::length_error&) {
        return byte_vector_allocation_error("Stored Payload snapshot storage");
    } catch (const std::filesystem::filesystem_error&) {
        return snapshot_io_error("Stored Payload failed to create workspace snapshot path");
    }
}

stored_payload::stored_payload(storage_adapter storage, std::uint64_t stored_size,
                               std::optional<std::uint64_t> cached_fingerprint) noexcept
    : storage_(std::move(storage)),
      stored_size_(stored_size),
      cached_fingerprint_(cached_fingerprint) {}

std::uint64_t stored_payload::size() const noexcept { return stored_size_; }

std::uint64_t stored_payload::fingerprint() const noexcept {
    if (!cached_fingerprint_.has_value()) {
        std::uint64_t fingerprint = fnv1a_offset_basis;
        std::visit(
            [&](const auto& adapter) {
                using adapter_type = std::decay_t<decltype(adapter)>;
                if constexpr (std::is_same_v<adapter_type, owned_bytes_adapter>) {
                    fingerprint_bytes(fingerprint, adapter.bytes);
                } else {
                    fingerprint = adapter.snapshot_fingerprint;
                }
            },
            storage_);
        cached_fingerprint_ = fingerprint;
    }
    return cached_fingerprint_.value();
}

result<bool> stored_payload::exactly_equals(const stored_payload& other,
                                            std::size_t comparison_chunk_size) const {
    if (comparison_chunk_size == 0U) {
        return error{error_code::invalid_argument,
                     "Stored Payload comparison chunk size must be non-zero"};
    }
    if (comparison_chunk_size > std::vector<std::byte>{}.max_size()) {
        return byte_vector_allocation_error("Stored Payload comparison scratch");
    }
    if (stored_size_ != other.stored_size_) {
        return false;
    }
    const auto* lhs = std::get_if<owned_bytes_adapter>(&storage_);
    const auto* rhs = std::get_if<owned_bytes_adapter>(&other.storage_);
    if (lhs != nullptr && rhs != nullptr) {
        return lhs->bytes == rhs->bytes;
    }

    // Adapter boundaries are not byte-identity boundaries: two snapshot
    // prefixes may split an equal logical sequence at different positions.
    auto lhs_reader = open_logical_reader();
    if (!lhs_reader) {
        return lhs_reader.error();
    }
    auto rhs_reader = other.open_logical_reader();
    if (!rhs_reader) {
        return rhs_reader.error();
    }

    std::span<const std::byte> lhs_chunk;
    std::span<const std::byte> rhs_chunk;
    std::size_t lhs_offset = 0U;
    std::size_t rhs_offset = 0U;
    std::uint64_t compared_size = 0U;
    while (compared_size != stored_size_) {
        if (lhs_offset == lhs_chunk.size()) {
            auto next = lhs_reader.value()->next(comparison_chunk_size);
            if (!next) {
                return next.error();
            }
            lhs_chunk = next.value();
            lhs_offset = 0U;
        }
        if (rhs_offset == rhs_chunk.size()) {
            auto next = rhs_reader.value()->next(comparison_chunk_size);
            if (!next) {
                return next.error();
            }
            rhs_chunk = next.value();
            rhs_offset = 0U;
        }
        if (lhs_chunk.empty() || rhs_chunk.empty()) {
            return snapshot_io_error("Stored Payload workspace snapshot changed");
        }

        const auto count = std::min(lhs_chunk.size() - lhs_offset, rhs_chunk.size() - rhs_offset);
        if (!std::equal(lhs_chunk.begin() + static_cast<std::ptrdiff_t>(lhs_offset),
                        lhs_chunk.begin() + static_cast<std::ptrdiff_t>(lhs_offset + count),
                        rhs_chunk.begin() + static_cast<std::ptrdiff_t>(rhs_offset))) {
            return false;
        }
        lhs_offset += count;
        rhs_offset += count;
        compared_size += static_cast<std::uint64_t>(count);
    }
    return lhs_reader.value()->finished() && rhs_reader.value()->finished();
}

result<void> stored_payload::emit(std::ostream& output, std::size_t emission_chunk_size) const {
    if (emission_chunk_size == 0U) {
        return error{error_code::invalid_argument,
                     "Stored Payload emission chunk size must be non-zero"};
    }

    auto reader = open_logical_reader();
    if (!reader) {
        return reader.error();
    }

    const auto bounded_chunk_size =
        (std::min)(emission_chunk_size,
                   static_cast<std::size_t>((std::numeric_limits<std::streamsize>::max)()));
    try {
        while (!reader.value()->finished()) {
            auto next = reader.value()->next(bounded_chunk_size);
            if (!next) {
                return next.error();
            }
            if (next.value().empty()) {
                return snapshot_io_error("Stored Payload workspace snapshot changed");
            }
            output.write(reinterpret_cast<const char*>(next.value().data()),
                         static_cast<std::streamsize>(next.value().size()));
            if (!output.good()) {
                return snapshot_io_error("Stored Payload output failed");
            }
        }
    } catch (...) {
        // Output adapters may throw arbitrary stream-buffer exceptions; callers
        // still receive the Stored Payload structured I/O failure contract.
        return snapshot_io_error("Stored Payload output failed");
    }
    return {};
}

result<std::unique_ptr<stored_payload::logical_reader>> stored_payload::open_logical_reader()
    const {
    try {
        if (const auto* owned = std::get_if<owned_bytes_adapter>(&storage_)) {
            std::unique_ptr<logical_reader> reader =
                std::make_unique<owned_logical_reader>(owned->bytes);
            return reader;
        }

        const auto& snapshot = std::get<workspace_snapshot_adapter>(storage_);
        auto opened = stable_host_file_session::open(host_file_path{snapshot.body_path},
                                                     snapshot_read_context);
        if (!opened) {
            return opened.error();
        }
        if (opened.value().size() != snapshot.body_size) {
            return snapshot_io_error("Stored Payload workspace snapshot changed");
        }
        std::unique_ptr<logical_reader> reader = std::make_unique<snapshot_logical_reader>(
            snapshot.prefix, std::move(opened).value(), snapshot.body_size);
        return reader;
    } catch (const std::bad_alloc&) {
        return byte_vector_allocation_error("Stored Payload comparison state");
    }
}

}  // namespace libbsa::detail
