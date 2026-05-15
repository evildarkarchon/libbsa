#include <catch2/catch_test_macros.hpp>

#include <detail/binary_io.hpp>

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

using libbsa::detail::binary_reader;
using libbsa::detail::binary_writer;

TEST_CASE("binary_io reader reads checked little-endian fields", "[unit][binary-io]")
{
  const auto bytes = std::to_array<std::byte>({
      std::byte{0x01},
      std::byte{0x34},
      std::byte{0x12},
      std::byte{0x78},
      std::byte{0x56},
      std::byte{0x34},
      std::byte{0x12},
  });
  binary_reader reader{bytes};

  REQUIRE(reader.read_u8().value() == 0x01);
  REQUIRE(reader.read_u16_le().value() == 0x1234);
  REQUIRE(reader.read_u32_le().value() == 0x12345678);
  REQUIRE(reader.position() == bytes.size());
  REQUIRE(reader.remaining() == 0);
}

TEST_CASE("binary_io writer writes little-endian fields", "[unit][binary-io]")
{
  binary_writer writer;

  REQUIRE(writer.write_u16_le(0x1234));
  REQUIRE(writer.write_u32_le(0x12345678));
  REQUIRE(writer.write_u64_le(0x0102030405060708ULL));

  const auto expected = std::to_array<std::byte>({
      std::byte{0x34},
      std::byte{0x12},
      std::byte{0x78},
      std::byte{0x56},
      std::byte{0x34},
      std::byte{0x12},
      std::byte{0x08},
      std::byte{0x07},
      std::byte{0x06},
      std::byte{0x05},
      std::byte{0x04},
      std::byte{0x03},
      std::byte{0x02},
      std::byte{0x01},
  });

  REQUIRE(std::ranges::equal(writer.bytes(), expected));
}

TEST_CASE("binary_io reader reports malformed truncation without overread", "[unit][malformed][binary-io]")
{
  const auto bytes = std::to_array<std::byte>({std::byte{0x01}, std::byte{0x02}});
  binary_reader reader{bytes};

  auto too_large = reader.read_u32_le();
  REQUIRE_FALSE(too_large);
  REQUIRE(too_large.error().code == libbsa::error_code::format_error);
  REQUIRE(reader.position() == 0);

  auto skipped = reader.skip(3);
  REQUIRE_FALSE(skipped);
  REQUIRE(skipped.error().code == libbsa::error_code::format_error);
  REQUIRE(reader.position() == 0);
}

TEST_CASE("binary_io reader read_bytes advances only on success", "[unit][malformed][binary-io]")
{
  const auto bytes = std::to_array<std::byte>({std::byte{0x01}, std::byte{0x02}, std::byte{0x03}});
  binary_reader reader{bytes};

  auto one = reader.read_bytes(1);
  REQUIRE(one);
  REQUIRE(one.value().size() == 1);
  REQUIRE(reader.position() == 1);

  auto too_many = reader.read_bytes(3);
  REQUIRE_FALSE(too_many);
  REQUIRE(too_many.error().code == libbsa::error_code::format_error);
  REQUIRE(reader.position() == 1);
}
