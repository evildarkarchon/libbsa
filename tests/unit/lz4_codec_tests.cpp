#include <catch2/catch_test_macros.hpp>

#include <libbsa/archive.hpp>

#include <detail/byte_vector.hpp>
#include <detail/lz4_block_codec.hpp>
#include <detail/lz4_frame_codec.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

std::vector<std::byte> lz4_vector() {
    constexpr std::string_view text = "libbsa lz4 frame and raw block vector";
    std::vector<std::byte> bytes;
    for (int repeat = 0; repeat < 8; ++repeat) {
        std::transform(text.begin(), text.end(), std::back_inserter(bytes),
                       [](char value) { return static_cast<std::byte>(value); });
    }
    return bytes;
}

std::size_t impossible_byte_vector_size() {
    const auto max_size = std::vector<std::byte>{}.max_size();
    if (max_size == std::numeric_limits<std::size_t>::max()) {
        SKIP("byte vector max_size cannot be overflowed on this standard library");
    }
    return max_size + 1U;
}

class recording_sink final : public libbsa::payload_sink {
   public:
    /// Records each accepted write so tests can verify exact bytes and chunk
    /// bounds.
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
    /// Writes `bytes` to a unique temporary file for stream-oriented codec tests.
    explicit temporary_payload_file(std::span<const std::byte> bytes)
        : path_{std::filesystem::temp_directory_path() /
                ("libbsa_lz4_codec_" +
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

TEST_CASE("lz4_codec frame and raw block round trip independently", "[unit][compression][lz4]") {
    const auto original = lz4_vector();

    auto frame = libbsa::detail::compress_lz4_frame(original);
    REQUIRE(frame);
    auto frame_decoded = libbsa::detail::decompress_lz4_frame_exact(frame.value(), original.size());
    REQUIRE(frame_decoded);
    REQUIRE(frame_decoded.value() == original);

    auto raw_block = libbsa::detail::compress_lz4_block(original);
    REQUIRE(raw_block);
    auto block_decoded =
        libbsa::detail::decompress_lz4_block_exact(raw_block.value(), original.size());
    REQUIRE(block_decoded);
    REQUIRE(block_decoded.value() == original);
}

TEST_CASE("lz4_codec rejects malformed frame and raw block payloads",
          "[unit][malformed][compression][lz4]") {
    const auto original = lz4_vector();
    auto frame = libbsa::detail::compress_lz4_frame(original).value();
    auto raw_block = libbsa::detail::compress_lz4_block(original).value();

    frame.resize(frame.size() / 2);
    raw_block.resize(raw_block.size() / 2);

    auto bad_frame = libbsa::detail::decompress_lz4_frame_exact(frame, original.size());
    REQUIRE_FALSE(bad_frame);
    REQUIRE(bad_frame.error().code == libbsa::error_code::format_error);

    auto bad_block = libbsa::detail::decompress_lz4_block_exact(raw_block, original.size());
    REQUIRE_FALSE(bad_block);
    REQUIRE(bad_block.error().code == libbsa::error_code::format_error);
}

TEST_CASE("lz4_codec frame exact-to-sink writes bounded chunks", "[unit][compression][lz4][sink]") {
    constexpr std::size_t chunk_size = 17U;
    const auto original = lz4_vector();
    auto frame = libbsa::detail::compress_lz4_frame(original);
    REQUIRE(frame);
    std::vector<std::byte> archive_bytes{std::byte{0xA5}};
    archive_bytes.insert(archive_bytes.end(), frame.value().begin(), frame.value().end());
    archive_bytes.push_back(std::byte{0x5A});
    temporary_payload_file file{archive_bytes};
    std::ifstream input{file.path(), std::ios::binary};
    REQUIRE(input.good());
    recording_sink sink;

    auto decoded = libbsa::detail::decompress_lz4_frame_exact_to_sink(
        input, 1U, frame.value().size(), original.size(), sink, chunk_size,
        "test LZ4 frame payload");

    REQUIRE(decoded);
    REQUIRE(sink.bytes() == original);
    REQUIRE_FALSE(sink.write_sizes().empty());
    for (const auto write_size : sink.write_sizes()) {
        REQUIRE(write_size <= chunk_size);
    }
}

TEST_CASE("lz4_codec frame exact-to-sink rejects malformed and overproducing frames",
          "[unit][malformed][compression][lz4][sink]") {
    const auto original = lz4_vector();

    SECTION("truncated frame") {
        auto frame = libbsa::detail::compress_lz4_frame(original).value();
        frame.resize(frame.size() / 2U);
        temporary_payload_file file{frame};
        std::ifstream input{file.path(), std::ios::binary};
        REQUIRE(input.good());
        recording_sink sink;

        auto decoded = libbsa::detail::decompress_lz4_frame_exact_to_sink(
            input, 0U, frame.size(), original.size(), sink, 13U, "truncated LZ4 frame payload");

        REQUIRE_FALSE(decoded);
        REQUIRE(decoded.error().code == libbsa::error_code::format_error);
    }

    SECTION("decoded output exceeds metadata") {
        auto frame = libbsa::detail::compress_lz4_frame(original).value();
        temporary_payload_file file{frame};
        std::ifstream input{file.path(), std::ios::binary};
        REQUIRE(input.good());
        recording_sink sink;
        const auto declared_size = original.size() - 1U;

        auto decoded = libbsa::detail::decompress_lz4_frame_exact_to_sink(
            input, 0U, frame.size(), declared_size, sink, 13U, "overproducing LZ4 frame payload");

        REQUIRE_FALSE(decoded);
        REQUIRE(decoded.error().code == libbsa::error_code::format_error);
        REQUIRE(sink.bytes().size() <= declared_size);
    }

    SECTION("raw LZ4 block is not a frame") {
        auto raw_block = libbsa::detail::compress_lz4_block(original).value();
        temporary_payload_file file{raw_block};
        std::ifstream input{file.path(), std::ios::binary};
        REQUIRE(input.good());
        recording_sink sink;

        auto decoded = libbsa::detail::decompress_lz4_frame_exact_to_sink(
            input, 0U, raw_block.size(), original.size(), sink, 13U, "non-frame LZ4 payload");

        REQUIRE_FALSE(decoded);
        REQUIRE(decoded.error().code == libbsa::error_code::format_error);
    }
}

TEST_CASE("lz4_codec rejects cross-format and exact-size mismatches",
          "[unit][malformed][compression][lz4]") {
    const auto original = lz4_vector();
    auto frame = libbsa::detail::compress_lz4_frame(original).value();
    auto raw_block = libbsa::detail::compress_lz4_block(original).value();

    auto raw_as_frame = libbsa::detail::decompress_lz4_frame_exact(raw_block, original.size());
    REQUIRE_FALSE(raw_as_frame);
    REQUIRE(raw_as_frame.error().code == libbsa::error_code::format_error);

    auto frame_as_raw = libbsa::detail::decompress_lz4_block_exact(frame, original.size());
    REQUIRE_FALSE(frame_as_raw);
    REQUIRE(frame_as_raw.error().code == libbsa::error_code::format_error);

    auto frame_wrong_size = libbsa::detail::decompress_lz4_frame_exact(frame, original.size() + 1);
    REQUIRE_FALSE(frame_wrong_size);
    REQUIRE(frame_wrong_size.error().code == libbsa::error_code::format_error);

    auto block_wrong_size =
        libbsa::detail::decompress_lz4_block_exact(raw_block, original.size() - 1);
    REQUIRE_FALSE(block_wrong_size);
    REQUIRE(block_wrong_size.error().code == libbsa::error_code::format_error);
}

TEST_CASE("byte_vector helpers translate impossible byte-buffer growth",
          "[unit][allocation][byte_vector]") {
    const auto impossible_size = impossible_byte_vector_size();

    auto allocated = libbsa::detail::make_byte_vector(impossible_size, "test byte allocation");
    REQUIRE_FALSE(allocated);
    REQUIRE(allocated.error().code == libbsa::error_code::format_error);

    std::vector<std::byte> reserved_bytes;
    auto reserved =
        libbsa::detail::reserve_byte_vector(reserved_bytes, impossible_size, "test byte reserve");
    REQUIRE_FALSE(reserved);
    REQUIRE(reserved.error().code == libbsa::error_code::format_error);

    const std::byte source{};
    auto appended = libbsa::detail::append_byte_vector(
        reserved_bytes, std::span<const std::byte>{&source, impossible_size}, "test byte append");
    REQUIRE_FALSE(appended);
    REQUIRE(appended.error().code == libbsa::error_code::format_error);
}

TEST_CASE("lz4_codec translates impossible output allocations",
          "[unit][malformed][compression][lz4][allocation]") {
    const auto impossible_size = impossible_byte_vector_size();

    auto frame = libbsa::detail::decompress_lz4_frame_exact({}, impossible_size);
    REQUIRE_FALSE(frame);
    REQUIRE(frame.error().code == libbsa::error_code::format_error);

    auto block = libbsa::detail::decompress_lz4_block_exact({}, impossible_size);
    REQUIRE_FALSE(block);
    REQUIRE(block.error().code == libbsa::error_code::format_error);
}
