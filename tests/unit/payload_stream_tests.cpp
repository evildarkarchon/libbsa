#include <catch2/catch_test_macros.hpp>

#include <detail/payload_stream.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
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

  [[nodiscard]] std::size_t remaining() const noexcept override { return bytes_.size() - position_; }

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

class memory_sink final : public libbsa::detail::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
    return bytes.size();
  }

  [[nodiscard]] const std::vector<std::byte>& bytes() const noexcept { return bytes_; }

 private:
  std::vector<std::byte> bytes_;
};

class failing_sink final : public libbsa::detail::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte>) override {
    return libbsa::error{libbsa::error_code::format_error, "sink failed"};
  }
};

class partial_sink final : public libbsa::detail::payload_sink {
 public:
  libbsa::result<std::size_t> write(std::span<const std::byte> bytes) override { return bytes.empty() ? 0U : bytes.size() - 1U; }
};

std::vector<std::byte> payload_bytes() {
  std::vector<std::byte> bytes(256);
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::byte>(static_cast<std::uint8_t>(index));
  }
  return bytes;
}

} // namespace

TEST_CASE("payload_stream transfers exact bytes across chunk size 17", "[unit][payload-stream]") {
  auto expected = payload_bytes();
  memory_source source{expected};
  memory_sink sink;

  auto transferred = libbsa::detail::transfer_payload(source, sink, 17);

  REQUIRE(transferred);
  REQUIRE(source.remaining() == 0);
  REQUIRE(sink.bytes() == expected);
}

TEST_CASE("payload_stream propagates source and sink failures", "[unit][malformed][payload-stream]") {
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

TEST_CASE("payload_stream rejects partial sink writes and zero chunk size", "[unit][malformed][payload-stream]") {
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
