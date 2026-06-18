#include <catch2/catch_test_macros.hpp>

#include <detail/payload_stream.hpp>

#include <libbsa/archive.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

class memory_source final : public libbsa::detail::payload_source {
   public:
    explicit memory_source(std::vector<std::byte> bytes) : bytes_(std::move(bytes)) {}

    libbsa::result<std::size_t> read(std::span<std::byte> destination) override {
        const auto count = std::min(destination.size(), remaining());
        std::copy_n(bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
                    static_cast<std::ptrdiff_t>(count), destination.begin());
        position_ += count;
        return count;
    }

    [[nodiscard]] std::size_t remaining() const noexcept override {
        return bytes_.size() - position_;
    }

   private:
    std::vector<std::byte> bytes_;
    std::size_t position_{0};
};

class failing_source final : public libbsa::detail::payload_source {
   public:
    libbsa::result<std::size_t> read(std::span<std::byte>) override {
        return libbsa::error{libbsa::error_code::format_error, "source failed"};
    }

    [[nodiscard]] std::size_t remaining() const noexcept override { return 1; }
};

class memory_sink final : public libbsa::detail::payload_sink, public libbsa::payload_sink {
   public:
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
        write_sizes_.push_back(bytes.size());
        return bytes.size();
    }

    [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }
    [[nodiscard]] const std::vector<std::size_t>& write_sizes() const noexcept {
        return write_sizes_;
    }

   private:
    std::vector<std::byte> bytes_;
    std::vector<std::size_t> write_sizes_;
};

class failing_sink final : public libbsa::detail::payload_sink, public libbsa::payload_sink {
   public:
    libbsa::result<std::size_t> write(std::span<const std::byte>) override {
        return libbsa::error{libbsa::error_code::format_error, "sink failed"};
    }
};

class partial_sink final : public libbsa::detail::payload_sink, public libbsa::payload_sink {
   public:
    libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
        return bytes.empty() ? 0U : bytes.size() - 1U;
    }
};

class temporary_payload_file final {
   public:
    explicit temporary_payload_file(std::span<const std::byte> bytes)
        : path_{std::filesystem::temp_directory_path() /
                ("libbsa_payload_stream_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                 ".bin")} {
        std::ofstream output{path_, std::ios::binary};
        REQUIRE(output);
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(output);
    }

    temporary_payload_file(const temporary_payload_file&) = delete;
    temporary_payload_file& operator=(const temporary_payload_file&) = delete;

    ~temporary_payload_file() {
        std::error_code ignored;
        std::filesystem::remove(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

   private:
    std::filesystem::path path_;
};

std::vector<std::byte> payload_bytes() {
    std::vector<std::byte> bytes(256);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::byte>(static_cast<std::uint8_t>(index));
    }
    return bytes;
}

}  // namespace

TEST_CASE("payload_stream transfers exact bytes across chunk size 17", "[unit][payload-stream]") {
    auto expected = payload_bytes();
    memory_source source{expected};
    memory_sink sink;

    auto transferred = libbsa::detail::transfer_payload(source, sink, 17);

    REQUIRE(transferred);
    REQUIRE(source.remaining() == 0);
    REQUIRE(sink.bytes() == expected);
}

TEST_CASE("payload_stream propagates source and sink failures",
          "[unit][malformed][payload-stream]") {
    failing_source source;
    memory_sink sink;
    auto source_result = libbsa::detail::transfer_payload(source, sink, 17);
    REQUIRE_FALSE(source_result);
    REQUIRE(source_result.error().code == libbsa::error_code::format_error);

    auto bytes = payload_bytes();
    memory_source ok_source{bytes};
    failing_sink bad_sink;
    auto sink_result = libbsa::detail::transfer_payload(ok_source, bad_sink, 17);
    REQUIRE_FALSE(sink_result);
    REQUIRE(sink_result.error().code == libbsa::error_code::format_error);
}

TEST_CASE("payload_stream rejects partial sink writes and zero chunk size",
          "[unit][malformed][payload-stream]") {
    auto bytes = payload_bytes();
    memory_source source{bytes};
    partial_sink sink;
    auto partial = libbsa::detail::transfer_payload(source, sink, 17);
    REQUIRE_FALSE(partial);
    REQUIRE(partial.error().code == libbsa::error_code::io_error);

    memory_source zero_source{payload_bytes()};
    memory_sink zero_sink;
    auto zero_chunk = libbsa::detail::transfer_payload(zero_source, zero_sink, 0);
    REQUIRE_FALSE(zero_chunk);
    REQUIRE(zero_chunk.error().code == libbsa::error_code::invalid_argument);
}

TEST_CASE("payload_stream validates archive size and stream ranges",
          "[unit][malformed][payload-stream]") {
    auto checked = libbsa::detail::checked_payload_size(42U, "test payload");
    REQUIRE(checked);
    REQUIRE(checked.value() == 42U);

    if constexpr (sizeof(std::size_t) < sizeof(std::uint64_t)) {
        const auto too_large =
            static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) + 1U;
        auto rejected = libbsa::detail::checked_payload_size(too_large, "test payload");
        REQUIRE_FALSE(rejected);
        REQUIRE(rejected.error().code == libbsa::error_code::format_error);
    }

    const auto max_offset = static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max());
    const auto max_size = static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max());

    auto offset_limit =
        libbsa::detail::validate_payload_stream_range(max_offset + 1U, 0U, "test payload");
    REQUIRE_FALSE(offset_limit);
    REQUIRE(offset_limit.error().code == libbsa::error_code::format_error);

    auto size_limit =
        libbsa::detail::validate_payload_stream_range(0U, max_size + 1U, "test payload");
    REQUIRE_FALSE(size_limit);
    REQUIRE(size_limit.error().code == libbsa::error_code::format_error);

    auto span_limit = libbsa::detail::validate_payload_stream_range(max_offset, 1U, "test payload");
    REQUIRE_FALSE(span_limit);
    REQUIRE(span_limit.error().code == libbsa::error_code::format_error);
}

TEST_CASE("payload_stream validates single-vector materialization limits",
          "[unit][malformed][payload-stream][allocation]") {
    auto checked =
        libbsa::detail::checked_materialized_payload_size(42U, "test materialized payload");
    REQUIRE(checked);
    REQUIRE(checked.value() == 42U);

    const auto max_size = std::vector<std::byte>{}.max_size();
    if (max_size == std::numeric_limits<std::uint64_t>::max()) {
        SKIP("byte vector max_size cannot be overflowed on this standard library");
    }

    auto rejected = libbsa::detail::checked_materialized_payload_size(
        static_cast<std::uint64_t>(max_size) + 1U, "test materialized payload");
    REQUIRE_FALSE(rejected);
    REQUIRE(rejected.error().code == libbsa::error_code::format_error);
}

TEST_CASE("payload_stream reads exact archive ranges", "[unit][payload-stream]") {
    auto bytes = payload_bytes();
    temporary_payload_file file{bytes};
    std::ifstream input{file.path(), std::ios::binary};
    REQUIRE(input);
    input.setstate(std::ios::eofbit);

    auto read = libbsa::detail::read_payload_bytes_at(input, 17U, 23U, "test payload");

    REQUIRE(read);
    const std::vector<std::byte> expected{bytes.begin() + 17, bytes.begin() + 40};
    REQUIRE(read.value() == expected);
}

TEST_CASE("payload_stream rejects truncated exact archive reads",
          "[unit][malformed][payload-stream]") {
    auto bytes = payload_bytes();
    temporary_payload_file file{bytes};
    std::ifstream input{file.path(), std::ios::binary};
    REQUIRE(input);

    auto read =
        libbsa::detail::read_payload_bytes_at(input, bytes.size() - 8U, 16U, "test payload");

    REQUIRE_FALSE(read);
    REQUIRE(read.error().code == libbsa::error_code::format_error);
}

TEST_CASE("payload_stream streams raw ranges with bounded chunks", "[unit][payload-stream]") {
    auto bytes = payload_bytes();
    temporary_payload_file file{bytes};
    std::ifstream input{file.path(), std::ios::binary};
    REQUIRE(input);
    memory_sink sink;

    auto streamed = libbsa::detail::stream_payload_range(input, 8U, 19U, sink, 7U, "test payload");

    REQUIRE(streamed);
    const std::vector<std::byte> expected{bytes.begin() + 8, bytes.begin() + 27};
    REQUIRE(sink.bytes() == expected);
    REQUIRE(sink.write_sizes() == std::vector<std::size_t>{7U, 7U, 5U});
}

TEST_CASE("payload_stream writes decoded spans in chunks", "[unit][payload-stream]") {
    auto bytes = payload_bytes();
    memory_sink sink;

    auto written = libbsa::detail::write_payload_chunks(sink, bytes, 100U, "decoded payload");

    REQUIRE(written);
    REQUIRE(sink.bytes() == bytes);
    REQUIRE(sink.write_sizes() == std::vector<std::size_t>{100U, 100U, 56U});
}

TEST_CASE("payload_stream rejects partial sink writes through shared helpers",
          "[unit][malformed][payload-stream]") {
    auto bytes = payload_bytes();
    partial_sink exact_sink;
    auto exact = libbsa::detail::write_payload_exact(
        exact_sink, std::span<const std::byte>{bytes.data(), 8U}, "decoded payload");
    REQUIRE_FALSE(exact);
    REQUIRE(exact.error().code == libbsa::error_code::io_error);

    partial_sink chunked_sink;
    auto chunked =
        libbsa::detail::write_payload_chunks(chunked_sink, bytes, 17U, "decoded payload");
    REQUIRE_FALSE(chunked);
    REQUIRE(chunked.error().code == libbsa::error_code::io_error);
}
