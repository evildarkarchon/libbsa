#include <detail/binary_io.hpp>

#include <limits>

namespace libbsa::detail {
namespace {

libbsa::error truncated_error() {
  return {libbsa::error_code::format_error, "binary input ended before requested field"};
}

} // namespace

binary_reader::binary_reader(std::span<const std::byte> bytes) noexcept : bytes_(bytes) {}

std::size_t binary_reader::position() const noexcept { return position_; }

std::size_t binary_reader::remaining() const noexcept { return bytes_.size() - position_; }

bool binary_reader::can_read(std::size_t count) const noexcept { return count <= remaining(); }

result<std::uint8_t> binary_reader::read_u8() {
  auto bytes = read_bytes(1);
  if (!bytes) {
    return bytes.error();
  }
  return static_cast<std::uint8_t>(bytes.value()[0]);
}

result<std::uint16_t> binary_reader::read_u16_le() {
  auto bytes = read_bytes(2);
  if (!bytes) {
    return bytes.error();
  }
  const auto value = static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes.value()[0])) |
      static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes.value()[1])) << 8U));
  return value;
}

result<std::uint32_t> binary_reader::read_u32_le() {
  auto bytes = read_bytes(4);
  if (!bytes) {
    return bytes.error();
  }
  std::uint32_t value = 0;
  for (std::size_t index = 0; index < 4; ++index) {
    value |= static_cast<std::uint32_t>(static_cast<std::uint8_t>(bytes.value()[index])) << (index * 8U);
  }
  return value;
}

result<std::uint64_t> binary_reader::read_u64_le() {
  auto bytes = read_bytes(8);
  if (!bytes) {
    return bytes.error();
  }
  std::uint64_t value = 0;
  for (std::size_t index = 0; index < 8; ++index) {
    value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(bytes.value()[index])) << (index * 8U);
  }
  return value;
}

result<std::span<const std::byte>> binary_reader::read_bytes(std::size_t count) {
  if (!can_read(count)) {
    return truncated_error();
  }
  const auto start = position_;
  position_ += count;
  return bytes_.subspan(start, count);
}

result<void> binary_reader::skip(std::size_t count) {
  if (!can_read(count)) {
    return truncated_error();
  }
  position_ += count;
  return {};
}

result<void> binary_writer::write_u8(std::uint8_t value) {
  bytes_.push_back(static_cast<std::byte>(value));
  return {};
}

result<void> binary_writer::write_u16_le(std::uint16_t value) {
  for (std::size_t index = 0; index < 2; ++index) {
    bytes_.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
  }
  return {};
}

result<void> binary_writer::write_u32_le(std::uint32_t value) {
  for (std::size_t index = 0; index < 4; ++index) {
    bytes_.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
  }
  return {};
}

result<void> binary_writer::write_u64_le(std::uint64_t value) {
  for (std::size_t index = 0; index < 8; ++index) {
    bytes_.push_back(static_cast<std::byte>((value >> (index * 8U)) & 0xFFU));
  }
  return {};
}

result<void> binary_writer::write_bytes(std::span<const std::byte> bytes) {
  bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
  return {};
}

std::span<const std::byte> binary_writer::bytes() const noexcept { return bytes_; }

} // namespace libbsa::detail
