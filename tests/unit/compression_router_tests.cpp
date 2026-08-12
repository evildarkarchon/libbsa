#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>

#include <detail/compression_router.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

std::vector<std::byte> router_vector() {
    constexpr std::string_view text = "libbsa explicit compression router vector";
    std::vector<std::byte> bytes;
    for (int repeat = 0; repeat < 4; ++repeat) {
        std::transform(text.begin(), text.end(), std::back_inserter(bytes),
                       [](char value) { return static_cast<std::byte>(value); });
    }
    return bytes;
}

class recording_sink final : public libbsa::payload_sink {
   public:
    /// Records each accepted write so router tests can verify exact bytes and
    /// chunk bounds.
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

class temporary_payload_file final {
   public:
    /// Writes `bytes` to a unique temporary file for router sink tests.
    explicit temporary_payload_file(std::span<const std::byte> bytes)
        : path_{std::filesystem::temp_directory_path() /
                ("libbsa_compression_router_" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                 ".bin")} {
        std::ofstream output{path_, std::ios::binary | std::ios::trunc};
        REQUIRE(output.good());
        output.write(reinterpret_cast<const char*>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        REQUIRE(output.good());
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

}  // namespace

TEST_CASE("compression_router dispatches explicit codec methods",
          "[unit][compression][compression-router]") {
    const auto original = router_vector();
    for (auto method : {libbsa::detail::compression_method::deflate,
                        libbsa::detail::compression_method::lz4_frame,
                        libbsa::detail::compression_method::lz4_block}) {
        auto compressed = libbsa::detail::compress_payload(method, original);
        REQUIRE(compressed);
        auto decoded =
            libbsa::detail::decompress_payload_exact(method, compressed.value(), original.size());
        REQUIRE(decoded);
        REQUIRE(decoded.value() == original);
    }
}

TEST_CASE("compression_router none method preserves exact input bytes",
          "[unit][compression][compression-router]") {
    const auto original = router_vector();
    auto compressed =
        libbsa::detail::compress_payload(libbsa::detail::compression_method::none, original);
    REQUIRE(compressed);
    REQUIRE(compressed.value() == original);

    auto decoded = libbsa::detail::decompress_payload_exact(
        libbsa::detail::compression_method::none, original, original.size());
    REQUIRE(decoded);
    REQUIRE(decoded.value() == original);

    auto wrong_size = libbsa::detail::decompress_payload_exact(
        libbsa::detail::compression_method::none, original, original.size() + 1);
    REQUIRE_FALSE(wrong_size);
    REQUIRE(wrong_size.error().code == libbsa::error_code::format_error);
}

TEST_CASE("compression_router sink route streams LZ4 frame payloads",
          "[unit][compression][compression-router][sink]") {
    constexpr std::size_t chunk_size = 19U;
    const auto original = router_vector();
    auto compressed =
        libbsa::detail::compress_payload(libbsa::detail::compression_method::lz4_frame, original);
    REQUIRE(compressed);
    temporary_payload_file file{compressed.value()};
    std::ifstream input{file.path(), std::ios::binary};
    REQUIRE(input.good());
    recording_sink sink;

    auto decoded = libbsa::detail::decompress_payload_exact_to_sink(
        libbsa::detail::compression_method::lz4_frame, input, 0U, compressed.value().size(),
        original.size(), sink, chunk_size, "router LZ4 frame payload");

    REQUIRE(decoded);
    REQUIRE(sink.bytes() == original);
    REQUIRE_FALSE(sink.write_sizes().empty());
    for (const auto write_size : sink.write_sizes()) {
        REQUIRE(write_size <= chunk_size);
    }
}

TEST_CASE("compression_router sink fallbacks preserve bytes and exact-size errors",
          "[unit][compression][compression-router][sink]") {
    const auto original = router_vector();
    for (const auto method : {libbsa::detail::compression_method::deflate,
                              libbsa::detail::compression_method::lz4_block}) {
        auto compressed = libbsa::detail::compress_payload(method, original);
        REQUIRE(compressed);
        temporary_payload_file file{compressed.value()};
        std::ifstream input{file.path(), std::ios::binary};
        REQUIRE(input.good());
        recording_sink sink;

        auto decoded = libbsa::detail::decompress_payload_exact_to_sink(
            method, input, 0U, compressed.value().size(), original.size(), sink, 23U,
            "router fallback payload");

        REQUIRE(decoded);
        REQUIRE(sink.bytes() == original);

        std::ifstream mismatch_input{file.path(), std::ios::binary};
        REQUIRE(mismatch_input.good());
        recording_sink mismatch_sink;
        auto mismatch = libbsa::detail::decompress_payload_exact_to_sink(
            method, mismatch_input, 0U, compressed.value().size(), original.size() - 1U,
            mismatch_sink, 23U, "router fallback payload mismatch");
        REQUIRE_FALSE(mismatch);
        REQUIRE(mismatch.error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("compression_router rejects unsupported explicit enum values",
          "[unit][malformed][compression][compression-router]") {
    const auto original = router_vector();
    const auto unsupported = static_cast<libbsa::detail::compression_method>(255);

    auto compressed = libbsa::detail::compress_payload(unsupported, original);
    REQUIRE_FALSE(compressed);
    REQUIRE(compressed.error().code == libbsa::error_code::invalid_argument);

    auto decoded = libbsa::detail::decompress_payload_exact(unsupported, original, original.size());
    REQUIRE_FALSE(decoded);
    REQUIRE(decoded.error().code == libbsa::error_code::invalid_argument);

    temporary_payload_file file{original};
    std::ifstream input{file.path(), std::ios::binary};
    REQUIRE(input.good());
    recording_sink sink;
    auto sink_decoded = libbsa::detail::decompress_payload_exact_to_sink(
        unsupported, input, 0U, original.size(), original.size(), sink, 8U,
        "unsupported router payload");
    REQUIRE_FALSE(sink_decoded);
    REQUIRE(sink_decoded.error().code == libbsa::error_code::invalid_argument);
}
